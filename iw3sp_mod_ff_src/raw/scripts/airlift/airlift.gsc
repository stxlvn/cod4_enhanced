#include common_scripts\utility;
#include maps\_utility;

main()
{
	ReplaceFunc(maps\airlift::dialogue_nuke, ::dialogue_nuke);
}

dialogue_nuke()
{
	flag_wait( "nuke_section_start" );
	
	//HQ Radio Voice
	//Outlaw this is Command. We have a probable nuclear threat in the capital. Proceed to the minimum safe distance until the all clear is given by the NEST team.
	radio_dialogue_queue( "airlift_hqr_nuclearthreat" );
	
	wait(1);
	//Lt. Vasquez
	//Go! Go!
	radio_dialogue_queue( "airlift_vsq_gogo" );
	
	wait(5.5);

	setsaveddvar( "sm_sunSampleSizeNear", 1 ); // air
	setsaveddvar( "sm_sunShadowScale", 0.5 ); // optimization

	//Male Helicopter Pilot
	//Ladies and gentlemen this is your Captain speaking. We're in for some chop! Hang on! Jake gimme max power.
	radio_dialogue_queue( "airlift_mhp_inforchop" );
	
	wait(.5);
	//Co Pilot
	//Roger that.
	//radio_dialogue_queue( "airlift_cop_rogerthat" );

	level.scr_radio[ "airlift_hqr_confirmed_sub" ] = "airlift_hqr_confirmed_sub";
	
	//HQ Radio Voice
	//All U.S. forces, be advised, we have a confirmed nuclear threat in the city. NEST teams are on site and attempting to disarm. I repeat, we have a confirmed nu-(BOOM)
	level thread radio_dialogue( "airlift_hqr_confirmed_sub" );
	level thread radio_dialogue_queue( "airlift_hqr_confirmed" );
	
	flag_wait( "shockwave_about_to_hit_player");
	//Lt. Vasquez
	//Everyone hang onnnn!!!
	level thread radio_dialogue( "airlift_vsq_hangon" );
}
