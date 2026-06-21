#include maps\_utility;
#include common_scripts\utility;

init()
{
	precacheShader( "h1_damage_feedback" );
	precacheShader( "h1_damage_feedback_headshot" );

	level.damagefeedback_funcs = [];
	AddDamageFunction(::monitorDamage);
	AddDamageFunction(::so_ai_flashed_damage_feedback);

	enemies = getaiarray( "axis" );
	array_thread( enemies, ::DamageFeedbackFunctions );
	add_global_spawn_function( "axis", ::DamageFeedbackFunctions );

	level.player thread init_damage_feedback();
}

init_damage_feedback()
{
	self.hud_damagefeedback = newHudElem( self );
	self.hud_damagefeedback.alignX = "center";
	self.hud_damagefeedback.alignY = "middle";
	self.hud_damagefeedback.horzAlign = "center";
	self.hud_damagefeedback.vertAlign = "middle";
	self.hud_damagefeedback.alpha = 0;
	self.hud_damagefeedback.archived = true;
	self.hud_damagefeedback setShader( "h1_damage_feedback", 50, 50 * 2 );
	self.hud_damagefeedback.y = 25;	// aligns it to the center of the crosshair.

	self.hud_damagefeedback_headshot = newHudElem( self );
	self.hud_damagefeedback_headshot.alignX = "center";
	self.hud_damagefeedback_headshot.alignY = "middle";
	self.hud_damagefeedback_headshot.horzAlign = "center";
	self.hud_damagefeedback_headshot.vertAlign = "middle";
	self.hud_damagefeedback_headshot.alpha = 0;
	self.hud_damagefeedback_headshot.archived = true;
	self.hud_damagefeedback_headshot setShader( "h1_damage_feedback_headshot", 50, 50 * 2 );
	self.hud_damagefeedback_headshot.color = ( 1, 0.2, 0.2 );
	self.hud_damagefeedback_headshot.y = 25;	// aligns it to the center of the crosshair.
}

DamageFeedbackFunctions()
{
	for ( i = 0; i < level.damagefeedback_funcs.size; i++ )
	{
		func = level.damagefeedback_funcs[ i ];
		self thread [[ func[ "function" ] ]]();
	}
}

monitorDamage()
{
	level.player endon("death");

	while( isDefined( self ) && isAlive( self ) )
	{
		self waittill( "damage", damage, attacker, direction_vec, point, type, modelName, tagName );
		self damagefeedback_took_damage( damage, attacker, direction_vec, point, type, modelName, tagName );
	}
}

// from mw3 scripts
so_ai_flashed_damage_feedback()
{
	level.player endon( "death" );
	
	while( isDefined( self ) && isAlive( self ) )
	{
		// <origin> exists in the game function, but doesn't have Scr_AddVector return :<
		// https://discord.com/channels/1091304681822752778/1091317154420375562/1248003248225259622
		self waittill( "flashbang", /*flash_origin,*/ flash_dist, flash_angle, attacker, team );
		attacker updateDamageFeedback( self, false, false );
	}
}

damagefeedback_took_damage( damage, attacker, direction_vec, point, type, modelName, tagName )
{
	if ( !isPlayer( attacker ) )
		return;
	legal_bullet_types = [];
	legal_bullet_types[ "MOD_PISTOL_BULLET" ] = true;
	legal_bullet_types[ "MOD_RIFLE_BULLET" ] = true;
	if ( isDefined( self.bullet_resistance ) )
	{
		if ( isDefined( legal_bullet_types[ type ] ) )
		{
			if ( damage <= self.bullet_resistance )
				return;
		}
	}
	if ( !isAlive( self ) )
	{
		headshot = false;
		if ( ( self.damageLocation == "helmet" || self.damageLocation == "head" ) && isDefined( legal_bullet_types[ type ] ) )
			headshot = true;

		attacker updateDamageFeedback( self, true, headshot );
	}
	else
	{
		attacker updateDamageFeedback( self, false, false );
	}
}

updateDamageFeedback( attacked, kill, headshot )
{
	if ( !isPlayer( self ) )
		return;
	if ( !isDefined( attacked.team ) )
		return;
	if ( ( attacked.team == "allies" ) || ( attacked.team == "neutral" ) )
		return;
	/*
    if ( isDefined( attacked.magic_bullet_shield ) && attacked.magic_bullet_shield )
        return;
    if ( isDefined( attacked.godmode ) && attacked.godmode )
        return;
    if ( isDefined( attacked.script_godmode ) && attacked.script_godmode )
        return;
	*/
	self updateDamageFeedbackSnd();
    self updateDamageFeedbackHUD( attacked, kill, headshot );
}

updateDamageFeedbackSnd()
{
	if( !SoundHitEffectHasEnabled() )
		return;

	if ( !isPlayer( self ) )
		return;

	if ( soundExists( "SP_hit_alert_npc" ) )
		self playLocalSound( "SP_hit_alert_npc" );
}

updateDamageFeedbackHUD( attacked, kill, headshot )
{
	if( !HitMarkerHasEnabled() )
		return;

	if ( !isPlayer( self ) )
		return;

	fadeTime = 1;	//fade out crosshair damage indicator over this time

	//If in slomo, fade out damage indicator faster (the value entered for the slomo time fraction
    if ( isDefined( level.slowmo.speed_slow ) )
        fadeTime = level.slowmo.speed_slow;
	
	if ( kill )
		self.hud_damagefeedback.color = ( 1, 0.2, 0.2 );
	else
		self.hud_damagefeedback.color = ( 1, 1, 1 );
	
	if ( kill && headshot )
		self.hud_damagefeedback_headshot.alpha = 1;
	else
		self.hud_damagefeedback.alpha = 1;

	if ( kill && headshot )
		self.hud_damagefeedback_headshot fadeOverTime( fadeTime );
	else
		self.hud_damagefeedback fadeOverTime( fadeTime );

	if ( kill && headshot )
		self.hud_damagefeedback_headshot.alpha = 0;
	else
		self.hud_damagefeedback.alpha = 0;

	offset = 0 * 240;
	self.hud_damagefeedback.y = 25 - int( offset );
	self.hud_damagefeedback_headshot.y = 25 - int( offset );
}

SoundHitEffectHasEnabled()
{
	return getDvarInt( "snd_hitsoundDisabled" ) == 0;
}

HitMarkerHasEnabled()
{
	return getDvarInt( "cg_drawDamageFeedbackOption" ) == 1;
}

AddDamageFunction( function )
{
	func = [];
	func[ "function" ] = function;

	level.damagefeedback_funcs[ level.damagefeedback_funcs.size ] = func;
}

RemoveDamageFunction( function )
{
	new_damagefeedback_funcs = [];

	for ( i = 0; i < level.damagefeedback_funcs.size; i++ )
	{
		func_array = level.damagefeedback_funcs[ i ];
		if ( func_array[ "function" ] == function )
			continue;

		new_damagefeedback_funcs[ new_damagefeedback_funcs.size ] = func_array;
	}

	level.damagefeedback_funcs = new_damagefeedback_funcs;
}

GetDamageFunctionSize()
{
	return level.damagefeedback_funcs.size;
}