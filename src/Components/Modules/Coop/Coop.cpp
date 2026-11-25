#include "STDInc.hpp"
#include "CoopScripts.hpp"
#include "CoopQA.hpp"
#include "CoopNet.hpp"

// Co-op Phase 1: a second player in one SP process.
// Addresses are for iw3sp_mod.exe of IW3SP-MOD v4.1.5 (image base 0x400000). Reverse engineering notes: local/cod4/PORT-PLAN.md,
// local/cod4/re/iw3-counterparts.md (not in the repo). Offsets are raw on purpose: they were read from the exe, the mod's
// Structs.hpp is not checked field by field.

namespace Components
{
	namespace
	{
		// g_entities
		constexpr DWORD G_ENTITIES = 0xC81418;
		constexpr int ENT_SIZE = 0x274;
		constexpr int ENT_ORIGIN = 0xE0;     // r.currentOrigin
		constexpr int ENT_INUSE = 0xA8;      // r.inuse
		constexpr int ENT_CLIENT = 0x100;
		constexpr int ENT_SENTIENT = 0x108;
		constexpr int ENT_HANDLER = 0x11A;

		// level_locals_t
		constexpr DWORD LEVEL_CLIENTS = 0xE18E18;
		constexpr DWORD LEVEL_NUM_ENTITIES = 0xE18E24;
		constexpr DWORD LEVEL_MAXCLIENTS = 0xE18E48;
		constexpr DWORD LEVEL_TIME = 0xE18E50;
		constexpr DWORD SV_NUM_ENTITIES = 0x14BFB20;

		// gclient_s (0xB418)
		constexpr int GCLIENT_SIZE = 0xB418;
		constexpr int GC_PMTYPE = 0x4;
		constexpr int GC_ORIGIN = 0x1C;
		constexpr int GC_DELTA_ANGLES = 0x64;
		constexpr int GC_CLIENTNUM = 0xB8;
		constexpr int GC_OFFHAND = 0xBC;
		constexpr int GC_WEAPON = 0xC4;
		constexpr int GC_VIEWANGLES = 0xE4;
		constexpr int GC_CONNECTED = 0xB2D8;  // pers.connected: 1 connecting, 2 connected

		// client_t (0xD728)
		constexpr DWORD SVS_CLIENTS = 0x14C2BA0;
		constexpr int SVCL_SIZE = 0xD728;
		constexpr int SVCL_LASTCMD = 0x2410;
		constexpr int SVCL_GENTITY = 0x2448;

		constexpr int USERCMD_SIZE = 0x38;

		// functions
		constexpr DWORD ClientConnect_f = 0x4A61F0;       // client 0 only
		constexpr DWORD ClientSpawn_f = 0x4A62A0;         // cdecl(gentity_s*), any client
		constexpr DWORD ClientThink_f = 0x4A3D30;         // clientNum in EAX
		constexpr DWORD SelectSpawnPoint_f = 0x4A5930;    // cdecl(origin, angles)
		constexpr DWORD PlayerTrail_f = 0x4FD3F0;         // Sentient_UpdatePlayerTrail, cdecl(ent); global trail, player 1 only
		constexpr DWORD G_InitGentity_f = 0x4EFAF0;       // ent in ESI
		constexpr DWORD Sentient_Alloc_f = 0x4FCC30;
		constexpr DWORD G_InitActors_f = 0x4778C0;
		constexpr DWORD G_InitGame_after_f = 0x4ADCF0;
		constexpr DWORD ClientSpawn_init_f = 0x4A5AB0;    // called in ClientSpawn after the gclient memset, arg in EAX
		constexpr DWORD SV_Trace_f = 0x5C9AC0;            // results EDI, mins EAX; stack: start, maxs, end, ignore, mask, 0, 0, 0
		constexpr DWORD PLAYER_MINS = 0x67AF40;
		constexpr DWORD PLAYER_MAXS = 0x67AF34;
		constexpr int MASK_PLAYER = 0x281C011;             // the player's clipmask (ClientSpawn)

		alignas(16) std::uint8_t sv_clients[2 * SVCL_SIZE];
		alignas(16) std::uint8_t g_clients[2 * GCLIENT_SIZE];

		int players = 1;          // players in the running level
		int spawning = -1;        // client being spawned by us
		int lastLog = -1000000;
		FILE* logFile = nullptr;

		Game::dvar_s* coop_players = nullptr;
		Game::dvar_s* coop_bot = nullptr;
		Game::dvar_s* coop_botfire = nullptr;
		Game::dvar_s* coop_botgod = nullptr;
		Game::dvar_s* coop_botpose = nullptr;
		Game::dvar_s* coop_qa_nodown = nullptr;
		Game::dvar_s* coop_botkit = nullptr;

		// playerState_s fields used here are taken from the mod's Structs.hpp; its layout is checked at load against offsets read from the exe.
		bool psLayoutOk = false;

		// Test kit: player 2 gets player 1's weapons and ammo (the level scripts give weapons to level.player only).
		void MirrorKit()
		{
			auto* src = reinterpret_cast<Game::playerState_s*>(Coop::GameClient(0));
			auto* dst = reinterpret_cast<Game::playerState_s*>(Coop::GameClient(1));
			std::memcpy(dst->weapons, src->weapons, sizeof(dst->weapons));
			std::memcpy(dst->ammo, src->ammo, sizeof(dst->ammo));
			std::memcpy(dst->ammoclip, src->ammoclip, sizeof(dst->ammoclip));
			std::memcpy(dst->weaponmodels, src->weaponmodels, sizeof(dst->weaponmodels));
			dst->weapon = src->weapon;
			Coop::Log("kit: player 2 got player 1's weapons, current %u (clip %d, ammo %d)", src->weapon, src->ammoclip[src->weapon & 127],
				src->ammo[src->weapon & 127]);
		}

		struct Trace
		{
			float fraction;
			float normal[3];
			int surfaceFlags;
			int contents;
			const char* material;
			int hitType;
			std::uint16_t hitId, modelIndex, partName, partGroup;
			bool allsolid, startsolid, walkable;
			char pad;
		};
		static_assert(sizeof(Trace) == 0x2C);

		struct IgnoreEnt
		{
			int baseEntity;
			int parentEntity;
			bool ignoreSelf, ignoreParent, ignoreSiblings, ignoreChildren;
		};

