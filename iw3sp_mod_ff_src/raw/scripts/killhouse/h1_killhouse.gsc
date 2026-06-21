/****************************************************************************
Changes:
Added training logic from Call of Duty: Modern Warfare Remastered. It means that you can run other courses or just running to the Captain Price.
Changed objective logic for Sgt. Newcastle and Mac: one visible objective.
If player skip the Mac course then add line if player have bad time 'letyouskip'
Change the M203 and C4 hint for PC.
****************************************************************************/


#include common_scripts\utility;
#include maps\_utility;

main()
{
	precachestring( &"IW3SP_MOD_LOC_KILLHOUSE_DEMOLITIONS_TRAINING" );
	precachestring( &"IW3SP_MOD_LOC_KILLHOUSE_RUN_THE_OBSTACLE_COURSE" );

	flag_init("cargoshipTutoDone");

	level.player_completed_demolitions_training = false;
	level.player_completed_obstacle_course_training = false;
	level.player_skipped_obstacle_course = false;
	car = getent ( "destructible", "targetname" );
	car thread do_in_order( ::waittill_msg, "destroyed", ::flag_set, "car_destroyed" );
	car destructible_disable_explosion();

	ReplaceFunc(maps\killhouse::melee_training, ::melee_training);
}

melee_training()
{
	//while( !flag( "melee_entered" ) )
	//	wait ( 0.05 );
	level notify ( "melee_training" );
	
	maps\killhouse_code::registerObjective( "obj_melee", &"KILLHOUSE_MELEE_THE_WATERMELON", getEnt( "scr_watermelon", "targetname" ) );
	maps\killhouse_code::setObjectiveState( "obj_melee", "current" );
	
	if ( !flag ( "melee_complete" ) )
		thread maps\killhouse_code::generic_compass_hint_reminder( "melee_complete", 12 );
	
	//level.waters thread walk_to ( getnode ( "melon_node", "script_noteworthy" ) );
	
	if ( !flag ( "near_melee" ) && !flag ( "melee_complete" ) )
	{
		thread maps\killhouse_code::hint( &"KILLHOUSE_HINT_APPROACH_MELEE", 9999 );
		
		//All right Soap, come this way.
		level.waters maps\killhouse_code::execDialog( "comethisway" );
	}
	
	thread maps\killhouse::melee_run_dialog();
	
	if ( !flag ( "melee_complete" ) )
		flag_wait ( "near_melee" );
	
	while ( !flag ( "melee_complete" ) )
		maps\killhouse_code::keyHint( "melee" );
	
	flag_wait ( "melee_complete" );
	
	maps\killhouse_code::clear_hints();
	thread maps\killhouse::open_firing_range_door();

	
	flag_wait ( "melee_run_dialog" );

	wait .5;
	
	
	flag_set ( "spawn_frags" );
	
	
	//Lovely. Your fruit killing skills are remarkable.
	thread maps\killhouse_code::gaz_animation( "killhouse_gaz_talk_side" );
	level.waters maps\killhouse_code::execDialog( "fruitkilling" );
	
	maps\killhouse_code::setObjectiveState( "obj_melee", "done" );
	
	level notify ( "meleeTraining_end" );
	flag_clear ( "aa_sidearm_melee" );
	autosave_by_name( "melee_complete" );
	
	if ( level.short_training )
	{
		level.waters maps\killhouse_code::execDialog( "wantstosee" );
		//thread add_dialogue_line( "GAZ", "Captain Price wants to see you." );
		thread player_skipped_the_optional_objectives();
		thread maps\killhouse::report_to_price();
		wait 1;
		thread frag_training();
        thread obstacle_training();

		//KILLHOUSE_DEMOLITIONS_TRAINING
		//KILLHOUSE_RUN_THE_OBSTACLE_COURSE

		if(AssetIsExisting(22, "IW3SP_MOD_LOC_KILLHOUSE_DEMOLITIONS_TRAINING"))
			objectiveText = &"IW3SP_MOD_LOC_KILLHOUSE_DEMOLITIONS_TRAINING";
		else
			objectiveText = &"KILLHOUSE_GO_OUTSIDE_AND_REPORT";

		if(AssetIsExisting(22, "IW3SP_MOD_LOC_KILLHOUSE_RUN_THE_OBSTACLE_COURSE"))
			objectiveCourse = &"IW3SP_MOD_LOC_KILLHOUSE_RUN_THE_OBSTACLE_COURSE";
		else
			objectiveCourse = &"KILLHOUSE_RUN_THE_OBSTACLE_COURSE";

        maps\killhouse_code::registerobjective( "obj_demolitions", objectiveText, getent( "obj_frag_ammo", "targetname" ) );
        maps\killhouse_code::setobjectivestate( "obj_demolitions", "active" );
        maps\killhouse_code::registerobjective( "obj_obstacle", objectiveCourse, getent( "obstacleTraining_objective", "targetname" ) );
        maps\killhouse_code::setobjectivestate( "obj_obstacle", "active" );
	}
	else
	{
		level.waters maps\killhouse_code::execDialog( "allgoodhere" );
		//level.waters thread walk_to ( getnode ( "door_node", "script_noteworthy" ) );
	
		thread maps\killhouse::frag_Training();
	}
}

