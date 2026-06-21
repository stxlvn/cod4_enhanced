main()
{
	ReplaceFunc(maps\launchfacility_b::play_sound_on_speaker, ::play_sound_on_speaker);
}

play_sound_on_speaker( soundalias, interrupt )
{
	speaker_array = getentarray ( "speaker" ,"targetname" );
	soundalias_sub = soundalias + "_sub";

	if ( isdefined( interrupt ) )
	{
		level notify( "speaker_interrupt" );

		for( i=0; i<speaker_array.size; i++ )
		{
			speaker_array[i] stopsounds();
		}
		wait .5;
	}
	else if ( common_scripts\utility::flag( "speakers_active" ) )
		return;		
		
	level endon( "speaker_interrupt" );
	common_scripts\utility::flag_set( "speakers_active" );

	speaker_array = maps\_utility::get_array_of_closest( level.player.origin ,speaker_array, undefined, 2);

	speaker_array[0] playsound( soundalias_sub, "sounddone", true );
	speaker_array[1] playsound( soundalias );
	speaker_array[0] waittill( "sounddone" );

	common_scripts\utility::flag_clear( "speakers_active" );
}