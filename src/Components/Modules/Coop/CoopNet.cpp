#include "STDInc.hpp"
#include "Coop.hpp"
#include "CoopNet.hpp"

#include <unordered_set>

#pragma comment(lib, "ws2_32.lib")

// Phase 2: IW4 Spec Ops style lockstep between two PCs. Spec: local/cod4/IW4-NETCODE-SPEC.md (sections 3-7, 9).
//
// Session:  host `coop_host <map>` listens on coop_port; partner `coop_connect <ip>[:port]` sends coopConnect until coopAccept;
//           host replies coopAccept + coopLaunch <map> <serverId> <seed> <skill>, both run `devmap <map>`; the partner sends coopLoaded when
//           its level runs; the host's simulation is held until then (load barrier).
// Tick T:   G_RunFrame state 1 (the stock input hand-over 0x4403F0, called from 0x4A3F6A) = our ExecuteTick:
//           host: frame T = { T, state hash, block player 0 = host's new usercmds, block player 1 = partner's queued usercmds },
//                 sent to the partner (all unacked frames) and executed; partner: executes frame T (hash compared first).
//           The second stock hand-over (G_RunFrame state 7, 0x4BB11D) does nothing in a session.
// Pacing:   the server stepper call 0x5C85BD gets only as much time as frames allow (partner: frames held; host: partner not
//           too far behind, IW4's 10/5 frames), else it is skipped like SV_Frame's cl_paused path. sv_framerate_smoothing is
//           forced off so a frame runs whole inside its 50 ms step.
// View:     the partner's client gets snapshots of client 1 (the 3 calls of SV_BuildAndSendClientSnapshot 0x5C8A50, ECX + 0xD728).
//
// Not yet (TODO, see NOTES): reliable client commands in frames (host runs its own locally, the partner's are dropped), delta usercmds,
// determinism guards beyond the seed (getdvar filter, FX visibility, buttonpressed, real-time death delay), restart/save sync, drops.

namespace Components
{
	namespace
	{
		constexpr int PROTOCOL = 0x1D;
		constexpr int USERCMD = 0x38;
		constexpr int TICK = 50;
		constexpr DWORD CL_CMDS = 0x868734;       // usercmd_s cl.cmds[64]
		constexpr DWORD CL_CMDNUMBER = 0x869534;
		constexpr DWORD CL_CMDACK = 0x869538;
		constexpr DWORD CL_RELSEQ = 0x819678;     // client reliable commands (client -> server)
		constexpr DWORD CL_RELACK = 0x81967C;
		constexpr DWORD ClientCommand_f = 0x4AD860; // player 1 (entity 0) only
		constexpr DWORD HandOver_f = 0x4403F0;
		constexpr DWORD LEVEL_TIME = 0xE18E50;
		constexpr DWORD STEP_ACCUM = 0x14B10FC;
		constexpr DWORD HOLDRAND = 0x6DDAAC;
		constexpr int MAX_BEHIND = 10;
		constexpr int MIN_BEHIND = 5;
		constexpr int MAX_CMDS_PER_BLOCK = 8;
		constexpr int MAX_PACKET = 1400;

		using Cmd = std::array<std::uint8_t, USERCMD>;

		struct Frame
		{
			int time = 0;
			std::uint32_t hash = 0;
			std::vector<Cmd> cmds[2];
			std::vector<std::pair<int, std::string>> rel[2]; // reliable client commands (seq, text): menu responses, cheats
			std::vector<std::uint8_t> bytes;
		};

		constexpr int MAX_REL_PER_BLOCK = 16;
		constexpr int MAX_REL_TEXT = 1000;

		SOCKET sock = INVALID_SOCKET;
		sockaddr_in peer{};
		bool havePeer = false;
		int role = CoopNet::ROLE_OFF;
		bool accepted = false;      // partner: coopAccept seen
		bool launched = false;      // a session map was launched (Active)
		bool partnerLoaded = false; // host: coopLoaded received
		bool hostSawFrames = false; // partner: first frame received (stops coopLoaded)
		int seed = 0;
		std::uint32_t serverId = 0;
		std::uint32_t levelEpoch = 0; // level lives in this session (launch, death reload, save load): both PCs count the same
		std::uint32_t LifeId() { return serverId + levelEpoch; }
		// player 1's death reload (loadgame_continue) by simulation time: reloadTick = death + g_deathDelay on both PCs
		int reloadTick = 0;
		bool reloadPending = false;   // stepper held until the reload runs
		ULONGLONG reloadPendingSince = 0;

		std::string pendingCmd = "loadgame_continue\n"; // the level command of reloadTick (death reload, level exit)
		std::string hostMap;                     // partner: map of the host's newest level life (coopMap)
		std::uint32_t hostMapLife = 0;
		ULONGLONG hostMapSeen = 0;

		void QueueReload()
		{
			Coop::Log("net: level command at tick %d: %s", reloadTick, pendingCmd.c_str());
			reloadTick = 0;
			reloadPending = false;
			Game::Cbuf_AddText(0, pendingCmd.c_str());
			pendingCmd = "loadgame_continue\n";
		}
		ULONGLONG notRunningSince = 0;
		std::string mapName;
		ULONGLONG lastRecv = 0, lastConnectSend = 0, lastLaunchSend = 0, lastInputSend = 0, lastResend = 0, lastStatus = 0;

		std::map<int, Frame> frames;  // host: sent, unacked; partner: received, not executed
		int partnerAck = 0;           // host: newest contiguous frame time the partner holds
		bool hostHolding = false;
		std::vector<Cmd> partnerQueue; // host: partner cmds not yet in a frame
		int partnerQueuedTime = INT_MIN;
		int lastLocalCmd = 0;          // cl.cmdNumber already taken from the local client
		std::vector<std::pair<int, std::string>> partnerRelQueue; // host: partner's reliable commands not yet in a frame
		int partnerRelQueued = INT_MIN;
		int execRelSeq[2] = { INT_MIN, INT_MIN }; // newest reliable command executed per player
		int lastExecuted = 0;          // partner: newest executed frame time
		int hashOk = 0, hashBad = 0, firstBad = -1, missing = 0;
		int snapshotOffset = 0;        // added to ECX for SV_BuildAndSendClientSnapshot (0 or one client_t)
		int snapshotCalls = 0, snapshotShifted = 0;

		// briefing (pregame menu with the level film; the world is paused until UI_PlayerStart 0x565330 closes it)
		bool localBriefDone = false;   // this PC's pregame closed (film over, skipped, or no film)
		bool peerBriefDone = false;    // the other PC's
		ULONGLONG lastBriefSend = 0;
		ULONGLONG lastTickTime = 0;    // real time of the last executed tick (waiting text)
		bool wasRunning = false;       // the session level was running (to notice a local disconnect)
		char dropMessage[256] = {};    // set -> Com_Error(ERR_DROP) at the next stepper call
		constexpr DWORD UI_PlayerStart_f = 0x565330;
		constexpr DWORD UI_MENU_TYPE = 0x1E209B4; // g_currentMenuType, 5 = UIMENU_PREGAME
		constexpr int HOST_TIMEOUT = 10000;       // IW4 coop_hosttimeout

		Game::dvar_s* coop_port = nullptr;
		Game::dvar_s* coop_localbot = nullptr;
		Game::dvar_s* coop_localbotfire = nullptr;
		Game::dvar_s* coop_testgod = nullptr;

		int LevelTime()
		{
			return *reinterpret_cast<int*>(LEVEL_TIME);
		}

		// Tick the next server step executes: the stepper first runs SV_PreFrame (level.time += 50) when no frame is prepared
		// (0x14C30AC == 0, e.g. right after a load, where the last run frame was G_LoadLevel's).
		int NextTick()
		{
			return LevelTime() + (*reinterpret_cast<int*>(0x14C30AC) ? 0 : TICK);
		}

		bool LevelRunning()
		{
			const auto* running = Dvars::Functions::Dvar_FindVar("sv_running");
			return running && running->current.enabled;
		}

		const char* Addr(const sockaddr_in& a)
		{
			static char buf[64];
			snprintf(buf, sizeof(buf), "%s:%d", inet_ntoa(a.sin_addr), ntohs(a.sin_port));
			return buf;
		}

		bool Open(int port)
		{
			if (sock != INVALID_SOCKET)
				return true;
			WSADATA wsa;
			WSAStartup(MAKEWORD(2, 2), &wsa);
			sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (sock == INVALID_SOCKET)
			{
				Coop::Log("net: socket failed %d", WSAGetLastError());
				return false;
			}
			u_long nb = 1;
			ioctlsocket(sock, FIONBIO, &nb);
			int buf = 1 << 20;
			setsockopt(sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&buf), sizeof(buf));
			sockaddr_in a{};
			a.sin_family = AF_INET;
			a.sin_addr.s_addr = htonl(INADDR_ANY);
			a.sin_port = htons(static_cast<u_short>(port));
			if (bind(sock, reinterpret_cast<sockaddr*>(&a), sizeof(a)) == SOCKET_ERROR)
			{
				Coop::Log("net: bind to port %d failed %d", port, WSAGetLastError());
				closesocket(sock);
				sock = INVALID_SOCKET;
				return false;
			}
			sockaddr_in b{};
			int bl = sizeof(b);
			getsockname(sock, reinterpret_cast<sockaddr*>(&b), &bl);
			Coop::Log("net: UDP socket on port %d", ntohs(b.sin_port));
			return true;
		}

		void Close()
		{
			if (sock != INVALID_SOCKET)
				closesocket(sock);
			sock = INVALID_SOCKET;
		}

		Game::dvar_s* coop_droptest = nullptr;
		std::uint32_t dropRng = 12345;
		int droppedPackets = 0, sentPackets = 0;

