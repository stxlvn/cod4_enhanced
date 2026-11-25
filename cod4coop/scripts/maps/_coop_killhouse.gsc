// CoD4 co-op: the training level (killhouse, "F.N.G.") for two players.
// - Gaz's range (rifle: aim, hip fire, penetration, timed targets; then the sidearm): both at once, each in his own station with his
//   own targets. Player 1 keeps station one and its targets (lane 1); player 2 takes station two with targets borrowed from the deck
//   mock-up course, which this level never runs (deck_target, 21 pop-up targets: moved one station over, facing the stations).
// - The other courses (the watermelon, Captain Price's ship): one player after the other; a coin toss picks who goes first.
// Started from text patches of maps/killhouse.gsc and maps/killhouse_code.gsc (CoopScripts.cpp); the rest of the level is the game's.
// All of it is simulation (both PCs run it alike): no per-PC input decides anything.
#include common_scripts\utility;
#include maps\_utility;
#include maps\_anim;
#include maps\killhouse_code;

// ---- shared helpers ----

coop_ads( player )
{
	if ( getdvar( "coop_qa_ads" ) == "1" )
		return true; // QA: one-process tests cannot aim down the sights of player 1
	return player playerads() > 0.5;
}

coop_any_ads()
{
	return coop_ads( level.players[ 0 ] ) || coop_ads( level.players[ 1 ] );
}

coop_both_ads()
{
	return coop_ads( level.players[ 0 ] ) && coop_ads( level.players[ 1 ] );
}

coop_announce( text )
{
	iprintlnbold( text );
}

coop_name( player )
{
	if ( player == level.players[ 0 ] )
		return "Player 1";
	return "Player 2";
}

// the coin toss: the shared random seed gives both PCs the same result
coop_lot( course )
{
	order = [];
	first = randomint( 2 );
	order[ 0 ] = level.players[ first ];
	order[ 1 ] = level.players[ 1 - first ];
	coop_announce( "Coin toss: " + coop_name( order[ 0 ] ) + " goes first (" + course + ")" );
	setdvar( "coop_kh_lot_" + course, first + 1 ); // QA trace
	return order;
}

// a weapon on a table is there once: the second player gets his own copy where it lay (killhouse_code ammoRespawnThink only
// respawns it for level.player)
coop_second_item( targetname, weapon )
{
	org = undefined;
	cls = undefined;
	while ( true )
	{
		items = getentarray( targetname, "targetname" );
		if ( items.size && items[ 0 ].origin[ 2 ] > -5000 )
		{
			org = items[ 0 ].origin;
			cls = items[ 0 ].classname;
		}
		have = 0;
		for ( i = 0; i < level.players.size; i++ )
		{
			if ( level.players[ i ] hasweapon( weapon ) )
				have++;
		}
		if ( have == level.players.size )
			return;
		if ( have > 0 && isdefined( org ) )
			break;
		wait 0.05;
	}
	item = spawn( cls, org + ( 0, 0, 8 ), 1 );
	item set_ammo();
	wait 0.2;
	if ( isdefined( item ) )
		item.origin = org;
}

// ---- lane 2: player 2's targets ----

lane2_init()
{
	if ( isdefined( level.coop_lane2 ) )
		return;
	deck = getentarray( "deck_target", "script_noteworthy" );
	free = [];
	for ( i = 0; i < deck.size; i++ )
	{
		pivot = getent( deck[ i ].target, "targetname" );
		if ( deck[ i ].targetname != "hostile" || !isdefined( pivot ) )
			continue;
		if ( isdefined( pivot.script_noteworthy ) && pivot.script_noteworthy == "reverse" )
			continue;
		free[ free.size ] = deck[ i ];
	}
	// lane 1 + 104 units in x (the next station): the three dummies (rifle_target_dummy lane1 0 / 1 / 2), the aim-down target
	// (aim_down_target at the floor) and the aim-up target (lane 1's hangs from the ceiling at 208: here a high pop-up)
	spots = [];
	spots[ 0 ] = ( 3657, -300, 4 );
	spots[ 1 ] = ( 3622, -84, 4 );
	spots[ 2 ] = ( 3689, 220, 4 );
	spots[ 3 ] = ( 3657, -506, 4 );
	spots[ 4 ] = ( 3657, -434, 150 );
	level.coop_lane2 = [];
	for ( i = 0; i < spots.size && i < free.size; i++ )
	{
		t = free[ i ];
		t.coop_pivot = getent( t.target, "targetname" );
		t.coop_pivot.origin = spots[ i ];
		t.coop_pivot.angles = ( t.coop_pivot.angles[ 0 ], 270, t.coop_pivot.angles[ 2 ] );
		t.coop_raised = false;
		t setcandamage( false );
		level.coop_lane2[ i ] = t;
	}
	level.coop_lane2_hits = 0;
	setdvar( "coop_kh_lane2", level.coop_lane2.size ); // QA trace
}