		void SV_Trace(Trace* results, const float* start, const float* mins, const float* maxs, const float* end, IgnoreEnt* ignore, int mask)
		{
			__asm
			{
				push 0
				push 0
				push 0
				push mask
				push ignore
				push end
				push maxs
				push start
				mov eax, mins
				mov edi, results
				mov ecx, SV_Trace_f
				call ecx
				add esp, 0x20
			}
		}

		void ClientThink(int clientNum)
		{
			__asm
			{
				mov eax, clientNum
				mov ecx, ClientThink_f
				call ecx
			}
		}

		void G_InitGentity(std::uint8_t* ent)
		{
			__asm
			{
				mov esi, ent
				mov ecx, G_InitGentity_f
				call ecx
			}
		}

		short Angle2Short(float a)
		{
			return static_cast<short>(static_cast<int>(a * 65536.0f / 360.0f) & 0xFFFF);
		}

		int LevelTime()
		{
			return *reinterpret_cast<int*>(LEVEL_TIME);
		}

		bool consoleReady = false;

		// Downed players = IW4 Spec Ops last stand (maps\_coop.gsc does the rest). The mark lives in the player state: pm_flags bit 24
		// (SP uses bits 0-23), so it is saved and loaded with the player, reaches the player's own client in the snapshot (SP copies
		// the whole playerState) and the client's prediction runs the same last stand movement as the server.
		constexpr std::uint32_t PMF_COOP_DOWNED = 0x01000000;
		int reviveUse[2] = { 0, 0 };
		int botWeaponSelect = 0; // one-process bot: the last "sw" the server sent to player 2

		bool IsDown(int n)
		{
			if (n < 0 || n >= 2 || n >= players || !*reinterpret_cast<void**>(LEVEL_CLIENTS))
				return false;
			return (*reinterpret_cast<std::uint32_t*>(Coop::GameClient(n) + 0xC) & PMF_COOP_DOWNED) != 0;
		}

		void SetDown(int n, bool down)
		{
			if (n < 0 || n >= 2 || n >= players || !*reinterpret_cast<void**>(LEVEL_CLIENTS))
				return;
			auto& flags = *reinterpret_cast<std::uint32_t*>(Coop::GameClient(n) + 0xC);
			flags = down ? (flags | PMF_COOP_DOWNED) : (flags & ~PMF_COOP_DOWNED);
		}

		// The player's die handler (entityHandlers[0x11].die = player_die 0x4ADF60, pointer at 0x6DEBF4, called by G_Damage 0x4AE5E0):
		// - a player whose partner is up does not die: 1 health, marked down (IW4: EnableDeathShield + the "deathshield" notify);
		// - a downed player does not die either (death shield): the script ends the last stand (bleed-out / both down -> mission failed).
		// Both PCs run this at the same tick (same damage, same simulation).
		void __cdecl PlayerDie_Hook(std::uint8_t* self, std::uint8_t* inflictor, std::uint8_t* attacker, int damage, int mod, int weapon,
			const float* dir, int hitLoc)
		{
			const int me = static_cast<int>((self - reinterpret_cast<std::uint8_t*>(G_ENTITIES)) / ENT_SIZE);
			if (players > 1 && me >= 0 && me < 2 && !(coop_qa_nodown && coop_qa_nodown->current.enabled))
			{
				bool keep = IsDown(me);
				if (!keep)
				{
					const int other = 1 - me;
					std::uint8_t* oe = Coop::Entity(other);
					std::uint8_t* oc = Coop::GameClient(other);
					keep = oe[ENT_INUSE] && *reinterpret_cast<int*>(oe + 0x144) > 0 && !IsDown(other) &&
						*reinterpret_cast<int*>(oc + GC_CONNECTED) == 2 && *reinterpret_cast<int*>(oc + GC_PMTYPE) < 4;
					if (keep)
					{
						SetDown(me, true);
						Coop::Log("player %d down at tick %d (damage %d, means %d)", me + 1, LevelTime(), damage, mod);
					}
				}
				if (keep)
				{
					*reinterpret_cast<int*>(self + 0x144) = 1;
					*reinterpret_cast<std::uint32_t*>(self + 0x130) &= ~0x20u; // G_Damage's "client died" flag
					*reinterpret_cast<int*>(Coop::GameClient(me) + 0x124) = 1;  // ps.stats[STAT_HEALTH]
					return;
				}
			}
			Utils::Hook::Call<void(std::uint8_t*, std::uint8_t*, std::uint8_t*, int, int, int, const float*, int)>(0x4ADF60)(
				self, inflictor, attacker, damage, mod, weapon, dir, hitLoc);
		}

		// Last stand movement (IW4 PM_LASTSTAND, pm_type 5 in IW4 SP ClientThink 0x14015be30): eye height 22 (IW4 and IW3 MP use 22,
		// between crouch 40 and prone 11) and moves scaled by player_lastStandCrawlSpeedScale (IW4 default 0.2) instead of the stance's
		// scale. IW3 SP has no last stand pm_type; its pmove code (shared by the server and the client's prediction) gets it here:
		// - PM_CheckDuck 0x5B9E50 picks the stance's eye height (ps+0xF0) and calls PM_ViewHeightAdjust 0x5B99C0 at 0x5BA547 (ECX = pml,
		//   [esp+4] = pm, caller pops): for a downed player the target is 22 for that call only (the adjust moves the eye there at 180/s,
		//   the path IW3 MP last stand uses) and the crouch target (bounds, stance) is put back right after it;
		// - PM_CmdScaleForStance 0x5B82A0 (EAX = pm, result in ST0), called at 0x5B8539 and 0x5BAB5E: the crawl scale instead.
		Game::dvar_s* coop_crawlscale = nullptr;

		float __cdecl CrawlScale()
		{
			return coop_crawlscale ? coop_crawlscale->current.value : 0.2f;
		}

		void __declspec(naked) ViewHeightAdjust_Stub()
		{
			__asm
			{
				mov eax, [esp + 4]       // pm
				mov eax, [eax]           // ps
				test dword ptr [eax + 0xC], 0x01000000
				jnz down
				mov eax, 0x5B99C0
				jmp eax
			down:
				push esi
				mov esi, eax
				push dword ptr [esi + 0xF0] // the stance's eye height target
				mov dword ptr [esi + 0xF0], 22
				push dword ptr [esp + 0xC]  // pm (saved target, esi, return address above it)
				mov eax, 0x5B99C0
				call eax                    // ECX (pml) untouched
				add esp, 4
				pop dword ptr [esi + 0xF0]
				pop esi
				ret
			}
		}