		void SendRaw(const void* data, int len)
		{
			if (sock == INVALID_SOCKET || !havePeer)
				return;
			sentPackets++;
			// test: lose this share of the outgoing game packets (not OOB handshake text), own RNG (never the game's)
			if (coop_droptest && coop_droptest->current.integer > 0 && static_cast<const std::uint8_t*>(data)[0] == PROTOCOL)
			{
				dropRng = dropRng * 1103515245u + 12345u;
				if (static_cast<int>((dropRng >> 16) % 100) < coop_droptest->current.integer)
				{
					droppedPackets++;
					return;
				}
			}
			sendto(sock, static_cast<const char*>(data), len, 0, reinterpret_cast<const sockaddr*>(&peer), sizeof(peer));
		}

		void SendOOB(const std::string& text)
		{
			std::string p = "\xff\xff\xff\xff" + text;
			SendRaw(p.data(), static_cast<int>(p.size()));
		}

		void ResetSession()
		{
			frames.clear();
			partnerQueue.clear();
			partnerQueuedTime = INT_MIN;
			partnerAck = 0;
			hostHolding = false;
			partnerLoaded = false;
			hostSawFrames = false;
			lastExecuted = 0;
			hashOk = hashBad = missing = 0;
			firstBad = -1;
			lastLocalCmd = *reinterpret_cast<int*>(CL_CMDNUMBER);
			partnerRelQueue.clear();
			partnerRelQueued = INT_MIN;
			execRelSeq[0] = execRelSeq[1] = INT_MIN;
			*reinterpret_cast<int*>(CL_RELACK) = *reinterpret_cast<int*>(CL_RELSEQ); // commands from before the session are dropped
			localBriefDone = peerBriefDone = false;
			lastBriefSend = lastTickTime = 0;
			wasRunning = false;
		}

		// FNV-1a over the simulation state that must be equal on both PCs: tick, game RNG, every entity in use (number, origin, health).
		std::uint32_t StateHash()
		{
			std::uint32_t h = 2166136261u;
			auto mix = [&h](const void* p, size_t n)
			{
				const auto* b = static_cast<const std::uint8_t*>(p);
				for (size_t i = 0; i < n; i++)
					h = (h ^ b[i]) * 16777619u;
			};
			const int t = LevelTime();
			mix(&t, 4);
			mix(reinterpret_cast<void*>(HOLDRAND), 4);
			const int n = *reinterpret_cast<int*>(0xE18E24);
			for (int i = 0; i < n && i < 0x880; i++)
			{
				const std::uint8_t* e = Coop::Entity(i);
				if (!e[0xA8])
					continue;
				mix(&i, 4);
				mix(e + 0xE0, 12);  // r.currentOrigin
				mix(e + 0x144, 4);  // health
			}
			return h;
		}

		void Serialize(Frame& f)
		{
			auto& b = f.bytes;
			b.clear();
			auto put32 = [&b](std::uint32_t v) { for (int i = 0; i < 4; i++) b.push_back(static_cast<std::uint8_t>(v >> (8 * i))); };
			put32(static_cast<std::uint32_t>(f.time));
			put32(f.hash);
			for (int p = 0; p < 2; p++)
			{
				b.push_back(static_cast<std::uint8_t>(f.cmds[p].size()));
				for (const auto& c : f.cmds[p])
					b.insert(b.end(), c.begin(), c.end());
				b.push_back(static_cast<std::uint8_t>(f.rel[p].size()));
				for (const auto& [seq, text] : f.rel[p])
				{
					put32(static_cast<std::uint32_t>(seq));
					b.push_back(static_cast<std::uint8_t>(text.size()));
					b.push_back(static_cast<std::uint8_t>(text.size() >> 8));
					b.insert(b.end(), text.begin(), text.end());
				}
			}
		}

		bool Deserialize(const std::uint8_t* d, int len, Frame& f)
		{
			if (len < 10)
				return false;
			auto get32 = [d](int at) { return static_cast<std::uint32_t>(d[at] | (d[at + 1] << 8) | (d[at + 2] << 16) | (d[at + 3] << 24)); };
			f.time = static_cast<int>(get32(0));
			f.hash = get32(4);
			int at = 8;
			for (int p = 0; p < 2; p++)
			{
				if (at >= len)
					return false;
				const int n = d[at++];
				if (n > MAX_CMDS_PER_BLOCK || at + n * USERCMD > len)
					return false;
				f.cmds[p].clear();
				for (int i = 0; i < n; i++, at += USERCMD)
				{
					Cmd c;
					std::memcpy(c.data(), d + at, USERCMD);
					f.cmds[p].push_back(c);
				}
				if (at >= len)
					return false;
				const int nr = d[at++];
				f.rel[p].clear();
				for (int i = 0; i < nr; i++)
				{
					if (at + 6 > len)
						return false;
					const int seq = static_cast<int>(get32(at));
					const int tl = d[at + 4] | (d[at + 5] << 8);
					at += 6;
					if (tl > MAX_REL_TEXT || at + tl > len)
						return false;
					f.rel[p].emplace_back(seq, std::string(reinterpret_cast<const char*>(d + at), tl));
					at += tl;
				}
			}
			return true;
		}

		// host -> partner: [0x1d][1][serverId][n] n x { u16 len, frame }, oldest unacked frames first, as many as fit
		void SendFrames()
		{
			std::vector<std::uint8_t> pkt;
			auto start = [&pkt]()
			{
				pkt.assign({ static_cast<std::uint8_t>(PROTOCOL), 1 });
				for (int i = 0; i < 4; i++)
					pkt.push_back(static_cast<std::uint8_t>(LifeId() >> (8 * i)));
				pkt.push_back(0);
			};
			start();
			int packets = 0;
			for (auto& [t, f] : frames)
			{
				if (pkt.size() + 2 + f.bytes.size() > MAX_PACKET || pkt[6] == 20)
				{
					SendRaw(pkt.data(), static_cast<int>(pkt.size()));
					if (++packets >= 4)
						return;
					start();
				}
				pkt.push_back(static_cast<std::uint8_t>(f.bytes.size()));
				pkt.push_back(static_cast<std::uint8_t>(f.bytes.size() >> 8));
				pkt.insert(pkt.end(), f.bytes.begin(), f.bytes.end());
				pkt[6]++;
			}
			if (pkt[6])
				SendRaw(pkt.data(), static_cast<int>(pkt.size()));
		}

		// The local player's new usercmds from the client's ring (test bot fills them when coop_localbot is set).
		std::vector<Cmd> TakeLocalCmds(int self)
		{
			std::vector<Cmd> out;
			const int number = *reinterpret_cast<int*>(CL_CMDNUMBER);
			int from = lastLocalCmd + 1;
			if (number - from + 1 > MAX_CMDS_PER_BLOCK)
				from = number - MAX_CMDS_PER_BLOCK + 1;
			const char* bot = coop_localbot ? coop_localbot->current.string : "";
			for (int n = from; n <= number; n++)
			{
				Cmd c;
				std::memcpy(c.data(), reinterpret_cast<std::uint8_t*>(CL_CMDS) + (n & 63) * USERCMD, USERCMD);
				if (bot && bot[0])
					Coop::FillBotCmd(self, 1 - self, c.data(), bot, coop_localbotfire && coop_localbotfire->current.enabled);
				out.push_back(c);
			}
			lastLocalCmd = number;
			*reinterpret_cast<int*>(CL_CMDACK) = number;
			return out;
		}

		const char* ReliableText(int seq)
		{
			return reinterpret_cast<const char*>(0x819680 + ((seq & 0xFF) << 10)); // clc.reliableCommands[256][1024]
		}

		// host: its own new reliable commands go into frame block 0 (they run on both PCs when the frame runs)
		std::vector<std::pair<int, std::string>> TakeLocalReliable()
		{
			std::vector<std::pair<int, std::string>> out;
			int& seq = *reinterpret_cast<int*>(CL_RELSEQ);
			int& ack = *reinterpret_cast<int*>(CL_RELACK);
			while (ack != seq && static_cast<int>(out.size()) < MAX_REL_PER_BLOCK)
			{
				ack++;
				std::string text(ReliableText(ack));
				if (text.size() > MAX_REL_TEXT)
					text.resize(MAX_REL_TEXT);
				out.emplace_back(ack, std::move(text));
			}
			return out;
		}

