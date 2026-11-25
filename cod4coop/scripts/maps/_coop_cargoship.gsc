// CoD4 co-op: Crew Expendable (cargoship) for two players.
// - The package: Gaz opens the container when he gets near either player (the game: level.player); the manifest is taken by
//   whoever uses it first, once.
// - The explosion: both players are knocked down where they stand, each in his own first-person scene (view hands rig shown on
//   his PC only, coopshowonlyto); Price pulls player 1 up as in the game, Gaz pulls player 2 up with the same animation (without
//   Price's line). Both lose sprint and weapons; the bodies lie on their backs during it (_coop coop_scene_down).
// - The run on the sinking ship: every player near a quake stumbles (crouch, push, rumble, breathing), quakes and the last jolt
//   at the Seaknight follow each player; the slow speed (g_speed) is the server's, the same for both.
// - The Blackhawk ride and fast-rope: player 2 rides beside player 1 at the right door (the game's player ride animation on his
//   own hidden rig) and ropes down right after him, as the SAS do one after another on a rope; the Blackhawk waits for him
//   (he is in its unload queue). The bodies are out of the cabin while seated and slide down the rope (the SAS slide).
// - The jump: each player jumps on his own (standing in the jump zone, facing the Seaknight). Price catches the first as in the
//   game; the Seaknight waits for the second, whom Gaz catches beside Price on the ramp; then it leaves. If the second does not
//   jump in time it leaves without him: mission failed (never one player left behind).
// Started by text patches of maps\cargoship and maps\cargoship_code (CoopScripts.cpp). All of it is simulation (both PCs alike).
#include maps\_utility;
#include common_scripts\utility;
#include maps\_anim;

coop_on()
{
	return isdefined( level.players ) && level.players.size > 1;
}

coop_list()
{
	if ( isdefined( level.players ) )
		return level.players;
	list = [];
	list[ 0 ] = level.player;
	return list;
}

coop_nearest( org )
{
	list = coop_list();
	best = list[ 0 ];
	for ( i = 1; i < list.size; i++ )
	{
		if ( distance( list[ i ].origin, org ) < distance( best.origin, org ) )
			best = list[ i ];
	}
	return best;
}

coop_any_touching( trig )
{
	list = coop_list();
	for ( i = 0; i < list.size; i++ )
	{
		if ( list[ i ] istouching( trig ) )
			return true;
	}
	return false;
}

coop_quake_all( scale, duration, radius )
{
	list = coop_list();
	for ( i = 0; i < list.size; i++ )
		earthquake( scale, duration, list[ i ].origin, radius );
}

coop_rumble_all( name )
{
	list = coop_list();
	for ( i = 0; i < list.size; i++ )
		list[ i ] PlayRumbleOnEntity( name );
}

coop_allowsprint_all( on )
{
	list = coop_list();
	for ( i = 0; i < list.size; i++ )
		list[ i ] allowsprint( on );
}

// _utility::get_player_feet_from_view for any player
coop_feet_from_view( p )
{
	tagorigin = p.origin;
	upvec = anglestoup( p getplayerangles() );
	height = p GetPlayerViewHeight();
	player_eye = tagorigin + ( 0, 0, height );
	player_eye_fake = tagorigin + vector_multiply( upvec, height );
	return tagorigin + ( player_eye - player_eye_fake );
}

// Gaz plays Price's helping animations for player 2 under his own animname: the "generic" ones carry Price's voice notetracks
coop_helper_anims()
{
	if ( isdefined( level.scr_anim[ "coop_helper" ] ) )
		return;
	level.scr_anim[ "coop_helper" ][ "coop_explosion" ] = level.scr_anim[ "generic" ][ "price_explosion" ];
	level.scr_anim[ "coop_helper" ][ "coop_help" ] = level.scr_anim[ "generic" ][ "help_price" ];
}

// ---- the explosion ----

// escape_main: the heroes are blown back (escape_heroes2), except Gaz in co-op: he pulls player 2 up (coop_knockdown)
coop_heroes2()
{
	if ( !coop_on() )
	{
		array_thread( level.heroes3, maps\cargoship_code::escape_heroes2 );
		return;
	}
	keys = getarraykeys( level.heroes3 );
	for ( i = 0; i < keys.size; i++ )
	{
		if ( keys[ i ] != "grigsby" )
			level.heroes3[ keys[ i ] ] thread maps\cargoship_code::escape_heroes2();
	}
}