lane2_raise( t )
{
	if ( t.coop_raised )
		return;
	t.coop_raised = true;
	t playsound( "killhouse_target_up_metal" );
	t.coop_pivot rotatepitch( 90, 0.25 );
	wait 0.25;
	t.health = 100000;
	t setcandamage( true );
}

lane2_lower( t )
{
	if ( !t.coop_raised )
		return;
	t setcandamage( false );
	t.coop_raised = false;
	t playsound( "killhouse_target_up" );
	t.coop_pivot rotatepitch( -90, 0.25 );
	wait 0.3;
}

lane2_lower_all()
{
	for ( i = 0; i < 3; i++ )
		level.coop_lane2[ i ] thread lane2_lower_self();
}

lane2_lower_self()
{
	lane2_lower( self );
}

// one target up until player 2 hits it; mode "ads" / "hip" / "any" as station one's rules
lane2_wait_hit( t, mode )
{
	p = level.players[ 1 ];
	lane2_raise( t );
	while ( true )
	{
		t waittill( "damage", amount, attacker );
		t.health = 100000;
		if ( !isdefined( attacker ) || attacker != p )
			continue;
		if ( mode == "ads" && !coop_ads( p ) )
		{
			thread keyHint( "ads" );
			continue;
		}
		if ( mode == "hip" && coop_ads( p ) )
		{
			thread keyHint( "stop_ads", 2, true );
			continue;
		}
		break;
	}
	t notify( "hit" );
	level.coop_lane2_hits++;
	t playsound( "killhouse_buzzer" );
	lane2_lower( t );
}

lane2_dummies( mode, flagname )
{
	level.coop_lane2_hits = 0;
	for ( i = 0; i < 3; i++ )
		level.coop_lane2[ i ] thread lane2_dummy_thread( mode );
	while ( level.coop_lane2_hits < 3 )
		wait 0.05;
	flag_set( flagname );
}

lane2_dummy_thread( mode )
{
	lane2_wait_hit( self, mode );
}

// any of the three hit (the penetration step: lane 1 has the only plywood sheet of the map, lane 2 shoots without it)
lane2_any_dummy( flagname )
{
	level.coop_lane2_hits = 0;
	for ( i = 0; i < 3; i++ )
		lane2_raise( level.coop_lane2[ i ] );
	for ( i = 0; i < 3; i++ )
		level.coop_lane2[ i ] thread lane2_count_any();
	while ( level.coop_lane2_hits == 0 )
		wait 0.05;
	level notify( "coop_lane2_any" );
	flag_set( flagname );
}

lane2_count_any()
{
	level endon( "coop_lane2_any" );
	p = level.players[ 1 ];
	while ( true )
	{
		self waittill( "damage", amount, attacker );
		self.health = 100000;
		if ( isdefined( attacker ) && attacker == p )
			break;
	}
	level.coop_lane2_hits++;
	self playsound( "killhouse_buzzer" );
}

// the timed round (killhouse timedTargets for lane 2): one random target at a time, count the hits until "times_up"
lane2_timed()
{
	level endon( "times_up" );
	last = -1;
	while ( true )
	{
		pick = randomint( 3 );
		if ( pick == last )
			continue;
		last = pick;
		lane2_wait_hit( level.coop_lane2[ pick ], "any" );
		level.coop_num_hit2++;
		wait 0.1;
	}
}

