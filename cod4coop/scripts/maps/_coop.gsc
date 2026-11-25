// CoD4 co-op framework (started from maps\_load when the level has two players).
// Phase 3: player bodies. Single player draws no other players (the SP cgame skips ET_PLAYER), so every player gets a body:
// a script model dressed like a friendly soldier of this level, following its player every server frame with AI animations.
// It is part of the simulation (both PCs run this script identically); the native side leaves a PC's own player's body
// out of that PC's view (dvars coop_body0 / coop_body1 = entity numbers).
#include maps\_utility;
#using_animtree( "generic_human" );

main()
{
	level.coop_bodies = [];
	for ( i = 0; i < level.players.size; i++ )
	{
		level.players[ i ].coop_index = i;
		level.players[ i ] thread body_follow( i );
		level.players[ i ] thread downed_watch();
		level.players[ i ] thread fire_watch();
		level.players[ i ] thread hit_watch();
		level.players[ i ] thread qa_damage_watch();
	}
	for ( i = 1; i < level.players.size; i++ )
	{
		level.players[ i ] thread player_regen();
		level.players[ i ] thread weapon_watch();
	}
	thread loadout_mirror();
	thread qa_commands();
}

// ---- Phase 4: start of level ----

// maps\_loadout gives the level's start kit to level.player only: the other players get the same weapons and ammo.
loadout_mirror()
{
	wait 0.05; // after _load's give_loadout
	if ( isdefined( level.coop_no_kit_mirror ) )
		return; // a level module gives player 2 his own kit (ac130: the gunship's other gun)
	p1 = level.player;
	list = p1 getweaponslist();
	// the weapon in player 1's hands: _loadout's switchToWeapon has only been sent to his client yet ("none" for a few frames)
	current = p1 getcurrentweapon();
	for ( t = 0; t < 60 && current == "none" && list.size; t++ )
	{
		wait 0.05;
		current = p1 getcurrentweapon();
	}
	for ( i = 1; i < level.players.size; i++ )
	{
		p = level.players[ i ];
		// _load sets maxhealth on level.player only (level.player.maxhealth = level.player.health); 0 here broke the revive
		if ( p.maxhealth <= 0 )
			p.maxhealth = level.player.maxhealth;
		p takeallweapons();
		for ( k = 0; k < list.size; k++ )
			p giveweapon( list[ k ] );
		// ammo after all the weapons: giving an alt mode (gl_m4_grenadier) refills its base weapon's stock
		for ( k = 0; k < list.size; k++ )
		{
			p setweaponammoclip( list[ k ], p1 getweaponammoclip( list[ k ] ) );
			p setweaponammostock( list[ k ], p1 getweaponammostock( list[ k ] ) );
		}
		p setoffhandsecondaryclass( "flash" );
		if ( isdefined( current ) && current != "none" )
			p switchtoweapon( current );
	}
	// QA: the same kit (weapons, clip and stock ammo) on every player
	bad = 0;
	detail = "";
	for ( i = 1; i < level.players.size; i++ )
	{
		for ( k = 0; k < list.size; k++ )
		{
			w = list[ k ];
			if ( !level.players[ i ] hasweapon( w ) || level.players[ i ] getweaponammoclip( w ) != p1 getweaponammoclip( w ) || level.players[ i ] getweaponammostock( w ) != p1 getweaponammostock( w ) )
			{
				bad++;
				detail = w + " " + p1 getweaponammoclip( w ) + "/" + p1 getweaponammostock( w ) + " vs " + level.players[ i ] getweaponammoclip( w ) + "/" + level.players[ i ] getweaponammostock( w );
			}
		}
	}
	setdvar( "coop_qa_kit", list.size + " weapons, " + bad + " differ (" + detail + "), in hand " + current );
}

// ---- Phase 4: a weapon in the other players' hands ----
// The weapon in hand is the client's choice: switchToWeapon only sends "sw" to the player's client. After a loaded save the
// partner's client has none selected (its state is not in the save), and a level script may give weapons to level.player only.
// So once a second: a player who stands with nothing in his hands but has primaries takes the first one.
weapon_watch()
{
	while ( true )
	{
		wait 1;
		if ( isdefined( self.coop_isdown ) || self.health <= 0 )
			continue;
		if ( self getcurrentweapon() != "none" )
			continue;
		list = self getweaponslistprimaries();
		if ( list.size )
			self switchtoweapon( list[ 0 ] );
	}
}