// cargoship_code::escape_shellshock for both players
coop_escape_shellshock()
{
	setdvar( "coop_cs_shock", "start" ); // QA trace
	coop_helper_anims();
	thread maps\cargoship_code::escape_shellshock_vision();
	thread maps\cargoship_code::escape_shellshock_heartbeat();
	coop_rumble_all( "tank_rumble" );
	level thread notify_delay( "stop_heartbeat_sound", 18 );
	SetSavedDvar( "hud_gasMaskOverlay", 0 );
	clearallcorpses();

	maps\mo_tools::playerWeaponTake();
	for ( i = 1; i < level.players.size; i++ )
		level.players[ i ] DisableWeapons();
	guns = getentarray();
	for ( i = 0; i < guns.size; i++ )
	{
		if ( getsubstr( guns[ i ].classname, 0, 7 ) == "weapon_" )
			guns[ i ] delete();
	}
	door = getent( "hallways_door", "targetname" );
	clip = getent( door.target, "targetname" );
	door delete();
	clip delete();

	// each player's scene spot: player 1 the game's, player 2 beside it
	level.players[ 0 ] thread coop_knockdown( ( 620, -296, -360 ), level.heroes3[ "price" ] );
	level.players[ 1 ] thread coop_knockdown( ( 620, -296, -360 ) + level.coop_cs_knock_offset, level.heroes3[ "grigsby" ] );

	wait 8.45;
	level notify( "escape_show_waterlevel" );
	wait 14.74;
	wait .3;
}

// self is knocked down and pulled up by helper (cargoship_code::escape_shellshock + escape_new_explosion_scene for one player)
coop_knockdown( targetorigin, helper )
{
	self.coop_down_from = self getstance();
	self.coop_scene_down = true;
	setdvar( "coop_cs_knock" + self.coop_index, "start" ); // QA trace

	dummy = spawn_anim_model( "end_hands" );
	dummy.origin = coop_feet_from_view( self );
	self setstance( "stand" );
	dummy.angles = self getplayerangles();
	dummy hide();
	dummy coopshowonlyto( self.coop_index );
	self playerlinktoabsolute( dummy, "tag_player", 1 );

	org = spawn( "script_model", self.origin );
	org.angles = self getplayerangles();
	org setmodel( "tag_origin" );
	dummy linkto( org, "tag_origin" );
	org thread play_loop_sound_on_entity( "shellshock_loop" );

	dummy2 = spawn_anim_model( "end_hands" );
	dummy2 hide();
	node = spawn( "script_origin", targetorigin );
	node.angles = ( 0, 180, 0 );
	node anim_first_frame_solo( dummy2, "player_explosion" );
	waittillframeend;
	angles = dummy2 gettagangles( "tag_player" );
	pos = dummy2 gettagorigin( "tag_player" );
	node delete();
	dummy2 delete();

	org rotateto( angles, .6, 0, 0 );
	org moveto( pos, .6 );
	org waittill( "movedone" );
	setdvar( "coop_cs_knock" + self.coop_index, "scene" ); // QA trace

	dummy unlink();
	node = spawn( "script_origin", targetorigin );
	node.angles = ( 0, 180, 0 );
	origin = GetStartOrigin( node.origin, node.angles, dummy getanim( "player_explosion" ) );
	node.origin += dummy.origin - origin;
	node.origin = ( node.origin[ 0 ], node.origin[ 1 ], -360 );
	dummy show();
	node anim_first_frame_solo( dummy, "player_explosion" );

	if ( isdefined( helper ) && isalive( helper ) )
	{
		helper stopanimscripted();
		helper unlink();
		if ( helper == level.heroes3[ "price" ] )
			node thread anim_generic( helper, "price_explosion" );
		else
			helper thread coop_helper_up( node );
	}
	node anim_single_solo( dummy, "player_explosion" );

	if ( self == level.player )
		level notify( "escape_unlink_player" );
	self unlink();
	self.coop_scene_down = undefined;
	setdvar( "coop_cs_up" + self.coop_index, 1 ); // QA trace
	dummy delete();
	node delete();
	wait 13;
	org stop_loop_sound_on_entity( "shellshock_loop" );
	org delete();
}