// ---- lane 1: station one's own targets for player 1 only ----

// killhouse_code new_look_wait_for_target, with only player 1's hits counted
lane1_wait_look( player, up, down )
{
	self setcandamage( true );
	self rotateto( self.angles + ( 0, 0, up ), 0.25, 0, 0 );
	self playsound( "killhouse_target_up" );
	while ( true )
	{
		self waittill( "damage", amount, attacker );
		self.health = 100000;
		if ( !isdefined( attacker ) || attacker != player )
			continue;
		if ( coop_ads( player ) )
			break;
		thread keyHint( "ads" );
	}
	self playsound( "killhouse_buzzer" );
	self playsound( "killhouse_target_up" );
	self rotateto( self.angles + ( 0, 0, down ), 0.25, 0, 0 );
}

coop_at_station( n )
{
	p = level.players[ n ];
	if ( n == 0 )
		return p istouching( getent( "rifleTraining_stall", "targetname" ) );
	// station two: station one's trigger (3552 -850) one station over
	o = p.origin;
	return abs( o[ 0 ] - 3656 ) < 50 && o[ 1 ] > -905 && o[ 1 ] < -795;
}

// ---- Gaz's range for two (killhouse rifleTraining .. sideArm_Training) ----

rifle_training()
{
	level.player = level.players[ 0 ]; // station one's code (targetDummyThink, isADS) is player 1's
	p0 = level.players[ 0 ];
	p1 = level.players[ 1 ];
	lane2_init();
	thread coop_kh_qa();
	flag_trigger_init( "player_at_rifle_stall", getEnt( "rifleTraining_stall", "targetname" ), true );
	flag_wait( "inside_firing_range" );
	thread delay_objective_after_intro();
	thread move_gaz_once_player_past();

	// a rifle each from the table
	thread coop_second_item( "pickup_rifle", level.gunPrimary );
	while ( !( p0 getweaponammostock( level.gunPrimary ) ) || !( p1 getweaponammostock( level.gunPrimary ) ) )
		wait 0.05;
	setdvar( "coop_kh_step", "rifles" );

	maps\killhouse::close_firing_range_door();
	autosave_by_name( "rifle_training" );
	setObjectiveString( "obj_rifle", &"KILLHOUSE_ENTER_STATION_NUMBER" );
	setObjectiveLocation( "obj_rifle", getEnt( "obj_rifle_stall", "targetname" ) );
	thread gaz_animation( "killhouse_gaz_idleA", 1.5 );
	level.waters execDialog( "youknowdrill" );
	coop_announce( "Player 1: station one. Player 2: station two" );

	while ( !coop_at_station( 0 ) || !coop_at_station( 1 ) )
		wait 0.05;
	setdvar( "coop_kh_step", "stations" );
	if ( !coop_both_ads() )
	{
		if ( level.Xenon )
			thread keyHint( "ads_360" );
		else
			thread keyHint( "ads" );
		thread gaz_animation( "killhouse_gaz_idleA" );
		level.waters execDialog( "rifledownrange" );
	}
	while ( !coop_both_ads() )
		wait 0.05;
	thread coop_look_training();
}

coop_look_training()
{
	setdvar( "coop_kh_step", "look" );
	if ( level.Xenon )
		thread keyHint( "attack" );
	else
		thread keyHint( "pc_attack" );
	//Now. Shoot - each - target, while aiming down the sights.
	level.waters thread execDialog( "shooteachtarget" );
	flag_init( "coop_look1" );
	flag_init( "coop_look2" );
	thread coop_look_lane1();
	thread coop_look_lane2();
	flag_wait( "coop_look1" );
	flag_wait( "coop_look2" );

	// the invert-pitch question: player 1's menu as in the game (player 2 sets his own in the options)
	p0 = level.players[ 0 ];
	setDvar( "ui_start_inverted", 0 );
	if ( isdefined( getdvar( "ui_mousepitch" ) ) && getdvar( "ui_mousepitch" ) == "1" )
		setDvar( "ui_start_inverted", 1 );
	wait 0.1;
	if ( getdvar( "coop_qa_skipmenu" ) != "1" )
	{
		setDvar( "ui_invert_string", "@KILLHOUSE_AXIS_OPTION_MENU1_ALL" );
		p0 openMenu( "invert_axis_pc" );
		p0 freezecontrols( true );
		setblur( 2, 0.1 );
		p0 waittill( "menuresponse", menu, response );
		setblur( 0, 0.2 );
		p0 freezecontrols( false );
	}
	//Lovely
	level.waters execDialog( "lovely" );
	thread coop_hip_shooting();
}

