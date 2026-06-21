init()
{
    // The placeholder line from the alpha build is missing in all localizations except for English, lol.
	if(getDvarInt("loc_language") == 0)
        return;
    // Three coming out!
	level.scr_sound[ "second_floor_left_guy" ][ "three_coming_out" ]	= "null";
}