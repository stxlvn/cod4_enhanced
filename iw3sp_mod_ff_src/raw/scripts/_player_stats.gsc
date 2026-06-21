#include maps\_utility;
#include common_scripts\utility;

/*
TODO:
-track headshots?
*/

init()
{
	ReplaceFunc(maps\_intelligence::give_points, ::give_points);
	ReplaceFunc(maps\_endmission::_nextmission, ::_nextmission);
	level.player check_the_intel_count();
	level.player init_stats();
}

init_stats()
{	
	enemies = getaiarray( "axis" );
	array_thread( enemies, ::register_stats_think );
	add_global_spawn_function( "axis", ::register_stats_think );

	// kill
	self.stats[ "kills" ] = 0;
	self.stats[ "kills_melee" ] = 0;
	self.stats[ "kills_explosives" ] = 0;
	//self.stats[ "kills_juggernaut" ] = 0;
	//self.stats[ "kills_vehicle" ] = 0;
	//self.stats[ "kills_sentry" ] = 0;
	self.stats[ "headshots" ] = 0;

    // accuracy
	self.stats[ "shots_fired" ] = 0;
	self.stats[ "shots_hit" ] = 0;

	// weapon stats
	//self.stats[ "weapon" ] = [];

    self thread shots_fired_recorder();
	self thread player_death_track();
}

check_the_intel_count()
{
	intel_found = getDvarInt( "cheat_points" );
	self career_stat_increment( "intel", intel_found );
}

register_stats_think()
{
	level.player thread register_shot_hit( self );
	level.player thread register_kill_stat( self );
}

was_headshot()
{
	// Special field set in Survival AI when damage was scaled
	// up enough on a headshot to force a kill
	if ( IsDefined( self.died_of_headshot ) && self.died_of_headshot )
		return true;
		
	if ( !IsDefined( self.damageLocation ) )
		return false;

	return( self.damageLocation == "helmet" || self.damageLocation == "head" || self.damageLocation == "neck" );
}

register_kill_stat( target )
{
	target waittill( "death", attacker, cause, weapon );
	if( !isdefined( target ) || !isdefined( attacker ) || attacker != level.player )
		return;

	attacker register_kill(target, cause, weapon);
}

register_kill( killedEnt, cause, weaponName, killtype )
{
	assertEx( isdefined( cause ), "Tried to register a player stat for a kill that didn't have a method of death" );

	// overall
	level.player.stats[ "kills" ]++;
	level.player career_stat_increment( "kills", 1 );
	
	// if ( is_specialop() )
	// {
	// 	// ( "specops_player_kill", attacker, victim, weapon )
	// 	level notify( "specops_player_kill", player, killedEnt, weaponName, killtype );
	// }
	
	if ( isdefined( killedEnt ) )
	{
		if(	killedEnt was_headshot() )
		{
			self.stats[ "headshots" ]++ ;
			self career_stat_increment( "headshots", 1 );
		}
			
		// if ( isdefined( killedEnt.juggernaut ) )
		// {
		// 	player.stats[ "kills_juggernaut" ]++ ;
		// 	player career_stat_increment( "kills_juggernaut", 1 );
		// }

		// if ( isdefined( killedEnt.isSentryGun ) )
		// 	player.stats[ "kills_sentry" ]++ ;

		//if ( killedEnt.classname == "script_vehicle" )
		//{
		//	self.stats[ "kills_vehicle" ]++;
		//	self career_stat_increment( "kills_vehicle", 1 );
		//	aRiders = killedEnt.riders;

		//	if ( isdefined( aRiders ) )
		//	{
		//		for ( i = 0; i < aRiders.size; i++ )
		//		{
		//			rider = aRiders[ i ];
		//			if(isDefined(rider))
		//				self register_kill( rider, cause, weaponName, killtype );
		//		}
		//	}
		//}
	}

	if ( cause_is_explosive( cause ) )
	{
		self.stats[ "kills_explosives" ]++;
		self career_stat_increment( "kills_explosives", 1 );
	}
		
	// adding kills done per weapon
	if ( !IsDefined( weaponName ) )
		weaponName = self getCurrentWeapon();

	if ( issubstr( tolower( cause ), "melee" ) )
	{
		self.stats[ "kills_melee" ]++;
		self career_stat_increment( "kills_melee", 1 );

		// return from adding kills as this was a primary and kill wasn't from a shot
		if ( weaponinventorytype( weaponName ) == "primary" )
			return;
	}

	// assert( isdefined( weaponName ) );
	// if ( player is_new_weapon( weaponName ) )
	// {
	// 	player register_new_weapon( weaponName );
	// }
	
	// player.stats[ "weapon" ][ weaponName ].kills++ ;
}

career_stat_increment( stat, delta )
{	
	new_stat = int(StatsGetFromStruct("career", stat)) + delta;
	StatsSetFromStruct("career", stat, new_stat);
}