coop_look_lane1()
{
	p0 = level.players[ 0 ];
	aim_down_target = getEnt( "aim_down_target", "targetname" );
	aim_up_target = getEnt( "aim_up_target", "targetname" );
	aim_up_target lane1_wait_look( p0, -90, 90 );
	aim_down_target lane1_wait_look( p0, 90, -90 );
	flag_set( "coop_look1" );
}

coop_look_lane2()
{
	lane2_wait_hit( level.coop_lane2[ 4 ], "ads" );
	lane2_wait_hit( level.coop_lane2[ 3 ], "ads" );
	flag_set( "coop_look2" );
}

coop_hip_shooting()
{
	setdvar( "coop_kh_step", "hip" );
	wait 0.5;
	thread hint( &"KILLHOUSE_HINT_ADS_ACCURACY", 10, true );
	//Now, shoot at the targets while firing from the hip.
	level.waters execDialog( "firingfromhip" );
	setObjectiveString( "obj_rifle", &"KILLHOUSE_SHOOT_EACH_TARGET_WHILE1" );
	wait 1;
	if ( coop_any_ads() )
		thread keyHint( "stop_ads", undefined, true );
	while ( coop_any_ads() && getdvar( "coop_qa_ads" ) != "1" )
		wait 0.05;
	level.hip_fire_required = true;
	raiseTargetDummies( "rifle", undefined, undefined ); // station one (killhouse_code targetDummyThink: player 1's hits only)
	thread flag_when_lowered( "hip_targets_shot" );
	flag_init( "coop_hip2" );
	thread lane2_dummies( "hip", "coop_hip2" );
	// hints run as threads: a blocking keyHint waits for player 1's key (notifyOnCommand), which must not hold the flow
	if ( level.xenon )
		thread keyHint( "hip_attack", 8 );
	else
		thread keyHint( "pc_hip_attack", 8 );
	while ( level.targets_hit == 0 && level.coop_lane2_hits == 0 )
		wait 0.1;
	thread hint( &"KILLHOUSE_HINT_CROSSHAIR_CHANGES", 6, true );
	flag_wait( "hip_targets_shot" );
	flag_wait( "coop_hip2" );
	level.hip_fire_required = false;
	thread coop_penetration_shooting();
}

coop_penetration_shooting()
{
	setdvar( "coop_kh_step", "penetration" );
	wait 0.5;
	//Now I'm going to block the targets with a sheet of plywood.
	level.waters execDialog( "blocktargets" );
	raiseTargetDummies( "rifle", undefined, undefined );
	raisePlywoodWalls();
	flag_init( "coop_pen2" );
	thread lane2_any_dummy( "coop_pen2" );
	setObjectiveString( "obj_rifle", &"KILLHOUSE_SHOOT_A_TARGET_THROUGH" );
	level.targets_hit = 0;
	wait 0.5;
	//I want you to shoot the targets through the wood.
	level.waters execDialog( "shoottargets" );
	wait 0.2;
	level.waters thread execDialog( "bulletspenetrate" );
	while ( level.targets_hit == 0 || !flag( "coop_pen2" ) )
		wait 0.1;
	//Good.
	level.waters execDialog( "good" );
	setObjectiveState( "obj_rifle", "done" );
	lowerPlywoodWalls();
	lowerTargetDummies( "rifle", undefined, undefined );
	for ( i = 0; i < 3; i++ )
		lane2_lower( level.coop_lane2[ i ] );
	wait 0.5;
	flag_clear( "aa_rifle_training" );
	thread coop_timed_shooting();
}

