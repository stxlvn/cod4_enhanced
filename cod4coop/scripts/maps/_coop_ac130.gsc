// CoD4 co-op: Death from Above (ac130) for two gunners.
// Both players ride the gunship (level.ac130 "tag_player", the same view arcs). Each holds one of its three guns (105 mm, 40 mm,
// 25 mm; player 1 starts on the 105, player 2 on the 40). "Next weapon" goes to the next gun the other player does not hold:
// a gun in use cannot be taken.
// Each player's own: his gun, ammo and reload, field of view (cg_fov set on his PC only), crosshair overlay, gun list and thermal
// text on his own HUD (player coophudnext), thermal white / black hot (vision set on his PC only), the red X before clearance,
// the shell shock. Shared as in the game: the radio, the plane, the text blocks, timer and coordinates, earthquakes (both sit in
// the same plane), the 105's dark flash.
// Started by text patches of maps\_ac130 (CoopScripts.cpp): each function there that is one player's hands over to the one here
// when two players play the mission (not in the end credits, which reuse the level). All of it is simulation (both PCs run it
// alike); only coopsetsaveddvar / coopvisionlocal act on one PC, and they change nothing the simulation reads.
#include maps\_utility;
#include common_scripts\utility;

coop_on()
{
	return isdefined( level.players ) && level.players.size > 1 && !isdefined( level.credits_active );
}

coop_other( p )
{
	if ( p == level.players[ 0 ] )
		return level.players[ 1 ];
	return level.players[ 0 ];
}

// _ac130::init right after level.player's ignoreme (before his 105 is given): player 2 is a gunner too
coop_init()
{
	level.coop_no_kit_mirror = true; // _coop: his kit is the gunship's other gun, not a copy of player 1's
	level.coop_hide_bodies = true; // both bodies would sit at the gunner's tag, in the other one's view
	for ( i = 1; i < level.players.size; i++ )
	{
		p = level.players[ i ];
		p.ignoreme = true;
		p takeallweapons();
		p SetActionSlot( 3, "" );
	}
	coop_guns();
}

// the three guns (as _ac130::changeWeapons lists them) and who holds which
coop_guns()
{
	if ( isdefined( level.ac130_weapon ) )
		return;
	level.ac130_weapon = [];
	level.ac130_weapon[ 0 ] = coop_gun_def( "ac130_overlay_105mm", "55", "105mm", &"AC130_HUD_WEAPON_105MM", "ac130_105mm" );
	level.ac130_weapon[ 1 ] = coop_gun_def( "ac130_overlay_40mm", "25", "40mm", &"AC130_HUD_WEAPON_40MM", "ac130_40mm" );
	level.ac130_weapon[ 2 ] = coop_gun_def( "ac130_overlay_25mm", "10", "25mm", &"AC130_HUD_WEAPON_25MM", "ac130_25mm" );
	level.players[ 0 ].coop_gun = 0;
	level.players[ 1 ].coop_gun = 1;
	setdvar( "coop_ac_guns", "0-1" ); // QA trace: player 1's and player 2's gun index
}

coop_gun_def( overlay, fov, name, text, weapon )
{
	gun = spawnstruct();
	gun.overlay = overlay;
	gun.fov = fov;
	gun.name = name;
	gun.string = text;
	gun.weapon = weapon;
	if ( getdvar( "ac130_alternate_controls" ) != "0" )
		gun.weapon = weapon + "_alt";
	return gun;
}

// _ac130::changeWeapons
coop_change_weapons()
{
	coop_guns();
	level.currentWeapon = level.ac130_weapon[ level.players[ 0 ].coop_gun ].name;
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] thread coop_fire_effects();
	level.players[ 1 ] thread coop_first_gun();
	thread coop_qa_trace();
	// one entry in the command table: the key notifies the player who pressed it (CoopNet coopcmdn)
	level.players[ 0 ] notifyOnCommand( "switch weapons", "weapnext" );
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] thread coop_switch_loop();
}

// player 2's first gun one frame in: a weapon given at load time, before the first frame copied player 1's view hands to him
// (CoopNet, ps +0xE0), crashed the partner's cgame (null viewmodel DObj at 0x430A28), as _coop's loadout_mirror also waits
coop_first_gun()
{
	wait 0.05;
	self coop_set_gun( self.coop_gun, true );
}

