main()
{
    // Added the IW3SP-MOD Credits and 'Hold [USE] for skip.' hint.
	ReplaceFunc(maps\_credits::initCredits, ::initCredits);
}

initCredits()
{
	level.linesize = 1.35;
	level.headingsize = 1.75;
	level.linelist = [];
	
	maps\_utility::set_console_status();
    skipCreditsInit();
    initIW3SPMODCredits();
	maps\_credits::initIWCredits();
	maps\_credits::initActivisionCredits();
}

initIW3SPMODCredits()
{
    if( getdvar( "mapname" ) == "ac130" )
    {
        //maps\_credits::addLeftTitle( &"IW3SP_MOD_LOC_CREDIT_IW3SPMOD_TITLE" );
        maps\_credits::addLeftImage( "logo_iw3spmod", 256, 128, 3.875 ); // 3.1
        maps\_credits::addSpaceSmall();
        maps\_credits::addLeftTitle( &"IW3SP_MOD_LOC_CREDIT_MAIN_DEVELOPER_TITLE" );
        maps\_credits::addLeftName( &"IW3SP_MOD_LOC_CREDIT_JERRY" );
        maps\_credits::addSpace();
        maps\_credits::addLeftTitle( &"IW3SP_MOD_LOC_CREDIT_SPECIAL_THANKS_TITLE" );
        maps\_credits::addLeftName( &"IW3SP_MOD_LOC_CREDIT_NIKO" );
        maps\_credits::addLeftName( &"IW3SP_MOD_LOC_CREDIT_XOXOR" );
        maps\_credits::addLeftName( &"IW3SP_MOD_LOC_CREDIT_FED" );
        maps\_credits::addLeftName( &"IW3SP_MOD_LOC_CREDIT_VLAD" );
        maps\_credits::addLeftName( &"IW3SP_MOD_LOC_CREDIT_NUKEM9" );
        maps\_credits::addLeftName( &"IW3SP_MOD_LOC_CREDIT_XLABS" );
        maps\_credits::addLeftName( &"IW3SP_MOD_LOC_CREDIT_ALTERWARE" );
        maps\_credits::addLeftName( &"IW3SP_MOD_LOC_CREDIT_COD4X" );
        maps\_credits::addSpace();
	    maps\_credits::addSpace();        
    }
    else
    {
        //maps\_credits::addCenterHeading( &"IW3SP_MOD_LOC_CREDIT_IW3SPMOD_TITLE" );
        maps\_credits::addCenterImage( "logo_iw3spmod", 256, 128, 4.375 ); // 3.1
        maps\_credits::addSpaceSmall();
	    maps\_credits::addCenterDual( &"IW3SP_MOD_LOC_CREDIT_MAIN_DEVELOPER_TITLE", &"IW3SP_MOD_LOC_CREDIT_JERRY" );
        maps\_credits::addSpace();
        maps\_credits::addCenterHeading( &"IW3SP_MOD_LOC_CREDIT_SPECIAL_THANKS_TITLE" );
        maps\_credits::addCenterNameDouble( &"IW3SP_MOD_LOC_CREDIT_NIKO", &"IW3SP_MOD_LOC_CREDIT_XOXOR" );
        maps\_credits::addCenterNameDouble( &"IW3SP_MOD_LOC_CREDIT_FED", &"IW3SP_MOD_LOC_CREDIT_VLAD" );
        maps\_credits::addCenterNameDouble( &"IW3SP_MOD_LOC_CREDIT_XLABS", &"IW3SP_MOD_LOC_CREDIT_ALTERWARE" );
        maps\_credits::addCenterNameDouble( &"IW3SP_MOD_LOC_CREDIT_NUKEM9", &"IW3SP_MOD_LOC_CREDIT_COD4X" );
        maps\_credits::addSpace();
	    maps\_credits::addSpace();
    }
}