// Gaz pulls player 2 up (Price's animation, no Price line), then runs on with the others (escape_heroes2's end for him)
coop_helper_up( node )
{
	self.coop_oldanimname = self.animname;
	self gun_remove();
	guys = [];
	guys[ 0 ] = self;
	node anim_single( guys, "coop_explosion", undefined, undefined, undefined, "coop_helper" );
	self animscripts\shared::placeWeaponOn( self.weapon, "right" );
	self setgoalpos( self.origin );
	self.goalradius = 16;
	self.animname = self.coop_oldanimname;
	self allowedstances( "stand" );
}

// ---- the run on the sinking ship ----

// cargoship_code::escape_event: every player near the event stumbles (single player: the game's test, which then runs its own)
coop_escape_event_players( ev )
{
	if ( !coop_on() )
		return distance( level.player.origin, ev.origin ) < 700;
	for ( i = 0; i < level.players.size; i++ )
	{
		p = level.players[ i ];
		if ( distance( p.origin, ev.origin ) < 700 )
		{
			earthquake( 0.4, 1.25, p.origin, 1024 );
			p thread coop_escape_event_player();
		}
	}
	return false;
}

// cargoship_code::escape_event_player for self
coop_escape_event_player()
{
	self allowStand( false );
	self allowProne( false );
	self setstance( "crouch" );
	self setvelocity( ( 0, 1, 0 ) );
	thread maps\cargoship_code::player_speed_set( 135, .1 );
	delayThread( .5, maps\cargoship_code::player_speed_set, 185, 2.5 );
	self PlayRumbleOnEntity( "tank_rumble" );
	delayThread( .05, maps\cargoship_code::blur_overlay, 2.4, .25 );
	delayThread( .3, maps\cargoship_code::blur_overlay, 0, .5 );
	self delayThread( .1, ::play_sound_on_entity, "breathing_hurt" );
	setdvar( "coop_cs_stumble" + self.coop_index, gettime() ); // QA trace
	level waittill( "let player up" );
	self delayThread( .1, ::play_sound_on_entity, "breathing_better" );
	self allowStand( true );
	self allowProne( true );
	self setstance( "stand" );
}

// cargoship_code::escape_quake: the shaking floor at each player
coop_escape_quakes()
{
	if ( !coop_on() )
	{
		level.player thread maps\cargoship_code::escape_quake();
		return;
	}
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] thread coop_escape_quake();
}

coop_escape_quake()
{
	self endon( "stopquake" );
	while ( !flag( "escape_tilt_30" ) )
	{
		wait .1;
		earthquake( 0.15, .1, self.origin, 256 );
	}
	while ( !flag( "escape_cargohold1_enter" ) )
	{
		wait .1;
		earthquake( 0.175, .1, self.origin, 256 );
	}
	while ( !flag( "escape_hallway_lower_flag" ) )
	{
		wait .1;
		earthquake( 0.2, .1, self.origin, 256 );
	}
	while ( !isdefined( self.coop_jumped ) )
	{
		wait .1;
		earthquake( 0.225, .1, self.origin, 256 );
	}
}

// end_anim_thread: the last jolt for every player near Price when he reaches out (single player: the game's test)
coop_last_quakes( price )
{
	if ( !coop_on() )
		return distance( level.player.origin, price.origin ) < 512;
	for ( i = 0; i < level.players.size; i++ )
	{
		p = level.players[ i ];
		if ( distance( p.origin, price.origin ) < 512 )
			p thread coop_last_quake();
	}
	return false;
}

coop_last_quake()
{
	self setvelocity( ( 0, 90, 0 ) );
	earthquake( 0.25, .75, self.origin, 1024 );
	thread maps\cargoship_code::player_speed_set( 110, .1 );
	delayThread( .25, maps\cargoship_code::player_speed_set, 160, 2 );
	self PlayRumbleOnEntity( "tank_rumble" );
}

// ---- the Blackhawk ride and fast-rope ----

// mo_fastrope::fastrope_player_think (self = the Blackhawk) runs for level.player; the other players ride and rope down here
coop_fastrope_partners( heli )
{
	if ( !coop_on() )
		return;
	level.players[ 0 ].coop_scene_hide = true; // a seated body would stand through the cabin
	level.players[ 0 ] thread coop_fastrope_body_p1( heli );
	for ( i = 1; i < level.players.size; i++ )
		level.players[ i ] thread coop_fastrope_partner( heli );
}