coop_switch_loop()
{
	for ( ;; )
	{
		self waittill( "switch weapons" );
		// no weapon changes during a cinematic
		if ( isdefined( level.doing_cinematic ) )
		{
			wait 0.05;
			continue;
		}
		self notify( "shot weapon" );
		other = coop_other( self );
		next = self.coop_gun;
		for ( k = 0; k < level.ac130_weapon.size; k++ )
		{
			next++;
			if ( next >= level.ac130_weapon.size )
				next = 0;
			if ( next != other.coop_gun )
				break;
		}
		if ( next == self.coop_gun )
			continue;
		self coop_set_gun( next, false );
		self thread play_sound_on_entity( "ac130_weapon_switch" );
		flag_set( "player_changed_weapons" );
	}
}

// self takes gun i: overlay, gun list, field of view (his PC), the weapon itself, ammo. At the start player 1 keeps the game's own
// view (the original sets the field of view on switches only); player 2 starts on the 40 with its view.
coop_set_gun( i, initial )
{
	self.coop_gun = i;
	gun = level.ac130_weapon[ i ];
	if ( self == level.player )
	{
		level.currentWeapon = gun.name;
		level.playerWeapon = gun.weapon;
	}
	if ( isdefined( self.coop_hud ) && isdefined( self.coop_hud[ "crosshairs" ] ) )
		self.coop_hud[ "crosshairs" ] setshader( gun.overlay, 640, 480 );
	self thread coop_blink_crosshairs( gun.weapon );
	for ( k = 0; k < level.players.size; k++ )
		level.players[ k ] thread coop_blink_gun_list();
	if ( getdvar( "ac130_alternate_controls" ) == "0" && !( initial && i == 0 ) )
		self coopsetsaveddvar( "cg_fov", gun.fov );
	self takeallweapons();
	self giveweapon( gun.weapon );
	self switchtoweapon( gun.weapon );
	coop_set_ammo();
	setdvar( "coop_ac_guns", level.players[ 0 ].coop_gun + "-" + level.players[ 1 ].coop_gun ); // QA trace
}

// the gun list on self's HUD: his gun pulses between 0.6 and 1 (never darker than the others: the game's blink to 0 left the free
// gun, steady at 0.5, looking selected half of the time), the free gun 0.3, the other gunner's 0.1
coop_blink_gun_list()
{
	self notify( "coop_blink_list" );
	self endon( "coop_blink_list" );
	if ( !isdefined( self.coop_hud ) || !isdefined( self.coop_hud[ "weapon_text" ] ) )
		return;
	list = self.coop_hud[ "weapon_text" ];
	other = coop_other( self );
	for ( i = 0; i < list.size; i++ )
	{
		list[ i ].alpha = 0.3;
		if ( i == other.coop_gun )
			list[ i ].alpha = 0.1;
	}
	h = list[ self.coop_gun ];
	h.alpha = 1;
	for ( ;; )
	{
		h fadeOverTime( 0.4 );
		h.alpha = 0.6;
		wait 0.4;
		h fadeOverTime( 0.4 );
		h.alpha = 1;
		wait 0.4;
	}
}

// self's crosshair blinks while his 105 reloads (_ac130::blink_crosshairs)
coop_blink_crosshairs( weaponName )
{
	self notify( "coop_stop_blink" );
	self endon( "coop_stop_blink" );
	if ( !isdefined( self.coop_hud ) || !isdefined( self.coop_hud[ "crosshairs" ] ) )
		return;
	h = self.coop_hud[ "crosshairs" ];
	h.alpha = 1;
	if ( !issubstr( tolower( weaponName ), "105" ) )
		return;
	waittillframeend;
	if ( level.weaponReadyToFire[ weaponName ] )
		return;
	for ( ;; )
	{
		h fadeOverTime( 0.3 );
		h.alpha = 0;
		wait 0.3;
		h fadeOverTime( 0.3 );
		h.alpha = 1;
		wait 0.3;
	}
}

// _ac130::gun_fired_and_ready_105mm's blink_crosshairs_stop: the 105 is ready again
coop_crosshairs_stop()
{
	for ( i = 0; i < level.players.size; i++ )
	{
		p = level.players[ i ];
		p notify( "coop_stop_blink" );
		if ( isdefined( p.coop_hud ) && isdefined( p.coop_hud[ "crosshairs" ] ) )
			p.coop_hud[ "crosshairs" ].alpha = 1;
	}
}

// _ac130::fire_screenShake for self's gun
coop_fire_effects()
{
	for ( ;; )
	{
		self waittill( "weapon_fired" );
		name = level.ac130_weapon[ self.coop_gun ].name;
		cleared = !( getdvar( "ac130_pre_engagement_mode" ) == "2" && !flag( "clear_to_engage" ) );
		if ( name == "105mm" && cleared )
		{
			thread maps\_ac130::gun_fired_and_ready_105mm();
			earthquake( 0.2, 1, self.origin, 1000 );
		}
		else if ( name == "40mm" && cleared )
			earthquake( 0.1, 0.5, self.origin, 1000 );
		wait 0.05;
	}
}

