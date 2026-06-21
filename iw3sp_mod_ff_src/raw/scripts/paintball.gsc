// New 'Paintball' cheat from the Wii Version
init()
{
	maps\_cheat::addCheat( "sf_use_paintball", ::paintballMode );

	level.cheatDvars = getArrayKeys( level.cheatStates );
			
	for ( ;; )
	{
		for ( index = 0; index < level.cheatDvars.size; index++ )
			maps\_cheat::checkCheatChanged( level.cheatDvars[index] );

		wait 0.5;
	}
}

paintballMode( cheatValue )
{
	if ( level.script == "ac130" || level.script == "aftermath" )
		return;
	
	if ( cheatValue )
		setsaveddvar ( "paintball_mode",  1 );
	else
		setsaveddvar ( "paintball_mode",  0 );
}