coop_timed_shooting()
{
	setdvar( "coop_kh_step", "timed" );
	flag_set( "aa_timed_shooting_training" );
	//Now I'm going make the targets pop up one at a time.
	level.waters execDialog( "targetspop" );
	//Hit all of them as fast as you can.
	level.waters execDialog( "hitall" );
	registerObjective( "obj_timed_rifle", &"KILLHOUSE_SHOOT_EACH_TARGET_AS", getEnt( "obj_rifle_stall", "targetname" ) );
	setObjectiveState( "obj_timed_rifle", "current" );
	if ( coop_needs_reload( level.players[ 0 ] ) || coop_needs_reload( level.players[ 1 ] ) )
	{
		thread keyHint( "reload", 4 );
		while ( coop_needs_reload( level.players[ 0 ] ) || coop_needs_reload( level.players[ 1 ] ) )
			wait 0.1;
		wait 1.0;
	}

	tooslow_dialog = [];
	tooslow_dialog[ 0 ] = "stilltooslow";
	tooslow_dialog[ 1 ] = "again";
	tooslow_dialog[ 2 ] = "again2";
	tooslow_dialog[ 3 ] = "walkinpark";
	passed = [];
	passed[ 0 ] = false;
	passed[ 1 ] = false;
	numRepeats = 0;
	while ( true )
	{
		lowerTargetDummies( "rifle" );
		lane2_lower_all();
		level.num_hit = 0;
		level.coop_num_hit2 = 0;
		if ( !passed[ 0 ] )
			thread maps\killhouse::timedTargets();
		if ( !passed[ 1 ] )
			thread lane2_timed();
		wait 10;
		level notify( "times_up" );
		setdvar( "coop_kh_timed", level.num_hit + " " + level.coop_num_hit2 ); // QA trace
		if ( level.num_hit > 6 )
			passed[ 0 ] = true;
		if ( level.coop_num_hit2 > 6 )
			passed[ 1 ] = true;
		if ( passed[ 0 ] && passed[ 1 ] )
			break;
		wait 1;
		numRepeats++;
		lowerTargetDummies( "rifle" );
		lane2_lower_all();
		if ( !passed[ 0 ] && !passed[ 1 ] )
			coop_announce( "Both again" );
		else if ( !passed[ 0 ] )
			coop_announce( "Player 1 again" );
		else
			coop_announce( "Player 2 again" );
		if ( numRepeats == 1 )
			level.waters execDialog( "tryagain" );
		else
			level.waters execDialog( tooslow_dialog[ randomint( tooslow_dialog.size ) ] );
		wait 2;
		if ( ( !passed[ 0 ] && coop_needs_reload( level.players[ 0 ] ) ) || ( !passed[ 1 ] && coop_needs_reload( level.players[ 1 ] ) ) )
			thread keyHint( "reload", 4 );
		while ( ( !passed[ 0 ] && coop_needs_reload( level.players[ 0 ] ) ) || ( !passed[ 1 ] && coop_needs_reload( level.players[ 1 ] ) ) )
			wait 0.1;
		clear_hints();
	}
	flag_set( "spawn_sidearms" );
	wait 1;
	setObjectiveState( "obj_timed_rifle", "done" );
	//Proper good job mate!
	level.waters execDialog( "propergood" );
	level notify( "rifleTraining_end" );
	wait 0.5;
	flag_clear( "aa_timed_shooting_training" );
	thread coop_sidearm_training();
}

coop_needs_reload( player )
{
	if ( getdvar( "coop_qa_skipreload" ) == "1" )
		return false;
	return player getweaponammoclip( level.gunPrimary ) < level.gunPrimaryClipAmmo;
}