// player 1's body: hidden while seated, sliding while he ropes down (the game moves him; the body follows him)
coop_fastrope_body_p1( heli )
{
	heli.vehicle waittill( "reached_wait_node" );
	while ( coop_in_que( heli, self ) )
		wait 0.05;
	self.coop_scene_hide = undefined;
	self.coop_scene_anim = coop_rope_slide_anim( heli );
	wait 1;
	if ( isdefined( self.time ) )
		wait self.time;
	self.coop_scene_anim = undefined;
}

coop_rope_slide_anim( heli )
{
	return level.scr_anim[ maps\mo_fastrope::fastrope_animname( heli.type, 1 ) ][ "loop" ][ 0 ];
}

coop_in_que( heli, p )
{
	for ( i = 0; i < heli.que.size; i++ )
	{
		if ( heli.que[ i ] == p )
			return true;
	}
	return false;
}

#using_animtree( "fastrope" );
coop_fastrope_partner( heli )
{
	self endon( "death" );
	self DisableWeapons();
	self.coop_scene_hide = true;

	// his seat: the player's ride animation on an anchor beside player 1's (placed in tag_detach axes, then linked)
	tagangles = heli.model gettagangles( "tag_detach" );
	off = level.coop_cs_seat_offset;
	org = heli.model gettagorigin( "tag_detach" ) + vector_multiply( anglestoforward( tagangles ), off[ 0 ] ) + vector_multiply( anglestoright( tagangles ), off[ 1 ] ) + vector_multiply( anglestoup( tagangles ), off[ 2 ] );
	anchor = spawn( "script_model", org );
	anchor setmodel( "tag_origin" );
	anchor.angles = tagangles;
	anchor linkto( heli.model, "tag_detach" );
	camera = spawn( "script_model", org );
	camera setmodel( level.models[ "player" ][ "fastrope" ] );
	camera linkto( anchor, "tag_origin", ( 0, 0, 0 ), ( 0, 0, 0 ) );
	camera.animname = maps\mo_fastrope::fastrope_animname( heli.type, "player" );
	camera UseAnimTree( #animtree );
	camera hide();
	self playerlinktodelta( camera, "tag_player", 1, 100, 100, 30, 60 );
	anchor thread anim_single_solo( camera, "ride", "tag_origin" );
	camera thread coop_ride_end();
	self allowLean( false );
	self allowcrouch( false );
	self allowprone( false );

	// the unload queue right after player 1: the Blackhawk leaves once the queue is empty
	wait .1;
	pos = heli.que.size;
	for ( i = 0; i < heli.que.size; i++ )
	{
		if ( heli.que[ i ] == level.player )
			pos = i + 1;
	}
	heli.que = array_insert( heli.que, self, pos );
	setdvar( "coop_cs_rope", "seated" ); // QA trace
	wait 2;
	self setplayerangles( ( 15, 70, 0 ) );

	heli.vehicle waittill( "reached_wait_node" );
	// player 1 goes first; the next man takes the rope once he has left the queue, at the Blackhawk's unload spacing
	while ( coop_in_que( heli, level.player ) )
		wait 0.05;
	wait heli.unloadwait + 1;
	self coop_fastrope_viewshift( camera );
	self coop_fastrope_unload( heli, camera, anchor );
}

coop_ride_end()
{
	self waittillmatch( "single anim", "end" );
	self.coop_ride_done = true;
}

// mo_fastrope::fastrope_player_viewshift_lock for self: the view turns to the rope over 1 s, then the tighter link
coop_fastrope_viewshift( camera )
{
	camera2 = spawn( "script_origin", camera.origin );
	camera2.angles = self getplayerangles();
	time = 1;
	camera2 rotateto( camera gettagangles( "tag_player" ), time, time * .5, time * .5 );
	num = int( time * 20 );
	numtotal = num;
	self freezecontrols( true );
	while ( num )
	{
		num--;
		self setplayerangles( ( camera2.angles[ 0 ], camera2.angles[ 1 ], self getplayerangles()[ 2 ] ) );
		wait .05;
		if ( num > ( numtotal * .5 ) || num % 2 )
			continue;
		if ( !num )
			break;
		camera2 rotateto( camera gettagangles( "tag_player" ), ( time * ( num / numtotal ) ) );
	}
	self setplayerangles( ( camera2.angles[ 0 ], camera2.angles[ 1 ], self getplayerangles()[ 2 ] ) );
	self playerlinktodelta( camera, "tag_player", 1, 15, 15, 5, 5 );
	wait .1;
	self freezecontrols( false );
	camera2 delete();
	while ( !isdefined( camera.coop_ride_done ) )
		wait 0.05;
}

// mo_fastrope::fastrope_player_unload_lock for self: down the right rope, landing beside player 1's spot
coop_fastrope_unload( heli, camera, anchor )
{
	link = spawn( "script_origin", camera.origin );
	link.angles = camera.angles;
	camera linkto( link );
	time1 = 1;
	back = vector_multiply( anglestoforward( ( 0, 233, 0 ) ), -50 );

	heli.que = array_remove( heli.que, self );
	heli thread maps\mo_fastrope::fastrope_que_check();
	maps\mo_fastrope::fastrope_calc( self );
	self.time += .5;
	heli.lasttime = self.time;
	setdvar( "coop_cs_rope", "sliding" ); // QA trace

	zrange = ( self.range + 100 ) * -1;
	timefrac = time1 / ( self.time + .5 );
	zfrac = zrange * timefrac;
	link rotateto( ( 80, 233, 0 ), time1, time1 * .5, time1 * .5 );
	link moveto( link.origin + back + ( 0, 0, zfrac ), time1 );
	link thread anim_loop_solo( camera, "loop", undefined, "stopanimscripted" );
	camera thread play_sound_on_entity( "fastrope_start_plr" );
	camera thread play_loop_sound_on_entity( "fastrope_loop_plr" );
	self thread coop_rope_fx();
	self.coop_scene_hide = undefined;
	self.coop_scene_anim = coop_rope_slide_anim( heli );
	wait time1;

	time3 = .5;
	time2 = self.time - time1 + .5 - time3;
	zfrac = zrange - zfrac;
	back = vector_multiply( back, -1 );
	link moveto( link.origin + back + ( 0, 0, zfrac ), ( self.time + .5 - time1 ) );
	wait time2;
	link rotateto( ( 10, 233, 0 ), time3, time3 * .5, time3 * .5 );
	wait time3 - .25;
	camera notify( "stop sound" + "fastrope_loop_plr" );
	camera thread play_sound_on_entity( "fastrope_end_plr" );
	link moveto( ( 3200, 225, 337 ) + level.coop_cs_land_offset, .5 );
	link notify( "stopanimscripted" );
	self EnableWeapons();
	wait .4;
	self unlink();
	self allowLean( true );
	self allowcrouch( true );
	self allowprone( true );
	self.coop_scene_anim = undefined;
	setdvar( "coop_cs_rope", "landed " + self.origin ); // QA trace
	link delete();
	camera delete();
	anchor delete();
}

// mo_fastrope::play_fast_rope_fx for self
coop_rope_fx()
{
	fx = spawn( "script_model", self.origin );
	fx setmodel( "tag_origin" );
	fx linkto( self );
	playfxontag( level._effect[ "rain_drops_fastrope" ], fx, "tag_origin" );
	wait 3;
	fx delete();
}

// ---- the jump to the Seaknight ----

coop_facing_heli( p )
{
	vec1 = anglestoforward( p getplayerangles() );
	vec2 = vectornormalize( level.seaknight.model.origin - p.origin );
	return vectordot( vec1, vec2 ) > .75;
}

// cargoship::end_jump for both: each one jumps on his own
coop_end_jump()
{
	level endon( "end_seaknight_leaving" );
	jump = getent( "escape_player_jump", "targetname" );
	// one entry in the command table: the key notifies the player who pressed it (CoopNet coopcmdn)
	level.players[ 0 ] NotifyOnCommand( "jump", "+gostand" );
	level.players[ 0 ] NotifyOnCommand( "jump", "+moveup" );
	level.coop_jumped = [];
	thread coop_end_jump_hint( jump );
	jump waittill( "trigger" );
	for ( i = 0; i < level.players.size; i++ )
		level.players[ i ] thread coop_end_jump_player( jump );
}

coop_end_jump_player( jump )
{
	level endon( "end_seaknight_leaving" );
	while ( true )
	{
		self waittill( "jump" );
		dot = vectordot( anglestoforward( self getplayerangles() ), vectornormalize( level.seaknight.model.origin - self.origin ) );
		setdvar( "coop_cs_jumptry" + self.coop_index, self istouching( jump ) + " " + self getstance() + " " + dot + " heli " + level.seaknight.model.origin + " me " + self.origin ); // QA trace
		if ( self istouching( jump ) && self getstance() == "stand" && coop_facing_heli( self ) )
			break;
	}
	self.coop_jumped = true;
	self freezeControls( true );
	level.coop_jumped[ level.coop_jumped.size ] = self;
	setdvar( "coop_cs_jumped", level.coop_jumped.size ); // QA trace
	if ( level.coop_jumped.size == 1 )
		flag_set( "end_start_player_anim" );
	else
		level notify( "coop_second_jump" );
}

// the jump hint (the level's mantle text and icon, one for both screens) while a player who has not jumped could jump
coop_end_jump_hint( jump )
{
	level endon( "end_seaknight_leaving" );
	while ( true )
	{
		wait 0.05;
		show = 0;
		for ( i = 0; i < level.players.size; i++ )
		{
			p = level.players[ i ];
			if ( !isdefined( p.coop_jumped ) && p istouching( jump ) && coop_facing_heli( p ) )
				show = 1;
		}
		level.hud_mantle[ "text" ].alpha = show;
		level.hud_mantle[ "icon" ].alpha = show;
		if ( level.coop_jumped.size >= level.players.size )
			return;
	}
}

// cargoship::end_nojump for both
coop_end_nojump()
{
	level endon( "end_seaknight_leaving" );
	nojump = getent( "escape_player_nojump", "targetname" );
	nojump waittill( "trigger" );
	flag_set( "end_no_jump" );
	while ( true )
	{
		for ( i = 0; i < level.players.size; i++ )
		{
			p = level.players[ i ];
			if ( !isdefined( p.coop_jumped ) )
				p allowjump( !( p istouching( nojump ) ) );
		}
		wait 0.05;
	}
}

// cargoship::end_main from the first jump on: Price catches the first jumper as in the game; the Seaknight waits for the second,
// whom Gaz catches beside Price; then it leaves and the ending runs. No second jump in time: it leaves without him, mission failed.
coop_end_rescue()
{
	coop_helper_anims();
	level.fogvalue[ "near" ] = 500;
	level.fogvalue[ "half" ] = 15000;
	setExpFog( level.fogvalue[ "near" ], level.fogvalue[ "half" ], level.fogvalue[ "r" ], level.fogvalue[ "g" ], level.fogvalue[ "b" ], 10 );
	node = getent( "escape_end_anim_node", "targetname" );
	heli = level.seaknight.model;
	price = level.heroes3[ "price" ];
	gaz = level.heroes3[ "grigsby" ];

	level._sea_org rotateto( ( 0, 0, 0 ), .25 );
	thread flag_clear_delayed( "_sea_viewbob", .25 ); // the view stops following the ship's roll

	level.coop_jumped[ 0 ] coop_rescue( heli, "tag_detach", price, "generic", "help_price" );

	for ( t = 0; level.coop_jumped.size < 2 && t < level.coop_cs_second_jump_time; t += 0.05 )
		wait 0.05;
	if ( level.coop_jumped.size < 2 )
	{
		flag_set( "end_seaknight_leaving" );
		node notify( "stop_loop_heli" );
		node thread anim_single_solo( heli, "out" );
		setDvar( "ui_deadquote", level.missionFailedQuote[ "escape" ] );
		maps\_utility::missionFailedWrapper();
		return;
	}

	// the second one beside the first on the ramp
	// beside the first one across the ramp: placed in the tag's own axes, then linked keeping that offset (linkto's offset
	// argument put him 40 units off in world axes: outside the fuselage once pulled in)
	tagangles = heli gettagangles( "tag_detach" );
	anchor = spawn( "script_model", heli gettagorigin( "tag_detach" ) + vector_multiply( anglestoright( tagangles ), level.coop_cs_ramp_side ) );
	anchor setmodel( "tag_origin" );
	anchor.angles = tagangles;
	anchor linkto( heli, "tag_detach" );
	level.coop_cs_anchor = anchor;
	// Gaz catches him once he is aboard (escape_heroes_rescue's own scene would take him back otherwise); the jumper stands
	// frozen on the edge till then (at most 4 s)
	for ( t = 0; isdefined( gaz ) && isalive( gaz ) && !isdefined( gaz.coop_boarded ) && t < 4; t += 0.05 )
		wait 0.05;
	if ( isdefined( gaz ) && ( !isalive( gaz ) || !isdefined( gaz.coop_boarded ) ) )
		gaz = undefined;
	heli notify( "stop_loopgrigsby" ); // his idle in the Seaknight (escape_heroes_rescue)
	if ( isdefined( gaz ) )
		gaz unlink();
	level.coop_jumped[ 1 ] coop_rescue( anchor, "tag_origin", gaz, "coop_helper", "coop_help" );

	flag_set( "end_linked_player_to_rig" );
	node notify( "stop_loop_heli" );
	node thread anim_single_solo( heli, "out" );

	setsaveddvar( "compass", 0 );
	SetSavedDvar( "ammoCounterHide", "1" );
	SetSavedDvar( "hud_showStance", 0 );
	delayThread( .1, maps\_weather::lightningFlash, maps\cargoship_fx::normal, maps\cargoship_fx::flash );
	delayThread( 2, maps\_weather::lightningFlash, maps\cargoship_fx::normal, maps\cargoship_fx::flash );
	delayThread( 5, maps\_weather::lightningFlash, maps\cargoship_fx::normal, maps\cargoship_fx::flash );
	delayThread( 8, maps\_weather::lightningFlash, maps\cargoship_fx::normal, maps\cargoship_fx::flash );
	delayThread( 9, maps\_weather::lightningFlash, maps\cargoship_fx::normal, maps\cargoship_fx::flash );
	delayThread( 12, maps\_weather::lightningFlash, maps\cargoship_fx::normal, maps\cargoship_fx::flash );
	delayThread( 14, maps\_weather::lightningFlash, maps\cargoship_fx::normal, maps\cargoship_fx::flash );
	delayThread( 15, maps\_weather::lightningFlash, maps\cargoship_fx::normal, maps\cargoship_fx::flash );

	flag_wait( "gotcha" );
	flag_wait( "end_finished" );
	setdvar( "coop_cs_endpos", level.coop_jumped[ 0 ].origin + " " + level.coop_jumped[ 1 ].origin + " heli " + heli.origin + " views " + level.coop_jumped[ 0 ] getplayerangles() + " " + level.coop_jumped[ 1 ] getplayerangles() + " tag " + heli gettagangles( "tag_detach" ) + " anchor " + level.coop_cs_anchor.angles ); // QA trace
	thread maps\cargoship::end_screen();
}

// self (frozen) is taken by the camera to the view hands rig at ent's tag and caught by helper
coop_rescue( ent, tag, helper, animname, anime )
{
	self freezeControls( true );
	self DisableWeapons(); // as since the explosion in the game's run (the "end" start point gives them back)
	self.coop_scene_hide = true;
	dummy = spawn_anim_model( "end_hands" );
	dummy hide();
	ent anim_first_frame_solo( dummy, "player_rescue", tag );
	dummy linkto( ent, tag );
	dummy coopshowonlyto( self.coop_index );

	camera = spawn_anim_model( "end_hands" );
	camera hide();
	camera.origin = coop_feet_from_view( self );
	camera.angles = self getplayerangles();
	self PlayerLinkToAbsolute( camera, "tag_player", 1 );
	camera moveto( dummy gettagorigin( "tag_player" ), .25 );
	camera rotateto( dummy gettagangles( "tag_player" ), .25 );
	wait .25;
	camera delete();
	dummy show();
	self PlayerLinkToAbsolute( dummy, "tag_player", 1 );
	setdvar( "coop_cs_rescued" + self.coop_index, 1 ); // QA trace

	if ( isdefined( helper ) && isalive( helper ) )
	{
		helper linkto( ent, tag );
		guys = [];
		guys[ 0 ] = helper;
		ent thread anim_single( guys, anime, tag, undefined, undefined, animname );
	}
	ent thread anim_single_solo( dummy, "player_rescue", tag );
}

// level values (tuned in game: QA screenshots)
coop_cs_init()
{
	level.coop_cs_knock_offset = ( 0, 80, 0 );
	level.coop_cs_ramp_side = 24; // units to the right of the first one on the Seaknight's ramp (tag_detach axes)
	level.coop_cs_second_jump_time = 6;
	level.coop_cs_seat_offset = ( 0, 30, 0 ); // player 2's seat from player 1's (tag_detach forward / right / up)
	level.coop_cs_land_offset = ( 0, 48, 0 ); // player 2's landing spot on the bridge from player 1's (world)
}