		// partner -> host: [0x1d][0][serverId][ackTime][n] n x usercmd (its newest cmds; the host keeps the new ones)
		//                  then [m] m x { u32 seq, u16 len, text } (its reliable commands not yet executed)
		void SendInput()
		{
			std::vector<std::uint8_t> pkt{ static_cast<std::uint8_t>(PROTOCOL), 0 };
			auto put32 = [&pkt](std::uint32_t v) { for (int i = 0; i < 4; i++) pkt.push_back(static_cast<std::uint8_t>(v >> (8 * i))); };
			put32(LifeId());
			// newest contiguous frame held, counted from the tick this PC runs next (the pending level.time)
			int ack = NextTick() - TICK;
			while (frames.count(ack + TICK))
				ack += TICK;
			put32(static_cast<std::uint32_t>(ack));
			// newest cmds of the ring (resent each packet: losses heal, the host drops duplicates by serverTime)
			const int number = *reinterpret_cast<int*>(CL_CMDNUMBER);
			std::vector<Cmd> cmds;
			const char* bot = coop_localbot ? coop_localbot->current.string : "";
			for (int n = std::max(number - MAX_CMDS_PER_BLOCK + 1, 1); n <= number; n++)
			{
				Cmd c;
				std::memcpy(c.data(), reinterpret_cast<std::uint8_t*>(CL_CMDS) + (n & 63) * USERCMD, USERCMD);
				if (bot && bot[0])
					Coop::FillBotCmd(1, 0, c.data(), bot, coop_localbotfire && coop_localbotfire->current.enabled);
				cmds.push_back(c);
			}
			*reinterpret_cast<int*>(CL_CMDACK) = number;
			pkt.push_back(static_cast<std::uint8_t>(cmds.size()));
			for (const auto& c : cmds)
				pkt.insert(pkt.end(), c.begin(), c.end());
			const int seq = *reinterpret_cast<int*>(CL_RELSEQ);
			const int relAck = *reinterpret_cast<int*>(CL_RELACK); // set when a frame with the command runs here
			const int from = std::max(relAck + 1, seq - 7);
			pkt.push_back(static_cast<std::uint8_t>(seq - from + 1 > 0 ? seq - from + 1 : 0));
			for (int n = from; n <= seq; n++)
			{
				std::string text(ReliableText(n));
				if (text.size() > MAX_REL_TEXT)
					text.resize(MAX_REL_TEXT);
				put32(static_cast<std::uint32_t>(n));
				pkt.push_back(static_cast<std::uint8_t>(text.size()));
				pkt.push_back(static_cast<std::uint8_t>(text.size() >> 8));
				pkt.insert(pkt.end(), text.begin(), text.end());
			}
			SendRaw(pkt.data(), static_cast<int>(pkt.size()));
		}

		const DWORD clientCommandEnt0[] = { 0x4AD915, 0x4AD98D, 0x4AD9B9, 0x4AD9E5, 0x4ADA0E, 0x4ADA37, 0x4ADA60, 0x4ADA89, 0x4ADAB2,
			0x4ADADB, 0x4ADB22, 0x4ADB4E, 0x4ADB7A, 0x4ADBEE }; // PUSH / MOV EAX / MOV EDI imm32 0xC81418 (&g_entities[0])
		bool clientCommandSitesOk = false;
		char clientCommandText[1024];

		void CallClientCommand()
		{
			__asm
			{
				mov ecx, offset clientCommandText
				mov eax, 0x4AD860
				call eax
			}
		}

		void ClientCommandFor(int n, const std::string& text)
		{
			strncpy_s(clientCommandText, text.c_str(), _TRUNCATE);
			if (n != 0)
			{
				if (!clientCommandSitesOk)
				{
					Coop::Log("net: command of player %d NOT run (ClientCommand sites unknown): %s", n + 1, text.c_str());
					return;
				}
				for (const DWORD at : clientCommandEnt0)
					Utils::Hook::Set<DWORD>(at + 1, reinterpret_cast<DWORD>(Coop::Entity(n)));
			}
			CallClientCommand();
			if (n != 0)
				for (const DWORD at : clientCommandEnt0)
					Utils::Hook::Set<DWORD>(at + 1, 0xC81418);
		}

		void ExecuteInputs(const Frame& f);

		void Execute(const Frame& f)
		{
			// test only, set on BOTH PCs: players keep full health (done at the same point of the same tick on both, so it stays deterministic)
			if (coop_testgod && coop_testgod->current.enabled)
				for (int p = 0; p < 2; p++)
					if (*reinterpret_cast<int*>(Coop::Entity(p) + 0x144) > 0)
						*reinterpret_cast<int*>(Coop::Entity(p) + 0x144) = 100;
			// level scripts give the view hands only to level.player (setViewModel -> ps.viewmodelIndex +0xE0); without them player 2's
			// cgame gets no viewmodel DObj (CG_RegisterWeapon 0x430FF0) and crashes at 0x430A28 when he holds a weapon (run p2l).
			// Same hands for player 2, set at the same point of the tick on both PCs. (Campaign scripts will do it per level later.)
			{
				int& hands1 = *reinterpret_cast<int*>(Coop::GameClient(0) + 0xE0);
				int& hands2 = *reinterpret_cast<int*>(Coop::GameClient(1) + 0xE0);
				if (hands2 != hands1)
					hands2 = hands1;
			}
			ExecuteInputs(f);
		}

		void ExecuteInputs(const Frame& f)
		{
			for (int p = 0; p < 2; p++)
			{
				if (*reinterpret_cast<int*>(Coop::GameClient(p) + 0xB2D8) != 2)
					continue;
				for (const auto& [seq, text] : f.rel[p])
				{
					if (seq <= execRelSeq[p])
						continue;
					execRelSeq[p] = seq;
					Coop::Log("net: command of player %d (seq %d) at tick %d: %s", p + 1, seq, f.time, text.c_str());
					// QA (cheats only): "coopexit <map>" = what the script changelevel(map) does (GScr_ChangeLevel 0x4D2ED0), on both PCs
					// at this tick: map name 0xE0D988, flag 0xE198FC for G_RunFrame's level exit 0x4B9C60 -> 0x4B9E40 -> 0x4B9DC0
					const auto* cheats = Dvars::Functions::Dvar_FindVar("sv_cheats");
					if (text.rfind("coopexit ", 0) == 0 && cheats && cheats->current.enabled)
					{
						strncpy_s(reinterpret_cast<char*>(0xE0D988), 64, text.c_str() + 9, _TRUNCATE);
						*reinterpret_cast<int*>(0xE19908) = 0;
						*reinterpret_cast<int*>(0xE1990C) = 0;
						*reinterpret_cast<int*>(0xE198FC) = 1;
						continue;
					}
					// a key command notification of this player (CommandNotify_Hook)
					if (text.rfind("coopcmdn ", 0) == 0)
						Coop::CommandNotify(p, text.c_str() + 9);
					// QA (cheats only): "coophurt <damage>" = damage this player through G_Damage, on both PCs at this tick
					else if (text.rfind("coophurt ", 0) == 0 && cheats && cheats->current.enabled)
						Coop::Hurt(p, std::atoi(text.c_str() + 9));
					// QA (cheats only): "coopqaset <dvar> <value ...>" = set a coop_qa_* dvar on both PCs at this tick (the QA dvars level
					// scripts read: coop_qa_cmd, coop_qa_kh, ...; a dvar set on one PC alone would be a desync)
					else if (text.rfind("coopqaset coop_qa_", 0) == 0 && cheats && cheats->current.enabled)
					{
						const size_t sp = text.find(' ', 10);
						const std::string name = text.substr(10, sp == std::string::npos ? std::string::npos : sp - 10);
						std::string value = sp == std::string::npos ? "" : text.substr(sp + 1);
						if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
							value = value.substr(1, value.size() - 2);
						Dvars::Functions::Dvar_SetStringByName(name.c_str(), value.c_str());
					}
					else
						ClientCommandFor(p, text);
					if (p == CoopNet::LocalClient() && role == CoopNet::ROLE_PARTNER)
						*reinterpret_cast<int*>(CL_RELACK) = std::max(*reinterpret_cast<int*>(CL_RELACK), seq);
				}
				for (const auto& c : f.cmds[p])
				{
					const int st = *reinterpret_cast<const int*>(c.data());
					if (st > *reinterpret_cast<int*>(Coop::ServerClient(p) + 0x2410))
						Coop::Think(p, c.data());
				}
			}
		}

		// G_RunFrame state 1: the co-op frame for tick T = level.time.
		void __cdecl HandOver1()
		{
			if (!CoopNet::Active())
			{
				Utils::Hook::Call<void()>(HandOver_f)();
				return;
			}
			const int t = LevelTime();
			if (role == CoopNet::ROLE_HOST)
			{
				if (!partnerLoaded)
					return; // load-time frames (settle, G_LoadLevel): no input on either PC (reliable commands wait in the ring)
				Frame f;
				f.time = t;
				f.hash = StateHash();
				f.cmds[0] = TakeLocalCmds(0);
				f.rel[0] = TakeLocalReliable();
				f.rel[1] = partnerRelQueue;
				partnerRelQueue.clear();
				std::sort(partnerQueue.begin(), partnerQueue.end(), [](const Cmd& a, const Cmd& b)
				{
					return *reinterpret_cast<const int*>(a.data()) < *reinterpret_cast<const int*>(b.data());
				});
				if (partnerQueue.size() > MAX_CMDS_PER_BLOCK)
					partnerQueue.erase(partnerQueue.begin(), partnerQueue.end() - MAX_CMDS_PER_BLOCK);
				f.cmds[1] = partnerQueue;
				partnerQueue.clear();
				Serialize(f);
				Execute(f);
				lastTickTime = GetTickCount64();
				frames[t] = std::move(f);
				SendFrames();
				if (reloadTick && t >= reloadTick && !reloadPending)
				{
					reloadPending = true; // the host reloads once the partner holds this tick (Pump)
					reloadPendingSince = GetTickCount64();
					reloadTick = t;
				}
				return;
			}
			auto it = frames.find(t);
			if (it == frames.end())
			{
				if (hostSawFrames && missing++ < 20)
					Coop::Log("net: MISSING frame %d (executed up to %d)", t, lastExecuted);
				return;
			}
			const std::uint32_t mine = StateHash();
			if (mine == it->second.hash)
				hashOk++;
			else
			{
				hashBad++;
				if (firstBad < 0)
				{
					firstBad = t;
					Coop::Log("net: STATE HASH DIFFERS first at tick %d (host %08x, partner %08x)", t, it->second.hash, mine);
				}
			}
			Execute(it->second);
			lastExecuted = t;
			lastTickTime = GetTickCount64();
			if (reloadTick && t >= reloadTick)
			{
				reloadTick = t;
				reloadPending = true;
				QueueReload();
				reloadPending = true; // no more steps of this level life
			}
			frames.erase(frames.begin(), frames.upper_bound(t));
		}

