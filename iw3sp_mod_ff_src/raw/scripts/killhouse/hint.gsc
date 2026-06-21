#include common_scripts\utility;
#include maps\_utility;

main()
{
    precacheShader( "hud_dpad_ps3" );
    ReplaceFunc(maps\killhouse_code::auto_aim, ::auto_aim);
	ReplaceFunc(maps\killhouse_code::objective_hints, ::objective_hints);
}

objective_hints( completion_flag )
{
	level endon ( "mission failed" );
	level endon ( "navigationTraining_end" );
	level endon ( "reveal_dialog_starting" );
	
	maps\killhouse_code::compass_hint();
	
	wait 2;
	
	if ( level.console )
	{
		if ( level.xenon )
			maps\killhouse_code::keyHint( "objectives", 6.0);
		else
			maps\killhouse_code::hint( &"KILLHOUSE_HINT_CHECK_OBJECTIVES_SCORES_PS3", 6 ); 
	}
	else
	{ 
		maps\killhouse_code::keyHint( "objectives_pc", 6.0);
	}
	
	//level.marine1.lastNagTime = getTime();
	timePassed = 16;
	for ( ;; )
	{
		//if( distance( level.player.origin, level.marine1.origin ) < 512 )
		//	level.marine1 nagPlayer( "squadwaiting", 15.0 );

		if ( !flag( completion_flag ) && timePassed > 20.0 )
		{
			//hint( &"KILLHOUSE_HINT_OBJECTIVE_REMINDER", 6.0 );
			thread maps\killhouse_code::compass_reminder();
			RefreshHudCompass();
			//wait( 0.5 );
			//thread hint( &"KILLHOUSE_HINT_OBJECTIVE_REMINDER2", 10.0 );
			timePassed = 0;
		}

		if( flag("near_grenade_area") || flag("start_obstacle") )
			timePassed = 0;
		else
			timePassed += 0.05;

		wait ( 0.05 );
	}
}

M203_icon_hint()
{
	if ( getdvar( "chaplincheat" ) == "1" )
		return;

	maps\killhouse_code::clear_hints();
	//level endon ( "clearing_hints" );
	
	maps\killhouse_code::add_hint_background();
	//Notice you now have an icon of a grenade launcher on your HUD.
	level.hintElem = maps\_hud_util::createFontString( "objective", level.hint_text_size );
	level.hintElem.hidewheninmenu = true;
	level.hintElem maps\_hud_util::setPoint( "TOP", undefined, 0, 110 ); //35
	level.hintElem.sort = 0.5;

	level.hintElem setText( &"KILLHOUSE_HINT_LAUNCHER_ICON" );

    posY = 145;
    iconElem = undefined;
	
    if(getdvarint("gpad_in_use"))
    {
        if(getdvarint("gpad_style") == 0 )
            iconElem = maps\_hud_util::createIcon( "hud_dpad", 32, 32 );
        else
            iconElem = maps\_hud_util::createIcon( "hud_dpad_ps3", 32, 32 );
	    iconElem.hidewheninmenu = true;
	    iconElem maps\_hud_util::setPoint( "TOP", undefined, 0, posY );
        posY += 30;
    }
	
	iconElem2 = maps\_hud_util::createIcon( "hud_icon_40mm_grenade", 64, 32 );
	iconElem2.hidewheninmenu = true;
	iconElem2 maps\_hud_util::setPoint( "TOP", undefined, 0, posY );	

	level waittill ( "clearing_hints" );

    if(getdvarint("gpad_in_use"))
	    iconElem maps\_hud_util::setPoint( "CENTER", "BOTTOM", -320, -20, 1.0 );

	iconElem2 maps\_hud_util::setPoint( "CENTER", "BOTTOM", -320, -20, 1.0 );
	
    if(getdvarint("gpad_in_use"))
	    iconElem scaleovertime(1, 20, 20);

	iconElem2 scaleovertime(1, 20, 20);
	
	wait .70;
    if(getdvarint("gpad_in_use"))
    {
	    iconElem fadeovertime(.15);
	    iconElem.alpha = 0;
    }
	
	iconElem2 fadeovertime(.15);
	iconElem2.alpha = 0;
	wait .5;
	
	//clear_hints();
}