// ---- Phase 4: health regen of the other players ----
// maps\_gameskill::playerHealthRegen runs for level.player only (checked: player 2 stayed at 50 health, player 1 went back to
// 100 in 8 s). The same rules for the others: full health after playerHealth_RegularRegenDelay when not very hurt, slow regen
// (0.1 a frame after longRegenTime) when very hurt, and the difficulty's short protection after a big enough hit
// (playerInvul: attackerAccuracy 0 + ignoreRandomBulletDamage). Not ported: the red overlay, blur and breathing, which are
// level.player's script HUD and sounds (CoD4 SP has no per-client HUD elements: both PCs would show them).
player_regen()
{
	while ( !isdefined( level.playerHealth_RegularRegenDelay ) || !isdefined( level.healthOverlayCutoff ) )
		wait 0.05;
	self thread maps\_gameskill::playerHurtcheck(); // self.hurtAgain on damage
	regenRate = 0.1;
	oldratio = 1;
	lastinvulratio = 1;
	veryHurt = false;
	hurtTime = 0;
	for ( ;; )
	{
		wait 0.05;
		waittillframeend;
		if ( isdefined( self.coop_isdown ) )
		{
			oldratio = 1;
			lastinvulratio = 1;
			veryHurt = false;
			continue;
		}
		if ( self.health >= self.maxhealth )
		{
			lastinvulratio = 1;
			veryHurt = false;
			continue;
		}
		if ( self.health <= 0 )
			return;

		wasVeryHurt = veryHurt;
		ratio = self.health / self.maxhealth;
		if ( ratio <= level.healthOverlayCutoff )
		{
			veryHurt = true;
			if ( !wasVeryHurt )
				hurtTime = gettime();
		}
		if ( self.hurtAgain )
		{
			hurtTime = gettime();
			self.hurtAgain = false;
		}

		if ( ratio >= oldratio )
		{
			if ( gettime() - hurtTime < level.playerHealth_RegularRegenDelay )
				continue;
			if ( veryHurt )
			{
				newHealth = ratio;
				if ( gettime() > hurtTime + level.longRegenTime )
					newHealth += regenRate;
			}
			else
				newHealth = 1;
			if ( newHealth > 1.0 )
				newHealth = 1.0;
			if ( newHealth <= 0 )
				return;
			self setnormalhealth( newHealth );
			oldratio = self.health / self.maxhealth;
			continue;
		}

		oldratio = lastinvulratio;
		invulWorthyHealthDrop = oldratio - ratio >= level.worthyDamageRatio;
		if ( self.health <= 1 )
		{
			self setnormalhealth( 2 / self.maxhealth );
			invulWorthyHealthDrop = true;
		}
		oldratio = self.health / self.maxhealth;
		hurtTime = gettime();
		if ( !invulWorthyHealthDrop || getdvarfloat( "scr_playerInvulTimeScale" ) <= 0.0 )
			continue;
		if ( isdefined( self.coop_invul ) )
			continue;
		self.coop_invul = true;
		if ( veryHurt && !wasVeryHurt )
			invulTime = level.invulTime_onShield;
		else if ( veryHurt )
			invulTime = level.invulTime_postShield;
		else
			invulTime = level.invulTime_preShield;
		invulTime *= getdvarfloat( "scr_playerInvulTimeScale" );
		lastinvulratio = self.health / self.maxhealth;
		self thread player_invul( invulTime );
	}
}

// maps\_gameskill::playerInvul for this player
player_invul( timer )
{
	if ( timer > 0 )
	{
		self.attackerAccuracy = 0;
		self.ignoreRandomBulletDamage = true;
		wait timer;
	}
	self.attackerAccuracy = anim.player_attacker_accuracy;
	self.ignoreRandomBulletDamage = false;
	self.coop_invul = undefined;
}

// ---- Phase 4: downed and revive = IW4 Spec Ops last stand (MW2 maps\_coop.gsc) ----
// Native side (Coop.cpp): a player whose partner is up does not die, he is marked down in his player state (coopdowned) and is
// kept alive while down (death shield); the marked player moves like IW4 last stand: eye height 22, crawling at 0.2 of the speed
// (coop_crawlscale), crouched bounds. Script side, as MW2 player_coop_set_down_attributes / _part1 / _part2:
// down -> invulnerable, ignored by the AI, weapons off; 0.3 s later his pistol (when he has one), 0.25 s later vulnerable again,
// the AI sees him again once he fires or after 4 s; the next hit puts the pistol away, shell shock, invulnerable and ignored.
// The partner revives him by holding Use within 128 units for 1.5 s (both frozen, the bleed-out timer paused) -> full health.
// Bleed-out 120 / 120 / 90 / 60 s by difficulty; bleed-out or both down -> mission failed. The dvars coop_downtime<n> /
// coop_revive<n> feed each PC's on-screen text (native overlay).
downed_watch()
{
	while ( true )
	{
		if ( self coopdowned() && !isdefined( self.coop_isdown ) )
			self downed();
		wait 0.05;
	}
}

fire_watch()
{
	while ( true )
	{
		self waittill( "weapon_fired" );
		self.coop_lastfire = gettime();
	}
}

bleedout_time()
{
	switch ( level.gameskill )
	{
		case 2:
			return 90;
		case 3:
			return 60;
	}
	return 120;
}

downed()
{
	self.coop_isdown = true;
	self.coop_down_from = self getstance();
	self.coop_preincap_weapon = self getcurrentweapon(); // before the weapons go off (MW2 takes it in part 1)
	i = self.coop_index;
	partner = level.players[ 1 - i ];

	// player_coop_set_down_attributes (setting a player's health changes his maxhealth too: MW2 puts the original back)
	if ( !isdefined( self.coop_maxhealth ) )
		self.coop_maxhealth = self.maxhealth;
	if ( self.coop_maxhealth <= 0 )
		self.coop_maxhealth = 100;
	self enableinvulnerability();
	self.health = 2;
	self.maxhealth = self.coop_maxhealth;
	self.ignoreme = true;
	self.ignoreRandomBulletDamage = true;
	self disableweapons();
	self allowstand( false );
	self allowprone( false );
	self allowcrouch( true );
	self allowjump( false );
	self allowsprint( false );
	self setstance( "crouch" );
	self thread down_part1();

	left = bleedout_time() * 20; // server frames
	held = 0;
	revived = false;
	quote = "A downed player was not revived in time.";
	while ( left > 0 )
	{
		if ( isdefined( partner.coop_isdown ) || partner.health <= 0 )
		{
			quote = "Both players went down.";
			break;
		}
		// Use within 128 units (the native side takes it out of the partner's usercmd: coopreviveuse; usebuttonpressed covers a
		// player whose input does not go through it, player 1 in one process)
		if ( ( partner coopreviveuse() || partner usebuttonpressed() ) && distance( partner.origin, self.origin ) < 128 )
		{
			if ( !held )
				self revive_freeze( partner, true );
			held++;
		}
		else
		{
			if ( held )
				self revive_freeze( partner, false );
			held = 0;
			left--; // the bleed-out timer pauses while the partner revives (coop_pause_bleedout_timer)
		}
		setdvar( "coop_downtime" + i, int( ( left + 19 ) / 20 ) );
		setdvar( "coop_revive" + i, int( held * 100 / 30 ) );
		if ( held >= 30 ) // 1.5 s (player_coop_revive_buddy totalTime)
		{
			revived = true;
			break;
		}
		wait 0.05;
	}
	setdvar( "coop_downtime" + i, 0 );
	setdvar( "coop_revive" + i, 0 );
	if ( held )
		self revive_freeze( partner, false );
	if ( !revived )
	{
		self.coop_bled_out = true; // his body plays its death
		setdvar( "ui_deadquote", quote ); // the fail screen's line (the same text on both PCs)
		maps\_utility::missionFailedWrapper();
		return;
	}
	self notify( "coop_revived" );
	self get_up();
}