		void __cdecl HandOver7()
		{
			if (!CoopNet::Active())
				Utils::Hook::Call<void()>(HandOver_f)();
		}

		// Time the server stepper may use this engine frame; -1 = skip it (held), like SV_Frame's cl_paused path.
		int __cdecl StepCap(int msec)
		{
			if (dropMessage[0])
			{
				static char msg[256];
				std::memcpy(msg, dropMessage, sizeof(msg));
				dropMessage[0] = 0;
				Game::Com_Error(1, "%s", msg); // ERR_DROP: back to the main menu with the message (longjmp)
			}
			if (!CoopNet::Active())
				return msec;
			if (auto* smoothing = *reinterpret_cast<Game::dvar_s**>(0x14C30B0))
				smoothing->current.enabled = false;
			const int t = NextTick();
			const int accum = *reinterpret_cast<int*>(STEP_ACCUM);
			bool skip = false;
			if (role == CoopNet::ROLE_HOST)
			{
				const int behind = static_cast<int>(frames.size());
				if (!hostHolding && behind >= MAX_BEHIND)
				{
					hostHolding = true;
					Coop::Log("net: host holds at tick %d, partner %d frame(s) behind (acked %d)", t, behind, partnerAck);
				}
				else if (hostHolding && behind < MIN_BEHIND)
				{
					hostHolding = false;
					Coop::Log("net: host resumes at tick %d", t);
				}
				skip = !partnerLoaded || hostHolding || reloadPending;
			}
			else
			{
				int held = 0;
				while (frames.count(t + held * TICK))
					held++;
				if (!held || reloadPending)
					skip = true;
				else
				{
					const int cap = held * TICK + (TICK - 1) - accum;
					msec = held > 2 ? cap : std::min(msec, cap); // fast forward when more than 100 ms behind (IW4)
					if (msec < 0)
						msec = 0;
				}
			}
			if (skip)
			{
				*reinterpret_cast<int*>(0x85BC10) = msec;
				*reinterpret_cast<int*>(0x85BC14) = 0;
				return -1;
			}
			return msec;
		}

		void __declspec(naked) Stepper_Stub()
		{
			__asm
			{
				push ecx
				push edx
				push eax
				call StepCap
				add esp, 4
				pop edx
				pop ecx
				cmp eax, -1
				je skip
				push 0x5C8360
				ret
			skip:
				ret
			}
		}

		// Server commands to one player (Phase 4). SP queues every server command in client 0's reliable buffer, and each frame
		// SV_SendClientMessages hands that buffer to the local client (CL_ParseCommandString), whatever player that client shows.
		// Most commands are the level's (configstrings "cs", sounds, level audio "reverb"/"eq" on level.player, ...): both PCs need them
		// and they stay as they are. The player-targeted ones come from SV_GameSendServerCommand(clientNum, ...) inlined in the
		// senders below, each with "TEST reg,reg; JA skip" before clients + clientNum * 0xD728, i.e. client 0 only: player 2 got none
		// of his, and the partner's client ran player 1's (his weapon switched with player 1's). For these 13 senders:
		// - the JA is NOPed (bytes checked first), so a command for client 1 is sent too;
		// - their call to SV_SendServerCommand 0x5C7F00 goes through TargetedSend_Stub, which marks the command as targeted;
		// - after formatting (0x5C7F2F, EDI = client) a targeted command for this PC's player goes to client 0's buffer (the one this
		//   PC's client reads), one for the other PC's player is dropped (his own PC sends it: same simulation).
		// Left as they were: the 5 audio senders (eq, reverb, deactivatereverb, setchannelvol, deactivatechannelvol: the levels call
		// them on level.player for the level's sound, so both PCs keep getting them) and the console print at 0x4AD607 (its branch
		// goes on into client connection code). Server commands reach only the clients: the simulation is the same on both PCs.
		constexpr DWORD TARGETED_SENDERS[][2] = { // { JA of the client-0-only guard, CALL 0x5C7F00 of the client path }
			{ 0x4AC640, 0x4AC654 }, // print
			{ 0x4AD048, 0x4AD05C }, // god
			{ 0x4AD0D8, 0x4AD0EC }, // demigod
			{ 0x4AD168, 0x4AD17C }, // notarget
			{ 0x4AD209, 0x4AD21D }, // noclip
			{ 0x4AD2A9, 0x4AD2BD }, // ufo
			{ 0x4AD367, 0x4AD37B }, // print
			{ 0x4AD581, 0x4AD599 }, // print
			{ 0x4B4BA5, 0x4B4BB9 }, // gm GAME_PICKUP_AMMO / CLIPONLY_AMMO
			{ 0x4B4E8C, 0x4B4EA0 }, // menu_show_notify
			{ 0x4B51D0, 0x4B51E4 }, // gm
			{ 0x4F1896, 0x4F18AC }, // sw (G_SelectWeapon: switchToWeapon, give)
			{ 0x4F18E6, 0x4F18FC }, // offhand (setOffhandSecondaryClass)
		};

		int targetedSend = 0;
		int routedLogged = 0;

		void __declspec(naked) TargetedSend_Stub()
		{
			__asm
			{
				mov targetedSend, 1
				push 0x5C7F00
				ret
			}
		}

		std::uint8_t* __cdecl ServerCommandTarget(std::uint8_t* cl, const char* text)
		{
			const bool targeted = targetedSend != 0;
			targetedSend = 0;
			if (!targeted || !cl || Coop::Players() < 2)
				return cl;
			auto* clients = *reinterpret_cast<std::uint8_t**>(0x14C2BA0);
			if (!clients)
				return cl;
			const auto offset = cl - clients;
			const int n = offset >= 0 && offset % 0xD728 == 0 ? static_cast<int>(offset / 0xD728) : -1;
			const int local = CoopNet::Active() ? CoopNet::LocalClient() : 0;
			const bool mine = n == local;
			// a one-process test bot has no client: it takes the weapon the server selects for it ("sw <index>")
			if (!mine && !CoopNet::Active() && n == 1 && text && !std::strncmp(text, "sw ", 3))
				Coop::SetBotWeaponSelect(std::atoi(text + 3));
			if (routedLogged < 200)
			{
				routedLogged++;
				Coop::Log("net: command for player %d (%s) at tick %d: %.80s", n + 1, mine ? "this PC" : "the other PC", Coop::LevelTimeNow(),
					text ? text : "");
			}
			return mine ? clients : reinterpret_cast<std::uint8_t*>(-1);
		}

		void __declspec(naked) ServerCommandTarget_Stub()
		{
			__asm
			{
				pushad
				lea eax, [esp + 0x24]  // the formatted text (the sender's buffer at [esp+4] before PUSHAD)
				push eax
				push edi
				call ServerCommandTarget
				add esp, 8
				mov [esp], eax      // popad loads it into EDI (pushed last)
				popad
				cmp edi, -1
				je drop
				test edi, edi       // the replaced code: TEST EDI,EDI / JZ 0x5C7F40 / MOV EAX,[EDI+8]
				jz all
				mov eax, [edi + 8]
				push 0x5C7F36
				ret
			all:
				push 0x5C7F40
				ret
			drop:
				push 0x5C7F89       // POP EDI, free the text buffer, RET
				ret
			}
		}

		// Key command notifications (player notifyOnCommand, used by the tutorial hints of every level and by killhouse's course: keyHint,
		// the sprint to the finish): SP runs them on this PC at once and always on entity 0 — Cmd_CheckNotify 0x531300, for a command
		// this PC executes (a key: "+attack", "+sprint", ...), calls G_AddCommandNotify 0x4A8570 at 0x531365, which notifies
		// g_entities[0] (Scr_NotifyNum 0x5571D0). In a session that was a desync (the partner's keys notified player 1 on the partner's
		// PC only). Now the notification is a reliable command of this PC's player ("coopcmdn <notify>") that the frames run on both
		// PCs at one tick, on that player's entity (ExecuteInputs -> Coop::CommandNotify).
		void __cdecl CommandNotify_Hook(unsigned int notify)
		{
			if (!CoopNet::Active())
			{
				Utils::Hook::Call<void(unsigned int)>(0x4A8570)(notify);
				return;
			}
			const char* name = Game::SL_ConvertToString(notify & 0xFFFF);
			if (!name || !*name)
				return;
			int& seq = *reinterpret_cast<int*>(CL_RELSEQ);
			const int ack = *reinterpret_cast<int*>(CL_RELACK);
			if (seq - ack >= 255)
				return; // the ring is full: drop this notification rather than a command
			seq++;
			char* slot = reinterpret_cast<char*>(0x819680 + ((seq & 0xFF) << 10)); // clc.reliableCommands[256][1024]
			_snprintf_s(slot, 1024, _TRUNCATE, "coopcmdn %s", name);
		}

		// Bodies (Phase 3, maps/_coop.gsc): every player has a body entity (script model) in the simulation; this PC's own player's body
		// is left out of this PC's snapshots (svFlags SVF_NOCLIENT 0x1 at +0xA6 only while the snapshot is built), so the camera is not
		// inside it. The script publishes the entity numbers in the dvars coop_body0 / coop_body1 (same on both PCs).
		int hiddenBodyEnt = -1;
		std::uint8_t hiddenBodyFlags = 0;

		void __cdecl BeforeSnapshot()
		{
			hiddenBodyEnt = -1;
			if (Coop::Players() < 2)
				return;
			const int local = snapshotOffset ? 1 : 0;
			const auto* d = Dvars::Functions::Dvar_FindVar(local ? "coop_body1" : "coop_body0");
			if (!d || d->type != Game::dvar_type::string || !d->current.string)
				return;
			const int n = std::atoi(d->current.string);
			if (n < 2 || n >= 0x880)
				return;
			std::uint8_t* e = Coop::Entity(n);
			if (!e[0xA8])
				return;
			hiddenBodyFlags = e[0xA6];
			e[0xA6] |= 1;
			hiddenBodyEnt = n;
		}