register_shot_hit( target )
{
	if ( !isPlayer( self ) )
		return;
	assert( isdefined( self.stats ) );

	self endon("death");

	while( isDefined(target) && isAlive(target) )
	{
		target waittill( "damage", damage, attacker, direction_vec, point, type, modelName, tagName );
		if(attacker == level.player)
		{
			self.stats[ "shots_hit" ]++;
			self career_stat_increment( "bullets_hit", 1 );
		}
	}
}

shots_fired_recorder()
{
	self endon ("death");

	for ( ;; )
	{
		self waittill( "weapon_fired" );
		
		self.stats[ "shots_fired" ]++;
		self career_stat_increment( "bullets_fired", 1 );
	}
}

cause_is_explosive( cause )
{
	cause = tolower( cause );
	switch( cause )
	{
		case "mod_grenade":
		case "mod_grenade_splash":
		case "mod_projectile":
		case "mod_projectile_splash":
		case "mod_explosive":
		case "splash":
			return true;
		default:
			return false;
	}
	return false;
}

player_death_track()
{
	self waittill ("death");
	self career_stat_increment( "deaths", 1 );
}

give_points()
{
	curValue = getDvarInt( "cheat_points" );	
	curValue += 1;
	setDvar( "cheat_points", curValue );
	self career_stat_increment( "intel", 1 );
}

// recruit and regular difficulty
getStat_progression( difficulty )
{
	assert( isdefined( level.missionSettings ) );
	assert( isdefined( level.script ) );

	difficulty_string = getdvar("mis_difficulty");
	
	levels = 0;
	notplayed = [];
	skipped = false;
	// level.missionSettings.levels.size - 1 : the minus one is to remove the credits level
	for ( i = 0; i < level.missionSettings.levels.size-1; i++ )
	{
		if ( int( difficulty_string[ i ] ) >= difficulty )
			levels++;
	}

	completion = ( levels/(level.missionsettings.levels.size-1) )*100;
	return completion; // 0->100
}

getStat_easy()
{
	easy = 1;
	return getStat_progression( easy ); // 0->100	
}

getStat_regular()
{
	regular = 2;
	return getStat_progression( regular ); // 0->100	
}

getStat_hardened()
{
	hardened = 3;
	return getStat_progression( hardened ); // 0->100
}

getStat_veteran()
{
	veteran = 4;
	return getStat_progression( veteran ); // 0->100
}

getStat_intel()
{
	total_intel_items = 45;
	intel_percentage = ( int((StatsGetFromStruct("career", "intel")))/total_intel_items )*100;
	return intel_percentage; // 0->100
}

getTotalpercentCompleteSP()
{
	/*
	SP STATS:
	
	Game Progression  	60%    	-50
	Hardened Progress  	60%   	-25
	Veteran Progress  	60%    	-10
	Intel Items  		21/45   -15
	--------------------------------
	Total    			x%		-100
	Play Time			##:##:##
	*/
	
	stat_progression = max( getStat_easy(), getStat_regular() ); // easy is always higher than regular anyways...
	stat_progression_ratio = 0.5/1;
	/#
		PrintLn( ">> SP STAT REGULAR: " + stat_progression + "%" + "(" + stat_progression_ratio*100 + "%)" );
	#/
	
	stat_hardened = getStat_hardened();
	stat_hardened_ratio = 0.25/1;
	/#
		PrintLn( ">> SP STAT HARDENED: " + stat_hardened + "%" + "(" + stat_hardened_ratio*100 + "%)" );
	#/
	
	stat_veteran = getStat_veteran();
	stat_veteran_ratio = 0.1/1;
	/#
		PrintLn( ">> SP STAT VETERAN: " + stat_veteran + "%" + "(" + stat_veteran_ratio*100 + "%)" );
	#/
	
	stat_intel = getStat_intel();
	stat_intel_ratio = 0.15/1;
	/#
		PrintLn( ">> SP STAT INTEL: " + stat_intel + "%" + "(" + stat_intel_ratio*100 + "%)" );
	#/

	assertex( ( stat_progression_ratio + stat_hardened_ratio + stat_veteran_ratio + stat_intel_ratio ) <= 1.0, "Total sum of SP progress breakdown contributes to more than 100%!" );

	total_progress = 0.0;
	total_progress += stat_progression_ratio*stat_progression;
	total_progress += stat_hardened_ratio*stat_hardened;
	total_progress += stat_veteran_ratio*stat_veteran;
	//total_progress += stat_intel_ratio*stat_intel;

	assertex( total_progress <= 100.0, "Total Percentage calculation is out of bound, larger then 100%" );
	/#
		PrintLn( ">> SP STAT TOTAL: " + total_progress + "%" );
	#/
	
	return total_progress;	
}