		// Triggers a player touches (trigger_multiple: flag_set, spawn and course triggers ...): G_RunFrame state 1 calls
		// G_UpdatePlayerTriggers 0x4A3F80 (EAX = entity; it also moves his sentient's path node, then G_TouchTriggers 0x4A2F30 with
		// EDI = entity) at 0x4BAB99 for g_entities[0] only, so player 2 never fired a trigger (killhouse: the top of the ladder on his
		// turn). Now it runs for every player, in entity order, on both PCs at the same tick.
		void UpdatePlayerTriggersFor(std::uint8_t* ent)
		{
			__asm
			{
				mov eax, ent
				mov ecx, 0x4A3F80
				call ecx
			}
		}

		void __cdecl UpdatePlayerTriggersAll(std::uint8_t* first)
		{
			UpdatePlayerTriggersFor(first);
			for (int n = 1; n < players; n++)
			{
				std::uint8_t* ent = Coop::Entity(n);
				if (!ent[ENT_INUSE] || !*reinterpret_cast<void**>(ent + ENT_CLIENT) || !*reinterpret_cast<void**>(ent + ENT_SENTIENT))
					continue;
				if (*reinterpret_cast<int*>(Coop::GameClient(n) + GC_CONNECTED) != 2)
					continue;
				UpdatePlayerTriggersFor(ent);
			}
		}

		void __declspec(naked) UpdatePlayerTriggers_Stub()
		{
			__asm
			{
				push eax
				call UpdatePlayerTriggersAll
				add esp, 4
				ret
			}
		}

		void __declspec(naked) CmdScaleForStance_Stub()
		{
			__asm
			{
				mov ecx, [eax]           // pm->ps
				test dword ptr [ecx + 0xC], 0x01000000
				jnz down
				mov ecx, 0x5B82A0
				jmp ecx
			down:
				jmp CrawlScale           // float in ST0, like the original
			}
		}

		void DecidePlayers()
		{
			botWeaponSelect = 0;
			int want = coop_players ? coop_players->current.integer : 1;
			if (CoopNet::Active())
			{
				want = 2;
				*reinterpret_cast<int*>(0x6DDAAC) = CoopNet::Seed(); // holdrand: same game RNG on both PCs (stock: timeGetTime at map load)
				Coop::Log("G_InitGame: session seed %d", CoopNet::Seed());
				CoopNet::OnLevelStart();
			}
			players = want >= 2 ? 2 : 1;
			Coop::Log("G_InitGame: %d player(s), level.clients %p, svs.clients %p", players, *reinterpret_cast<void**>(LEVEL_CLIENTS),
				*reinterpret_cast<void**>(SVS_CLIENTS));
		}

		// G_InitGame, after the client links: entities 0..players-1 are reserved for the players.
		void FixEntityCounts()
		{
			*reinterpret_cast<int*>(LEVEL_NUM_ENTITIES) = players;
			*reinterpret_cast<int*>(SV_NUM_ENTITIES) = players;
		}

		// ClientSpawn memsets gclient_s and does not restore ps.clientNum (always 0 in stock SP): put it back before the first ClientThink.
		void FixClientNums()
		{
			for (int i = 1; i < players; i++)
				*reinterpret_cast<int*>(Coop::GameClient(i) + GC_CLIENTNUM) = i;
		}

		// naked stubs
		void __declspec(naked) G_InitActors_Stub()
		{
			__asm
			{
				pushad
				call DecidePlayers
				popad
				push eax
				mov eax, players
				mov dword ptr ds:[0xE18E48], eax // level.maxclients (a literal: a named constexpr is a memory operand here)
				pop eax
				push 0x4778C0 // G_InitActors
				ret
			}
		}

		void __declspec(naked) G_InitGameTail_Stub()
		{
			__asm
			{
				pushad
				call FixEntityCounts
				popad
				push 0x4ADCF0
				ret
			}
		}

		// ClientThink_real 0x4A38D0, dead client past its respawn time: 0x4A6050 starts the SP death reload (snd_fade, reload after
		// g_deathDelay on a real-time timer, 0xE19878). Only player 1's death does that: with two players a player only dies when his
		// partner is down or dead too (PlayerDie_Hook), and maps\_coop fails the mission then. The script's missionfailed()
		// (GScr_MissionFailed 0x4D3080) arms the same reload from 0x4D30CB. In a session both go by simulation time instead.
		void __cdecl DeathArmed()
		{
			CoopNet::OnDeathReloadArmed();
		}

		void __declspec(naked) DeathReload_Stub()
		{
			__asm
			{
				cmp ebx, dword ptr ds:[0xE18E18] // gclient in EBX == level.clients (client 0)?
				jne skip
				pushad
				mov eax, 0x4A6050
				call eax
				call DeathArmed // a session: the reload goes by simulation time on both PCs
				popad
			skip:
				ret
			}
		}

		void __declspec(naked) MissionFailedReload_Stub()
		{
			__asm
			{
				pushad
				mov eax, 0x4A6050
				call eax
				call DeathArmed // a session: the reload goes by simulation time on both PCs
				popad
				ret
			}
		}

		void __declspec(naked) ClientSpawnInit_Stub()
		{
			__asm
			{
				pushad
				call FixClientNums
				popad
				push 0x4A5AB0
				ret
			}
		}
	}

	void Coop::Log(const char* fmt, ...)
	{
		char buf[2048];
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);
		if (!logFile)
		{
			std::filesystem::create_directories("cod4coop");
			logFile = fopen("cod4coop\\coop.log", "w");
		}
		if (logFile)
		{
			SYSTEMTIME st;
			GetLocalTime(&st);
			fprintf(logFile, "%02d:%02d:%02d.%03d %s\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, buf);
			fflush(logFile);
		}
		if (consoleReady)
			Game::Com_Printf(0, "^5coop^7: %s\n", buf);
	}

	int Coop::Players()
	{
		return players;
	}

	std::uint8_t* Coop::Entity(int num)
	{
		return reinterpret_cast<std::uint8_t*>(G_ENTITIES + num * ENT_SIZE);
	}

	std::uint8_t* Coop::GameClient(int num)
	{
		return *reinterpret_cast<std::uint8_t**>(LEVEL_CLIENTS) + num * GCLIENT_SIZE;
	}

	std::uint8_t* Coop::ServerClient(int num)
	{
		return *reinterpret_cast<std::uint8_t**>(SVS_CLIENTS) + num * SVCL_SIZE;
	}