		void __cdecl AfterSnapshot()
		{
			if (hiddenBodyEnt >= 0)
				Coop::Entity(hiddenBodyEnt)[0xA6] = hiddenBodyFlags;
			hiddenBodyEnt = -1;
		}

		void __declspec(naked) Snapshot_Stub()
		{
			__asm
			{
				inc snapshotCalls
				cmp snapshotOffset, 0
				je go
				inc snapshotShifted
			go:
				pushad
				call BeforeSnapshot
				popad
				add ecx, snapshotOffset // a global: memory operand
				mov eax, 0x5C8A50
				call eax
				pushad
				call AfterSnapshot
				popad
				ret
			}
		}

		// ---- determinism guards (IW4-NETCODE-SPEC section 9): inputs that differ between PCs must not reach the simulation ----
		int sessionActive = 0; // mirror of CoopNet::Active() for naked stubs

		// Script getdvar / getdvarint / getdvarfloat: a player's saved settings (archive / userinfo flags: cg_fov, takecoverwarnings,
		// input_invertpitch, profile progress mis_*) differ per PC; in a session scripts see their default value on both PCs (IW4 gives
		// the script default for every dvar that is not read-only / gameplay / script-saved / cheat).
		template <DWORD Original>
		void __cdecl GetDvarGuard()
		{
			if (!sessionActive)
			{
				Utils::Hook::Call<void()>(Original)();
				return;
			}
			const char* name = Game::Scr_GetString(0);
			Game::dvar_s* dv = name ? Dvars::Functions::Dvar_FindVar(name) : nullptr;
			if (!dv || !(dv->flags & (Game::saved | Game::user_info)))
			{
				Utils::Hook::Call<void()>(Original)();
				return;
			}
			static std::unordered_set<std::string> logged;
			if (logged.insert(name).second)
				Coop::Log("net: script reads the player setting %s: default used in co-op", name);
			const Game::DvarValue keep = dv->current;
			dv->current = dv->reset;
			Utils::Hook::Call<void()>(Original)();
			dv->current = keep;
		}

		// Player method buttonPressed( key ) reads this PC's keyboard: 0 in a session (IW4 removed it; flares and
		// the debug menu use it: campaign scripts must read usercmd buttons instead).
		void __cdecl ButtonPressedGuard(Game::scr_entref_t entref)
		{
			if (!sessionActive)
			{
				Utils::Hook::Call<void(Game::scr_entref_t)>(0x4A8500)(entref);
				return;
			}
			static std::unordered_set<std::string> logged;
			const char* key = Game::Scr_GetString(0);
			if (key && logged.insert(key).second)
				Coop::Log("net: script buttonPressed(\"%s\") = 0 in co-op", key);
			Game::Scr_AddInt(0);
		}

		// FX visibility (smoke between an AI and its target, 0x473350): the smoke list comes from this PC's client FX -> 1.0 in a session
		// (IW4 0x22d160). fastcall start in ECX, end in EDI, result in ST0; one caller 0x4FA092.
		void __declspec(naked) FxVisibility_Stub()
		{
			__asm
			{
				cmp sessionActive, 0
				je stock
				fld1
				ret
			stock:
				push 0x473350
				ret
			}
		}

		// SV_Frame 0x5C8515 asks 0x570FC0 "a pausing menu is open?" and then pauses the world (cl_paused 1). In a session menus do not
		// pause once the level runs (IW4: cl_paused = !coop): the player with the menu open just stands (no moves from his client).
		// The briefing (pregame) still pauses: it is the load barrier.
		int __cdecl PausingMenuCheck()
		{
			const int open = Utils::Hook::Call<int()>(0x570FC0)();
			if (!sessionActive || !localBriefDone)
				return open;
			auto* paused = *reinterpret_cast<Game::dvar_s**>(0xF8CE50);
			if (paused && paused->current.integer)
				paused->current.integer = 0;
			return 0;
		}

		// Level exits issued by G_RunFrame state 1 (0x4B9E40 / 0x4B9DC0: spmap / spdevmap <next>, cinematic, loadgame_continue) at a tick
		// both PCs reach: the partner runs it at once; the host holds and runs it once the partner holds that tick (else the host
		// leaves the level before the partner got its last frames).
		void __cdecl DeferLevelCommand(const char* text)
		{
			pendingCmd = text ? text : "";
			if (!pendingCmd.empty() && pendingCmd.back() != '\n')
				pendingCmd += "\n";
			reloadTick = LevelTime();
			reloadPending = true;
			reloadPendingSince = GetTickCount64();
			Coop::Log("net: level exit at tick %d: %s", reloadTick, text ? text : "");
			if (role == CoopNet::ROLE_PARTNER)
			{
				QueueReload();
				reloadPending = true; // no more steps of this level life
			}
		}

		void __declspec(naked) LevelCommand_Stub()
		{
			__asm
			{
				cmp sessionActive, 0
				je stock
				pushad
				push eax
				call DeferLevelCommand
				add esp, 4
				popad
				ret
			stock:
				push 0x530320 // Cbuf_AddText(ECX local client, EAX text)
				ret
			}
		}

		// Difficulty: level.gameSkill = getdvarint("g_gameskill") (maps\_gameskill), not a saved setting for the getdvar guard, so
		// each PC read its own: different AI accuracy, damage, bleed-out time = two simulations (MW2 rule: the host's difficulty for
		// both). The host sends its g_gameskill in coopLaunch; the partner plays the session with it and gets its own back after.
		int partnerOwnSkill = -1;

		// g_gameskill is read only: the menus set it with the server commands difficultyEasy / Medium / Hard / Fu (SV_Difficulty*,
		// which also set player_damageMultiplier and player_deathInvulnerableTime for that skill)
		const char* SkillCommand(int skill)
		{
			static const char* const names[] = { "difficultyEasy", "difficultyMedium", "difficultyHard", "difficultyFu" };
			return names[skill < 0 ? 0 : skill > 3 ? 3 : skill];
		}

		int HostSkill()
		{
			const auto* d = Dvars::Functions::Dvar_FindVar("g_gameskill");
			return d ? d->current.integer : 1;
		}

		void UseHostSkill(int skill)
		{
			const auto* d = Dvars::Functions::Dvar_FindVar("g_gameskill");
			if (!d || skill < 0 || skill > 3)
				return;
			if (partnerOwnSkill < 0)
				partnerOwnSkill = d->current.integer;
			if (d->current.integer != skill)
			{
				Coop::Log("net: difficulty %d (the host's; this PC had %d)", skill, d->current.integer);
				Command::Execute(SkillCommand(skill), true);
			}
		}

		void RestoreOwnSkill()
		{
			if (partnerOwnSkill < 0)
				return;
			Command::Execute(SkillCommand(partnerOwnSkill), true);
			partnerOwnSkill = -1;
		}

		// End of the session: tell the other PC, stop. message: shown as an ERR_DROP error (back to the main menu) when a level runs.
		void EndSession(const char* reason, const char* message)
		{
			if (reason && havePeer)
				SendOOB(Utils::String::VA("coopDis %s", reason));
			Coop::Log("net: session over (%s)%s%s", reason ? reason : "ended by the other player", message ? ": " : "", message ? message : "");
			role = CoopNet::ROLE_OFF;
			launched = false;
			sessionActive = 0;
			RestoreOwnSkill();
			havePeer = false;
			snapshotOffset = 0;
			frames.clear();
			Close();
			if (message && LevelRunning())
				strncpy_s(dropMessage, message, _TRUNCATE);
		}

		// UI_PlayerStart: the pregame menu (level film) closes on this PC: film over, skipped by a click/key, or no film.
		void __cdecl PlayerStart_Hook()
		{
			Utils::Hook::Call<void()>(UI_PlayerStart_f)();
			if (CoopNet::Active() && !localBriefDone)
			{
				localBriefDone = true;
				lastBriefSend = 0;
				Coop::Log("net: briefing done here%s", peerBriefDone ? " (after the other player)" : "");
			}
		}

		void Launch()
		{
			launched = true;
			sessionActive = 1;
			levelEpoch = 0;
			ResetSession();
			// the partner's view = player 2 from the very first snapshot (SV_SpawnServer's), else cg.clientNum stays 0 (run p2c)
			snapshotOffset = role == CoopNet::ROLE_PARTNER ? 0xD728 : 0;
			lastRecv = GetTickCount64();
			Coop::Log("net: launching %s (serverId %08x, seed %d) as %s", mapName.c_str(), serverId, seed,
				role == CoopNet::ROLE_HOST ? "host" : "partner");
			Command::Execute(Utils::String::VA("devmap %s", mapName.c_str()), false);
		}