skipCreditsInit()
{
    level endon("skip_kill");

	level.skipDone = false;
    level.skipHoldUse = false;
    level.skipPressedUseFirstTime = false;
    level.skipFadeInTime = 0.5;
    level.skipFadeOutTime = 0.5;
    level.skipBarWidth = 128;
    level.skipBarHeight = 2;

    level.skipText = newHudElem();
	level.skipText.alignX = "right";
	level.skipText.alignY = "bottom";
	level.skipText.fontScale = 1.25;
	level.skipText.x = -10;
	level.skipText.y = -25;
	level.skipText.horzAlign = "right";
	level.skipText.vertAlign = "bottom";
	level.skipText.foreground = true;
	level.skipText settext ( &"IW3SP_MOD_LOC_CREDIT_HOLD_USE_TO_SKIP" );
	level.skipText.alpha = 0.0;

    level.skipBarProgress = newHudElem();
    level.skipBarProgress.sort = 1;
	level.skipBarProgress.alignX = "left";
	level.skipBarProgress.alignY = "bottom";
	level.skipBarProgress.x = -138;
	level.skipBarProgress.y = -20;
    level.skipBarProgress.horzAlign = "right";
	level.skipBarProgress.vertAlign = "bottom";
    level.skipBarProgress.width = level.skipBarWidth;
	level.skipBarProgress.height = level.skipBarHeight;
	level.skipBarProgress.foreground = true;
	level.skipBarProgress.alpha = 1.0;
    level.skipBarProgress.color = ( 1.0, 1.0, 1.0 );
    level.skipBarProgress setShader( "white", level.skipBarProgress.width, level.skipBarProgress.height );

    level.skipBarBackground = newHudElem();
    level.skipBarBackground.sort = -2;
	level.skipBarBackground.alignX = "right";
	level.skipBarBackground.alignY = "bottom";
	level.skipBarBackground.x = -10;
	level.skipBarBackground.y = -20;
    level.skipBarBackground.width = level.skipBarWidth;
	level.skipBarBackground.height = level.skipBarHeight;
	level.skipBarBackground.horzAlign = "right";
	level.skipBarBackground.vertAlign = "bottom";
	level.skipBarBackground.foreground = true;
	level.skipBarBackground.alpha = 1.0;
    level.skipBarBackground.color = ( 0.2, 0.2, 0.2 );
    level.skipBarBackground setShader( "black", level.skipBarBackground.width, level.skipBarBackground.height );

    // Loop checks
    thread skipFadeInCheck();
    thread checkSkipButton();
    level.skipBarProgress thread skipBarFill();

    level.skipText thread fadeInElement();
    level.skipBarBackground thread fadeInElement(); 
    level.skipBarProgress thread fadeInElement();
    level.skipText thread fadeOutElement( 7, true );
    level.skipBarBackground thread fadeOutElement( 7, true ); 
    level.skipBarProgress thread fadeOutElement( 7, true );
}

checkSkipButton()
{
    level endon("skip_kill");

    while( true )
    {
        while( !level.player useButtonPressed() )
            wait 0.05;
        level.skipHoldUse = true;
        if(!level.skipPressedUseFirstTime) 
        {
            level.skipPressedUseFirstTime = true;
            level notify( "kill_function" );
        }

        while( level.player useButtonPressed() )
            wait 0.05;
        level.skipHoldUse = false;
    }
}

skipFadeInCheck()
{
    level endon("skip_kill");

    while( true )
    {
        if(level.skipPressedUseFirstTime)
        {
            if( level.skipHoldUse )
            {
                level.skipText thread fadeInElement(0.2);
                level.skipBarBackground thread fadeInElement(0.2); 
                level.skipBarProgress thread fadeInElement(0.2);
            }
            else
            {
                level.skipText thread fadeOutElement();
                level.skipBarBackground thread fadeOutElement(); 
                level.skipBarProgress thread fadeOutElement();
            }
        }
        wait 0.05;
    }
}

skipBarFill()
{
    level notify("skip_timer_stopped");

    interval = .05;
    timesofar = 0;
    planttime = 3;
    barFrac = 0;

    while ( true )
    {
        while( level.skipHoldUse )
        {
            timesofar += interval;
		    barFrac = timesofar/planttime;
		    if(barFrac > 1)
		    	barFrac = 1;
		    if(barFrac < 0)
		    	barFrac = 0;
		    barWidth = int( level.skipBarBackground.width * barFrac );
		    self setShader( "white", barWidth, self.height );
		    if(timesofar >= planttime)
            {
                level.skipDone = true;
                break;
            }
            wait (interval);
        }

        while( !level.skipHoldUse )
        {
            timesofar = 0;
            self setShader( "white", 1, level.skipBarProgress.height );
            self.alpha = 0.0;
            if(level.skipDone)
                break;
            wait (interval);
        }

        if(level.skipDone)
        {
            level.skipText thread fadeOutElement();
            level.skipBarBackground thread fadeOutElement(); 
            self thread fadeOutElement();
            level notify( "skip_timer_stopped" );
            level notify( "skip_kill" );
            if( getdvar( "mapname" ) == "ac130" )
                thread credits_skipped( true );
            else
                thread credits_skipped();
            break;
        }
        
        wait (interval);
    }
}

credits_skipped( from_level )
{
    duration = 1.25;

    // Fade screen
	overlay = newHudElem();
	overlay.x = 0;
	overlay.y = 0;
	overlay setshader( "black", 640, 480 );
	overlay.alignX = "left";
	overlay.alignY = "top";
	overlay.horzAlign = "fullscreen";
	overlay.vertAlign = "fullscreen";
	overlay.alpha = 0;
	overlay.sort = 1;
	
	overlay fadeOverTime( duration );
	overlay.alpha = 1;

    if(isDefined(from_level))
    {
        if ( isdefined( level.credits_frommenu ) )
	    	changelevel( "" );
	    else
	    	maps\_endmission::credits_end();
    }
    else
        changelevel( "" );
}

fadeInElement( wait_delay )
{
    if(isDefined(wait_delay))
	    wait (wait_delay); 
    else
        wait 0.5;
	self fadeOverTime(level.skipFadeOutTime);	
	self.alpha = 1.0;
}

fadeOutElement( wait_delay, firstTime )
{
    if(isDefined(firstTime))
        level endon ("kill_function");

    if(isDefined(wait_delay))
	    wait (wait_delay); 
    else
        wait 0.5;
	self fadeOverTime(level.skipFadeOutTime);	
	self.alpha = 0.0;
}