frag_Training()
{
	level endon ( "PlayerSkipTheOptionalCourses" );

	if( flag( "cargoshipTutoDone" ) )
		return;

	flag_set ( "aa_frag" );
	level notify ( "fragTraining_begin" );

	autosave_by_name( "frag_training" );
	
	flag_set ( "start_frag_training" );	
	
	//thread generic_compass_hint_reminder( "got_frags", 7 );
	
	// maps\killhouse_code::registerObjective( "obj_demolitions", &"KILLHOUSE_GO_OUTSIDE_AND_REPORT", getEnt( "obj_frag_ammo", "targetname" ) );
	// maps\killhouse_code::setObjectiveState( "obj_demolitions", "current" );

	flag_wait ( "near_grenade_area" );

	maps\killhouse_code::setObjectiveString( "obj_demolitions", &"KILLHOUSE_PICK_UP_THE_FRAG_GRENADES" );
	maps\killhouse_code::setObjectiveState( "obj_demolitions", "current" );

	//It's time for some fun mate. Let's blow some shit up...
	level.newcastle maps\killhouse_code::execDialog( "timeforfun" );
	
	if ( !( level.player GetWeaponAmmoStock( "fraggrenade" ) ) &&  (!( maps\killhouse_code::in_pit() ) ) )
	{
		//Pick up those frag grenades and get in the safety pit.
		level.newcastle maps\killhouse_code::execDialog( "pickupfrag" );
	}
	
	while ( level.player GetWeaponAmmoStock( "fraggrenade" ) < 3 )
		wait ( 0.05 );
	
	flag_set ( "got_frags" );
	
	getEnt( "grenade_too_low", "targetname" ) thread maps\killhouse_code::frag_too_low_hint();
	
	thread maps\killhouse_code::frag_trigger_think( "frag_target_1", getEnt( "grenade_damage_trigger1", "targetname" ) );
	thread maps\killhouse_code::frag_trigger_think( "frag_target_2", getEnt( "grenade_damage_trigger2", "targetname" ) );
	thread maps\killhouse_code::frag_trigger_think( "frag_target_3", getEnt( "grenade_damage_trigger3", "targetname" ) );
	
	
	maps\killhouse_code::setObjectiveLocation( "obj_demolitions", getEnt( "safety_pit", "targetname" ) );
	maps\killhouse_code::setObjectiveString( "obj_demolitions", &"KILLHOUSE_ENTER_THE_SAFETY_PIT" );
	
	thread maps\killhouse_code::dialog_nag_till_in_pit();
	
	getEnt( "safety_pit", "targetname" ) waittill ( "trigger" );
	flag_set ( "in_pit_with_frags" );
	
	//level.newcastle thread walk_to( getnode ( "watch_pit_node", "script_noteworthy" ) );
	level.newcastle setgoalnode ( getnode ( "watch_pit_node", "script_noteworthy" ) );
	
	//Now throw a grenade into windows two, three and four.
	level.newcastle thread maps\killhouse_code::execDialog( "throwgrenade" );
	thread maps\killhouse_code::keyHint( "frag" );
	
	maps\killhouse_code::setObjectiveString( "obj_demolitions", &"KILLHOUSE_THROW_A_GRENADE_INTO" );
	maps\killhouse_code::setObjectiveLocation( "obj_demolitions", getEnt( "safety_pit", "targetname" )  );

	wait ( 0.1 );

	numRemaining = 0;
	for ( index = 1; index < 4; index++ )
	{
		if ( flag( "frag_target_" + index ) )
			continue;

		numRemaining++;
	}


	//level.marine4 execDialog( "firstwindow" );

	//level.marine4.lastNagTime = getTime();
	while( numRemaining )
	{
		curRemaining = 0;

		nextTarget = "";
		if ( !flag( "frag_target_1" ) )
		{
			curRemaining++;
			nextTarget = "firstwindow";
		}
		if ( !flag( "frag_target_2" ) )
		{
			curRemaining++;
			if ( nextTarget == "" )
				nextTarget = "secondwindow";
		}
		if ( !flag( "frag_target_3" ) )
		{
			curRemaining++;
			if ( nextTarget == "" )
				nextTarget = "dumpster";
		}

		if ( !curRemaining )
			break;

		if ( curRemaining != numRemaining )
		{
			//level.marine4 execDialog( nextTarget );
			//level.marine4.lastNagTime = getTime();
		}
		else
		{
			//level.marine4 nagPlayer( nextTarget + "again", 10.0 );
		}

		numRemaining = curRemaining;

		wait ( 0.05 );
	}

	//maps\killhouse_code::setObjectiveState( "obj_demolitions", "done" );

	wait ( 1.0 );
	//level.marine4 thread execDialog( "gotorange" );

	flag_set ( "fragTraining_end" );
	thread launcherTraining();
	flag_clear ( "aa_frag" );
}