		void HandleOOB(const sockaddr_in& from, const std::string& text)
		{
			std::istringstream in(text);
			std::string cmd;
			in >> cmd;
			if (role == CoopNet::ROLE_HOST && cmd == "coopConnect")
			{
				int proto = 0;
				in >> proto;
				if (proto != PROTOCOL)
				{
					peer = from;
					havePeer = true;
					SendOOB("coopReject COOP_WRONGPROTOCOL");
					havePeer = false;
					return;
				}
				if (havePeer && (peer.sin_addr.s_addr != from.sin_addr.s_addr || peer.sin_port != from.sin_port) && launched)
				{
					const sockaddr_in keep = peer;
					peer = from;
					SendOOB("coopReject COOP_FULL");
					peer = keep;
					return;
				}
				peer = from;
				havePeer = true;
				Coop::Log("net: coopConnect from %s", Addr(from));
				SendOOB(Utils::String::VA("coopAccept 1 %s %u", mapName.c_str(), serverId));
				SendOOB(Utils::String::VA("coopLaunch %s %u %d %d", mapName.c_str(), serverId, seed, HostSkill()));
				if (!launched)
					Launch();
				return;
			}
			if (role == CoopNet::ROLE_HOST && cmd == "coopLoaded")
			{
				std::uint32_t id = 0;
				in >> id;
				if (id == LifeId() && !partnerLoaded)
				{
					partnerLoaded = true;
					Coop::Log("net: partner loaded, simulation starts at tick %d", NextTick());
				}
				return;
			}
			if (role == CoopNet::ROLE_PARTNER && cmd == "coopAccept")
			{
				if (!accepted)
					Coop::Log("net: accepted by %s", Addr(from));
				accepted = true;
				return;
			}
			if (role == CoopNet::ROLE_PARTNER && cmd == "coopLaunch")
			{
				std::string map;
				std::uint32_t id = 0;
				int s = 0;
				int skill = -1;
				in >> map >> id >> s >> skill;
				accepted = true;
				UseHostSkill(skill);
				if (launched && id == serverId)
					return;
				mapName = map;
				serverId = id;
				seed = s;
				Launch();
				return;
			}
			if (cmd == "coopBriefDone")
			{
				std::uint32_t id = 0;
				in >> id;
				if (id == LifeId() && !peerBriefDone)
				{
					peerBriefDone = true;
					Coop::Log("net: the other player's briefing is done");
				}
				return;
			}
			if (cmd == "coopHb")
				return;
			if (cmd == "coopMap" && role == CoopNet::ROLE_PARTNER)
			{
				std::string map;
				std::uint32_t life = 0;
				in >> map >> life;
				if (life > hostMapLife)
				{
					hostMap = map;
					hostMapLife = life;
					hostMapSeen = GetTickCount64();
				}
				return;
			}
			if (cmd == "coopReject" || cmd == "coopDis")
			{
				std::string reason;
				in >> reason;
				Coop::Log("net: %s %s from %s", cmd.c_str(), reason.c_str(), Addr(from));
				if (cmd == "coopDis" && CoopNet::Active())
					EndSession(nullptr, "The other player left the co-op game.");
				return;
			}
		}

		void HandleBinary(const std::uint8_t* d, int len)
		{
			if (len < 6 || d[0] != PROTOCOL)
				return;
			const std::uint32_t id = static_cast<std::uint32_t>(d[2] | (d[3] << 8) | (d[4] << 16) | (d[5] << 24));
			if (id != LifeId())
				return; // another level life (the other PC has not reloaded yet, or late packets of the last one)
			if (role == CoopNet::ROLE_HOST && d[1] == 0 && len >= 11)
			{
				const int ack = static_cast<int>(d[6] | (d[7] << 8) | (d[8] << 16) | (d[9] << 24));
				if (ack > partnerAck)
				{
					partnerAck = ack;
					frames.erase(frames.begin(), frames.upper_bound(ack));
				}
				const int n = d[10];
				for (int i = 0, at = 11; i < n && at + USERCMD <= len; i++, at += USERCMD)
				{
					const int st = *reinterpret_cast<const int*>(d + at);
					if (st <= partnerQueuedTime)
						continue;
					Cmd c;
					std::memcpy(c.data(), d + at, USERCMD);
					partnerQueue.push_back(c);
					partnerQueuedTime = st;
				}
				int at = 11 + n * USERCMD;
				if (at < len)
				{
					const int m = d[at++];
					for (int i = 0; i < m && at + 6 <= len; i++)
					{
						const int seq = static_cast<int>(d[at] | (d[at + 1] << 8) | (d[at + 2] << 16) | (d[at + 3] << 24));
						const int tl = d[at + 4] | (d[at + 5] << 8);
						at += 6;
						if (tl > MAX_REL_TEXT || at + tl > len)
							break;
						if (seq > partnerRelQueued && static_cast<int>(partnerRelQueue.size()) < MAX_REL_PER_BLOCK)
						{
							partnerRelQueue.emplace_back(seq, std::string(reinterpret_cast<const char*>(d + at), tl));
							partnerRelQueued = seq;
						}
						at += tl;
					}
				}
				return;
			}
			if (role == CoopNet::ROLE_PARTNER && d[1] == 1 && len >= 7)
			{
				const int n = d[6];
				int at = 7;
				for (int i = 0; i < n && at + 2 <= len; i++)
				{
					const int fl = d[at] | (d[at + 1] << 8);
					at += 2;
					if (at + fl > len)
						break;
					Frame f;
					if (Deserialize(d + at, fl, f) && f.time > lastExecuted && !frames.count(f.time))
					{
						if (!hostSawFrames)
							Coop::Log("net: first frame %d", f.time);
						hostSawFrames = true;
						f.bytes.assign(d + at, d + at + fl);
						frames[f.time] = std::move(f);
					}
					at += fl;
				}
			}
		}