	// SV_InitGameVM calls ClientConnect (client 0 only); connect the second client right after it.
	int __cdecl Coop::ClientConnect_Hook()
	{
		const int result = Utils::Hook::Call<int()>(ClientConnect_f)();
		if (players > 1)
			ConnectSecondClient();
		return result;
	}

	// What SVC_DirectConnect 0x5C5920 and ClientConnect 0x4A61F0 do for client 0, for client 1.
	void Coop::ConnectSecondClient()
	{
		std::uint8_t* svcl = ServerClient(1);
		std::memset(svcl, 0, SVCL_SIZE);
		*reinterpret_cast<int*>(svcl) = 1;
		*reinterpret_cast<int*>(svcl + 0xD724) = 1;
		*reinterpret_cast<int*>(svcl + SVCL_LASTCMD) = LevelTime();
		*reinterpret_cast<std::uint8_t**>(svcl + SVCL_GENTITY) = Entity(1);

		std::uint8_t* cl = GameClient(1);
		std::memset(cl, 0, GCLIENT_SIZE);
		*reinterpret_cast<int*>(cl + GC_CONNECTED) = 1;
		*reinterpret_cast<float*>(cl + 0xB350) = 1.0f;
		*reinterpret_cast<float*>(cl + 0x5C8) = 1.0f;
		const int maxHealth = *reinterpret_cast<int*>(*reinterpret_cast<std::uint8_t**>(0xDCF77C) + 0xC);
		*reinterpret_cast<int*>(cl + 0xB34C) = maxHealth;
		*reinterpret_cast<int*>(cl + 0x12C) = maxHealth;
		*reinterpret_cast<int*>(cl + GC_CLIENTNUM) = 1;

		std::uint8_t* ent = Entity(1);
		G_InitGentity(ent);
		ent[ENT_HANDLER] = 0x11;
		*reinterpret_cast<std::uint8_t**>(ent + ENT_CLIENT) = cl;
		auto* sentient = Utils::Hook::Call<std::uint8_t*()>(Sentient_Alloc_f)();
		if (!sentient)
		{
			Log("ConnectSecondClient: no sentient left for player 2");
			return;
		}
		*reinterpret_cast<std::uint8_t**>(ent + ENT_SENTIENT) = sentient;
		*reinterpret_cast<float*>(sentient + 0x20) = *reinterpret_cast<float*>(0x6C7E88);
		*reinterpret_cast<std::uint8_t**>(sentient) = ent;
		*reinterpret_cast<float*>(sentient + 0x50) = 1.0f;
		*reinterpret_cast<int*>(sentient + 4) = 2; // TEAM_ALLIES, as player 1
		Log("client 1 connected: ent %p client %p sentient %p maxHealth %d", ent, cl, sentient, maxHealth);
	}

	// G_LoadLevel spawns client 0 (ClientBegin is inlined there); spawn client 1 after it.
	void __cdecl Coop::ClientSpawn_Hook(std::uint8_t* ent)
	{
		Utils::Hook::Call<void(std::uint8_t*)>(ClientSpawn_f)(ent);
		if (players < 2 || ent != Entity(0))
			return;
		std::uint8_t* cl = GameClient(1);
		if (*reinterpret_cast<int*>(cl + GC_CONNECTED) == 0)
			return;
		*reinterpret_cast<int*>(cl + GC_CONNECTED) = 2;
		spawning = 1;
		Utils::Hook::Call<void(std::uint8_t*)>(ClientSpawn_f)(Entity(1));
		spawning = -1;
		const float* o = reinterpret_cast<float*>(cl + GC_ORIGIN);
		const float* p = reinterpret_cast<float*>(Entity(0) + ENT_ORIGIN);
		Log("client 1 spawned at (%.0f %.0f %.0f), player 1 at (%.0f %.0f %.0f), clientNum %d, inuse %d",
			o[0], o[1], o[2], p[0], p[1], p[2], *reinterpret_cast<int*>(cl + GC_CLIENTNUM), Entity(1)[ENT_INUSE]);
	}

	// A free spot for a standing player near center: rings of candidates, hull test, ground below, line of sight from center.
	bool Coop::FindSpotNear(const float* center, float yawDeg, float* out)
	{
		const float* mins = reinterpret_cast<float*>(PLAYER_MINS);
		const float* maxs = reinterpret_cast<float*>(PLAYER_MAXS);
		IgnoreEnt ignore{ 0x87F, -1, true, false, true, true };
		const float zero[3] = { 0, 0, 0 };
		const float radii[] = { 56.0f, 80.0f, 112.0f, 160.0f };
		const float dirs[] = { -90, 90, 180, -45, 45, -135, 135, 0 };
		for (float r : radii)
		{
			for (float d : dirs)
			{
				const float a = (yawDeg + d) * 3.14159265f / 180.0f;
				float c[3] = { center[0] + std::cos(a) * r, center[1] + std::sin(a) * r, center[2] + 18.0f };
				Trace tr{};
				// line of sight at chest height from player 1
				float eye[3] = { center[0], center[1], center[2] + 40.0f };
				float ceye[3] = { c[0], c[1], c[2] + 22.0f };
				SV_Trace(&tr, eye, zero, zero, ceye, &ignore, 1);
				if (tr.fraction < 1.0f)
					continue;
				SV_Trace(&tr, c, mins, maxs, c, &ignore, MASK_PLAYER);
				if (tr.startsolid || tr.allsolid)
					continue;
				float down[3] = { c[0], c[1], c[2] - 80.0f };
				SV_Trace(&tr, c, mins, maxs, down, &ignore, MASK_PLAYER);
				if (tr.startsolid || tr.fraction >= 1.0f || tr.normal[2] < 0.7f)
					continue;
				out[0] = c[0];
				out[1] = c[1];
				out[2] = c[2] + (down[2] - c[2]) * tr.fraction + 1.0f;
				return true;
			}
		}
		return false;
	}

	int __cdecl Coop::SelectSpawnPoint_Hook(float* origin, float* angles)
	{
		const int spot = Utils::Hook::Call<int(float*, float*)>(SelectSpawnPoint_f)(origin, angles);
		if (spawning > 0)
		{
			const float* p1 = reinterpret_cast<float*>(Entity(0) + ENT_ORIGIN);
			float out[3];
			if (FindSpotNear(p1, angles[1], out))
			{
				origin[0] = out[0];
				origin[1] = out[1];
				origin[2] = out[2];
				Log("spawn spot for client %d: (%.0f %.0f %.0f)", spawning, out[0], out[1], out[2]);
			}
			else
			{
				Log("spawn spot for client %d: none free near player 1, using the level spawn", spawning);
			}
		}
		return spot;
	}

