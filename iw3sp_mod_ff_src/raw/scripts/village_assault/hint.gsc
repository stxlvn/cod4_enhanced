#include common_scripts\utility;
#include maps\_utility;

main()
{
	ReplaceFunc(maps\village_assault_code::air_support_hint_print_activate, ::air_support_hint_print_activate);
}

air_support_hint_print_activate()
{
	level endon( "alasad_sequence_started" );
	
	while( !maps\village_assault_code::player_activated_air_support() )
	{	
		if ( level.air_support_hint_print_dialog_next == 0 )
		{
			// Soap! Call in air support on that building!	
			level.price thread maps\_anim::anim_single_solo( level.price, "airsupport" );
		}
		else if ( level.air_support_hint_print_dialog_next == 1 )
		{
			// Use our air support to soften up that building!
			level.price thread maps\_anim::anim_single_solo( level.price, "softenup" );
		}
		else
		{
			// Mosin Two-Five here. We are ready to attack and are standing by for new orders.
			thread radio_dialogue_queue( "readytoattack" );
		}
		level.air_support_hint_print_dialog_next++;
		
		if ( !flag( "gave_air_support_hint" ) )
		{
			flag_set( "gave_air_support_hint" );
			if ( isdefined( level.console ) && level.console || getdvarint("gpad_in_use") )
				thread maps\village_assault_code::display_air_support_hint_console();
			else
				thread maps\village_assault_code::display_air_support_hint_pc();
		}
		wait 5;
		level.air_support_hint_delete = true;
		wait 60;
	}
}