		void Pump()
		{
			sessionActive = CoopNet::Active() ? 1 : 0;
			// IW3SP-MOD's own code reads the local player's game client through Game::g_clients (stock 0xE0DA00, no longer used since
			// level.clients is a 2-entry array of ours): point it at this PC's player.
			Game::g_clients = *reinterpret_cast<void**>(0xE18E18) ? reinterpret_cast<Game::gclient_s*>(Coop::GameClient(CoopNet::LocalClient()))
				: reinterpret_cast<Game::gclient_s*>(0xE0DA00); // no level yet: the stock (zeroed) storage, as before
			snapshotOffset = (CoopNet::Active() && role == CoopNet::ROLE_PARTNER) ? 0xD728 : 0;
			if (role == CoopNet::ROLE_OFF || sock == INVALID_SOCKET)
				return;
			std::uint8_t buf[65536];
			for (;;)
			{
				sockaddr_in from{};
				int fl = sizeof(from);
				const int len = recvfrom(sock, reinterpret_cast<char*>(buf), sizeof(buf), 0, reinterpret_cast<sockaddr*>(&from), &fl);
				if (len <= 0)
					break;
				lastRecv = GetTickCount64();
				if (len >= 4 && *reinterpret_cast<std::uint32_t*>(buf) == 0xFFFFFFFF)
					HandleOOB(from, std::string(reinterpret_cast<char*>(buf) + 4, len - 4));
				else if (havePeer && from.sin_addr.s_addr == peer.sin_addr.s_addr && from.sin_port == peer.sin_port)
					HandleBinary(buf, len);
			}
			const ULONGLONG now = GetTickCount64();
			if (CoopNet::Active())
			{
				const bool running = LevelRunning();
				if (running)
				{
					wasRunning = true;
					notRunningSince = 0;
				}
				else if (wasRunning)
				{
					// a reload stops the server for a moment; gone for 3 s = this player left the level (menu, quit, another map)
					if (!notRunningSince)
						notRunningSince = now;
					else if (now - notRunningSince > 3000)
					{
						EndSession("COOP_LEFT", nullptr);
						return;
					}
				}
				// a level life without a briefing (save load, death reload): ready once the client runs unpaused
				if (!localBriefDone && running && *reinterpret_cast<int*>(0x85BD7C) == 4 && *reinterpret_cast<int*>(UI_MENU_TYPE) != 5 &&
					!Utils::Hook::Call<int()>(0x570FC0)() && !(*reinterpret_cast<Game::dvar_s**>(0xF8CE50))->current.integer)
				{
					localBriefDone = true;
					Coop::Log("net: ready here (no briefing in this level life)");
				}
				// shared briefing: done on one PC = skipped on the other (owner's rule from MW2)
				if (peerBriefDone && !localBriefDone && *reinterpret_cast<int*>(UI_MENU_TYPE) == 5)
				{
					Coop::Log("net: skipping the briefing here, the other player is done");
					PlayerStart_Hook();
				}
				const bool started = role == CoopNet::ROLE_HOST ? partnerLoaded : hostSawFrames;
				if (localBriefDone && !started && now - lastBriefSend > 500)
				{
					lastBriefSend = now;
					SendOOB(Utils::String::VA("coopBriefDone %u", LifeId()));
				}
				// the other PC silent: 90 s while loading (IW4 coop_maxServerLoadWaitTime), 10 s once both run (coop_hosttimeout)
				const ULONGLONG limit = started ? HOST_TIMEOUT : 90000;
				if (lastRecv && now - lastRecv > limit)
				{
					EndSession("COOP_TIMEOUT", "The connection with the other player was lost.");
					return;
				}
				static ULONGLONG lastHb = 0;
				if (now - lastHb >= 500)
				{
					lastHb = now;
					SendOOB("coopHb");
					if (role == CoopNet::ROLE_HOST)
					{
						const auto* mapname = Dvars::Functions::Dvar_FindVar("mapname");
						SendOOB(Utils::String::VA("coopMap %s %u", mapname ? mapname->current.string : "", LifeId()));
					}
				}
				// the host is in a newer level life of another map and this PC did not follow by itself within 3 s: load that map
				if (role == CoopNet::ROLE_PARTNER && hostMapLife > LifeId() && !hostMap.empty() && now - hostMapSeen > 3000)
				{
					const auto* mapname = Dvars::Functions::Dvar_FindVar("mapname");
					if (mapname && hostMap != mapname->current.string)
					{
						Coop::Log("net: the host changed the map to %s: loading it", hostMap.c_str());
						const auto* cheats = Dvars::Functions::Dvar_FindVar("sv_cheats");
						Game::Cbuf_AddText(0, Utils::String::VA("%s %s\n", cheats && cheats->current.enabled ? "spdevmap" : "spmap", hostMap.c_str()));
					}
					hostMapSeen = now; // once per 3 s at most
				}
			}
			if (role == CoopNet::ROLE_PARTNER)
			{
				if (!accepted && now - lastConnectSend > 500)
				{
					lastConnectSend = now;
					SendOOB(Utils::String::VA("coopConnect %d", PROTOCOL));
				}
				// loaded = the session's level runs here with both players spawned
				const bool loaded = launched && LevelRunning() && Coop::Players() == 2 && Coop::Entity(1)[0xA8];
				if (loaded && localBriefDone && !hostSawFrames && now - lastLaunchSend > 500)
				{
					lastLaunchSend = now;
					SendOOB(Utils::String::VA("coopLoaded %u", LifeId()));
				}
				if (launched && LevelRunning() && now - lastInputSend >= 10)
				{
					lastInputSend = now;
					SendInput();
				}
			}
			else if (role == CoopNet::ROLE_HOST && launched && havePeer)
			{
				if (reloadPending && reloadTick && (partnerAck >= reloadTick || now - reloadPendingSince > 5000))
				{
					if (partnerAck < reloadTick)
						Coop::Log("net: partner did not confirm tick %d in 5 s, reloading anyway", reloadTick);
					QueueReload();
					reloadPending = true; // held until the new level life
				}
				if (!partnerLoaded && now - lastLaunchSend > 1000)
				{
					lastLaunchSend = now;
					SendOOB(Utils::String::VA("coopLaunch %s %u %d %d", mapName.c_str(), serverId, seed, HostSkill()));
				}
				if (!frames.empty() && now - lastResend >= 30)
				{
					lastResend = now;
					SendFrames();
				}
			}
			if (launched && LevelRunning() && now - lastStatus >= 2000)
			{
				lastStatus = now;
				const float* o0 = reinterpret_cast<float*>(Coop::Entity(0) + 0xE0);
				const float* o1 = reinterpret_cast<float*>(Coop::Entity(1) + 0xE0);
				const auto* pps = reinterpret_cast<std::uint8_t*>(0x714BA8); // cg.predictedPlayerState
				const auto* snapP = *reinterpret_cast<std::uint8_t**>(0x6FA5B4); // cg.snap / cg.nextSnap (cg_s at 0x6FA590)
				const auto* nextP = *reinterpret_cast<std::uint8_t**>(0x6FA5B8);
				Coop::Log("net: view: skipRendering %d menuType %d cl_paused %d connState %d | cg.time %d snap %d next %d frameInterp %.2f",
					*reinterpret_cast<std::uint8_t*>(0x1E20960), *reinterpret_cast<int*>(UI_MENU_TYPE),
					(*reinterpret_cast<Game::dvar_s**>(0xF8CE50))->current.integer, *reinterpret_cast<int*>(0x85BD7C),
					*reinterpret_cast<int*>(0x714B9C), snapP ? *reinterpret_cast<const int*>(snapP + 8) : -1, nextP ? *reinterpret_cast<const int*>(nextP + 8) : -1,
					*reinterpret_cast<float*>(0x714B90));
				Coop::Log("net: view: refdef vieworg (%.0f %.0f %.0f) yaw %.0f (cg_s layout %s)", Game::cgs->refdef.vieworg[0],
					Game::cgs->refdef.vieworg[1], Game::cgs->refdef.vieworg[2], Game::cgs->refdefViewAngles[1],
					offsetof(Game::cg_s, predictedPlayerState) == 0x1A618 ? "ok" : "MISMATCH");
				Coop::Log("net: view: snapshots %d (to client 1: %d), cg.clientNum %d, offset 0x%x | cg pps clientNum %d commandTime %d origin (%.0f %.0f %.0f)",
					snapshotCalls, snapshotShifted, *reinterpret_cast<int*>(0x6FA590), snapshotOffset, *reinterpret_cast<const int*>(pps + 0xB8),
					*reinterpret_cast<const int*>(pps), reinterpret_cast<const float*>(pps + 0x1C)[0], reinterpret_cast<const float*>(pps + 0x1C)[1],
					reinterpret_cast<const float*>(pps + 0x1C)[2]);
				Coop::Log("net: %s tick %d frames %zu ack %d hold %d | hash ok %d bad %d missing %d | p1 (%.0f %.0f %.0f) hp %d p2 (%.0f %.0f %.0f) hp %d | silent %llu ms | dropped %d/%d",
					role == CoopNet::ROLE_HOST ? "host" : "partner", LevelTime(), frames.size(), role == CoopNet::ROLE_HOST ? partnerAck : lastExecuted,
					hostHolding, hashOk, hashBad, missing, o0[0], o0[1], o0[2], *reinterpret_cast<int*>(Coop::Entity(0) + 0x144),
					o1[0], o1[1], o1[2], *reinterpret_cast<int*>(Coop::Entity(1) + 0x144), lastRecv ? now - lastRecv : 0ULL, droppedPackets, sentPackets);
			}
		}