coop_sidearm_training()
{
	setdvar( "coop_kh_step", "sidearm" );
	p0 = level.players[ 0 ];
	p1 = level.players[ 1 ];
	flag_set( "aa_sidearm_melee" );
	level notify( "sideArmTraining_begin" );
	autosave_by_name( "sidearm_training" );
	flag_set( "spawn_sidearms" );
	thread gaz_animation( "killhouse_gaz_point_side" );
	level.waters execDialog( "getasidearm" );
	registerObjective( "obj_sidearm", &"KILLHOUSE_GET_A_PISTOL_FROM_THE", getEnt( "obj_rifle_ammo", "targetname" ) );
	setObjectiveState( "obj_sidearm", "current" );
	thread coop_second_item( "pickup_sidearm", "usp" );
	while ( !p0 hasweapon( "usp" ) || !p1 hasweapon( "usp" ) )
		wait 0.05;
	setObjectiveString( "obj_sidearm", &"KILLHOUSE_SWITCH_TO_YOUR_RIFLE" );
	level.waters execDialog( "switchtorifle" );
	if ( p0 getcurrentweapon() != level.gunPrimary || p1 getcurrentweapon() != level.gunPrimary )
		thread keyHint( "primary" );
	while ( p0 getcurrentweapon() != level.gunPrimary || p1 getcurrentweapon() != level.gunPrimary )
		wait 0.05;
	clear_hints();
	thread gaz_animation( "killhouse_gaz_point_side" );
	level.waters execDialog( "pulloutsidearm" );
	if ( p0 getcurrentweapon() != level.gunSidearm || p1 getcurrentweapon() != level.gunSidearm )
		thread keyHint( "sidearm" );
	while ( p0 getcurrentweapon() != level.gunSidearm || p1 getcurrentweapon() != level.gunSidearm )
		wait 0.05;
	clear_hints();
	level.waters execDialog( "switchingfaster" );
	setObjectiveState( "obj_sidearm", "done" );
	flag_set( "sidearm_complete" );
	level notify( "sideArmTraining_end" );
	wait 0.5;
	thread coop_melee_training();
}

// ---- the watermelon, one after the other ----

coop_melee_training()
{
	setdvar( "coop_kh_step", "melee" );
	level notify( "melee_training" );
	registerObjective( "obj_melee", &"KILLHOUSE_MELEE_THE_WATERMELON", getEnt( "scr_watermelon", "targetname" ) );
	setObjectiveState( "obj_melee", "current" );
	order = coop_lot( "melon" );
	melon = getEnt( "scr_watermelon", "targetname" );
	melon_model = getEnt( melon.target, "targetname" );
	//All right Soap, come this way.
	level.waters execDialog( "comethisway" );
	thread maps\killhouse::melee_run_dialog();
	for ( k = 0; k < 2; k++ )
	{
		p = order[ k ];
		coop_announce( coop_name( p ) + ": knife the watermelon" );
		melon_model show();
		melon_model solid();
		while ( true )
		{
			melon waittill( "trigger", who );
			if ( isdefined( who ) && who == p )
				break;
		}
		setdvar( "coop_kh_melon", k + 1 ); // QA trace
		melon playsound( "melee_knife_hit_watermelon" );
		playfx( level._effect[ "watermelon" ], melon_model.origin );
		melon_model hide();
		melon_model notsolid();
		if ( k == 0 )
			wait 1.5;
	}
	flag_set( "melee_complete" );
	clear_hints();
	thread maps\killhouse::open_firing_range_door();
	flag_wait( "melee_run_dialog" );
	wait 0.5;
	//Lovely. Your fruit killing skills are remarkable.
	thread gaz_animation( "killhouse_gaz_talk_side" );
	level.waters execDialog( "fruitkilling" );
	setObjectiveState( "obj_melee", "done" );
	level notify( "meleeTraining_end" );
	flag_clear( "aa_sidearm_melee" );
	autosave_by_name( "melee_complete" );
	level.waters execDialog( "wantstosee" );
	setdvar( "coop_kh_step", "price" );
	thread maps\killhouse::report_to_price();
}

// ---- Captain Price's ship, one after the other ----