// _ac130::weaponFiredThread: a shot starts its gun's reload
coop_weapon_fired()
{
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] thread coop_weapon_fired_loop();
}

coop_weapon_fired_loop()
{
	for ( ;; )
	{
		self waittill( "weapon_fired" );
		list = self GetWeaponsListPrimaries();
		if ( !isdefined( list[ 0 ] ) || !level.weaponReadyToFire[ list[ 0 ] ] )
			continue;
		setdvar( "coop_ac_fired" + self.coop_index, list[ 0 ] ); // QA trace
		self thread coop_blink_crosshairs( list[ 0 ] );
		thread maps\_ac130::weaponReload( list[ 0 ] );
	}
}

// _ac130::setAmmo for both: one round in each ready gun once cleared to engage
coop_set_ammo()
{
	count = 0;
	if ( flag( "clear_to_engage" ) )
		count = 1;
	for ( i = 0; i < level.players.size; i++ )
	{
		list = level.players[ i ] GetWeaponsListPrimaries();
		for ( k = 0; k < list.size; k++ )
		{
			if ( level.weaponReadyToFire[ list[ k ] ] )
				level.players[ i ] SetWeaponAmmoClip( list[ k ], count );
		}
	}
}

// _ac130::invertThermal: each player switches his own thermal mode (white / black hot) on his own screen
coop_thermal()
{
	level.players[ 0 ] notifyOnCommand( "switch thermal", "+usereload" );
	level.players[ 0 ] notifyOnCommand( "switch thermal", "+activate" );
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] thread coop_thermal_loop();
}

coop_thermal_loop()
{
	inverted = false;
	for ( ;; )
	{
		self waittill( "switch thermal" );
		if ( isdefined( level.doing_cinematic ) )
		{
			wait 0.05;
			continue;
		}
		inverted = !inverted;
		vision = "ac130";
		text = &"AC130_HUD_THERMAL_WHOT";
		if ( inverted )
		{
			vision = "ac130_inverted";
			text = &"AC130_HUD_THERMAL_BHOT";
		}
		self coopvisionlocal( vision, 0 );
		if ( isdefined( self.coop_hud ) && isdefined( self.coop_hud[ "thermal_mode" ] ) )
			self.coop_hud[ "thermal_mode" ] settext( text );
		setdvar( "coop_ac_thermal" + self.coop_index, vision ); // QA trace
	}
}

// _ac130::nofireCrossHair: each player's own red X while he holds fire before the clearance
coop_nofire()
{
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] thread coop_nofire_player();
	thread coop_nofire_clear();
}

coop_nofire_clear()
{
	level waittill( "clear_to_engage" );
	coop_set_ammo();
	setdvar( "coop_ac_clear", 1 ); // QA trace
}

coop_nofire_player()
{
	self coophudnext();
	x = newHudElem();
	x.x = 0;
	x.y = 0;
	x.alignX = "center";
	x.alignY = "middle";
	x.horzAlign = "center";
	x.vertAlign = "middle";
	x setshader( "ac130_overlay_nofire", 64, 64 );
	x.alpha = 0;
	thread coop_nofire_remove( x );
	level endon( "clear_to_engage" );
	for ( ;; )
	{
		while ( self attackButtonPressed() )
		{
			if ( isdefined( level.doing_cinematic ) )
			{
				wait 0.05;
				break;
			}
			x.alpha = 1;
			x fadeOverTime( 1.0 );
			x.alpha = 0;
			wait 1.0;
		}
		wait 0.05;
	}
}

coop_nofire_remove( x )
{
	level waittill( "clear_to_engage" );
	x destroy();
}

// _ac130::shotFired for each gunner's rounds
coop_shot_fired()
{
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] thread coop_shot_fired_loop();
}

coop_shot_fired_loop()
{
	for ( ;; )
	{
		self waittill( "projectile_impact", weaponName, position, radius );
		thread maps\_ac130::shotFiredFriendlyProximity( weaponName, position );
		if ( issubstr( tolower( weaponName ), "105" ) )
			thread maps\_ac130::shotFiredDarkScreenOverlay();
		thread maps\_ac130::shotFiredBadPlace( position, weaponName );
		if ( getdvar( "ac130_ragdoll_deaths" ) == "1" )
			thread maps\_ac130::shotFiredPhysicsSphere( position, weaponName );
		wait 0.05;
	}
}

// _ac130::ac130ShellShock
coop_shellshock( duration )
{
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] shellshock( "ac130", duration );
}

