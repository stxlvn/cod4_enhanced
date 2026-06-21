// Script '_names.gsc' works once time when map is running
// P.S: Use and save in 1252 encoding!
init()
{
	level.current_language = GetLanguage();

	ReplaceFunc(maps\_names::get_name_for_nationality, ::get_name_for_nationality);
	ReplaceFunc(maps\_names::getRankFromName, ::getRankFromName);

	//iPrintLnBold( "Current language: " + GetLanguage() );

	RenameAIName();
}

// Need make refactoring the code...
RenameAIName()
{
	while( true )
	{
		ai = getaiarray();
		for( i = 0; i < ai.size; i++ )
		{
			// Check the name by the two ways
			if( isDefined( ai[i].script_friendname ) )
			{
				if( IsHero ( ai[i].script_friendname ) )
				{
					if( IsMainHero( ai[i].script_friendname ) )
					{
						if( HasCaptainPriceName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Capitaine Price" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Capitano Price" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Capitán Price" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetCaptainPriceName() );
						}
						else if( HasGazName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetGazName() );
							else
								ChangeName( ai[i], ai[i].script_friendname ); // Keep the original name for all localizations.
						}
						else if( HasSSgtGriggsName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Griggs" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Griggs" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento 1º Griggs" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSSgtGriggsName() );
						}
						else if( HasLtVasquezName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Lieutenant Vasquez" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Tenente Vasquez" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Teniente Vasquez" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetLtVasquezName() );
						}
						else if( HasCaptainMacMillanName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Capitaine MacMillan" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Capitano MacMillan" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Capitán MacMillan" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetCaptainMacMillanName() );
						}
						else if( HasSgtKamarovName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Kamarov" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Kamarov" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento Kamarov" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSgtKamarovName() );
						}
						else if( HasNikolaiName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetNikolaiName() );
							else
								ChangeName( ai[i], ai[i].script_friendname ); // Keep the original name for all localizations.
						}
						else if( HasViktorZakhaevName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetViktorZakhaevName() );
							else
								ChangeName( ai[i], ai[i].script_friendname ); // Keep the original name for all localizations.
						}
					}
					else if( isSecondaryHero( ai[i].script_friendname ) )
					{
						if ( HasSgtWallcroftName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Wallcroft" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Wallcroft" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento Wallcroft" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSgtWallcroftName() );
						}
						else if ( HasPvtGriffenName( ai[i].script_friendname ) )
						{
							level waittill( "intro_movie_done" ); // cargoship.gsc [344 line]

							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Soldat Griffen", true );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Soldato Griffen", true );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Soldado Griffen", true );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetPvtGriffenName(), true );
						}
						else if ( HasCptPelayoName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Capitaine Pelayo" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Capitano Pelayo" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Capitán Pelayo" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetCptPelayoName() );
						}
						else if ( HasCptKeatingName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Capitaine Keating" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Capitano Keating" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Capitán Keating" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetCptKeatingName() );
						}
						else if ( HasLtVolkerName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Lieutenant Volker" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Tenente Volker" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Teniente Volker" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetLtVolkerName() );
						}
						else if ( HasPvtMasseyName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Soldat Massey" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Soldato Massey" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Soldado Massey" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetPvtMasseyName() );
						}
						else if ( HasPvtRoycewiczName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Soldat Roycewicz" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Soldato Roycewicz" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Soldado Roycewicz" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetPvtRoycewiczName() );
						}
						else if ( HasPvtWestName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Soldat West" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Soldato West" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Soldado West" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetPvtWestName() );
						}
						else if ( HasSgtScullyName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Scully" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Scully" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento Scully" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSgtScullyName() );
						}
						else if ( HasCW4SmithName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetCW4SmithName() );
							else
								ChangeName( ai[i], ai[i].script_friendname ); // Keep the original name for all localizations.
						}
						else if ( HasSgtLovejoyName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Lovejoy" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Lovejoy" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento Lovejoy" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSgtLovejoyName() );
						}
						else if ( HasPvtLootzName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Soldat Lootz" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Soldato Lootz" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Soldado Lootz" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetPvtLootzName() );
						}
						else if ( HasMacName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetMacName() );
							else
								ChangeName( ai[i], ai[i].script_friendname ); // Keep the original name for all localizations.
						}
						else if ( HasSgtNewcastleName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Newcastle" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Newcastle" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento Newcastle" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSgtNewcastleName() );
						}
						else if ( HasSgtAremName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Arem" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Arem" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento Arem" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSgtAremName() );
						}	
						else if ( HasSgtBartonName( ai[i].script_friendname ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Barton" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].script_friendname );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Barton" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento Barton" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSgtBartonName() );
						}
					}
				}
			}

			if( isDefined( ai[i].name ) )
			{
				if( IsHero ( ai[i].name ) )
				{
					if( IsMainHero( ai[i].name ) )
					{
						if( HasCaptainPriceName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Capitaine Price" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Capitano Price" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Capitán Price" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetCaptainPriceName() );
						}
						else if( HasGazName( ai[i].name ) )
						{
							if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetGazName() );
							else
								ChangeName( ai[i], ai[i].name ); // Keep the original name for all localizations.
						}
						else if( HasSSgtGriggsName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Griggs" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Griggs" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento 1º Griggs" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSSgtGriggsName() );
						}
						else if( HasLtVasquezName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Lieutenant Vasquez" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Tenente Vasquez" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Teniente Vasquez" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetLtVasquezName() );
						}
						else if( HasCaptainMacMillanName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Capitaine MacMillan" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Capitano MacMillan" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Capitán MacMillan" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetCaptainMacMillanName() );
						}
						else if( HasSgtKamarovName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Kamarov" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Kamarov" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento Kamarov" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSgtKamarovName() );
						}
						else if( HasNikolaiName( ai[i].name ) )
						{
							if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetNikolaiName() );
							else
								ChangeName( ai[i], ai[i].name ); // Keep the original name for all localizations.
						}
						else if( HasViktorZakhaevName( ai[i].name ) )
						{
							if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetViktorZakhaevName() );
							else
								ChangeName( ai[i], ai[i].name ); // Keep the original name for all localizations.
						}
					}
					else if( isSecondaryHero( ai[i].name ) )
					{
						if ( HasSgtWallcroftName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Wallcroft" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Wallcroft" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento Wallcroft" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSgtWallcroftName() );
						}
						else if ( HasPvtGriffenName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Soldat Griffen" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Soldato Griffen" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Soldado Griffen" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetPvtGriffenName() );
						}
						else if ( HasCptPelayoName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Capitaine Pelayo" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Capitano Pelayo" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Capitán Pelayo" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetCptPelayoName() );
						}
						else if ( HasCptKeatingName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Capitaine Keating" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Capitano Keating" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Capitán Keating" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetCptKeatingName() );
						}
						else if ( HasLtVolkerName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Lieutenant Volker" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Tenente Volker" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Teniente Volker" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetLtVolkerName() );
						}
						else if ( HasPvtMasseyName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Soldat Massey" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Soldato Massey" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Soldado Massey" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetPvtMasseyName() );
						}
						else if ( HasPvtRoycewiczName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Soldat Roycewicz" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Soldato Roycewicz" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Soldado Roycewicz" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetPvtRoycewiczName() );
						}
						else if ( HasPvtWestName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Soldat West" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Soldato West" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Soldado West" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetPvtWestName() );
						}
						else if ( HasSgtScullyName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Scully" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Scully" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento Scully" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSgtScullyName() );
						}
						else if ( HasCW4SmithName( ai[i].name ) )
						{
							if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetCW4SmithName() );
							else
								ChangeName( ai[i], ai[i].name ); // Keep the original name for all localizations.
						}
						else if ( HasSgtLovejoyName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Lovejoy" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Lovejoy" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento Lovejoy" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSgtLovejoyName() );
						}
						else if ( HasPvtLootzName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Soldat Lootz" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Soldato Lootz" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Soldado Lootz" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetPvtLootzName() );
						}
						else if ( HasMacName( ai[i].name ) )
						{
							if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetMacName() );
							else
								ChangeName( ai[i], ai[i].name ); // Keep the original name for all localizations.
						}
						else if ( HasSgtNewcastleName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Newcastle" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Newcastle" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento Newcastle" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSgtNewcastleName() );
						}
						else if ( HasSgtAremName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Arem" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Arem" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento Arem" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSgtAremName() );
						}	
						else if ( HasSgtBartonName( ai[i].name ) )
						{
							if( GetLanguage() == "english" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "french" )
								ChangeName( ai[i], "Sergent Barton" );
							else if( GetLanguage() == "german" )
								ChangeName( ai[i], ai[i].name );
							else if( GetLanguage() == "italian" )
								ChangeName( ai[i], "Sergente Barton" );
							else if( GetLanguage() == "spanish" )
								ChangeName( ai[i], "Sargento Barton" );
							else if( GetLanguage() == "russian" )
								ChangeName( ai[i], scripts\_names_ru::GetSgtBartonName() );
						}
					}
				}
			}
		}
		wait( 0.05 );
	}
}