	// Both places that hand player 1's usercmds to the game (G_RunFrame state 1 via 0x4A3F60, and state 7) call the player trail next.
	void __cdecl Coop::PlayerTrail_Hook(std::uint8_t* ent)
	{
		Utils::Hook::Call<void(std::uint8_t*)>(PlayerTrail_f)(ent);
		if (players > 1 && !CoopNet::Active() && *reinterpret_cast<int*>(GameClient(1) + GC_CONNECTED) == 2)
			BotThink();
	}

	// damage player n through G_Damage (QA of downed / revive): console coop_hurt (one process) or the session command coophurt
	void Coop::Hurt(int n, int damage)
	{
		Hurt(n, damage, 0, -1000.0f);
	}

	// mod: means of death index (IW3: 2 rifle bullet, 4 grenade splash, 14 explosive, ...); fromYaw: the world yaw the hit comes from
	// (-1000 = straight down, as before)
	void Coop::Hurt(int n, int damage, int mod, float fromYaw)
	{
		if (n < 0 || n >= players || !Entity(n)[ENT_INUSE])
			return;
		Log("hurt: player %d damage %d at tick %d (health %d)", n + 1, damage, LevelTime(), *reinterpret_cast<int*>(Entity(n) + 0x144));
		// the "damage" notify reads the direction and the point: real vectors (null crashed in Scr_AddVector)
		float dir[3] = { 0.0f, 0.0f, -1.0f };
		if (fromYaw > -999.0f)
		{
			const float r = fromYaw * 3.14159265f / 180.0f;
			dir[0] = -std::cos(r); // the damage travels from that side towards the player
			dir[1] = -std::sin(r);
			dir[2] = 0.0f;
		}
		const float* o = reinterpret_cast<float*>(Entity(n) + ENT_ORIGIN);
		const float point[3] = { o[0], o[1], o[2] + 40.0f };
		// G_Damage(targ, inflictor, attacker, dir, point, damage, dflags, mod, weapon, hitLoc, model, part): 12 stack arguments
		// (0x4AE8FC passes the last two, model and part name, to the "damage" notify; 0 = undefined)
		Utils::Hook::Call<void(void*, void*, void*, const float*, const float*, int, int, int, int, int, int, int)>(0x4AE5E0)(
			Entity(n), nullptr, nullptr, dir, point, damage, 0, mod, -1, 0, 0, 0);
	}

	// a key command notification of player n (CoopNet CommandNotify_Hook): notify his entity, as G_AddCommandNotify 0x4A8570 does
	// for entity 0; the notify string id is the one notifyOnCommand registered (table 0xF82408 command / 0xF8240A notify, count
	// 0xF7FBCC), so no new script string is made
	void Coop::CommandNotify(int n, const char* name)
	{
		if (n < 0 || n >= players || !name || !Entity(n)[ENT_INUSE])
			return;
		const int count = *reinterpret_cast<int*>(0xF7FBCC);
		for (int i = 0; i < count && i < 64; i++)
		{
			const unsigned int id = *reinterpret_cast<std::uint16_t*>(0xF8240A + i * 4);
			const char* s = id ? Game::SL_ConvertToString(id) : nullptr;
			if (!s || std::strcmp(s, name))
				continue;
			const int entnum = n;
			__asm
			{
				push id
				push 0
				push entnum
				xor eax, eax // no parameters
				mov ecx, 0x5571D0 // Scr_NotifyNum( entnum, classnum, notify ), EAX = parameter count
				call ecx
				add esp, 12
			}
			return;
		}
	}

	void Coop::SetBotWeaponSelect(int weapon)
	{
		botWeaponSelect = weapon;
	}

	bool Coop::Downed(int n)
	{
		return IsDown(n);
	}

	int Coop::LevelTimeNow()
	{
		return LevelTime();
	}

	// Test input for player 2 (Phase 1). Lockstep input replaces this in Phase 2.
	// Revive use (IW4 uses a usable entity on the downed player, so Use revives instead of taking a weapon on the ground; IW3 SP
	// script models cannot be made usable): Use (buttons 0x28 = use | usereload) pressed within 128 units of the downed partner is
	// taken out of the player's usercmd before the game sees it (no weapon pickup, no door) and kept for the script
	// (coopreviveuse). Both PCs run the same usercmds, so both decide the same.
	void Coop::Think(int n, const std::uint8_t* cmd)
	{
		std::uint8_t* last = ServerClient(n) + SVCL_LASTCMD;
		std::memcpy(last, cmd, USERCMD_SIZE);
		if (n >= 0 && n < 2)
		{
			reviveUse[n] = 0;
			auto& buttons = *reinterpret_cast<int*>(last + 4);
			if (players > 1 && (buttons & 0x28) && IsDown(1 - n) && !IsDown(n))
			{
				const float* a = reinterpret_cast<float*>(Entity(n) + ENT_ORIGIN);
				const float* b = reinterpret_cast<float*>(Entity(1 - n) + ENT_ORIGIN);
				const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
				if (dx * dx + dy * dy + dz * dz < 128.0f * 128.0f)
				{
					reviveUse[n] = 1;
					buttons &= ~0x28;
				}
			}
		}
		ClientThink(n);
	}