launcherTraining()
{
	level endon ( "PlayerSkipTheOptionalCourses" );

	if( flag( "cargoshipTutoDone" ) )
		return;

	flag_set ( "aa_launcher" );
	level notify ( "launcherTraining_begin" );

	autosave_by_name( "launcher_training" );
	
	flag_set ( "spawn_launcher" );
	
	level.newcastle setgoalnode ( getnode ( "watch_table_node", "script_noteworthy" ) );
	//level.newcastle thread walk_to ( getnode ( "watch_table_node", "script_noteworthy" ) );
	
	//Now let's try something with a little more 'mojo'. I don't know how much experience you've got with demolitions, so just do as I say, all right?
	//level.newcastle execDialog( "moremojo" );	
	
	if ( !level.player hasWeapon( "m203_m4" ) )
	{
		//Come back here and pick up this grenade launcher.
		level.newcastle maps\killhouse_code::execDialog( "pickuplauncher" );	
	}
		
	flag_trigger_init( "launcher_wall_target", getEnt( "launcher_wall_trigger", "script_noteworthy" ) );

	maps\killhouse_code::setObjectiveString( "obj_demolitions", &"KILLHOUSE_PICK_UP_THE_RIFLE_WITH" );
	maps\killhouse_code::setObjectiveLocation( "obj_demolitions", getEnt( "obj_frag_ammo", "targetname" )  );

	thread maps\killhouse_code::keyHint( "swap_launcher" );

	while ( !level.player hasWeapon( "m203_m4" ) )
		wait ( 0.05 );
	maps\killhouse_code::clear_hints();
	thread scripts\killhouse\hint::M203_icon_hint();
	RefreshHudAmmoCounter();
	
	
	level.player giveMaxAmmo( "m203_m4" );

	maps\killhouse_code::setObjectiveString( "obj_demolitions", &"KILLHOUSE_RETURN_TO_THE_SAFETY" );
	maps\killhouse_code::setObjectiveLocation( "obj_demolitions", getEnt( "safety_pit", "targetname" )  );
	
	//Notice you now have an icon of a grenade launcher on your HUD.
	//level.newcastle execDialog( "icononhud" );	
	
	if ( !( level.player istouching( getEnt( "safety_pit", "targetname" ) ) ) )
	{
		//Now get back into the safety pit.
		level.newcastle maps\killhouse_code::execDialog( "nowbacktopit" );	
	}
	getEnt( "safety_pit", "targetname" ) waittill ( "trigger" );
	
	//level.newcastle thread walk_to( getnode ( "watch_pit_node", "script_noteworthy" ) );
	level.newcastle setgoalnode ( getnode ( "watch_pit_node", "script_noteworthy" ) );
	
	if ( !(level.player getCurrentWeapon() == "m203_m4") )
	{
		//Equip the grenade launcher.
		level.newcastle maps\killhouse_code::execDialog( "equiplauncher" );
		thread maps\killhouse_code::keyHint( "firemode" );
		RefreshHudAmmoCounter();
	}
	
	while ( !(level.player getCurrentWeapon() == "m203_m4") )
	{
		thread maps\killhouse_code::keyHint( "firemode" );
		wait ( 1.0 );
	}

	maps\killhouse_code::clear_hints();

	
	maps\killhouse_code::setObjectiveString( "obj_demolitions", &"KILLHOUSE_FIRE_AT_THE_WALL_WITH" );
	maps\killhouse_code::setObjectiveLocation( "obj_demolitions", getEnt( "safety_pit", "targetname" )  );
	wait ( 0.1 );

	//Fire at the wall with the number one on it.
	level.newcastle maps\killhouse_code::execDialog( "firewall1" );	
	
	if ( level.xenon )
		thread maps\killhouse_code::keyHint( "attack" );
	else
		thread maps\killhouse_code::keyHint( "pc_attack" );//PC and PS3 are both press
	
	while ( !flag( "launcher_wall_target" ) )
	{
		//level.player giveMaxAmmo( "m203_m4" );
		wait ( 0.05 );
	}
	maps\killhouse_code::clear_hints();
	wait ( 0.1 );

	//Notice it didn't explode.
	level.newcastle maps\killhouse_code::execDialog( "didntexplode" );
	
	//As you know, all grenade launchers have a minimum safe arming distance.
	level.newcastle maps\killhouse_code::execDialog( "safearming" );	
	
	//The grenade wont explode unless it travels that distance.
	//level.newcastle execDialog( "wontexplode" );	
	
	//Right. Now pop a grenade in each window, five, six and seven.
	level.newcastle thread maps\killhouse_code::execDialog( "56and7" );	
	
	
	array_thread ( getEntArray( "gl_too_low", "targetname" ), maps\killhouse::gl_too_low_hint );
	
	thread maps\killhouse::launcher_trigger_think( "launcher_target_1", getEnt( "launcher_damage_trigger1", "targetname" ) );
	thread maps\killhouse::launcher_trigger_think( "launcher_target_2", getEnt( "launcher_damage_trigger2", "targetname" ) );
	thread maps\killhouse::launcher_trigger_think( "launcher_target_3", getEnt( "launcher_damage_trigger3", "targetname" ) );


	maps\killhouse_code::setObjectiveString( "obj_demolitions", &"KILLHOUSE_FIRE_YOUR_GRENADE_LAUNCHER" );

	numRemaining = 0;
	for ( index = 1; index < 4; index++ )
	{
		if ( flag( "launcher_target_" + index ) )
			continue;

		numRemaining++;
	}


	//level.marine5.lastNagTime = getTime();
	while( numRemaining )
	{
		curRemaining = 0;

		nextTarget = "";
		if ( !flag( "launcher_target_1" ) )
		{
			curRemaining++;
			nextTarget = "hittwo";
		}
		if ( !flag( "launcher_target_2" ) )
		{
			curRemaining++;
			if ( nextTarget == "" )
				nextTarget = "hitthree";
		}
		if ( !flag( "launcher_target_3" ) )
		{
			curRemaining++;
			if ( nextTarget == "" )
				nextTarget = "hitfour";
		}

		if ( !curRemaining )
			break;

		//level.player giveMaxAmmo( "m203_m4" );

		if ( curRemaining != numRemaining )
		{
			//level.marine5.lastNagTime = getTime();
		}
		else
		{
			//level.marine5 nagPlayer( nextTarget, 8.0 );
		}

		numRemaining = curRemaining;

		wait ( 0.05 );
	}

	level notify ( "launcherTraining_end" );
	//maps\killhouse_code::setObjectiveState( "obj_demolitions", "done" );

	wait ( 1.0 );
	//level.marine5 execDialog( "oorah" );
	wait ( 1.0 );

	flag_clear ( "aa_launcher" );
	thread c4_Training();
}

