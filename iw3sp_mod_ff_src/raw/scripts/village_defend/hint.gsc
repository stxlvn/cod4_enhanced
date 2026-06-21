#include common_scripts\utility;
#include maps\_utility;

main()
{
    ReplaceFunc(maps\village_defend::minigun_fallback, ::minigun_fallback);
    ReplaceFunc(maps\village_defend::airstrike_command, ::airstrike_command);
}

minigun_fallback()
{
	flag_wait( "ridgeline_targeted" );

	autosave_by_name( "ridgeline_under_mortar_fire" );
	
	redshirt1 = getent( "redshirt1", "targetname" );
	redshirt2 = getent( "redshirt2", "targetname" );
	
	redshirt_node1 = getnode( "fallback_redshirt1", "targetname" );
	redshirt_node2 = getnode( "fallback_redshirt2", "targetname" );

	//Left trigger spin functionality for console controllers
	if( level.console || getdvarint("gpad_in_use") )
	{
		level thread maps\village_defend::minigun_firstuse_check();
		//level thread minigun_session_check();
		
		add_hint_string( "minigun_spin_left_trigger", &"SCRIPT_PLATFORM_SPOOL_MINIGUN", maps\village_defend::should_break_minigun_spin_hint );
	}
	
	//"They're targeting our position with mortars. It's time to fall back."
	//iprintln( "They're targeting our position with mortars. It's time to fall back." );
	radio_dialogue_queue( "targetingour" );
	
	thread maps\village_defend::minigun_orders();
	
	wait 2;
	
	//"Two, falling back."
	radio_dialogue_queue( "twofallingback" );
	
	redshirt1 allowedstances ( "stand", "crouch", "prone" );
	redshirt1 setgoalnode( redshirt_node1 );
	wait randomfloatrange( 0.7, 1.2 );
	
	//"Three, on the move."
	radio_dialogue_queue( "threeonthemove" );
	
	redshirt2 allowedstances ( "stand", "crouch", "prone" );
	redshirt2 setgoalnode( redshirt_node2 );
	
	priceNode = getnode( "fallback_price", "targetname" );
	level.price allowedstances ( "stand", "crouch", "prone" );
	level.price setgoalnode( priceNode );
	
	//"Three here. Two's in the far eastern building. We've got the eastern road locked down."
	radio_dialogue_queue( "easternroadlocked" );
	
	level.price.baseaccuracy = 1;
	level.price.ignoreSuppression = true;
	
	redshirt1.baseaccuracy = 1;
	redshirt1.ignoreSuppression = true;
	
	redshirt2.baseaccuracy = 1;
	redshirt2.ignoreSuppression = true;
	
	thread maps\village_defend_code::friendly_pushplayer( "off" );
	
	wait 5;
	
	thread maps\village_defend::intro_church_tower_explode();
}

airstrike_command()
{
	flag_wait( "airstrikes_ready" );
	
	thread maps\village_defend::callStrike( level.player.origin, 1, ( 0, 110, 0 ) );
	wait 1.2;
	thread maps\village_defend::callStrike( level.player.origin, 1, ( 0, 96, 0 ) );
	wait 1;
	thread maps\village_defend::callStrike( level.player.origin, 1, ( 0, 126, 0 ) );
	
	wait 5;
	
	//"Bravo Six, this is Falcon One standing by to provide close air support. Gimme a target over."
	radio_dialogue_queue( "casready" );
	
	if ( isdefined( level.console ) && level.console || getdvarint("gpad_in_use"))
		thread maps\village_defend::airstrike_hint_console();
	else
		thread maps\village_defend::airstrike_hint_pc();
	
	level.player giveWeapon( "airstrike_support" );
	level.player SetActionSlot( 2, "weapon" , "airstrike_support" );
	
	thread maps\village_defend::airstrike_support();
	
	wait 3;
	
	flag_set( "falcon_one_finished_talking" );
	
	flag_wait( "return_trip_begins" );
	
	thread maps\village_defend::airstrike_frequency_check();
	
	level endon ( "stop_airstrike_reminders" );
	level endon ( "no_airstrike_ammo" );
	
	while( 1 )
	{
		startTime = gettime();
		wait 70;
		
		if( !level.airstrikeCalledRecently )
		{
			//"Bravo Six, this is Falcon One standing by to provide close air support. Gimme a target over."
			radio_dialogue_queue( "casready" );
			
			level.airstrikeCalledRecently = false;	
		}
	}
}