	// Moves, angles, buttons and weapon of a test bot for player self (serverTime is left as it is).
	void Coop::FillBotCmd(int self, int other, std::uint8_t* cmd, const char* mode, bool fire)
	{
		std::uint8_t* cl = GameClient(self);
		const float* o = reinterpret_cast<float*>(cl + GC_ORIGIN);
		const float* p1 = reinterpret_cast<float*>(Entity(other) + ENT_ORIGIN);
		const float* delta = reinterpret_cast<float*>(cl + GC_DELTA_ANGLES);
		const float dx = p1[0] - o[0], dy = p1[1] - o[1];
		const float dist = std::sqrt(dx * dx + dy * dy);
		float yaw = reinterpret_cast<float*>(cl + GC_VIEWANGLES)[1];
		int forward = 0;
		const int t = LevelTime();
		bool reviving = false;
		if (!std::strcmp(mode, "follow"))
		{
			yaw = std::atan2(dy, dx) * 180.0f / 3.14159265f;
			forward = dist > 160.0f ? 127 : 0;
			if (IsDown(other) && !IsDown(self))
			{
				forward = dist > 40.0f ? 127 : 0; // a downed partner: go to him and hold use (QA of the revive, IW4 use radius 128)
				reviving = dist <= 100.0f;
			}
		}
		else if (!std::strcmp(mode, "front"))
		{
			// stand 150 units in front of the other player's view, facing him
			const float pyaw = reinterpret_cast<float*>(GameClient(other) + GC_VIEWANGLES)[1] * 3.14159265f / 180.0f;
			const float tx = p1[0] + std::cos(pyaw) * 150.0f - o[0], ty = p1[1] + std::sin(pyaw) * 150.0f - o[1];
			if (std::sqrt(tx * tx + ty * ty) > 24.0f)
			{
				yaw = std::atan2(ty, tx) * 180.0f / 3.14159265f;
				forward = 127;
			}
			else
			{
				yaw = std::atan2(dy, dx) * 180.0f / 3.14159265f;
			}
		}
		else if (!std::strcmp(mode, "walk"))
		{
			yaw = static_cast<float>((t / 3000) % 4) * 90.0f;
			forward = (t % 3000) < 2000 ? 127 : 0;
		}
		int buttons = 0;
		if (fire && (t % 4000) < 150 && !reviving)
			buttons |= 1; // attack
		if (reviving)
			buttons |= 1 << 3; // CMD_BUTTON_ACTIVATE (use)
		*reinterpret_cast<int*>(cmd + 0x4) = buttons;
		reinterpret_cast<int*>(cmd + 0x8)[0] = Angle2Short(0.0f - delta[0]);
		reinterpret_cast<int*>(cmd + 0x8)[1] = Angle2Short(yaw - delta[1]);
		reinterpret_cast<int*>(cmd + 0x8)[2] = Angle2Short(0.0f - delta[2]);
		// the weapon: a session bot keeps its client's selection (the server's weapon select, e.g. the script's switchToWeapon of the
		// start kit, reaches that client); the one-process bot has no client: its weapon in hand, else the other player's
		if (!cmd[0x14])
		{
			int w = *reinterpret_cast<int*>(cl + GC_WEAPON);
			if (!CoopNet::Active() && botWeaponSelect > 0)
				w = botWeaponSelect; // the server's switchToWeapon for the bot
			if (!w && !CoopNet::Active())
				w = *reinterpret_cast<int*>(GameClient(other) + GC_WEAPON);
			cmd[0x14] = static_cast<std::uint8_t>(w);
		}
		cmd[0x15] = static_cast<std::uint8_t>(*reinterpret_cast<int*>(cl + GC_OFFHAND));
		cmd[0x16] = static_cast<std::uint8_t>(forward);
		cmd[0x17] = 0;
		cmd[0x18] = 0;

		// QA of the body's poses (coop_botpose: words on top of the mode): moves, stance buttons (crouch 0x200 / prone 0x100 held),
		// sprint 0x2, ADS 0x800, actions pulsed so each happens once in a while (attack 1, reload 0x10, melee 0x4, frag 0x4000,
		// jump = gostand 0x400), view pitch
		const char* pose = coop_botpose ? coop_botpose->current.string : "";
		if (pose && *pose)
		{
			auto has = [pose](const char* word) { return std::strstr(pose, word) != nullptr; };
			auto move = [](int v) { return static_cast<std::uint8_t>(static_cast<std::int8_t>(v)); };
			const int full = has("slow") ? 50 : 127;
			auto& b = *reinterpret_cast<int*>(cmd + 4);
			if (has("forward"))
				cmd[0x16] = move(full);
			if (has("back"))
				cmd[0x16] = move(-full);
			if (has("left"))
				cmd[0x17] = move(-full);
			if (has("right"))
				cmd[0x17] = move(full);
			if (has("sprint"))
				b |= 0x2;
			if (has("crouch"))
				b |= 0x200;
			if (has("prone"))
				b |= 0x100;
			if (has("ads"))
				b |= 0x800;
			if (has("fire") && (t % 1000) < 100)
				b |= 1;
			if (has("reload") && (t % 4000) < 100)
				b |= 0x10;
			if (has("melee") && (t % 3000) < 100)
				b |= 0x4;
			if (has("frag") && (t % 5000) < 150)
				b |= 0x4000;
			if (has("jump") && (t % 2000) < 100)
				b |= 0x400; // +gostand (CoD4 jumps with it: the usercmd has no upmove)
			if (has("copyweapon"))
				cmd[0x14] = static_cast<std::uint8_t>(*reinterpret_cast<int*>(GameClient(other) + GC_WEAPON)); // the other player's weapon
			if (has("lookup"))
				reinterpret_cast<int*>(cmd + 0x8)[0] = Angle2Short(-40.0f - delta[0]);
			if (has("lookdown"))
				reinterpret_cast<int*>(cmd + 0x8)[0] = Angle2Short(40.0f - delta[0]);
		}
	}

	// One-process test (Phase 1): player 2 is a bot thinking right after player 1's input.
	void Coop::BotThink()
	{
		std::uint8_t* cl = GameClient(1);
		const int serverTime = *reinterpret_cast<int*>(ServerClient(0) + SVCL_LASTCMD);
		if (serverTime <= *reinterpret_cast<int*>(cl)) // ps.commandTime
			return;
		std::uint8_t cmd[USERCMD_SIZE] = {};
		*reinterpret_cast<int*>(cmd + 0x0) = serverTime;
		const char* mode = coop_bot ? coop_bot->current.string : "follow";
		if (coop_botkit && coop_botkit->current.enabled && psLayoutOk && *reinterpret_cast<int*>(GameClient(0) + GC_WEAPON) != 0 &&
			*reinterpret_cast<int*>(cl + GC_WEAPON) == 0 && *reinterpret_cast<int*>(cl + GC_PMTYPE) < 4)
			MirrorKit();
		if (coop_botgod && coop_botgod->current.enabled && reinterpret_cast<Game::gentity_s*>(Entity(1))->health > 0)
			reinterpret_cast<Game::gentity_s*>(Entity(1))->health = 100; // test only: keep player 2 alive
		FillBotCmd(1, 0, cmd, mode, coop_botfire && coop_botfire->current.enabled);
		Think(1, cmd);

		const int t = LevelTime();
		if (t - lastLog >= 1000 || t < lastLog)
		{
			lastLog = t;
			const float* o = reinterpret_cast<float*>(cl + GC_ORIGIN);
			const float* p1 = reinterpret_cast<float*>(Entity(0) + ENT_ORIGIN);
			const float dx = p1[0] - o[0], dy = p1[1] - o[1];
			const auto* e1 = reinterpret_cast<Game::gentity_s*>(Entity(1));
			const auto* e0 = reinterpret_cast<Game::gentity_s*>(Entity(0));
			const auto* ps2 = reinterpret_cast<Game::playerState_s*>(cl);
			int clip = -1; // all rounds in clips: falls when player 2 fires (ammoclip is indexed by clip index, not weapon)
			if (psLayoutOk)
			{
				clip = 0;
				for (int k = 0; k < 128; k++)
					clip += ps2->ammoclip[k];
			}
			Log("t %d p2 (%.0f %.0f %.0f) hp %d pm %d weapon %d clips %d | p1 (%.0f %.0f %.0f) hp %d weapon %d | dist %.0f mode %s",
				t, o[0], o[1], o[2], e1->health, *reinterpret_cast<int*>(cl + GC_PMTYPE), *reinterpret_cast<int*>(cl + GC_WEAPON), clip,
				p1[0], p1[1], p1[2], e0->health, *reinterpret_cast<int*>(GameClient(0) + GC_WEAPON), std::sqrt(dx * dx + dy * dy), mode);
		}
	}