c4_Training()
{
	level endon ( "PlayerSkipTheOptionalCourses" );

	if( flag( "cargoshipTutoDone" ) )
		return;

	flag_set ( "aa_c4" );
	level notify ( "explosivesTraining_begin" );
	
	autosave_by_name( "c4_training" );
	
	pickupTrigger = flag_trigger_init( "explosives_pickup", getEnt( "c4_pickup", "targetname" ) );
	C4_models = getEntArray( pickupTrigger.target, "targetname" );
	
	pickupTrigger setHintString (&"KILLHOUSE_C4_PICKUP");
	pickupTrigger trigger_on();
	for ( i = 0; i < C4_models.size; i++ )
		C4_models[ i ] show();

	maps\killhouse_code::setObjectiveLocation( "obj_demolitions", pickupTrigger );
	maps\killhouse_code::setObjectiveString( "obj_demolitions", &"KILLHOUSE_PICK_UP_THE_C4_EXPLOSIVE" );

	//Come back around and pick up some C4 off the table.
	level.newcastle thread maps\killhouse_code::execDialog( "c4offtable" );

	//level.newcastle thread walk_to ( getnode ( "watch_c4_node", "script_noteworthy" ) );
	level.newcastle setgoalnode ( getnode ( "watch_c4_node", "script_noteworthy" ) );
	
	thread maps\killhouse_code::keyHint( "swap_explosives" );

	while ( !flag( "explosives_pickup" ) )
		wait ( 0.05 );
	
	level.player playsound ( "detpack_pickup" );
	for ( i = 0; i < C4_models.size; i++ )
		C4_models[ i ] hide();
	pickupTrigger trigger_off();
	old_weapon = level.player GetCurrentWeapon ();
	level.player giveWeapon("c4");
	level.player SetWeaponAmmoClip( "c4", 1 );
	level.player SetActionSlot( 2, "weapon" , "c4" );
	
	thread scripts\killhouse\hint::C4_icon_hint();
	RefreshHudAmmoCounter();
	thread maps\killhouse::flag_when_c4_thrown();
	
	wait .5;
	
	if ( !(level.player getCurrentWeapon() == "c4") )
	{
		level.newcastle maps\killhouse_code::execDialog( "equipc4" );	//Equip the C4, Soap.
		//thread keyHint( "equip_C4" );
		level notify ( "c4_equiped" );
		level.hintElem setText( &"KILLHOUSE_HINT_EQUIP_C4" );
		RefreshHudAmmoCounter();
	}
	
	while ( !(level.player getCurrentWeapon() == "c4") )
	{
		wait ( 1.0 );
	}
	
	
	flag_set ( "c4_equiped" );
	level.hintbackground destroy();
	double_line = true;
	maps\killhouse_code::add_hint_background( double_line );
	level.hintElem setText( &"KILLHOUSE_HINT_HUD_CHANGES" );
	RefreshHudAmmoCounter();
	//thread hint( &"KILLHOUSE_HINT_HUD_CHANGES", 9999 );
	
	//It seems my ex-wife was kind enough to donate her car to furthering your education Soap.
	level.newcastle maps\killhouse_code::execDialog( "exwifecar" );
	
	maps\killhouse_code::setObjectiveLocation( "obj_demolitions", getEnt( "c4_target", "targetname" )  );
	
	level notify ( "C4_the_car" );
	
	if ( !flag ( "c4_thrown" ) )
		level.newcastle maps\killhouse_code::execDialog( "throwc4car" );	//Throw some C4 on the car.
	
	if ( ( !flag ( "near_car" ) ) && ( !flag ( "c4_thrown" ) ) )
	{
		//thread hint( &"KILLHOUSE_HINT_APPROACH_C4_THROW", 9999 );
		level.hintbackground destroy();
		maps\killhouse_code::add_hint_background();
		level.hintElem setText( &"KILLHOUSE_HINT_APPROACH_C4_THROW" );
		flag_wait ( "near_car" );
	}
	
	
	if ( !flag ( "c4_thrown" ) )
		maps\killhouse_code::keyHint( "throw_C4" );
	
	flag_wait ( "c4_thrown" );
	
	wait .5;
	
	//Sgt Newcastle - When planting C4 is your objective, you will see a glowing marker in the world that indicates where to plant it.
	//thread add_dialogue_line( "newcastle", "When planting C4 is your objective, you will see a glowing marker in the world that indicates where to plant it." );
	double_line = true;
	thread maps\killhouse_code::hint( &"KILLHOUSE_C4_OBJECTIVE", 9999, double_line );
	wait 4;
	
	//Place the C4 on the indicated spot.
	level.newcastle thread maps\killhouse_code::execDialog( "placec4" );
	
	c4_target = getent( "c4_target", "targetname" );
	c4_target maps\_c4::c4_location( undefined, undefined, undefined, c4_target.origin );
	level thread do_in_order( ::waittill_msg, "c4_in_place", ::flag_set, "C4_planted" );

	wait ( 1.0 );
	
	maps\killhouse_code::setObjectiveString( "obj_demolitions", &"KILLHOUSE_PLANT_THE_C4_EXPLOSIVE" );
	maps\killhouse_code::setObjectiveLocation( "obj_demolitions", c4_target  );
	
    thread maps\killhouse_code::keyhint( "plant_explosives", 9999, 1 );

    flag_wait ( "C4_planted" );
	
	c4_target thread maps\killhouse::force_detonation();
	
	maps\killhouse_code::clear_hints();

	// maps\killhouse_code::setObjectiveState( "obj_demolitions", "done" );
	
	//level.newcastle execDialog( "morec4" );	//Go get some more C4 from the table.
	//level.newcastle execDialog( "behindwall" );	//Now come over here behind the safety wall.
	
	if( !flag( "car_destroyed" ) )
		level.newcastle maps\killhouse_code::execDialog( "safedistance" );	//Now get to a safe distance from the explosives.
	
	
	while ( ( distance( c4_target.origin, level.player.origin ) <= 256 ) && !flag( "car_destroyed" ) )
		wait 0.05;
	
	if( !flag( "car_destroyed" ) )
	{
		level.newcastle maps\killhouse_code::execDialog( "fireinhole" );	//Fire in the hole!
		thread maps\killhouse_code::keyHint( "detonate_C4", 9999 );
	}
	
	flag_wait ( "car_destroyed" );
	thread maps\killhouse::switch_in_two( old_weapon );
	
	maps\killhouse_code::clear_hints();
	
	if(AssetIsExisting(22, "IW3SP_MOD_LOC_KILLHOUSE_DEMOLITIONS_TRAINING"))
		objectiveText = &"IW3SP_MOD_LOC_KILLHOUSE_DEMOLITIONS_TRAINING";
	else 
		objectiveText = &"KILLHOUSE_GO_OUTSIDE_AND_REPORT";
	
	maps\killhouse_code::setObjectiveString( "obj_demolitions", objectiveText );
	maps\killhouse_code::setObjectiveState( "obj_demolitions", "done" );
	
	level.player_completed_demolitions_training = true;

	thread C4_complete_dialog();
	
	level notify ( "explosivesTraining_end" );
	flag_clear ( "aa_c4" );
}