IsHero( name )
{
	if( IsMainHero( name ) )
		return true;
	else if ( isSecondaryHero( name ) )
		return true;
	return false;
}

IsMainHero( name )
{
	if( HasCaptainPriceName( name ) || HasGazName( name ) || HasSSgtGriggsName( name ) || HasLtVasquezName( name ) || HasCaptainMacMillanName( name ) ||
		HasSgtKamarovName( name ) || HasNikolaiName( name )|| HasViktorZakhaevName( name ) )
		return true;
	return false;
}

isSecondaryHero( name )
{
	if( HasSgtWallcroftName( name ) || HasPvtGriffenName( name ) || HasCptPelayoName( name ) || HasCptKeatingName( name ) || 
		HasLtVolkerName( name ) || HasPvtMasseyName( name ) || HasPvtRoycewiczName( name ) || HasPvtWestName( name ) || 
		HasSgtScullyName( name ) || HasCW4SmithName( name ) || HasSgtLovejoyName( name ) || HasPvtLootzName( name ) ||
		HasMacName( name) || HasSgtNewcastleName ( name ) || HasSgtAremName( name ) || HasSgtBartonName( name ) )
		return true;
	return false;
}

ChangeName( entity, name, ignore_checks )
{
	if( isDefined( ignore_checks ) )
		ignore_checks = true;
	else
		ignore_checks = false;

	if( ShouldChangeTheName( entity, ignore_checks ) )
	{
		entity.name = name;
		entity.script_friendname = name;
	}
}