		bool ParseAddr(const std::string& s, sockaddr_in& out)
		{
			std::string host = s;
			int port = 28960;
			const size_t c = s.find(':');
			if (c != std::string::npos)
			{
				host = s.substr(0, c);
				port = std::atoi(s.c_str() + c + 1);
			}
			out = {};
			out.sin_family = AF_INET;
			out.sin_port = htons(static_cast<u_short>(port));
			out.sin_addr.s_addr = inet_addr(host.c_str());
			return out.sin_addr.s_addr != INADDR_NONE;
		}
	}

	// "Waiting for the other player" over the game while this PC's simulation waits (no tick for 0.5 s after the briefing).
	namespace
	{
		int DvarInt(const char* name)
		{
			const auto* d = Dvars::Functions::Dvar_FindVar(name);
			if (!d)
				return 0;
			if (d->type == Game::dvar_type::string)
				return d->current.string ? std::atoi(d->current.string) : 0;
			if (d->type == Game::dvar_type::integer)
				return d->current.integer;
			return 0;
		}

		void DrawCentered(const char* text, float y, float scale, const float* color)
		{
			const float glow[4] = { 0.2f, 0.3f, 0.6f, 0.6f };
			auto* const placement = Game::ScrPlace_GetUnsafeFullPlacement();
			Game::Font_s* font = Game::R_RegisterFont("fonts/objectiveFont", sizeof("fonts/objectiveFont"));
			const float width = static_cast<float>(Game::UI_TextWidth(text, 0, font, scale));
			Game::UI_DrawTextWithGlow(1, 1, placement, text, font, 320.0f - width / 2.0f, y, scale, color, 3, glow, 0, 0);
		}

		// Downed and revive texts like IW4 Spec Ops (maps\_coop.gsc player_coop_downed_hud: "Bleeding out" / partner text and a timer
		// below the centre (IW4: 75 units; here 100, under CoD4's own use hints such as "Press [Use] to swap for ..."), "Being revived..." / "Reviving partner..." while the Use button is held). maps/_coop.gsc publishes
		// coop_downtime<n> = seconds left and coop_revive<n> = revive percent; each PC shows its own player's side.
		void DrawDowned()
		{
			if (Coop::Players() < 2 || !LevelRunning())
				return;
			if (*reinterpret_cast<int*>(0xE19904)) // level.missionFailed (GScr_MissionFailed 0x4D3080): the fail screen is up
				return;
			const int me = CoopNet::Active() ? CoopNet::LocalClient() : 0;
			const float white[4] = { 1.0f, 1.0f, 1.0f, 0.95f };
			for (const int n : { me, 1 - me })
			{
				if (!Coop::Downed(n))
					continue;
				const int left = DvarInt(n ? "coop_downtime1" : "coop_downtime0");
				const int revive = DvarInt(n ? "coop_revive1" : "coop_revive0");
				// IW4 colours the text by the time left (stages: yellow, then red near the end)
				const float yellow[4] = { 1.0f, 0.85f, 0.2f, 0.95f };
				const float red[4] = { 1.0f, 0.3f, 0.25f, 0.95f };
				const float* color = left > 20 ? yellow : red;
				const bool mine = n == me;
				if (revive > 0)
				{
					DrawCentered(mine ? "Being revived..." : "Reviving partner...", 340.0f, 0.45f, white);
					DrawCentered(Utils::String::VA("%d%%", revive), 365.0f, 0.4f, white);
				}
				else
				{
					DrawCentered(mine ? "Bleeding out" : "Partner bleeding out", 340.0f, 0.45f, color);
					DrawCentered(Utils::String::VA("%d:%02d", left / 60, left % 60), 365.0f, 0.4f, color);
					if (!mine)
						DrawCentered("Hold Use next to your partner to revive", 387.0f, 0.35f, white);
				}
				return; // one downed player at a time (both down = mission failed)
			}
		}
	}

	void CoopNet::DrawOverlay()
	{
		DrawDowned();
		if (!Active() || !localBriefDone || !LevelRunning())
			return;
		if (lastTickTime && GetTickCount64() - lastTickTime < 500)
			return;
		const float color[4] = { 1.0f, 1.0f, 1.0f, 0.9f };
		DrawCentered("Waiting for the other player...", 250.0f, 0.45f, color);
	}

	// G_InitGame in a session: a new level life (launch, death reload, loadgame, map_restart). Both PCs run it for the same reasons
	// (same simulation), so their epochs agree; the lockstep restarts with a new load barrier.
	void CoopNet::OnLevelStart()
	{
		if (!Active())
			return;
		levelEpoch++;
		reloadTick = 0;
		reloadPending = false;
		frames.clear();
		partnerQueue.clear();
		partnerQueuedTime = INT_MIN;
		partnerRelQueue.clear();
		partnerRelQueued = INT_MIN;
		partnerAck = 0;
		hostHolding = false;
		partnerLoaded = false;
		hostSawFrames = false;
		lastExecuted = 0;
		lastTickTime = 0;
		localBriefDone = peerBriefDone = false;
		lastBriefSend = 0;
		lastLocalCmd = *reinterpret_cast<int*>(CL_CMDNUMBER);
		lastRecv = GetTickCount64();
		Coop::Log("net: level life %u starts (%s)", levelEpoch, role == ROLE_HOST ? "host" : "partner");
	}

	// Player 1 died and 0x4A6050 armed the SP death reload with a real-time timer (0xE19878, read by 0x4B9E40): in a session the timer
	// is cleared and the reload happens at the same simulation tick on both PCs (death + g_deathDelay).
	void CoopNet::OnDeathReloadArmed()
	{
		if (!Active())
			return;
		*reinterpret_cast<int*>(0xE19878) = 0;
		if (reloadTick || reloadPending)
			return;
		const int delay = (*reinterpret_cast<Game::dvar_s**>(0xDCF7A0))->current.integer;
		reloadTick = LevelTime() + delay;
		Coop::Log("net: death reload armed at tick %d (player 1 died or mission failed), reload at tick %d", LevelTime(), reloadTick);
	}

	int CoopNet::Role()
	{
		return role;
	}

	int CoopNet::LocalClient()
	{
		return role == ROLE_PARTNER ? 1 : 0;
	}

	bool CoopNet::Active()
	{
		return role != ROLE_OFF && launched;
	}

	int CoopNet::Seed()
	{
		return seed;
	}

	void CoopNet::Install()
	{
		Events::OnDvarInit([]
		{
			coop_port = Dvars::Register::Dvar_RegisterInt("coop_port", "UDP port of the co-op session (host 28960; partner 0 = any)", 28960, 0, 65535, Game::none);
			coop_localbot = Dvars::Register::Dvar_RegisterString("coop_localbot", "Test: a bot drives this PC's player in a session (follow, front, walk, stand; empty = off)", "", Game::none);
			coop_localbotfire = Dvars::Register::Dvar_RegisterBool("coop_localbotfire", "Test: this PC's bot fires every 4 s", false, Game::none);
			coop_droptest = Dvars::Register::Dvar_RegisterInt("coop_droptest", "Test: percent of outgoing game packets to drop", 0, 0, 90, Game::none);
			coop_testgod = Dvars::Register::Dvar_RegisterBool("coop_testgod", "Test, set on both PCs: both players keep full health", false, Game::none);
		});

		Command::Add("coop_host", [](Command::Params* params)
		{
			if (params->size() < 2)
			{
				Coop::Log("usage: coop_host <map>");
				return;
			}
			if (!Open(coop_port ? coop_port->current.integer : 28960))
				return;
			role = ROLE_HOST;
			mapName = params->get(1);
			seed = static_cast<int>(timeGetTime());
			serverId = static_cast<std::uint32_t>(seed) * 2654435761u | 1;
			launched = false;
			havePeer = false;
			Coop::Log("net: hosting %s, waiting for a partner (seed %d)", mapName.c_str(), seed);
		});

		Command::Add("coop_connect", [](Command::Params* params)
		{
			if (params->size() < 2 || !ParseAddr(params->get(1), peer))
			{
				Coop::Log("usage: coop_connect <ip>[:port]");
				return;
			}
			if (!Open(coop_port ? coop_port->current.integer : 0))
				return;
			role = ROLE_PARTNER;
			havePeer = true;
			accepted = false;
			launched = false;
			lastConnectSend = 0;
			Coop::Log("net: connecting to %s", Addr(peer));
		});

		// QA / menus: close this PC's briefing (as a click on it would)
		Command::Add("coop_skipbriefing", []
		{
			if (*reinterpret_cast<int*>(UI_MENU_TYPE) == 5)
				PlayerStart_Hook();
			else
				Coop::Log("net: coop_skipbriefing: no briefing open (menu type %d)", *reinterpret_cast<int*>(UI_MENU_TYPE));
		});

		Command::Add("coop_stop", []
		{
			EndSession("COOP_STOPPED", nullptr);
		});

		Utils::Hook(0x4A3F6A, HandOver1, HOOK_CALL).install()->quick();     // G_RunFrame state 1 input hand-over
		Utils::Hook(0x4BB11D, HandOver7, HOOK_CALL).install()->quick();     // G_RunFrame state 7 input hand-over
		Utils::Hook(0x5C85BD, Stepper_Stub, HOOK_CALL).install()->quick();  // SV_Frame -> 50 ms stepper
		Utils::Hook(0x5C74EA, Snapshot_Stub, HOOK_CALL).install()->quick(); // SV_SpawnServer
		Utils::Hook(0x5C82E7, Snapshot_Stub, HOOK_CALL).install()->quick(); // per server frame
		Utils::Hook(0x5C6773, Snapshot_Stub, HOOK_CALL).install()->quick(); // SV_CheckLoadLevel

		clientCommandSitesOk = true;
		for (const DWORD at : clientCommandEnt0)
		{
			const BYTE op = *reinterpret_cast<BYTE*>(at);
			if ((op != 0x68 && op != 0xB8 && op != 0xBF) || *reinterpret_cast<DWORD*>(at + 1) != 0xC81418)
				clientCommandSitesOk = false;
		}
		Coop::Log("net: ClientCommand entity sites %s", clientCommandSitesOk ? "OK" : "DO NOT MATCH (player 2 commands off)");

		// determinism guards: script function table entries {name, function, developer} and the FX visibility call
		Utils::Hook::Set<void*>(0x6DDE80, reinterpret_cast<void*>(GetDvarGuard<0x4C85C0>)); // getdvar
		Utils::Hook::Set<void*>(0x6DDE8C, reinterpret_cast<void*>(GetDvarGuard<0x4C8630>)); // getdvarint
		Utils::Hook::Set<void*>(0x6DDEB0, reinterpret_cast<void*>(GetDvarGuard<0x4C8630>)); // (developer alias)
		Utils::Hook::Set<void*>(0x6DDE98, reinterpret_cast<void*>(GetDvarGuard<0x4C86D0>)); // getdvarfloat
		Utils::Hook::Set<void*>(0x6DDEBC, reinterpret_cast<void*>(GetDvarGuard<0x4C86D0>)); // (developer alias)
		Utils::Hook::Set<void*>(0x6818F4, reinterpret_cast<void*>(ButtonPressedGuard));    // player method buttonpressed
		Utils::Hook(0x4FA092, FxVisibility_Stub, HOOK_CALL).install()->quick();
		Utils::Hook(0x5C8515, PausingMenuCheck, HOOK_CALL).install()->quick();  // menus do not pause a session
		Utils::Hook(0x531365, CommandNotify_Hook, HOOK_CALL).install()->quick(); // key command notifications through the frames
		// player-targeted server commands for player 2 too, each to the PC of its player
		{
			// all or nothing: guards opened without the routing would fill client 1's buffer until "Server command overflow"
			int matched = 0;
			for (const auto& t : TARGETED_SENDERS)
			{
				const auto* ja = reinterpret_cast<const std::uint8_t*>(t[0]);
				const auto* call = reinterpret_cast<const std::uint8_t*>(t[1]);
				if (ja[0] == 0x77 && call[0] == 0xE8 && t[1] + 5 + *reinterpret_cast<const std::int32_t*>(call + 1) == 0x5C7F00)
					matched++;
			}
			const auto* t = reinterpret_cast<const std::uint8_t*>(0x5C7F2F);
			if (matched == static_cast<int>(std::size(TARGETED_SENDERS)) && t[0] == 0x85 && t[1] == 0xFF && t[2] == 0x74)
			{
				Utils::Hook(0x5C7F2F, ServerCommandTarget_Stub, HOOK_JUMP).install()->quick();
				for (const auto& s : TARGETED_SENDERS)
				{
					Utils::Hook(s[1], TargetedSend_Stub, HOOK_CALL).install()->quick();
					Utils::Hook::Nop(s[0], 2);
				}
			}
			else
				Coop::Log("net: server command routing NOT installed (%d of %zu senders matched)", matched, std::size(TARGETED_SENDERS));
		}
		for (const DWORD at : { 0x4B9E9Du, 0x4B9EDBu }) // level exit Cbuf_AddText calls in 0x4B9E40
			Utils::Hook(at, LevelCommand_Stub, HOOK_CALL).install()->quick();
		for (const DWORD at : { 0x4B9DD0u, 0x4B9E1Au, 0x4B9E33u }) // tail JMPs to Cbuf_AddText in 0x4B9DC0 (a CALL there fell through: run p2q)
			Utils::Hook(at, LevelCommand_Stub, HOOK_JUMP).install()->quick();

		// every caller of UI_PlayerStart: Com_Error path, UI_Refresh (film over), uiScript playerstart (skip), SV_SpawnServer, UI_SetActiveMenu
		for (const DWORD at : { 0x533657u, 0x5653A0u, 0x5671DAu, 0x5C76C4u, 0x567F70u })
			Utils::Hook(at, PlayerStart_Hook, HOOK_CALL).install()->quick();

		Scheduler::Loop(Pump, Scheduler::Pipeline::MAIN);

		// the game quits: tell the other PC at once (else it notices after coop_hosttimeout)
		Scheduler::OnGameShutdown([]
		{
			if (CoopNet::Active())
				EndSession("COOP_QUIT", nullptr);
		});
	}
}