C4_complete_dialog()
{
	level.newcastle maps\killhouse_code::execDialog( "chuckle" );	//< satisfied chuckling > 
	level.newcastle maps\killhouse_code::execDialog( "muchimproved" );	//Much improved.
	
	//if ( !flag( "start_obstacle" ) )
	level.newcastle maps\killhouse_code::execDialog( "passedeval" ); //All right Soap, you passed the weapons evaluation.
}

obstacle_Training()
{
	level endon ( "PlayerSkipTheOptionalCourses" );

	if( flag( "cargoshipTutoDone" ) )
		return;

	flag_set ( "aa_obstacle" );
	level notify ( "obstacleTraining_start" );
	
	// maps\killhouse_code::registerObjective( "obj_obstacle", &"KILLHOUSE_RUN_THE_OBSTACLE_COURSE", getEnt( "obstacleTraining_objective", "targetname" ) );
	// maps\killhouse_code::setObjectiveState( "obj_obstacle", "current" );
	
	//thread generic_compass_hint_reminder( "start_obstacle", 7 );
	
	getEnt( "obstacle_course_start", "targetname" ) waittill ( "trigger" );	

	flag_set ( "start_obstacle" );

	flag_trigger_init( "prone_entered", getEnt( "obstacleTraining_prone", "targetname" ) );
	thread maps\killhouse_code::obstacleTraining_buddies();
	
	thread maps\killhouse::obstacleTraining_dialog();

	flag_wait( "start_course" );
	
	maps\killhouse_code::setObjectiveLocation( "obj_obstacle", getEnt( "obj_course_end", "targetname" )  );
	
	move_mac_triggers = getentarray( "move_mac", "targetname" );
	array_thread( move_mac_triggers, maps\killhouse_code::move_mac );
	
	getEnt( "obstacleTraining_mantle", "targetname" ) waittill ( "trigger" );
	thread maps\killhouse_code::keyHint( "mantle", 5.0 );

	flag_wait( "obstacleTraining_crouch" );
	thread maps\killhouse_code::keyHint( "crouch" );

	flag_wait( "obstacleTraining_mantle2" );
	thread maps\killhouse_code::keyHint( "mantle", 5.0 );
	
	flag_wait( "prone_entered" );
	thread maps\killhouse_code::keyHint( "prone" );

	getEnt( "obstacleTraining_Standup", "targetname" ) waittill ( "trigger" );
	thread maps\killhouse_code::keyHint( "stand", 5.0 );
	maps\killhouse_code::clear_hints_on_stand();

	wait .1;
	
	//player must sprint
	if ( level.xenon )// ghetto but PS3 requires "Press X" and 360 requires "Click X"
		maps\killhouse_code::keyHint( "sprint" ); 
	else
		maps\killhouse_code::keyHint( "sprint_pc" );
		
	flag_set ( "player_sprinted" );
	
	if( !flag( "obstacle_course_end" ) )
		thread maps\killhouse_code::second_sprint_hint();
	
	flag_wait( "obstacle_course_end" );
	level notify ( "kill_sprint_hint" );
	maps\killhouse_code::clear_hints();
	maps\killhouse_code::setObjectiveState( "obj_obstacle", "done" );
	level.player_completed_obstacle_course_training = true;
	flag_clear ( "aa_obstacle" );
}

player_skipped_the_optional_objectives()
{
	while( true )
	{
		if(flag( "open_ship_hanger" ) )
		{
			if(!level.player_completed_demolitions_training)
				maps\killhouse_code::setObjectiveState( "obj_demolitions", "empty" );

			if(!level.player_completed_obstacle_course_training)
			{
				level.player_skipped_obstacle_course = true;
				maps\killhouse_code::setObjectiveState( "obj_obstacle", "empty" );
			}

			flag_set( "cargoshipTutoDone" );
			level notify("in_pit_with_frags");
			level notify("PlayerSkipTheOptionalCourses");
			level notify("obstacleTraining_end"); //for skip the Mac dialog
			break;		
		}
		wait (0.05);
	}
}