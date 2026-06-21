main()
{
	ReplaceFunc(maps\launchfacility_a::loudspeaker, ::loudspeaker);
}

loudspeaker()
{
	level.scr_sound["launchfacility_a_rul_motherland_sub"] = "launchfacility_a_rul_motherland_sub";
	level.scr_sound["launchfacility_a_rul_avengefallen_sub"] = "launchfacility_a_rul_avengefallen_sub";
	level.scr_sound["launchfacility_a_rul_restorehonor_sub"] = "launchfacility_a_rul_restorehonor_sub";

	common_scripts\utility::flag_wait("container_loudspeaker");

	//Facility perimeter has been compromised. All units proceed to defensive positions. Unknown enemy force moving in from the South or West. Facility is now on high alert.
	maps\launchfacility_a::dialogue_loudspeaker("launchfacility_a_rul_highalert");

	common_scripts\utility::flag_wait( "inside_perimeter" );
	wait(1);
	//Russian Loudspeaker
	//We are under attack! Lock down all points of entry immediately. 	megaphone/loudspeaker	
	level thread maps\launchfacility_a::dialogue_loudspeaker("launchfacility_a_rul_underattack");								
	
	common_scripts\utility::flag_wait( "migs_flyby1" );
	
	wait(7.5);
	//Russian Loudspeaker
	//Enemy units confirmed to be American special forces. Exercise extreme caution. Red Spetznaz units are en route to intercept.	megaphone/loudspeaker									
	maps\launchfacility_a::dialogue_loudspeaker("launchfacility_a_rul_redspentznaz");


	if ( !common_scripts\utility::flag( "obj_enemy_armor_complete" ) )
	{
		//Russian Loudspeaker
		//Fight for the glory of the Motherland! Fight the corruption of our once great nation!	loudspeaker
		maps\launchfacility_a::dialogue_loudspeaker("launchfacility_a_rul_motherland_sub");
    	wait (3);
	}
	if ( !common_scripts\utility::flag( "obj_enemy_armor_complete" ) )
	{
		//Russian Loudspeaker
		//Comrades! Remember the fallen and avenge them! Let us ensure that their sacrifices were not made in vain!	loudspeaker	
		maps\launchfacility_a::dialogue_loudspeaker("launchfacility_a_rul_avengefallen_sub");
    	wait (3);
	}
	if ( !common_scripts\utility::flag( "obj_enemy_armor_complete" ) )
	{
		//Russian Loudspeaker
		//We shall restore the honor of the days when this nation was feared by all others! We will not be stopped! We will not be turned aside so easily!	loudspeaker	
		maps\launchfacility_a::dialogue_loudspeaker("launchfacility_a_rul_restorehonor_sub");
	}

	while ( !common_scripts\utility::flag( "obj_enemy_armor_complete" ) )
	{
		//Russian Loudspeaker
		//Fight for the glory of the Motherland! Fight the corruption of our once great nation!	loudspeaker									
		maps\launchfacility_a::dialogue_loudspeaker("launchfacility_a_rul_motherland");
		
		if ( common_scripts\utility::flag( "obj_enemy_armor_complete" ) )
			break;
		wait (3);

		if ( common_scripts\utility::flag( "obj_enemy_armor_complete" ) )
			break;
		//Russian Loudspeaker
		//Comrades! Remember the fallen and avenge them! Let us ensure that their sacrifices were not made in vain!	loudspeaker									
		maps\launchfacility_a::dialogue_loudspeaker("launchfacility_a_rul_avengefallen");		

		if ( common_scripts\utility::flag( "obj_enemy_armor_complete" ) )
			break;		
		wait (3);
		
		if ( common_scripts\utility::flag( "obj_enemy_armor_complete" ) )
			break;
		//Russian Loudspeaker	
		//We shall restore the honor of the days when this nation was feared by all others! We will not be stopped! We will not be turned aside so easily!	loudspeaker									
		maps\launchfacility_a::dialogue_loudspeaker("launchfacility_a_rul_restorehonor");
			
	}

	//Russian Loudspeaker
	//Preparing launch tubes 2 through 6 for firing. Standby. 	megaphone/loudspeaker									
	maps\launchfacility_a::dialogue_loudspeaker("launchfacility_a_rul_preptubes");

	while ( true )
	{
		//Russian Loudspeaker
		//Fight for the glory of the Motherland! Fight the corruption of our once great nation!	loudspeaker									
		maps\launchfacility_a::dialogue_loudspeaker("launchfacility_a_rul_motherland");
		
		wait(2);
		
		//Russian Loudspeaker
		//Comrades! Remember the fallen and avenge them! Let us ensure that their sacrifices were not made in vain!	loudspeaker									
		maps\launchfacility_a::dialogue_loudspeaker("launchfacility_a_rul_avengefallen");		

		wait (3);

		//Russian Loudspeaker	
		//We shall restore the honor of the days when this nation was feared by all others! We will not be stopped! We will not be turned aside so easily!	loudspeaker									
		maps\launchfacility_a::dialogue_loudspeaker("launchfacility_a_rul_restorehonor");	
	}
}