// player_coop_freeze_players (self = the downed player)
revive_freeze( partner, frozen )
{
	partner freezecontrols( frozen );
	self freezecontrols( frozen );
	if ( frozen )
	{
		partner disableweapons();
		self disableweapons();
		return;
	}
	partner enableweapons();
	if ( isdefined( self.coop_down_armed ) && !isdefined( self.coop_down_part2 ) )
		self enableweapons();
}

// player_coop_set_down_part1 / _part2
down_part1()
{
	self endon( "coop_revived" );
	wait 0.3;
	// player_coop_force_switch_to_pistol (MW2 gives a Beretta when he has none; CoD4 can only give weapons the level loaded)
	pistol = undefined;
	list = self getweaponslistprimaries();
	for ( k = 0; k < list.size; k++ )
	{
		if ( weaponclass( list[ k ] ) == "pistol" )
			pistol = list[ k ];
	}
	if ( isdefined( pistol ) )
	{
		self switchtoweapon( pistol );
		self.coop_down_armed = true;
		self enableweapons();
	}
	wait 0.25;
	self disableinvulnerability();
	self thread down_draw_attention();

	self waittill( "damage" );
	self.coop_down_part2 = true;
	self disableweapons();
	self thread down_dying_effect();
	self.ignoreme = true;
	self.ignoreRandomBulletDamage = true;
	self enableinvulnerability();
}

// player_coop_down_draw_attention: the AI ignores him until he fires, or 4 s
down_draw_attention()
{
	self endon( "coop_revived" );
	self endon( "damage" );
	self waittill_notify_or_timeout( "weapon_fired", 4 );
	self.ignoreme = false;
	self.ignoreRandomBulletDamage = false;
}

// player_dying_effect
down_dying_effect()
{
	self endon( "coop_revived" );
	while ( true )
	{
		self shellshock( "default", 60 );
		wait 60;
	}
}

// player_coop_set_original_attributes + player_coop_getup
get_up()
{
	self coopsetdowned( 0 ); // native: normal movement again
	self.coop_isdown = undefined;
	self.coop_down_armed = undefined;
	self.coop_down_part2 = undefined;
	self stopshellshock();
	self disableweapons();
	if ( isdefined( self.coop_preincap_weapon ) && self.coop_preincap_weapon != "none" && self hasweapon( self.coop_preincap_weapon ) )
		self switchtoweapon( self.coop_preincap_weapon );
	self.coop_preincap_weapon = undefined;
	self.maxhealth = self.coop_maxhealth;
	self.health = self.coop_maxhealth;
	self.ignoreme = false;
	self.ignoreRandomBulletDamage = false;
	self allowstand( true );
	self allowprone( true );
	self allowjump( true );
	self allowsprint( true );
	self setstance( "stand" );
	self enableweapons();
	wait 1.0;
	self disableinvulnerability();
}

