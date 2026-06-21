main()
{
    waittillframeend;
	level.xenon = (getDvarInt("gpad_in_use") && getDvarInt("gpad_style") == 0 );
	level.ps3 = (getDvarInt("gpad_in_use") && getDvarInt("gpad_style") == 1 );
	level.console = (getDvarInt("gpad_in_use"));

	// for text font scale and fontname.
	if(level.script == "ac130" && getdvar( "credits_load" ) == "1")
	{
		level.console = (getDvarInt("console_menu_style"));
	}
}