// started with killhouse cargoship_training (its loop runs each run for level.player)
cargoship_turns()
{
	setdvar( "coop_kh_turns", "start" ); // QA trace
	if ( isdefined( level.coop_ship_turns ) )
		return;
	level.coop_ship_turns = true;
	thread coop_kh_qa();
	flag_init( "coop_ship_done" );
	// the fields the level's own loops read from level.player every frame, for whoever runs (_friendlyfire: participation)
	for ( i = 0; i < level.players.size; i++ )
	{
		if ( !isdefined( level.players[ i ].participation ) )
			level.players[ i ].participation = 0;
	}
	order = coop_lot( "ship" );
	first = order[ 0 ];
	second = order[ 1 ];
	level.player = first;
	coop_spectator( second, true );
	setdvar( "coop_kh_step", "ship1" );
	level waittill( "test_cleared" );
	wait 8; // Price's verdict
	coop_spectator( first, true );
	coop_spectator( second, false );
	level.player = second;
	// his MP5 and four flash bangs (the first run took the ones on the table)
	second giveweapon( "mp5" );
	second switchtoweapon( "mp5" );
	second giveweapon( "flash_grenade" );
	second setweaponammostock( "flash_grenade", 4 );
	top = getent( "start_pre_rope", "targetname" );
	second setorigin( top.origin );
	coop_announce( coop_name( second ) + ": your turn" );
	setdvar( "coop_kh_step", "ship2" );
	level waittill( "test_cleared" );
	wait 8;
	level.player = level.players[ 0 ];
	coop_spectator( first, false );
	setdvar( "coop_kh_step", "debrief" );
	flag_set( "coop_ship_done" );
}

// the player waiting for his turn watches from the gallery, frozen, out of the course's triggers
coop_spectator( player, on )
{
	if ( on )
	{
		spot = coop_spectator_spot( player );
		player setorigin( spot[ "origin" ] );
		player setplayerangles( spot[ "angles" ] );
		player freezecontrols( true );
		player enableinvulnerability();
	}
	else
	{
		player freezecontrols( false );
		player disableinvulnerability();
	}
}

coop_spectator_spot( player )
{
	spot = [];
	// next to the debrief monitors, looking at the ship
	spot[ "origin" ] = ( -300, -900, 30 );
	spot[ "angles" ] = ( -10, 45, 0 );
	if ( getdvar( "coop_qa_specspot" ) != "" )
	{
		w = strtok( getdvar( "coop_qa_specspot" ), " " );
		spot[ "origin" ] = ( int( w[ 0 ] ), int( w[ 1 ] ), int( w[ 2 ] ) );
		spot[ "angles" ] = ( 0, int( w[ 3 ] ), 0 );
	}
	return spot;
}

// killhouse cargoship_training's debrief after the first run: in co-op after both runs
debrief_gate()
{
	if ( isdefined( level.players ) && level.players.size > 1 )
		flag_wait( "coop_ship_done" );
	thread maps\killhouse::debrief();
}

// ---- QA (coop_qa_kh): "autohit 1" = every 0.3 s a hit on each raised target of both stations by its own player (the flow
// without aiming bots), "autohit 0" stops; "melon <n>" = player n's knife on the watermelon ----
coop_kh_qa()
{
	if ( isdefined( level.coop_kh_qa_started ) )
		return;
	level.coop_kh_qa_started = true;
	setdvar( "coop_qa_kh", "" );
	level.coop_qa_autohit = false;
	thread coop_kh_autohit();
	thread coop_kh_qa_flags();
	while ( true )
	{
		wait 0.05;
		cmd = getdvar( "coop_qa_kh" );
		if ( cmd == "" )
			continue;
		setdvar( "coop_qa_kh", "" );
		w = strtok( cmd, " " );
		if ( w[ 0 ] == "autohit" && w.size > 1 )
			level.coop_qa_autohit = w[ 1 ] == "1";
		else if ( w[ 0 ] == "melon" && w.size > 1 )
			getEnt( "scr_watermelon", "targetname" ) notify( "trigger", level.players[ int( w[ 1 ] ) ] );
		// Price's ship, for the player whose run it is (level.player)
		else if ( w[ 0 ] == "tpactive" && w.size >= 5 )
		{
			level.player setorigin( ( int( w[ 1 ] ), int( w[ 2 ] ), int( w[ 3 ] ) ) );
			level.player setplayerangles( ( 0, int( w[ 4 ] ), 0 ) );
		}
		else if ( w[ 0 ] == "kitactive" )
		{
			level.player giveweapon( "mp5" );
			level.player switchtoweapon( "mp5" );
			level.player giveweapon( "flash_grenade" );
			level.player setweaponammostock( "flash_grenade", 4 );
		}
		else if ( w[ 0 ] == "rope" )
			getent( "top_of_rope_trigger", "targetname" ) notify( "trigger", level.player );
		else if ( w[ 0 ] == "flash" && w.size > 1 )
		{
			vol = getent( w[ 1 ], "targetname" ); // position_three_vol / position_6_vol
			vol notify( "flashed" );
			level.volumes_flashed++;
		}
		else if ( w[ 0 ] == "sprinted" )
			level.player notify( "sprinted" );
		else if ( w[ 0 ] == "autohitship" && w.size > 1 )
			level.coop_qa_autohitship = w[ 1 ] == "1";
	}
}