ShouldChangeTheName( entity, ignore_checks )
{
	if( isDefined( ignore_checks ) )
	{
		return true;
	}
	else
	{
		name = entity.name;

		if( name == entity.name )
			return true;
		else if( name == entity.script_friendname )
			return true;
		else if( level.current_language != GetLanguage() )
			return true;
		return false;
	}
}

GetLanguage()
{
	language_index = getDvarInt( "loc_language" );
	language_name = "";

	switch( language_index )
	{
		case 0: language_name = "english"; break;
		case 1: language_name = "french"; break;
		case 2: language_name = "german"; break;
		case 3: language_name = "italian"; break;
		case 4: language_name = "spanish"; break;
		case 5: language_name = "british"; break;
		case 6: language_name = "russian"; break;
		case 7: language_name = "polish"; break;
		case 8: language_name = "korean"; break;
		case 9: language_name = "taiwanese"; break;
		case 10: language_name = "japanese"; break;
		case 11: language_name = "chinese"; break;
		case 12: language_name = "thai"; break;
		case 13: language_name = "leet"; break;
		case 14: language_name = "czech"; break;
		case 15: language_name = "portuguese"; break;
		case 16: language_name = "slovak"; break;
		default: language_name = "english"; break;
	}

	return language_name;
}

GetPrivateRankName()
{
	private_rank_name = "";
	if( GetLanguage() == "english" ) private_rank_name = "Pvt. ";
	else if( GetLanguage() == "french" ) private_rank_name = "Pvt. ";
	else if( GetLanguage() == "german" ) private_rank_name = "Pvt. ";
	else if( GetLanguage() == "italian" ) private_rank_name = "Soldato ";
	else if( GetLanguage() == "spanish" ) private_rank_name = "Soldado ";
	else if( GetLanguage() == "russian" ) private_rank_name = scripts\_names_ru::GetPrivateRank();
	else private_rank_name = "Pvt. ";
	return private_rank_name;
}

GetCorporalRankName()
{
	corporal_rank_name = "";
	if( GetLanguage() == "english" ) corporal_rank_name = "Cpl. ";
	else if( GetLanguage() == "french" ) corporal_rank_name = "Cpl. ";
	else if( GetLanguage() == "german" ) corporal_rank_name = "Cpl. ";
	else if( GetLanguage() == "italian" ) corporal_rank_name = "Caporale ";
	else if( GetLanguage() == "spanish" ) corporal_rank_name = "Cabo ";
	else if( GetLanguage() == "russian" ) corporal_rank_name = scripts\_names_ru::GetCorporalRank();
	else corporal_rank_name = "Cpl. ";
	return corporal_rank_name;
}

GetSergeantRankName()
{
	sergeant_rank_name = "";
	if( GetLanguage() == "english" ) sergeant_rank_name = "Sgt. ";
	else if( GetLanguage() == "french" ) sergeant_rank_name = "Sgt. ";
	else if( GetLanguage() == "german" ) sergeant_rank_name = "Sgt. ";
	else if( GetLanguage() == "italian" ) sergeant_rank_name = "Sergente ";
	else if( GetLanguage() == "spanish" ) sergeant_rank_name = "Sargento ";
	else if( GetLanguage() == "russian" ) sergeant_rank_name = scripts\_names_ru::GetSergeantRank();
	else sergeant_rank_name = "Sgt. ";
	return sergeant_rank_name;
}

