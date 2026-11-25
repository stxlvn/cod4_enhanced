#pragma once

namespace Components
{
	// Co-op core (Phase 1: a second player in one SP process, driven by a test bot).
	// IW3 SP is built for one client; this module gives it the two-client storage IW4 SP (Spec Ops) has natively:
	// svs.clients and level.clients point at 2-entry arrays of our own, level.maxclients = 2 when co-op is on,
	// entity 1 is the second player, and the second client is connected and spawned next to the first.
	class Coop : public Component
	{
	public:
		Coop();

		static void Log(const char* fmt, ...);
		// Number of players in the current level (1 = plain single player).
		static int Players();
		// Entity / client pointers (raw: the mod's structs are not checked against the exe for every field).
		static std::uint8_t* Entity(int num);
		static std::uint8_t* GameClient(int num);
		static std::uint8_t* ServerClient(int num);

	private:
		static int __cdecl ClientConnect_Hook();
		static void ConnectSecondClient();
		static void __cdecl ClientSpawn_Hook(std::uint8_t* ent);
		static int __cdecl SelectSpawnPoint_Hook(float* origin, float* angles);
		static void __cdecl PlayerTrail_Hook(std::uint8_t* ent);
		static void BotThink();
		static bool FindSpotNear(const float* center, float yawDeg, float* out);
	};
}