updateSpPercent()
{
	completion_percentage = int( getTotalpercentCompleteSP()*100 );
	
	if( getdvarint( "mis_cheat" ) == 0 )
	{
		assertex( ( completion_percentage >= 0 && completion_percentage <= 10000 ), "SP's Completion percentage [ " + completion_percentage + "% ] is outside of 0 to 100 range!" );
		level.player career_stat_increment( "percentCompleteSP", completion_percentage );
	}
	
	return completion_percentage;
}

_nextmission()
{
	level.nextmission = true;
	level.player enableinvulnerability();	
	
	if ( arcadeMode() )
	{
		level.arcadeMode_success = true;
		thread maps\_arcadeMode::arcadeMode_ends();
		flag_wait( "arcademode_ending_complete" );
	}
	
	levelIndex = undefined;
	
	setsaveddvar( "ui_nextMission", "1" );
	setdvar( "ui_showPopup", "0" );
	setdvar( "ui_popupString", "" );
	
	maps\_gameskill::auto_adust_zone_complete( "aa_main_" + level.script );
	
	levelIndex = level.missionSettings maps\_endmission::getLevelIndex( level.script );
	
	if ( !isDefined( levelIndex ) )
		return;
		
	if( level.script != "jeepride" && level.script != "airplane" )
		maps\_utility::level_end_save();

	// update mission difficult dvar
	level.missionSettings maps\_endmission::setLevelCompleted( levelIndex );
	
//	if( !( level.missionSettings getLowestSkill() ) && !(level.script == "jeepride") )
	
	if( getdvarint("mis_01") < levelindex+1 && (level.script == "jeepride") && getdvarint("mis_cheat") == 0 )
	{
        setdvar( "ui_sp_unlock", "0" ); // set reset value to 0
        setdvar( "ui_sp_unlock", "1" );
    }
   
	if( getdvarint("mis_01") < levelindex+1 )// && !(level.script == "jeepride") )
		maps\_endmission::_setMissionDvar( "mis_01", levelIndex+1 );
		
	completion_percentage = updateSpPercent();

	UpdateGamerProfile();

	if ( level.missionSettings maps\_endmission::hasAchievement( levelIndex ) )
		maps\_utility::giveachievement_wrapper( level.missionSettings maps\_endmission::getAchievement( levelIndex ) );
		
	if ( level.missionSettings maps\_endmission::hasLevelVeteranAward( levelIndex ) && maps\_endmission::getLevelCompleted( levelIndex ) == 4
		 && level.missionSettings maps\_endmission::check_other_hasLevelVeteranAchievement( levelIndex ) )
		maps\_utility::giveachievement_wrapper( level.missionSettings maps\_endmission::getLevelVeteranAward( levelIndex ) );
	
	if ( level.missionSettings maps\_endmission::hasMissionHardenedAward() && 
		level.missionSettings maps\_endmission::getLowestSkill() > 2 )
		maps\_utility::giveachievement_wrapper( level.missionSettings maps\_endmission::getHardenedAward() );

	nextLevelIndex = level.missionSettings.levels.size;
	if ( level.script == "airplane" )
	{
		setsaveddvar( "ui_nextMission", "0" );
		//setdvar( "ui_victoryquote", "@VICTORYQUOTE_IW_THANKS_FOR_PLAYING" );
		missionSuccess("killhouse"); // for lack of better things to do..
		return;
	}
	else
	{
		nextLevelIndex = levelIndex + 1;
	}

	if ( arcadeMode() )
	{
		if ( !getdvarint( "arcademode_full" ) )
		{
			setsaveddvar( "ui_nextMission", "0" );
			missionSuccess( level.script );
			return;
		}

		if ( level.script == "cargoship" )
		{
			changelevel( "blackout", level.missionSettings maps\_endmission::getKeepWeapons( levelIndex ) );
			return;
		}
		else
		if ( level.script == "airlift" )
		{
			changelevel( "village_assault", level.missionSettings maps\_endmission::getKeepWeapons( levelIndex ) );
			return;
		}
		else
		if ( level.script == "jeepride" )
		{
			changelevel( "airplane", level.missionSettings maps\_endmission::getKeepWeapons( levelIndex ) );
			return;
		}
	}

	if ( level.script == "jeepride" )
	{
		setdvar( "credits_load", "1" ); 
		changelevel( "ac130", level.missionSettings maps\_endmission::getKeepWeapons( levelIndex ) );
		return;
	}
	
	if ( level.missionSettings maps\_endmission::skipssuccess( levelIndex ) )
		changelevel( level.missionSettings maps\_endmission::getLevelName( nextLevelIndex ), level.missionSettings maps\_endmission::getKeepWeapons( levelIndex ) );
	else
		missionSuccess( level.missionSettings maps\_endmission::getLevelName( nextLevelIndex ), level.missionSettings maps\_endmission::getKeepWeapons( levelIndex ) );
}