GetLieutenantRankName()
{
	lieutenant_rank_name = "";
	if( GetLanguage() == "english" ) lieutenant_rank_name = "Lt. ";
	else if( GetLanguage() == "french" ) lieutenant_rank_name = "Lt. ";
	else if( GetLanguage() == "german" ) lieutenant_rank_name = "Lt. ";
	else if( GetLanguage() == "italian" ) lieutenant_rank_name = "Tenente ";
	else if( GetLanguage() == "spanish" ) lieutenant_rank_name = "Teniente ";
	else if( GetLanguage() == "russian" ) lieutenant_rank_name = scripts\_names_ru::GetLieutenantRank();
	else lieutenant_rank_name = "Lt. ";
	return lieutenant_rank_name;
}

GetCaptainRankName()
{
	captain_rank_name = "";
	if( GetLanguage() == "english" ) captain_rank_name = "Cpt. ";
	else if( GetLanguage() == "french" ) captain_rank_name = "Cpt. ";
	else if( GetLanguage() == "german" ) captain_rank_name = "Cpt. ";
	else if( GetLanguage() == "Cpt." ) captain_rank_name = "Capitano ";
	else if( GetLanguage() == "spanish" ) captain_rank_name = "Capitán ";
	else if( GetLanguage() == "russian" ) captain_rank_name = scripts\_names_ru::GetCaptainRank();
	else captain_rank_name = "Cpt. ";
	return captain_rank_name;
}

// from maps/_names.gsc
get_name_for_nationality( nationality )
{
	assertex( isdefined( level.nameIndex[ nationality ] ), nationality );

	level.nameIndex[ nationality ] = (level.nameIndex[ nationality ] + 1) % level.names[ nationality ].size;

	lastname = "";
	if( GetLanguage() == "russian" )
		lastname = scripts\_names_ru::GetTranslatedName(level.names[ nationality ][ level.nameIndex[ nationality ] ]);
	else
		lastname = level.names[ nationality ][ level.nameIndex[ nationality ] ];

	rank = randomint (10);
	if (rank > 5)
	{
		fullname = GetPrivateRankName() + lastname;
		self.airank = "private";
	}
	else if (rank > 2)
	{
		fullname = GetCorporalRankName() + lastname;
		self.airank = "corporal";
	}
	else
	{
		fullname = GetSergeantRankName() + lastname;
		self.airank = "sergeant";
	}

	if ( self.team == "axis" )
		self.ainame = fullname;
	else
		self.name = fullname;
}

getRankFromName( name )
{
	if (!isDefined( name ) )
		self.airank = "private";

	tokens = strtok( name, " " );
	assert ( tokens.size );
	shortRank = tokens[0];
	if( shortRank == GetPrivateRankName() || shortRank == "Pfc." )
		self.airank = "private";
	else if( shortRank == GetCorporalRankName() )
		self.airank = "corporal";
	else if( shortRank == GetSergeantRankName() )
		self.airank = "sergeant";
	else if( shortRank == GetLieutenantRankName() )
		self.airank = "lieutenant";
	else if( shortRank == GetCaptainRankName() || shortRank == "Captain" )
		self.airank = "captain";
	else 
	{
		println("sentient has invalid rank " + shortRank + "!");
		self.airank = "private";
	}
}


// MAIN CHARACTERS NAMES
HasCaptainPriceName( name ) { return name == "Captain Price"; }
HasGazName( name ) { return name == "Gaz"; }
HasSSgtGriggsName( name ) { return name == "SSgt. Griggs"; }
HasLtVasquezName( name ) { return name == "Lt. Vasquez"; }
HasCaptainMacMillanName( name ) { return name == "Cpt. MacMillan"; }
HasSgtKamarovName( name ) { return name == "Sgt. Kamarov"; }
HasNikolaiName( name ) { return name == "Nikolai"; }
HasViktorZakhaevName( name ) { return name == "V. Zakhaev"; }
// SECONDARY CHARACTERS NAMES
HasSgtWallcroftName( name ) { return name == "Sgt. Wallcroft"; }
HasPvtGriffenName( name ) { return name == "Pvt. Griffen"; }
HasCptPelayoName( name ) { return name == "Cpt. Pelayo"; }
HasCptKeatingName( name ) { return name == "Cpt. Keating"; }
HasLtVolkerName( name ) { return name == "Lt. Volker"; }
HasPvtMasseyName( name ) { return name == "Pvt. Massey"; }
HasPvtRoycewiczName( name ) { return name == "Pvt. Roycewicz"; }
HasPvtWestName( name ) { return name == "Pvt. West"; }
HasSgtScullyName( name ) { return name == "Sgt. Scully"; }
HasCW4SmithName( name ) { return name == "CW4. Smith"; }
HasSgtLovejoyName( name ) { return name == "Sgt. Lovejoy"; }
HasPvtLootzName( name ) { return name == "Pvt. Lootz"; }
HasMacName( name ) { return name == "Mac"; }
HasSgtNewcastleName( name ) { return name == "Sgt. Newcastle"; }
HasSgtAremName( name ) { return name == "Sgt. Arem"; }
HasSgtBartonName( name ) { return name == "Sgt. Barton"; }