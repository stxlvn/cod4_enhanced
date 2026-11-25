// CoD4 co-op: The Coup for two players. The whole level is one scene from the captive president's eyes, written for level.player
// only (captive state, the rigs he is linked to, shell shocks, rumble, depth of field). Owner's call (2026-11-25): in this mission
// the client watches the host's camera. So player 2 is a captive too and his PC shows player 1's camera for the whole level
// (coopspectate: player 1's view, head look, shell shocks, depth of field and HUD, smooth, native), while in the simulation he is
// linked to the same rigs as player 1 (same place for triggers, sounds and the snapshot's visibility) with both bodies hidden.
// Started by text patches of coup.gsc / coup_anim.gsc / coup_code.gsc (CoopScripts.cpp). All of it is simulation (both PCs alike);
// coopspectate acts on player 2's PC only.
#include common_scripts\utility;
#include maps\_utility;
#include maps\_anim;

// the players other than level.player
coop_partners()
{
	list = [];
	if ( !isdefined( level.players ) )
		return list;
	for ( i = 0; i < level.players.size; i++ )
	{
		if ( level.players[ i ] != level.player )
			list[ list.size ] = level.players[ i ];
	}
	return list;
}

// main: player 1 has been made a captive (no crouch / prone, no weapons, ignored, invulnerable): the others too, watching his camera
coop_captives()
{
	level.player.coop_scene_hide = true;
	list = coop_partners();
	for ( i = 0; i < list.size; i++ )
	{
		p = list[ i ];
		p allowcrouch( false );
		p allowprone( false );
		p takeAllWeapons();
		p.ignoreme = true;
		p EnableInvulnerability();
		p.coop_scene_hide = true;
		p coopspectate( level.player getentitynumber() );
	}
	setdvar( "coop_coup_captives", list.size + 1 ); // QA trace
}

// level.player linked to a scene rig's tag_player (playerlinkto): the others linked to the same tag
coop_share_view( rig, right, left, top, bottom )
{
	list = coop_partners();
	for ( i = 0; i < list.size; i++ )
	{
		p = list[ i ];
		p unlink();
		p setorigin( rig gettagorigin( "tag_player" ) );
		p setplayerangles( level.player getplayerangles() );
		p playerlinkto( rig, "tag_player", 1, right, left, top, bottom );
	}
	setdvar( "coop_coup_view", "shared_" + right ); // QA trace
}

// the same with playerlinktodelta (the drive: level.player on his seat rig in the car)
coop_share_view_delta( rig, right, left, top, bottom )
{
	list = coop_partners();
	for ( i = 0; i < list.size; i++ )
	{
		p = list[ i ];
		p unlink();
		p setorigin( rig gettagorigin( "tag_player" ) );
		p setplayerangles( level.player getplayerangles() );
		p playerlinktodelta( rig, "tag_player", 1, right, left, top, bottom );
	}
	setdvar( "coop_coup_view", "drive" ); // QA trace
}

// execCarExit: level.player is linked to an origin that moves from his seat view to the car exit rig over 2 s: the others too
coop_carexit_partners( origin )
{
	list = coop_partners();
	for ( i = 0; i < list.size; i++ )
		list[ i ] thread coop_carexit_follow( origin );
}

coop_carexit_follow( origin )
{
	self unlink();
	wait 0.05;
	self playerlinktodelta( origin, "tag_origin", 1, 0, 0, 0, 0 );
}

coop_shellshock_all( name, time )
{
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] shellshock( name, time );
}

coop_rumble_all( name )
{
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] PlayRumbleOnEntity( name );
}

coop_freeze_all( on )
{
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] freezeControls( on );
}

// coup_anim playerHitDamage (the hit in the car): down to 10% health, the red screen, for every player
coop_hit_damage_all()
{
	for ( i = 0; i < level.players.size; i++ )
	{
		p = level.players[ i ];
		newHealth = p.health * 0.10;
		damage = ( p.health - newHealth ) / getdvarfloat( "player_damageMultiplier" );
		p doDamage( damage, p.origin );
	}
}

// coup_code setdefaultdepthoffield runs every frame for level.player: the same depth of field on the others
coop_dof_partners()
{
	list = coop_partners();
	for ( i = 0; i < list.size; i++ )
		list[ i ] setDepthOfField( level.dofDefault[ "nearStart" ], level.dofDefault[ "nearEnd" ], level.dofDefault[ "farStart" ], level.dofDefault[ "farEnd" ], level.dofDefault[ "nearBlur" ], level.dofDefault[ "farBlur" ] );
}
