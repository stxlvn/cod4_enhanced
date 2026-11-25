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
		level.players[ i ] thread body_follow( i );
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