C4_icon_hint()
{
	if ( getdvar( "chaplincheat" ) == "1" )
		return;

	maps\killhouse_code::clear_hints();
	level endon ( "clearing_hints" );

	maps\killhouse_code::add_hint_background();
	level.hintElem = maps\_hud_util::createFontString( "objective", level.hint_text_size );
	level.hintElem.hidewheninmenu = true;
	level.hintElem maps\_hud_util::setPoint( "TOP", undefined, 0, 110 );
	level.hintElem.sort = 0.5;

	level.hintElem setText( &"KILLHOUSE_HINT_C4_ICON" );
	//level.hintElem endon ( "death" );

    posX = -16;
	
    if(getdvarint("gpad_in_use"))
    {
        if(getdvarint("gpad_style") == 0 )
            level.iconElem = maps\_hud_util::createIcon( "hud_dpad", 32, 32 );
        else
            level.iconElem = maps\_hud_util::createIcon( "hud_dpad_ps3", 32, 32 );
	    level.iconElem.hidewheninmenu = true;
	    level.iconElem maps\_hud_util::setPoint( "TOP", undefined, posX, 155 );
        posX += 32;
    }
	
	level.iconElem2 = maps\_hud_util::createIcon( "hud_icon_c4", 32, 32 );
	level.iconElem2.hidewheninmenu = true;
	level.iconElem2 maps\_hud_util::setPoint( "TOP", undefined, posX, 155 );


	level waittill ( "c4_equiped" );

    if(getdvarint("gpad_in_use"))
    {
	    level.iconElem3 = maps\_hud_util::createIcon( "hud_arrow_down", 24, 24 );
	    level.iconElem3.hidewheninmenu = true;
	    level.iconElem3 maps\_hud_util::setPoint( "TOP", undefined, -16, 159 );
	    level.iconElem3.sort = 1;
	    level.iconElem3.color = (1,1,0);
	    level.iconElem3.alpha = .7;
    }


	level waittill ( "C4_the_car" );
	
    if(getdvarint("gpad_in_use"))
    {
        level.iconElem maps\_hud_util::setPoint( "CENTER", "BOTTOM", -320, -20, 1.0 );
	    level.iconElem3 maps\_hud_util::setPoint( "CENTER", "BOTTOM", -320, -20, 1.0 );
    }
	
	level.iconElem2 maps\_hud_util::setPoint( "CENTER", "BOTTOM", -320, -20, 1.0 );
	
    if(getdvarint("gpad_in_use"))
    {
	    level.iconElem scaleovertime(1, 20, 20);
	    level.iconElem3 scaleovertime(1, 15, 15);
    }

	level.iconElem2 scaleovertime(1, 20, 20);
	
	wait .85;
    if(getdvarint("gpad_in_use"))
    {
	    level.iconElem fadeovertime(.15);
	    level.iconElem.alpha = 0;

	    level.iconElem3 fadeovertime(.15);
	    level.iconElem3.alpha = 0;
    }

	
	level.iconElem2 fadeovertime(.15);
	level.iconElem2.alpha = 0;

    if(getdvarint("gpad_in_use"))
    {
        level.iconElem destroy();
        level.iconElem3 destroy();
    }
	
	level.iconElem2 destroy();
}

auto_aim()
{
	if ( level.console )
	{
		if(isdefined( getdvar("input_autoaim") ) || isdefined( getdvar("gpad_aimassist") ) )
			if ( getdvar("input_autoaim") == "1" || getdvar( "gpad_aimassist") == "1" )
				return true;
	}
	return false;
}