// _ac130::attachPlayer for both gunners
coop_attach()
{
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] playerLinkToDelta( level.ac130, "tag_player", 1.0, 65, 65, 40, 40 );
	wait 0.05;
	for ( i = 0; i < level.players.size; i++ )
	{
		p = level.players[ i ];
		p allowProne( false );
		p allowCrouch( false );
		p setplayerangles( level.ac130 getTagAngles( "tag_player" ) );
	}
	SetSavedDvar( "ammoCounterHide", "1" );
	SetSavedDvar( "hud_showStance", 0 );
}

// _ac130::overlay: the shared text blocks, timer, coordinates and grain once; the crosshair, thermal text and gun list per player
coop_overlay()
{
	level.HUDItem = [];
	if ( getdvar( "ac130_hud_text_misc" ) == "1" )
	{
		level.HUDItem[ "hud_text_top" ] = coop_hud_text( undefined, "left", "top", 0, 0, &"AC130_HUD_TOP_BAR" );
		level.HUDItem[ "hud_text_left" ] = coop_hud_text( undefined, "left", "top", 0, 60, &"AC130_HUD_LEFT_BLOCK" );
		level.HUDItem[ "hud_text_right" ] = coop_hud_text( undefined, "right", "top", 0, 50, &"AC130_HUD_RIGHT_BLOCK" );
		level.HUDItem[ "hud_text_bottom" ] = coop_hud_text( undefined, "center", "bottom", 0, 0, &"AC130_HUD_BOTTOM_BLOCK" );
	}
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] coop_overlay_player();
	level.HUDItem[ "crosshairs" ] = level.players[ 0 ].coop_hud[ "crosshairs" ];
	thread maps\_ac130::hud_timer();
	thread maps\_ac130::overlay_coords();
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] thread coop_blink_gun_list();

	level.HUDItem[ "grain" ] = newHudElem();
	level.HUDItem[ "grain" ].x = 0;
	level.HUDItem[ "grain" ].y = 0;
	level.HUDItem[ "grain" ].alignX = "left";
	level.HUDItem[ "grain" ].alignY = "top";
	level.HUDItem[ "grain" ].horzAlign = "fullscreen";
	level.HUDItem[ "grain" ].vertAlign = "fullscreen";
	level.HUDItem[ "grain" ] setshader( "ac130_overlay_grain", 640, 480 );
	level.HUDItem[ "grain" ].alpha = 0.4;
	level.HUDItem[ "grain" ].sort = -3;

	thread maps\_ac130::ac130ShellShock();
	wait 0.05;
	setsaveddvar( "g_friendlynamedist", 0 );
	setsaveddvar( "compass", 0 );
	setblur( 1.2, 0 );
	setsaveddvar( "cg_tracerwidth", 16 );
}

coop_overlay_player()
{
	self.coop_hud = [];
	gun = level.ac130_weapon[ self.coop_gun ];
	self coophudnext();
	c = newHudElem();
	c.x = 0;
	c.y = 0;
	c.alignX = "center";
	c.alignY = "middle";
	c.horzAlign = "center";
	c.vertAlign = "middle";
	c setshader( gun.overlay, 640, 480 );
	c.sort = -2;
	self.coop_hud[ "crosshairs" ] = c;
	if ( getdvar( "ac130_hud_text_thermal" ) == "1" )
		self.coop_hud[ "thermal_mode" ] = coop_hud_text( self, "right", "top", -80, 50, &"AC130_HUD_THERMAL_WHOT" );
	if ( getdvar( "ac130_hud_text_weapons" ) == "1" )
	{
		list = [];
		for ( i = 0; i < level.ac130_weapon.size; i++ )
			list[ i ] = coop_hud_text( self, "left", "bottom", 0, i * -30, level.ac130_weapon[ i ].string );
		self.coop_hud[ "weapon_text" ] = list;
	}
}

// a text element of the gunner HUD (owner: undefined = both players' screens)
coop_hud_text( owner, align, valign, x, y, text )
{
	if ( isdefined( owner ) )
		owner coophudnext();
	h = newHudElem();
	h.x = x;
	h.y = y;
	h.alignX = align;
	h.alignY = valign;
	h.horzAlign = align;
	h.vertAlign = valign;
	h.fontScale = 2.5;
	h settext( text );
	h.alpha = 1.0;
	return h;
}

// QA trace: the weapon in each gunner's hands (server side), coop_ac_cur = "<player 1's>/<player 2's>"
coop_qa_trace()
{
	for ( ;; )
	{
		wait 0.5;
		setdvar( "coop_ac_cur", level.players[ 0 ] getcurrentweapon() + "/" + level.players[ 1 ] getcurrentweapon() );
	}
}
