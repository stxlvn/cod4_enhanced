// CoD4 co-op framework (started from maps\_load when the level has two players).
// Phase 3: player bodies. Single player draws no other players (the SP cgame skips ET_PLAYER), so every player gets a body:
// a script model dressed like a friendly soldier of this level, following its player every server frame with AI animations.
// It is part of the simulation (both PCs run this script identically); the native side leaves a PC's own player's body
// out of that PC's view (dvars coop_body0 / coop_body1 = entity numbers).
#include maps\_utility;
#using_animtree( "generic_human" );

main()
{
	level.coop_bodies = [];
	for ( i = 0; i < level.players.size; i++ )
	{
		level.players[ i ].coop_index = i;
		level.players[ i ] thread body_follow( i );
		level.players[ i ] thread downed_watch();
	}
	thread loadout_mirror();
}

// ---- Phase 4: start of level ----

// maps\_loadout gives the level's start kit to level.player only: the other players get the same weapons and ammo.
loadout_mirror()
{
	wait 0.05; // after _load's give_loadout
	p1 = level.player;
	list = p1 getweaponslist();
	current = p1 getcurrentweapon();
	for ( i = 1; i < level.players.size; i++ )
	{
		p = level.players[ i ];
		// _load sets maxhealth on level.player only (level.player.maxhealth = level.player.health); 0 here broke the revive
		if ( p.maxhealth <= 0 )
			p.maxhealth = level.player.maxhealth;
		p takeallweapons();
		for ( k = 0; k < list.size; k++ )
		{
			p giveweapon( list[ k ] );
			p setweaponammoclip( list[ k ], p1 getweaponammoclip( list[ k ] ) );
			p setweaponammostock( list[ k ], p1 getweaponammostock( list[ k ] ) );
		}
		p setoffhandsecondaryclass( "flash" );
		if ( isdefined( current ) && current != "none" )
			p switchtoweapon( current );
	}
}

// ---- Phase 4: downed and revive (Spec Ops style) ----
// The native player die handler keeps a player alive at 1 health while his partner is up (coopdowned). Down: prone, no weapons,
// ignored by the AI, invulnerable; the partner revives him by holding Use next to him for 3 s; nobody in 45 s, or both down:
// mission failed. The dvars coop_downtime<n> / coop_revive<n> feed each PC's on-screen text (native overlay).
downed_watch()
{
	while ( true )
	{
		if ( self coopdowned() && !isdefined( self.coop_isdown ) )
			self downed();
		wait 0.05;
	}
}

downed()
{
	self.coop_isdown = true;
	i = self.coop_index;
	partner = level.players[ 1 - i ];
	self enableinvulnerability();
	self disableweapons();
	self allowstand( false );
	self allowcrouch( false );
	self allowprone( true );
	self setstance( "prone" );
	self.ignoreme = true;

	left = 45 * 20; // server frames
	held = 0;
	revived = false;
	quote = "A downed player was not revived in time.";
	while ( left > 0 )
	{
		if ( isdefined( partner.coop_isdown ) || partner.health <= 0 )
		{
			quote = "Both players went down.";
			break;
		}
		if ( partner usebuttonpressed() && distance( partner.origin, self.origin ) < 80 )
			held++;
		else
			held = 0;
		setdvar( "coop_downtime" + i, int( ( left + 19 ) / 20 ) );
		setdvar( "coop_revive" + i, int( held * 100 / 60 ) );
		if ( held >= 60 )
		{
			revived = true;
			break;
		}
		left--;
		wait 0.05;
	}
	setdvar( "coop_downtime" + i, 0 );
	setdvar( "coop_revive" + i, 0 );
	if ( !revived )
	{
		setdvar( "ui_deadquote", quote ); // the fail screen's line (the same text on both PCs)
		maps\_utility::missionFailedWrapper();
		return;
	}
	self coopsetdowned( 0 );
	self.coop_isdown = undefined;
	full = self.maxhealth;
	if ( full <= 0 )
		full = 100;
	self.health = int( full / 2 );
	self.ignoreme = false;
	self allowstand( true );
	self allowcrouch( true );
	self enableweapons();
	self setstance( "crouch" );
	self disableinvulnerability();
}

body_follow( index )
{
	// the look of a friendly soldier of this level (first ally there is)
	guy = undefined;
	while ( true )
	{
		allies = getaiarray( "allies" );
		if ( allies.size )
		{
			guy = allies[ 0 ];
			break;
		}
		wait 1;
	}

	body = spawn( "script_model", self.origin );
	body setmodel( guy.model );
	// the anim tree before any attach (as the game's drones: setmodel, UseAnimTree, attach): attach rebuilds the model with the tree;
	// with the tree set after the attachments the body stayed in its bind pose
	body useAnimTree( #animtree );
	count = guy getattachsize();
	for ( k = 0; k < count; k++ )
	{
		tag = guy getattachtagname( k );
		model = guy getattachmodelname( k );
		if ( tag == "tag_weapon_right" || tag == "tag_weapon_left" || tag == "tag_inhand" )
			continue; // the AI's weapon: the body carries its player's weapon instead
		body attach( model, tag );
	}
	body notsolid();
	level.coop_bodies[ index ] = body;
	setdvar( "coop_body" + index, body getentitynumber() );

	key = "";
	gun = "";
	last = self.origin;
	while ( true )
	{
		pos = self.origin;
		speed = distance( pos, last ) * 20; // units per second (server frames are 50 ms)
		last = pos;
		view = self getplayerangles();
		body.origin = pos;
		body.angles = ( 0, view[ 1 ], 0 );

		// the player's weapon in the body's hand
		weapon = self getcurrentweapon();
		want_gun = "";
		if ( isdefined( weapon ) && weapon != "none" && self.health > 0 )
			want_gun = getweaponmodel( weapon );
		if ( want_gun != gun )
		{
			if ( gun != "" )
				body detach( gun, "tag_weapon_right" );
			if ( want_gun != "" )
				body attach( want_gun, "tag_weapon_right" );
			gun = want_gun;
		}

		want = body_anim_key( self, speed );
		if ( want != key )
		{
			key = want;
			body setanimknoball( body_anim( key ), %root, 1, 0.2, 1 ); // the whole branch up to the root gets weight 1
		}
		wait 0.05;
	}
}

body_anim_key( player, speed )
{
	if ( player.health <= 0 )
		return "death";
	if ( isdefined( player.coop_isdown ) )
		return "prone";
	stance = player getstance();
	if ( stance == "prone" )
	{
		if ( speed > 20 )
			return "prone_move";
		return "prone";
	}
	if ( stance == "crouch" )
	{
		if ( speed > 20 )
			return "crouch_move";
		return "crouch";
	}
	if ( speed > 150 )
		return "run";
	if ( speed > 20 )
		return "walk";
	return "stand";
}

body_anim( key )
{
	switch ( key )
	{
		case "death":
			return %exposed_crouch_death_twist;
		case "prone_move":
			return %prone_crawl;
		case "prone":
			return %prone_aim_5;
		case "crouch_move":
			return %crouch_fastwalk_F;
		case "crouch":
			return %exposed_crouch_aim_5;
		case "run":
			return %run_lowready_F;
		case "walk":
			return %walk_lowready_F;
	}
	return %exposed_aim_5; // base poses: the idle_alert anims are overlays (alone they left the bind pose)
}
