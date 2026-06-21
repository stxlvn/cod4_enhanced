
#include common_scripts\utility;
#include maps\_utility;

main()
{	
	thread replace_hint_size();
    // Best squad record
	ReplaceFunc(maps\killhouse::cargoship_training, ::cargoship_training);
}

replace_hint_size()
{
	waittillframeend;
	if(getDvarInt("console_menu_style"))
		level.hint_text_size = 1.6;
	else
		level.hint_text_size = 1.2;
}

cargoship_training()
{
	level endon ( "clear_course" );
	level endon ( "mission failed" );
	cargoship_targets = getentarray( "cargoship_target", "script_noteworthy" );
	array_thread( cargoship_targets, maps\killhouse_code::cargoship_targets );
	thread maps\killhouse_code::rope();
	top_of_rope_trigger = getent( "top_of_rope_trigger", "targetname" );
	near_rope = getent( "near_rope", "targetname" );
	top_of_rope = getent( "top_of_rope", "targetname" );
	top_of_ladder_trigger = getent( "top_of_ladder_trigger", "targetname" );
	position_one = getent( "position_one", "targetname" );
	two = getent( "position_two", "targetname" );
	three = getent( "position_three", "targetname" );
	four = getent( "position_four", "targetname" );
	five = getent( "position_five", "targetname" );
	six = getent( "position_six", "targetname" );
	sprint = getent ( "sprint", "targetname" );
	final_obj = getent( "final_obj", "targetname" );
	setdvar( "killhouse_too_slow", "0" );
	
	volume = getent( three.script_noteworthy, "targetname" );
	volume2 = getent( six.script_noteworthy, "targetname" );
	first_time = true;
	previous_time = 0;
	previous_selection = "none";

	// Added 24.05.2023 (6:53 PM)
	selection = undefined;
	player_bestrecord = undefined;   //i don't know which your record.
    player_reach_record_gaz = false; //player not reach record Gaz.
	
	flash_volumes = getentarray ( "flash_volume", "script_noteworthy" );
	jump_off_trigger = getent ( "jump_off_trigger", "targetname" );
	
	
	flag_wait( "at_top_of_ladder" );
	maps\killhouse_code::clear_hints(); //remove ladder hint
	thread autosave_by_name( "ladder_top" );
	jump_off_trigger thread maps\killhouse_code::jumpoff_monitor();
	
	flag_wait ( "reveal_dialog_done" );
	
	while ( 1 )
	{
		if ( first_time )
		{
			if ( ( !( level.player getCurrentWeapon() == "mp5" ) ) || ( level.player GetWeaponAmmoStock( "flash_grenade" ) < 4 ) )
			{
				//"Pick up that MP5 and four flashbangs." );
				level.price thread maps\killhouse_code::execDialog( "pickupmp5" );
				maps\killhouse_code::setObjectiveString( "obj_price", &"KILLHOUSE_EQUIP_THE_MP5_AND_PICK" );
				maps\killhouse_code::setObjectiveLocation( "obj_price", getent( "obj_flashes", "targetname" ) );
			}
		}
		else
		{
			jump_off_trigger thread maps\killhouse_code::jumpoff_monitor();
			
			//"Replace any flash bangs you used." 
			level.price maps\killhouse_code::execDialog( "replaceflash" );
			if ( !(level.player getCurrentWeapon() == "mp5") )
			{
				//"Equip your MP5." 
				level.price maps\killhouse_code::execDialog( "equipmp5" );
			}
		}
		
		nag_time = 0;
		while ( !(level.player getCurrentWeapon() == "mp5") )
		{
			if ( (level.player istouching ( near_rope ) ) && ( nag_time > 4 ) )
			{
				//"Soap. Equip your MP5." );
				level.price thread maps\killhouse_code::execDialog( "soapequipmp5" );
				nag_time = 0;
			}
			wait ( 1.0 );
			nag_time++;
		}
		
		nag_time = 0;
		while ( level.player GetWeaponAmmoStock( "flash_grenade" ) < 4 )
		{
			if ( (level.player istouching ( near_rope ) ) && ( nag_time > 4 ) )
			{
				level.price thread maps\killhouse_code::execDialog( "soap4flash" );
				//"Soap. Pick up four flash bangs." );
				nag_time = 0;
			}
			wait ( 1.0 );
			nag_time++;
		}
		if ( first_time )
			flag_set ( "got_flashes" );
		
		
		thread maps\killhouse_code::flashbang_ammo_monitor( flash_volumes );
			
		if ( first_time )
		{
			level.price maps\killhouse_code::execDialog( "ropedeck" );	//On my go, I want you to rope down to the deck and rush to position 1.
			level.price maps\killhouse_code::execDialog( "stormstairs" );	//After that you will storm down the stairs to position 2.
			level.price maps\killhouse_code::execDialog( "hit3and4" );	//Then hit position 3 and 4 following my precise instructions at each position.
		}

		if( first_time )
		{
			if( !flag("obstacle_course_end") )
				level.player_skipped_obstacle_course = true;
		}

		
		//"Grab the rope when your ready."
		level.price thread maps\killhouse_code::execDialog( "grabrope" );
		delaythread ( 1.5, maps\killhouse_code::rope_obj );
		
		flag_set ( "activate_rope" );
		level notify ( "show_glowing_rope" );
	
		thread autosave_by_name( "starting_bridge_attack" );
		
		level waittill ( "starting_rope" );
		level notify ( "hide_glowing_rope" );
		flag_clear ( "activate_rope" );
		
		level notify ( "okay_if_friendlies_in_line_of_fire" );
		level notify ( "starting_cargoship_obj" );
		maps\killhouse_code::setObjectiveString( "obj_price", &"KILLHOUSE_CLEAR_THE_CARGOSHIP_BRIDGE" );
		maps\killhouse_code::setObjectiveLocation( "obj_price", position_one );
		
		level.price thread maps\killhouse_code::execDialog( "gogogo" );
		
		if ( getdvarint( "killhouse_too_slow" ) >= 1 )
			thread maps\killhouse_code::startTimer( 120 );
		else
			thread maps\killhouse_code::startTimer( 60 );
		
		thread maps\killhouse_code::accuracy_bonus();
		if ( isdefined ( level.IW_deck ) )
			level.IW_deck destroy();
		
		position_one maps\killhouse_code::wait_till_pos_cleared();
		
		maps\killhouse_code::setObjectiveLocation( "obj_price", two );
		level.price thread maps\killhouse_code::execDialog( "position2" );
		
		two maps\killhouse_code::wait_till_pos_cleared();
			
		maps\killhouse_code::setObjectiveLocation( "obj_price", three );
		
		three thread maps\killhouse_code::flash_dialog_three( volume );
		three maps\killhouse_code::wait_till_flashed( volume );
		maps\killhouse_code::clear_hints();
			
		level.price thread maps\killhouse_code::execDialog( "position4" );
		maps\killhouse_code::setObjectiveLocation( "obj_price", four );
		
		four maps\killhouse_code::wait_till_pos_cleared();
			
		maps\killhouse_code::setObjectiveLocation( "obj_price", five );
		level.price thread maps\killhouse_code::execDialog( "5go" );
		
		five maps\killhouse_code::wait_till_pos_cleared();
			
		maps\killhouse_code::setObjectiveLocation( "obj_price", six );
		level.price thread maps\killhouse_code::execDialog( "6go" );
		
		six thread maps\killhouse_code::flash_dialog_six( volume2 );
		six maps\killhouse_code::wait_till_flashed( volume2 );
		maps\killhouse_code::clear_hints();
		six maps\killhouse_code::wait_till_pos_cleared( "skip_trigger" );
		
		
		maps\killhouse_code::setObjectiveLocation( "obj_price", final_obj );
		level.price thread maps\killhouse_code::execDialog( "finalgo" );
		maps\killhouse::notify_on_sprint();
		thread maps\killhouse::flag_on_notify( "sprinted" );
		
		flag_wait ( "sprint" );
		flag_set ( "ready_to_finish" );
		
		level.price thread maps\killhouse_code::execDialog( "sprint" );
		thread maps\killhouse_code::dialog_sprint_reminders();
		
		if ( level.xenon || getdvarint("gpad_style") == 0 )
			thread maps\killhouse_code::keyHint( "sprint" ); 
		else
			thread maps\killhouse_code::keyHint( "sprint_pc" );
		
		flag_wait ( "sprinted" );
	
		if( !flag( "at_finish" ) )
			thread maps\killhouse_code::second_sprint_hint();
	
		flag_wait( "at_finish" );
		level notify ( "kill_sprint_hint" );
		maps\killhouse_code::clear_hints();
		
		flag_clear ( "ready_to_finish" );
		flag_clear ( "at_finish" );
		flag_clear( "at_top_of_ladder" );
		flag_clear ( "sprinted" );
		flag_clear ( "sprint" );
		
		level notify ( "test_cleared" );
		if ( first_time )
			thread maps\killhouse::debrief();
		final_time = maps\killhouse_code::killTimer( 15.1, false );
		
		if( !( maps\_cheat::is_cheating() ) && ! ( flag("has_cheated") ) )
			level.player UploadTime( 19, final_time );
		
		if ( player_reach_record_gaz == false )
		{
		    if( final_time < 19.0 )
		    {
		        player_bestrecord = final_time;
		        selection = "newrecord";
				player_reach_record_gaz = true;
		    }
			else
				selection = dialog_end_of_course( first_time, final_time, previous_time, previous_selection );
		}
	   	else if ( final_time < ( player_bestrecord ) && player_reach_record_gaz == true )
	    {
		    player_bestrecord = final_time;
			selection = "newrecord";
	    }	
		else 
		    selection = dialog_end_of_course( first_time, final_time, previous_time, previous_selection );

		level.price maps\killhouse_code::execDialog( selection );
	
		previous_selection = selection;
		previous_time = final_time;
		first_time = false;
		
		//"Climb up the ladder if you want another go." );
		level.price maps\killhouse_code::execDialog( "anothergo" );
		
		//"Otherwise come over to the monitors for a debrief." );
		level.price maps\killhouse_code::execDialog( "debrief" );
		
		maps\killhouse_code::clear_hints();
		maps\killhouse_code::setObjectiveState( "obj_price", "done" );

		flag_wait( "at_top_of_ladder" );
		//top_of_ladder_trigger waittill ( "trigger" );
	}
}

