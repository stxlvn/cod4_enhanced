#pragma once

namespace Components
{
	// Phase 2: IW4-style lockstep between two PCs (see local/cod4/IW4-NETCODE-SPEC.md).
	// Both PCs run the whole level; only player input crosses the wire. Every 50 ms tick the host builds a co-op frame
	// (tick time, state hash, per player his new usercmds), sends it and executes it; the partner executes the same frame
	// and may only step a tick it holds the frame for. Host = player 1 (client 0), partner = player 2 (client 1).
	class CoopNet
	{
	public:
		enum Role { ROLE_OFF = 0, ROLE_HOST = 1, ROLE_PARTNER = 2 };

		static void Install();
		static int Role();
		static int LocalClient();
		// a lockstep session drives the running level
		static bool Active();
		// session seed for G_InitGame's holdrand (valid when Active)
		static int Seed();
		// overlay text while this PC waits for the other one (called from the HUD draw hook)
		static void DrawOverlay();
		// G_InitGame in a session: a new level life, the lockstep restarts with a new load barrier
		static void OnLevelStart();
		// player 1 died, the SP death reload was armed (ClientThink_real); a session reloads by simulation time
		static void __cdecl OnDeathReloadArmed();
	};
}