body_follow( index )
{
	// the look of a friendly soldier of this level (first ally there is)
	guy = undefined;
	while ( true )
	{
		allies = getaiarray( "allies" );
		if ( allies.size )
		{
			guy = allies[ 0 ];
			break;
		}
		wait 1;
	}

	body = spawn( "script_model", self.origin );
	body setmodel( guy.model );
	// the anim tree before any attach (as the game's drones: setmodel, UseAnimTree, attach): attach rebuilds the model with the tree;
	// with the tree set after the attachments the body stayed in its bind pose
	body useAnimTree( #animtree );
	count = guy getattachsize();
	for ( k = 0; k < count; k++ )
	{
		tag = guy getattachtagname( k );
		model = guy getattachmodelname( k );
		if ( tag == "tag_weapon_right" || tag == "tag_weapon_left" || tag == "tag_inhand" )
			continue; // the AI's weapon: the body carries its player's weapon instead
		body attach( model, tag );
	}
	body notsolid();
	level.coop_bodies[ index ] = body;
	setdvar( "coop_body" + index, body getentitynumber() );

	key = "";
	gun = "";
	fall_end = undefined;
	fired = 0;
	last = self.origin;
	while ( true )
	{
		pos = self.origin;
		vel = ( pos - last ) * 20; // units per second (server frames are 50 ms)
		speed = length( ( vel[ 0 ], vel[ 1 ], 0 ) );
		last = pos;
		view = self getplayerangles();
		body.origin = pos;
		body.angles = ( 0, view[ 1 ], 0 );
		// a level can hide both bodies (ac130: both gunners sit at the same tag, a body would fill the other one's view), a scene one
		// player's (self.coop_scene_hide: he is a first-person rig, e.g. caught by Price on the Seaknight's ramp)
		hide_now = isdefined( level.coop_hide_bodies ) || isdefined( self.coop_scene_hide );
		if ( hide_now != isdefined( body.coop_hidden ) )
		{
			if ( hide_now )
			{
				body hide();
				body.coop_hidden = true;
			}
			else
			{
				body show();
				body.coop_hidden = undefined;
			}
		}

		// the player's weapon in the body's hand
		weapon = self getcurrentweapon();
		want_gun = "";
		if ( isdefined( weapon ) && weapon != "none" && self.health > 0 )
			want_gun = getweaponmodel( weapon );
		if ( want_gun != gun )
		{
			if ( gun != "" )
				body detach( gun, "tag_weapon_right" );
			if ( want_gun != "" )
				body attach( want_gun, "tag_weapon_right" );
			gun = want_gun;
		}

		// last stand, or a scene that knocks the player down (self.coop_scene_down, e.g. the cargoship explosion): on his back
		if ( isdefined( self.coop_isdown ) || isdefined( self.coop_scene_down ) )
		{
			body.coop_key = ""; // the standing poses start again after a revive
			// last stand: the CoD4 AI's wounded-on-the-back set (animscripts\pain.gsc dying crawl) for IW4's pb_laststand_*:
			// fall from his stance onto the back, then idle / crawl / fire with the pistol, death when he bleeds out
			if ( !isdefined( fall_end ) )
			{
				fall = body_fall_anim( self.coop_down_from );
				body setanimknoballrestart( fall, %root, 1, 0.2, 1 );
				key = "down_fall";
				fall_end = gettime() + int( getanimlength( fall ) * 1000 );
			}
			else if ( gettime() >= fall_end )
			{
				want = "down_idle";
				if ( isdefined( self.coop_bled_out ) )
					want = "down_death";
				else if ( isdefined( self.coop_lastfire ) && gettime() - self.coop_lastfire < 400 )
					want = "down_fire";
				else if ( speed > 5 )
					want = "down_crawl";
				if ( want != key || ( want == "down_fire" && self.coop_lastfire != fired ) )
				{
					key = want;
					if ( want == "down_fire" )
						fired = self.coop_lastfire;
					body setanimknoballrestart( body_anim( key ), %root, 1, 0.2, 1 );
				}
			}
		}
		else
		{
			fall_end = undefined;
			key = "";
			// the move over about three frames: one frame's origin step is noisy (a diagonal walk flipped F / L every frame)
			if ( !isdefined( body.coop_vel ) )
				body.coop_vel = vel;
			body.coop_vel = ( body.coop_vel + vel ) * 0.5;
			smooth = body.coop_vel;
			self body_update( body, length( ( smooth[ 0 ], smooth[ 1 ], 0 ) ), smooth, view );
		}
		wait 0.05;
	}
}

// ---- Phase 4: the body's poses = IW4's player animations (mp/playeranim.script: pb_* whole body, pt_* torso) with their
// CoD4 SP analogs, the AI's own tables (animscripts\combat.gsc set_animarray_standing / crouching / prone, pain.gsc, death.gsc).
// IW4 movetype / event                          CoD4 analog
//   idle, idlecr, idleprone (+ pistol)          exposed_aim_5, exposed_crouch_aim_5, prone_aim_5, pistol_stand_aim_5, pistol_crouchaim_idle;
//                                               view pitch on the AI's aim additives (aim_8 up, aim_2 down, +-45 as the AI's limits)
//   walk, walkbk, strafing left / right         walk_lowready_F / B / L / R
//   run, runbk, strafing left / right           run_lowready_F / B / L / R
//   sprint                                      sprint1_loop
//   walkcr, runcr, walkcrbk, runcrbk, strafing  crouch_fastwalk_F / B / L / R
//   walkprone, walkpronebk                      prone_crawl
//   climbup, climbdown (ladder)                 ladder_climbup, ladder_climbdown (rate by the climb speed, still when he stops)
//   mantle_*                                    traverse40
//   jump / land                                 traverse_jumpdown_40
//   crouch_to_prone, prone_to_crouch, ...       crouch_2_prone, prone_2_crouch, stand_2_prone, prone_2_stand, exposed_stand_2_crouch,
//                                               exposed_crouch_2_stand (when the stance changes standing still)
//   fireweapon                                  add_fire (the AI's additive recoil) exposed_shoot_semi1
//   reload                                      exposed_reload, exposed_crouch_reload, prone_reload, pistol_stand_reload_A,
//                                               run_lowready_reload (moving)
//   meleeattack                                 melee_1, prone_melee
//   fireweapon (grenade)                        exposed_grenadeThrowB, crouch_grenade_throw, prone_grenade_A
//   flinch_forward / backward / left / right    minor_pain: minorpain_chest / head _front / _back / _left / _right, weight by damage
//   shellshock, stunned                         exposed_flashbang_v1
//   DEATH (by movetype, explosion by direction) exposed_death, exposed_crouch_death_twist, prone_death_quickdeath, pistol_death_1,
//                                               death_run_forward_crumple, death_explosion_stand_F / B / L / R_v1
//   laststand set                               see the downed branch above
// Inputs (all from the simulation, so both PCs animate alike; GSC has no hex literals: 16384 = 0x4000 ...): stance, the move
// from the origin delta against the view yaw,
// pm_flags (coopflags: mantle 0x4, ladder 0x8, jumping 0x4000, sprinting 0x8000, shellshocked 0x10000), weaponstate
// (coopweaponstate: reload 7-11, melee 12-14, offhand 15-20, sprint 22-24), the weapon class, "weapon_fired", "damage".
body_update( body, speed, vel, view )
{
	t = gettime();
	stance = self getstance();
	flags = self coopflags();
	ws = self coopweaponstate();
	weapon = self getcurrentweapon();
	pistol = false;
	if ( isdefined( weapon ) && weapon != "none" && weaponclass( weapon ) == "pistol" )
		pistol = true;
	if ( !isdefined( body.coop_key ) || !isdefined( body.coop_ws ) )
	{
		body.coop_key = "";
		body.coop_action = "";
		body.coop_action_end = 0;
		body.coop_stance = stance;
		body.coop_ws = ws;
		body.coop_flags = flags;
		body.coop_fired = 0;
		body.coop_hitseen = 0;
		body.coop_aim = false;
	}

	if ( self.health <= 0 )
	{
		if ( body.coop_key != "death" )
		{
			body.coop_key = "death";
			body.coop_action = "";
			setdvar( "coop_pose" + self.coop_index, "death" );
			body setanimknoballrestart( self body_death_anim( stance, speed, pistol ), %root, 1, 0.2, 1 );
		}
		return;
	}

	// IW4 events: one-shot animations started by a change of the player's state
	start = "";
	if ( ( flags & 4 ) && !( body.coop_flags & 4 ) )
		start = "mantle";
	else if ( body_in( ws, 7, 11 ) && !body_in( body.coop_ws, 7, 11 ) )
		start = "reload";
	else if ( body_in( ws, 12, 14 ) && !body_in( body.coop_ws, 12, 14 ) )
		start = "melee";
	else if ( body_in( ws, 15, 20 ) && !body_in( body.coop_ws, 15, 20 ) )
		start = "throw";
	else if ( ( flags & 16384 ) && !( body.coop_flags & 16384 ) && !( flags & 8 ) && stance == "stand" && body.coop_stance == "stand" )
		start = "jump"; // 0x4000 is also set when he drops prone (the jump slowdown): a jump only standing
	else if ( stance != body.coop_stance && speed < 30 )
		start = body.coop_stance + "_2_" + stance;
	else if ( ( flags & 65536 ) && !( body.coop_flags & 65536 ) )
		start = "shock";
	body.coop_flags = flags;
	body.coop_ws = ws;
	body.coop_stance = stance;
	if ( start != "" )
	{
		a = body_action_anim( start, stance, speed, pistol );
		if ( isdefined( a ) )
		{
			body.coop_action = start;
			body.coop_action_end = t + int( getanimlength( a ) * 1000 );
			body.coop_key = start;
			body.coop_aim = false;
			setdvar( "coop_pose" + self.coop_index, start );
			body setanimknoballrestart( a, %root, 1, 0.15, 1 );
		}
	}

	// an action holds the body until its animation ends; reload, melee and throw also end with the player's state
	if ( body.coop_action != "" )
	{
		over = t >= body.coop_action_end;
		if ( body.coop_action == "reload" && !body_in( ws, 7, 11 ) )
			over = true;
		if ( body.coop_action == "melee" && !body_in( ws, 12, 14 ) )
			over = true;
		if ( body.coop_action == "throw" && !body_in( ws, 15, 20 ) )
			over = true;
		if ( body.coop_action == "jump" && !( flags & 16384 ) )
			over = true;
		if ( ( issubstr( body.coop_action, "_2_" ) || body.coop_action == "shock" ) && speed > 30 )
			over = true;
		if ( !over )
		{
			self body_fire_and_flinch( body, t );
			return;
		}
		body.coop_action = "";
		body.coop_key = "";
	}

	// IW4 movetypes: the base pose
	dir = body_move_dir( vel, view[ 1 ], body.coop_dir );
	body.coop_dir = dir;
	rate = 1;
	if ( flags & 8 )
	{
		key = "ladder_up";
		if ( vel[ 2 ] < -5 )
			key = "ladder_down";
		rate = 0;
		if ( abs( vel[ 2 ] ) > 5 )
			rate = 1;
	}
	else if ( stance == "prone" )
	{
		key = "prone";
		if ( speed > 5 )
			key = "prone_move";
	}
	else if ( stance == "crouch" )
	{
		key = "crouch";
		if ( speed > 15 )
			key = "crouch_" + dir;
	}
	else if ( speed > 15 && ( ( flags & 32768 ) || body_in( ws, 22, 24 ) ) )
		key = "sprint";
	else if ( speed > 140 || ( speed > 115 && issubstr( body.coop_key, "run_" ) ) ) // hysteresis: backwards runs at about 130
		key = "run_" + dir;
	else if ( speed > 15 )
		key = "walk_" + dir;
	else
		key = "stand";
	if ( pistol && ( key == "stand" || key == "crouch" ) )
		key = key + "_pistol";
	a = body_anim( key );
	if ( key != body.coop_key )
	{
		body.coop_key = key;
		setdvar( "coop_pose" + self.coop_index, key ); // QA trace
		body setanimknoball( a, %root, 1, 0.2, 1 ); // the whole branch up to the root gets weight 1
		body.coop_aim = body body_aim_setup( key );
	}
	// the move animations play at the player's speed (no sliding feet); the ladder stands still when he does
	if ( body_is_move( key ) )
	{
		own = body_anim_speed( key, a );
		if ( own > 1 )
		{
			rate = speed / own;
			if ( rate < 0.5 )
				rate = 0.5;
			if ( rate > 1.8 )
				rate = 1.8;
		}
		body setanim( a, 1, 0.2, rate );
	}
	else if ( key == "ladder_up" || key == "ladder_down" )
		body setanim( a, 1, 0.2, rate );
	if ( body.coop_aim )
		body body_aim_pitch( view[ 0 ] );
	self body_fire_and_flinch( body, t );
}

body_in( value, low, high )
{
	return value >= low && value <= high;
}

// IW4's strafing / back conditions: the move against the view, the bigger of forward and side; the direction it had stays
// until the other part is clearly bigger (diagonal moves flipped between two animations every frame)
body_move_dir( vel, yaw, had )
{
	forward = anglestoforward( ( 0, yaw, 0 ) );
	right = anglestoright( ( 0, yaw, 0 ) );
	f = vel[ 0 ] * forward[ 0 ] + vel[ 1 ] * forward[ 1 ];
	r = vel[ 0 ] * right[ 0 ] + vel[ 1 ] * right[ 1 ];
	if ( isdefined( had ) )
	{
		if ( ( had == "F" || had == "B" ) && abs( f ) * 1.4 >= abs( r ) )
			r = 0;
		else if ( ( had == "L" || had == "R" ) && abs( r ) * 1.4 >= abs( f ) )
			f = 0;
	}
	if ( abs( f ) >= abs( r ) )
	{
		if ( f >= 0 )
			return "F";
		return "B";
	}
	if ( r >= 0 )
		return "R";
	return "L";
}

body_is_move( key )
{
	return issubstr( key, "walk_" ) || issubstr( key, "run_" ) || issubstr( key, "crouch_" ) || key == "sprint" || key == "prone_move";
}

// the animation's own ground speed (root motion over its length), cached per key
body_anim_speed( key, a )
{
	if ( !isdefined( level.coop_animspeed ) )
		level.coop_animspeed = [];
	if ( !isdefined( level.coop_animspeed[ key ] ) )
	{
		d = getmovedelta( a, 0, 1 );
		len = getanimlength( a );
		level.coop_animspeed[ key ] = 0;
		if ( len > 0 )
			level.coop_animspeed[ key ] = length( ( d[ 0 ], d[ 1 ], 0 ) ) / len;
	}
	return level.coop_animspeed[ key ];
}

// the AI's aim additives for the still poses (combat.gsc setupAim: add_aim_up / add_aim_down knobs, then the aim_8 / aim_2 weights)
body_aim_setup( key )
{
	up = undefined;
	down = undefined;
	switch ( key )
	{
		case "stand":
			up = %exposed_aim_8;
			down = %exposed_aim_2;
			break;
		case "stand_pistol":
			up = %pistol_stand_aim_8_add;
			down = %pistol_stand_aim_2_add;
			break;
		case "crouch":
			up = %exposed_crouch_aim_8;
			down = %exposed_crouch_aim_2;
			break;
		case "prone":
			up = %prone_aim_8_add;
			down = %prone_aim_2_add;
			break;
	}
	if ( !isdefined( up ) )
		return false;
	self setanimknoblimited( up, 1, 0 );
	self setanimknoblimited( down, 1, 0 );
	return true;
}

// view pitch (positive looks down) on the aim additives, +-45 degrees full weight (the AI's upAimLimit / downAimLimit)
body_aim_pitch( pitch )
{
	up = 0 - pitch;
	if ( up > 0 )
	{
		w = up / 45;
		if ( w > 1 )
			w = 1;
		self setanimlimited( %aim_2, 0, 0.05 );
		self setanimlimited( %aim_8, w, 0.05 );
	}
	else
	{
		w = pitch / 45;
		if ( w > 1 )
			w = 1;
		self setanimlimited( %aim_8, 0, 0.05 );
		self setanimlimited( %aim_2, w, 0.05 );
	}
}

// fireweapon: the AI's additive recoil; flinch: minor_pain by hit direction (hit_watch), weight by damage as pain.gsc PlayHitAnimation
body_fire_and_flinch( body, t )
{
	if ( isdefined( self.coop_lastfire ) && self.coop_lastfire != body.coop_fired )
	{
		body.coop_fired = self.coop_lastfire;
		body setanim( %add_fire, 1, 0.05, 1 );
		body setanimknobrestart( %exposed_shoot_semi1, 1, 0, 1 );
		body.coop_fire_end = t + 250;
	}
	else if ( isdefined( body.coop_fire_end ) && t > body.coop_fire_end )
	{
		body clearanim( %add_fire, 0.2 );
		body.coop_fire_end = undefined;
	}
	if ( isdefined( self.coop_hit ) && self.coop_hit != body.coop_hitseen )
	{
		body.coop_hitseen = self.coop_hit;
		w = self.coop_hit_weight;
		body clearanim( %minor_pain, 0.1 );
		body setanim( self.coop_hit_anim, 1, 0.05, 1 );
		body setanim( %minor_pain, w, 0.05 / w, 1 );
		body.coop_hit_end = t + 350;
	}
	else if ( isdefined( body.coop_hit_end ) && t > body.coop_hit_end )
	{
		body clearanim( %minor_pain, 0.2 );
		body.coop_hit_end = undefined;
	}
}

// IW4 flinch_* / death by hit direction: where the damage came from against the view, and how
hit_watch()
{
	while ( true )
	{
		self waittill( "damage", amount, attacker, dir, point, type );
		self.coop_lastdamagetype = type;
		self.coop_lastdamagedir = dir;
		if ( self.health <= 0 || isdefined( self.coop_isdown ) || !isdefined( dir ) )
			continue;
		side = body_hit_side( dir, self getplayerangles()[ 1 ] );
		place = "chest";
		if ( isdefined( point ) && self getstance() == "stand" && point[ 2 ] > self.origin[ 2 ] + 52 )
			place = "head";
		self.coop_hit_anim = body_flinch_anim( place, side );
		self.coop_hit_weight = ( amount + 50.0 ) / 250;
		if ( self.coop_hit_weight > 1 )
			self.coop_hit_weight = 1;
		self.coop_hit = gettime();
	}
}

// "front" = the hit came from in front of him
body_hit_side( dir, yaw )
{
	from = vectortoangles( ( 0 - dir[ 0 ], 0 - dir[ 1 ], 0 ) );
	rel = from[ 1 ] - yaw;
	while ( rel > 180 )
		rel -= 360;
	while ( rel < -180 )
		rel += 360;
	if ( rel >= -45 && rel <= 45 )
		return "front";
	if ( rel > 45 && rel < 135 )
		return "left";
	if ( rel < -45 && rel > -135 )
		return "right";
	return "back";
}

body_flinch_anim( place, side )
{
	if ( place == "head" )
	{
		switch ( side )
		{
			case "front":
				return %minorpain_head_front;
			case "back":
				return %minorpain_head_back;
			case "left":
				return %minorpain_head_left;
		}
		return %minorpain_head_right;
	}
	switch ( side )
	{
		case "front":
			return %minorpain_chest_front;
		case "back":
			return %minorpain_chest_back;
		case "left":
			return %minorpain_chest_left;
	}
	return %minorpain_chest_right;
}

body_action_anim( action, stance, speed, pistol )
{
	switch ( action )
	{
		case "reload":
			if ( stance == "prone" )
				return %prone_reload;
			if ( stance == "crouch" )
				return %exposed_crouch_reload;
			if ( pistol )
				return %pistol_stand_reload_A;
			if ( speed > 60 )
				return %run_lowready_reload;
			return %exposed_reload;
		case "melee":
			if ( stance == "prone" )
				return %prone_melee;
			return %melee_1;
		case "throw":
			if ( stance == "prone" )
				return %prone_grenade_A;
			if ( stance == "crouch" )
				return %crouch_grenade_throw;
			return %exposed_grenadeThrowB;
		case "mantle":
			return %traverse40;
		case "jump":
			return %traverse_jumpdown_40;
		case "shock":
			return %exposed_flashbang_v1;
		case "stand_2_crouch":
			return %exposed_stand_2_crouch;
		case "crouch_2_stand":
			return %exposed_crouch_2_stand;
		case "crouch_2_prone":
			return %crouch_2_prone;
		case "prone_2_crouch":
			return %prone_2_crouch;
		case "stand_2_prone":
			return %stand_2_prone;
		case "prone_2_stand":
			return %prone_2_stand;
	}
	return undefined;
}

// IW4 DEATH: by movetype, explosions by the direction of the blast
body_death_anim( stance, speed, pistol )
{
	type = self.coop_lastdamagetype;
	if ( isdefined( type ) && stance != "prone" && ( issubstr( type, "GRENADE" ) || issubstr( type, "EXPLOSIVE" ) || issubstr( type, "PROJECTILE" ) ) && isdefined( self.coop_lastdamagedir ) )
	{
		switch ( body_hit_side( self.coop_lastdamagedir, self getplayerangles()[ 1 ] ) )
		{
			case "front":
				return %death_explosion_stand_B_v1; // blown backwards
			case "back":
				return %death_explosion_stand_F_v1;
			case "left":
				return %death_explosion_stand_R_v1;
		}
		return %death_explosion_stand_L_v1;
	}
	if ( stance == "prone" )
		return %prone_death_quickdeath;
	if ( stance == "crouch" )
		return %exposed_crouch_death_twist;
	if ( speed > 130 )
		return %death_run_forward_crumple;
	if ( pistol )
		return %pistol_death_1;
	return %exposed_death;
}

body_anim( key )
{
	switch ( key )
	{
		case "down_idle":
			return %dying_back_idle;
		case "down_crawl":
			return %dying_crawl_back;
		case "down_fire":
			return %dying_back_fire;
		case "down_death":
			return %dying_back_death_v1;
		case "stand_pistol":
			return %pistol_stand_aim_5;
		case "crouch":
			return %exposed_crouch_aim_5;
		case "crouch_pistol":
			return %pistol_crouchaim_idle;
		case "prone":
			return %prone_aim_5;
		case "prone_move":
			return %prone_crawl;
		case "walk_F":
			return %walk_lowready_F;
		case "walk_B":
			return %walk_lowready_B;
		case "walk_L":
			return %walk_lowready_L;
		case "walk_R":
			return %walk_lowready_R;
		case "run_F":
			return %run_lowready_F;
		case "run_B":
			return %run_lowready_B;
		case "run_L":
			return %run_lowready_L;
		case "run_R":
			return %run_lowready_R;
		case "crouch_F":
			return %crouch_fastwalk_F;
		case "crouch_B":
			return %crouch_fastwalk_B;
		case "crouch_L":
			return %crouch_fastwalk_L;
		case "crouch_R":
			return %crouch_fastwalk_R;
		case "sprint":
			return %sprint1_loop;
		case "ladder_up":
			return %ladder_climbup;
		case "ladder_down":
			return %ladder_climbdown;
	}
	return %exposed_aim_5; // stand (the idle_alert anims are overlays: alone they left the bind pose)
}

// the fall onto the back from the stance he went down in (animscripts\pain.gsc stand_2_back / crouch_2_back / prone_2_back)
body_fall_anim( stance )
{
	if ( stance == "prone" )
		return %dying_crawl_2_back;
	if ( stance == "crouch" )
		return %dying_crouch_2_back;
	return %dying_stand_2_back_v1;
}

// QA: hits per player in the dvars coop_hits<n> (count, the last attacker kind, damage and time); coop_qa_keephealth 1 (set on
// both PCs) puts a standing player back to full health every frame, so a long fight can be counted without anybody going down.
qa_damage_watch()
{
	hits = 0;
	ai_hits = 0;
	self thread qa_keep_health();
	while ( true )
	{
		self waittill( "damage", amount, attacker );
		hits++;
		kind = "other";
		if ( isdefined( attacker ) && isai( attacker ) )
		{
			kind = "ai";
			ai_hits++;
		}
		down = 0;
		if ( isdefined( self.coop_isdown ) )
			down = 1;
		setdvar( "coop_hits" + self.coop_index, hits + " ai " + ai_hits + " last " + kind + " " + amount + " down " + down + " at " + gettime() );
	}
}

qa_keep_health()
{
	while ( true )
	{
		wait 0.05;
		if ( getdvar( "coop_qa_keephealth" ) == "1" && !isdefined( self.coop_isdown ) && self.health > 0 && self.health < self.maxhealth )
			self.health = self.maxhealth;
	}
}

// QA (one process / both PCs alike): coop_qa_cmd runs one command and clears itself.
//   "tp <n> <x> <y> <z> <yaw>"   put player n there
//   "shock <n> <seconds>"         shell shock player n ("default")
//   "give <n> <weapon>"           give player n a weapon and switch to it
//   "switch <n> <weapon>"         switch player n to a weapon he has
//   "notify <n> <message>"        notify player n (e.g. "switch weapons", as his key would)
//   "notifyall <message>"         the same notify on every player in one frame
//   "nearenemy <n> <dist> <dz>"   player n put dist units in front of the enemy AI nearest to player 1, facing it
//   "flagwatch <n> <flag>"        mirror a level flag into dvar coop_f_<flag> (0 / 1)
//   "flagset <n> <flag>"          set a level flag
//   "heroes3tp <n> <key> <x> <y> <z>"  move level.heroes3[ key ] there
//   "trigon <n> <script_noteworthy>"   turn those triggers on
qa_commands()
{
	setdvar( "coop_qa_cmd", "" );
	while ( true )
	{
		wait 0.05;
		cmd = getdvar( "coop_qa_cmd" );
		if ( cmd == "" )
			continue;
		setdvar( "coop_qa_cmd", "" );
		w = strtok( cmd, " " );
		if ( w.size < 2 )
			continue;
		if ( w[ 0 ] == "flagwatch" && w.size >= 3 )
		{
			// the level flag w[ 2 ] mirrored into dvar coop_f_<flag> (0 / 1) for the test runner's waitdvar
			thread qa_flag_mirror( w[ 2 ] );
			continue;
		}
		if ( w[ 0 ] == "flagset" && w.size >= 3 )
		{
			common_scripts\utility::flag_set( w[ 2 ] ); // a level flag a test start point leaves unset
			continue;
		}
		if ( w[ 0 ] == "heroes3tp" && w.size >= 6 && isdefined( level.heroes3 ) && isdefined( level.heroes3[ w[ 2 ] ] ) )
		{
			// a hero of the level (level.heroes3[ key ]) moved like the level's own start points do (cargoship jumptoActor)
			level.heroes3[ w[ 2 ] ] thread qa_actor_tp( ( int( w[ 3 ] ), int( w[ 4 ] ), int( w[ 5 ] ) ) );
			continue;
		}
		if ( w[ 0 ] == "trigon" && w.size >= 3 )
		{
			// the triggers with this script_noteworthy on (a test start point that skips where the level turns them on)
			trigs = getentarray( w[ 2 ], "script_noteworthy" );
			for ( k = 0; k < trigs.size; k++ )
				trigs[ k ] common_scripts\utility::trigger_on();
			continue;
		}
		if ( w[ 0 ] == "nearenemy" && w.size >= 4 )
		{
			// player n put <dist> units in front of the nearest enemy AI (to player 1), facing it
			ai = getaiarray( "axis" );
			best = undefined;
			for ( k = 0; k < ai.size; k++ )
			{
				if ( !isdefined( best ) || distance( ai[ k ].origin, level.player.origin ) < distance( best.origin, level.player.origin ) )
					best = ai[ k ];
			}
			if ( isdefined( best ) && int( w[ 1 ] ) < level.players.size )
			{
				p = level.players[ int( w[ 1 ] ) ];
				fwd = anglestoforward( ( 0, best.angles[ 1 ], 0 ) );
				p setorigin( best.origin + vector_multiply( fwd, int( w[ 2 ] ) ) + ( 0, 0, int( w[ 3 ] ) ) );
				p setplayerangles( ( 0, best.angles[ 1 ] + 180, 0 ) );
				setdvar( "coop_qa_nearenemy", best getentitynumber() + " at " + best.origin );
			}
			continue;
		}
		if ( w[ 0 ] == "notifyall" )
		{
			// the same notify on every player in one frame, player 1 first (both keys at one tick)
			msg = w[ 1 ];
			for ( k = 2; k < w.size; k++ )
				msg = msg + " " + w[ k ];
			for ( k = 0; k < level.players.size; k++ )
				level.players[ k ] notify( msg );
			continue;
		}
		n = int( w[ 1 ] );
		if ( n < 0 || n >= level.players.size )
			continue;
		p = level.players[ n ];
		if ( w[ 0 ] == "tp" && w.size >= 6 )
		{
			p setorigin( ( int( w[ 2 ] ), int( w[ 3 ] ), int( w[ 4 ] ) ) );
			p setplayerangles( ( 0, int( w[ 5 ] ), 0 ) );
		}
		else if ( w[ 0 ] == "shock" && w.size >= 3 )
			p shellshock( "default", int( w[ 2 ] ) );
		else if ( w[ 0 ] == "give" && w.size >= 3 )
		{
			p giveweapon( w[ 2 ] );
			p switchtoweapon( w[ 2 ] );
		}
		else if ( w[ 0 ] == "switch" && w.size >= 3 )
			p switchtoweapon( w[ 2 ] );
		else if ( w[ 0 ] == "notify" && w.size >= 3 )
		{
			// a script notify on player n (the rest of the words), e.g. what his key's notifyOnCommand would send
			msg = w[ 2 ];
			for ( k = 3; k < w.size; k++ )
				msg = msg + " " + w[ k ];
			p notify( msg );
		}
	}
}

qa_flag_mirror( name )
{
	was = -1;
	while ( true )
	{
		v = 0;
		if ( isdefined( level.flag[ name ] ) && level.flag[ name ] )
			v = 1;
		if ( v != was )
		{
			was = v;
			setdvar( "coop_f_" + name, v );
		}
		wait 0.05;
	}
}

qa_actor_tp( org )
{
	self notify( "overtakenow" );
	self unlink();
	self stopanimscripted();
	link = spawn( "script_origin", self.origin );
	self linkto( link );
	link moveto( org, .2 );
	wait .25;
	self unlink();
	link delete();
	self setgoalpos( org );
}