dialog_end_of_course( first_time, final_time, previous_time, previous_selection )
{
	if ( ! first_time )
	{
		if ( ( previous_time + 2 ) < final_time )
		{
			if ( ( randomint ( 2 ) ) > 0 )	
			{
				//Don't waste our time Soap. The idea is to take less time, not more.	
				selection = ( "lesstime" ); 
				return selection;
			}
			else
			{	
				if(level.player_skipped_obstacle_course) 
					selection = ( "lesstime" ); //Don't waste our time Soap. The idea is to take less time, not more
				else
					selection = ( "letyouskip" ); //You're getting' slower. Perhaps it was a mistake to let you skip the obstacle course.
				return selection;
			}
		}
		
		if ( previous_time > ( final_time + 3 ) )
		{
			if ( ( randomint ( 2 ) ) > 0 )	
			{
				//That was an improvement, but it's not hard to improve on garbage. Try it again.	
				selection = ( "tryitagain" ); 
				return selection;
			}
			else
			{	
				//That was better. Not great. But better.	
				selection = ( "notgreat" ); 
				return selection;
			}
		}
		
		if ( ( level.bonus_time < 1.8 ) && ( previous_selection != "sloppy" ) )
		{
			//Fast, but sloppy. You need to work on your accuracy.	
			selection = ( "sloppy" ); 
			return selection;
		}
	}
	
	num = randomint ( 2 );
	if ( num == 0 )	
	{
		//All right Soap, that's enough. You'll do.	
		selection = ( "youlldo" );
		return selection;
	}
	//else if ( num == 1 )	
	//{
		//I've seen better, but that'll do.	
	//	selection = ( "seenbetter2" ); 
	//	return selection;
	//}
	else	
	{
		//"Pretty good, Soap. But I've seen better." );
		selection = "seenbetter";
		return selection;
	}
}