// QA: the ship course's flags in dvars coop_khf_<flag> (0 / 1) for the test runner
coop_kh_qa_flags()
{
	names = [];
	names[ 0 ] = "open_ship_hanger";
	names[ 1 ] = "on_ladder";
	names[ 2 ] = "at_top_of_ladder";
	names[ 3 ] = "reveal_dialog_done";
	names[ 4 ] = "activate_rope";
	names[ 5 ] = "sprint";
	names[ 6 ] = "at_finish";
	names[ 7 ] = "sprinted";
	names[ 8 ] = "ready_to_finish";
	was = [];
	hits = -1;
	kit = "";
	while ( true )
	{
		wait 0.05;
		for ( i = 0; i < names.size; i++ )
		{
			v = 0;
			if ( isdefined( level.flag[ names[ i ] ] ) && level.flag[ names[ i ] ] )
				v = 1;
			if ( !isdefined( was[ i ] ) || was[ i ] != v )
			{
				was[ i ] = v;
				setdvar( "coop_khf_" + names[ i ], v );
			}
		}
		k = coop_name( level.player ) + " " + level.player getcurrentweapon() + " " + level.player getweaponammostock( "flash_grenade" );
		if ( k != kit )
		{
			kit = k;
			setdvar( "coop_kh_kit", k );
		}
		if ( isdefined( level.targets_hit ) && level.targets_hit != hits )
		{
			hits = level.targets_hit;
			setdvar( "coop_kh_hits", hits );
		}
		// the enemy targets only: a hit on a "friendly" one fails the mission (killhouse_code fail_if_damage_waiter)
		if ( isdefined( level.coop_qa_autohitship ) && level.coop_qa_autohitship )
		{
			targets = getentarray( "cargoship_target", "script_noteworthy" );
			for ( i = 0; i < targets.size; i++ )
			{
				if ( isdefined( targets[ i ].targetname ) && targets[ i ].targetname == "friendly" )
					continue;
				targets[ i ] notify( "damage", 50, level.player, ( 0, 0, 1 ), targets[ i ].origin, "MOD_RIFLE_BULLET" );
			}
		}
	}
}

coop_kh_autohit()
{
	while ( true )
	{
		wait 0.3;
		if ( !level.coop_qa_autohit )
			continue;
		p0 = level.players[ 0 ];
		p1 = level.players[ 1 ];
		dummies = getTargetDummies( "rifle" );
		for ( i = 0; i < dummies.size; i++ )
		{
			if ( dummies[ i ].raised && !dummies[ i ].moving )
				dummies[ i ] notify( "damage", 50, p0 );
		}
		look = [];
		look[ 0 ] = getEnt( "aim_up_target", "targetname" );
		look[ 1 ] = getEnt( "aim_down_target", "targetname" );
		for ( i = 0; i < look.size; i++ )
			look[ i ] notify( "damage", 50, p0 );
		for ( i = 0; isdefined( level.coop_lane2 ) && i < level.coop_lane2.size; i++ )
		{
			if ( level.coop_lane2[ i ].coop_raised )
				level.coop_lane2[ i ] notify( "damage", 50, p1 );
		}
	}
}