	Coop::Coop()
	{
		Events::OnDvarInit([]
		{
			consoleReady = true;
			coop_players = Dvars::Register::Dvar_RegisterInt("coop_players", "Players in the next level (1 or 2; 2 = co-op test: player 2 is a bot)", 1, 1, 2, Game::none);
			coop_bot = Dvars::Register::Dvar_RegisterString("coop_bot", "Test bot for player 2: follow, front, walk, stand", "follow", Game::none);
			coop_botfire = Dvars::Register::Dvar_RegisterBool("coop_botfire", "Test bot fires every 4 s", false, Game::none);
			coop_botgod = Dvars::Register::Dvar_RegisterBool("coop_botgod", "Test: player 2 cannot die", false, Game::none);
			coop_qa_nodown = Dvars::Register::Dvar_RegisterBool("coop_qa_nodown", "Test: players die without going down first", false, Game::none);
			coop_botpose = Dvars::Register::Dvar_RegisterString("coop_botpose", "Test: bot actions on top of its mode (forward back left right "
				"slow sprint crouch prone ads fire reload melee frag jump copyweapon lookup lookdown)", "", Game::none);
			coop_botkit = Dvars::Register::Dvar_RegisterBool("coop_botkit", "Test: player 2 gets player 1's weapons as soon as player 1 has one", false, Game::none);
			coop_crawlscale = Dvars::Register::Dvar_RegisterFloat("coop_crawlscale", "Downed player's crawl speed scale (IW4 player_lastStandCrawlSpeedScale)", 0.2f, 0.0f, 1.0f, Game::none);
		});

		// Two-entry client arrays of our own (stock: one client_t at 0x14A39D0, one gclient_s at 0xE0DA00 packed against level_locals).
		Utils::Hook::Set<DWORD>(0x5C6A6C, reinterpret_cast<DWORD>(sv_clients));   // SV_Startup: svs.clients = ...
		Utils::Hook::Set<DWORD>(0x4B9922, 2 * GCLIENT_SIZE);                        // G_InitGame: memset size
		Utils::Hook::Set<DWORD>(0x4B9928, reinterpret_cast<DWORD>(g_clients));      // G_InitGame: memset dest
		Utils::Hook::Set<DWORD>(0x4B994A, reinterpret_cast<DWORD>(g_clients));      // G_InitGame: level.clients = ...

		// Savegames with two clients: the field writer/reader allow client index (+1) up to 2, the main-state writer saves both clients.
		// G_LoadMainState 0x4C5887 already accepts clientnum 1 (error only when > 1).
		Utils::Hook::Set<BYTE>(0x4C327C, 2);   // WriteField1: client index <= 1 -> <= 2 ("client out of range")
		Utils::Hook::Set<BYTE>(0x4C36B6, 2);   // ReadField: same
		Utils::Hook::Set<BYTE>(0x4C4BC6, 2);   // G_SaveMainState: for (i = 0; i < 1 -> 2; i++) write connected clients

		Utils::Hook(0x4B9967, G_InitActors_Stub, HOOK_CALL).install()->quick();     // level.maxclients before the client links
		Utils::Hook(0x4B99D2, G_InitGameTail_Stub, HOOK_CALL).install()->quick();   // level/sv.num_entities after them
		Utils::Hook(0x5C6647, ClientConnect_Hook, HOOK_CALL).install()->quick();    // SV_InitGameVM
		Utils::Hook(0x4BA53A, ClientSpawn_Hook, HOOK_CALL).install()->quick();      // G_LoadLevel
		Utils::Hook(0x4A62C1, SelectSpawnPoint_Hook, HOOK_CALL).install()->quick(); // ClientSpawn
		Utils::Hook(0x4A64D3, ClientSpawnInit_Stub, HOOK_CALL).install()->quick();  // ClientSpawn, after the memset
		Utils::Hook(0x4A3D1B, DeathReload_Stub, HOOK_CALL).install()->quick();      // ClientThink_real: death reload only for player 1
		Utils::Hook(0x4D30CB, MissionFailedReload_Stub, HOOK_CALL).install()->quick(); // GScr_MissionFailed: the same reload
		Utils::Hook(0x5BA547, ViewHeightAdjust_Stub, HOOK_CALL).install()->quick();    // last stand eye height 22 (PM_CheckDuck)
		Utils::Hook(0x5B8539, CmdScaleForStance_Stub, HOOK_CALL).install()->quick();   // last stand crawl speed
		Utils::Hook(0x5BAB5E, CmdScaleForStance_Stub, HOOK_CALL).install()->quick();
		Utils::Hook(0x4BB123, PlayerTrail_Hook, HOOK_CALL).install()->quick();      // G_RunFrame state 7
		Utils::Hook(0x4A3F70, PlayerTrail_Hook, HOOK_CALL).install()->quick();      // G_RunFrame state 1 (0x4A3F60)
		Utils::Hook(0x4BAB99, UpdatePlayerTriggers_Stub, HOOK_CALL).install()->quick(); // G_UpdatePlayerTriggers for every player

		psLayoutOk = sizeof(Game::playerState_s) == 0xB2D8 && offsetof(Game::playerState_s, clientNum) == GC_CLIENTNUM &&
			offsetof(Game::playerState_s, viewangles) == GC_VIEWANGLES && offsetof(Game::playerState_s, weapon) == GC_WEAPON;
		// coop_give: player 1's weapons to player 2 (test kit)
		GSC::AddMethod("coopdowned", [](Game::scr_entref_t entref)
		{
			Game::Scr_AddInt(IsDown(entref.entnum) ? 1 : 0);
		}, false);
		// the body's animation inputs (maps\_coop body_follow): playerState pm_flags (+0xC: ladder 0x8, mantle 0x4, jumping 0x4000,
		// sprinting 0x8000, ...) and weaponstate (+0xC8: reload 7-11, melee 12-14, offhand 15-20, ...); the same on both PCs
		GSC::AddMethod("coopflags", [](Game::scr_entref_t entref)
		{
			Game::Scr_AddInt(entref.entnum < static_cast<unsigned>(players) ? *reinterpret_cast<int*>(GameClient(entref.entnum) + 0xC) : 0);
		}, false);
		GSC::AddMethod("coopweaponstate", [](Game::scr_entref_t entref)
		{
			Game::Scr_AddInt(entref.entnum < static_cast<unsigned>(players) ? *reinterpret_cast<int*>(GameClient(entref.entnum) + 0xC8) : 0);
		}, false);
		GSC::AddMethod("coopreviveuse", [](Game::scr_entref_t entref)
		{
			Game::Scr_AddInt(entref.entnum < 2 ? reviveUse[entref.entnum] : 0);
		}, false);
		GSC::AddMethod("coopsetdowned", [](Game::scr_entref_t entref)
		{
			SetDown(entref.entnum, Game::Scr_GetInt(0) != 0);
		}, false);
		Utils::Hook::Set<void*>(0x6DEBF4, reinterpret_cast<void*>(PlayerDie_Hook)); // entityHandlers[0x11].die

		// coop_hurt <client> <damage>: damage a player through G_Damage (QA of downed / revive). Inflictor and attacker: the world.
		Command::Add("coop_hurt", [](Command::Params* params)
		{
			if (params->size() < 3)
				return;
			const int mod = params->size() > 3 ? std::atoi(params->get(3)) : 0;
			const float yaw = params->size() > 4 ? static_cast<float>(std::atof(params->get(4))) : -1000.0f;
			Hurt(std::atoi(params->get(1)), std::atoi(params->get(2)), mod, yaw);
		});

		Command::Add("coop_give", []
		{
			if (!psLayoutOk || players < 2)
			{
				Log("coop_give: not available (layout ok %d, players %d)", psLayoutOk, players);
				return;
			}
			MirrorKit();
		});

		// coop_status: both players' state into cod4coop\coop.log (QA)
		Command::Add("coop_status", []
		{
			const auto* running = Dvars::Functions::Dvar_FindVar("sv_running");
			if (!running || !running->current.enabled)
			{
				Log("status: no level running");
				return;
			}
			const auto* sp = Dvars::Functions::Dvar_FindVar("coop_script_players");
			const auto* skill = Dvars::Functions::Dvar_FindVar("g_gameskill");
			Log("status: g_gameskill %d", skill ? skill->current.integer : -1);
			Log("status: level.time %d maxclients %d num_entities %d players %d | script level.players.size %s", LevelTime(),
				*reinterpret_cast<int*>(LEVEL_MAXCLIENTS), *reinterpret_cast<int*>(LEVEL_NUM_ENTITIES), players, sp ? sp->current.string : "(unset)");
			for (int i = 0; i < players; i++)
			{
				const auto* hits = Dvars::Functions::Dvar_FindVar(i ? "coop_hits1" : "coop_hits0");
				if (hits && hits->type == Game::dvar_type::string && hits->current.string)
					Log("status: hits on player %d: %s", i + 1, hits->current.string);
				const auto* bd = Dvars::Functions::Dvar_FindVar(i ? "coop_body1" : "coop_body0");
				if (bd && bd->type == Game::dvar_type::string && bd->current.string && *bd->current.string)
				{
					const int bn = std::atoi(bd->current.string);
					if (bn > 1 && bn < 0x880)
					{
						const float* bo = reinterpret_cast<float*>(Entity(bn) + ENT_ORIGIN);
						Log("status: body of player %d = entity %d inuse %d at (%.0f %.0f %.0f)", i + 1, bn, Entity(bn)[ENT_INUSE], bo[0], bo[1], bo[2]);
					}
				}
				std::uint8_t* cl = GameClient(i);
				std::uint8_t* ent = Entity(i);
				const float* o = reinterpret_cast<float*>(cl + GC_ORIGIN);
				Log("status: client %d weapons %08x %08x", i, *reinterpret_cast<std::uint32_t*>(cl + 0x534), *reinterpret_cast<std::uint32_t*>(cl + 0x538));
				Log("status: client %d downed %d pmf %x eye %d/%.1f connected %d clientNum %d pm %d weapon %d origin (%.0f %.0f %.0f) view yaw %.0f | ent inuse %d health %d client %p sentient %p",
					i, IsDown(i) ? 1 : 0, *reinterpret_cast<std::uint32_t*>(cl + 0xC), *reinterpret_cast<int*>(cl + 0xF0), *reinterpret_cast<float*>(cl + 0xF4),
					*reinterpret_cast<int*>(cl + GC_CONNECTED), *reinterpret_cast<int*>(cl + GC_CLIENTNUM), *reinterpret_cast<int*>(cl + GC_PMTYPE), *reinterpret_cast<int*>(cl + GC_WEAPON),
					o[0], o[1], o[2], reinterpret_cast<float*>(cl + GC_VIEWANGLES)[1], ent[ENT_INUSE],
					reinterpret_cast<Game::gentity_s*>(ent)->health, *reinterpret_cast<void**>(ent + ENT_CLIENT), *reinterpret_cast<void**>(ent + ENT_SENTIENT));
			}
		});

		CoopScripts::Install();
		CoopQA::Install();
		CoopNet::Install();

		Log("playerState_s layout %s (size 0x%x, ammo +0x%x, weapons +0x%x)", psLayoutOk ? "matches the exe" : "DOES NOT MATCH",
			static_cast<int>(sizeof(Game::playerState_s)), static_cast<int>(offsetof(Game::playerState_s, ammo)),
			static_cast<int>(offsetof(Game::playerState_s, weapons)));
		Log("co-op module loaded (gentity_s.health at +0x%x, client +0x%x, sentient +0x%x)",
			static_cast<int>(offsetof(Game::gentity_s, health)), static_cast<int>(offsetof(Game::gentity_s, client)),
			static_cast<int>(offsetof(Game::gentity_s, sentient)));
	}
}
