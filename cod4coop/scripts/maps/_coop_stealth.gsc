// CoD4 co-op: the stealth system (maps\_stealth_logic / _stealth_behavior: Blackout, All Ghillied Up, Ultimatum) for both players.
// The levels start it on level.player only (stealth_ai): player 2 kept the code's default sight distance (seen from anywhere) and
// had no _stealth struct, which the enemy logic reads when he is its enemy. Now every player runs it with his own stance and
// movement (friendly_getstance_player read level.player's stance), so the enemies see each one as far as his own stance, speed,
// shadow and the level's detection ranges allow, the same rules as the host. The level's per-sequence modifiers (movement
// scale, movement detection cap) apply to both; kills, corpses and spotted reactions treat either player as "the player".
// Started by text patches of those scripts (CoopScripts.cpp). All of it is simulation (both PCs alike).
#include maps\_utility;
#include common_scripts\utility;

coop_list()
{
	if ( isdefined( level.players ) )
		return level.players;
	list = [];
	list[ 0 ] = level.player;
	return list;
}

coop_nearest( org )
{
	list = coop_list();
	best = list[ 0 ];
	for ( i = 1; i < list.size; i++ )
	{
		if ( isalive( list[ i ] ) && distance( list[ i ].origin, org ) < distance( best.origin, org ) )
			best = list[ i ];
	}
	return best;
}

// "level.player thread stealth_ai();" of the levels
coop_stealth_players()
{
	list = coop_list();
	for ( i = 0; i < list.size; i++ )
		list[ i ] thread maps\_stealth_logic::stealth_ai();
	setdvar( "coop_stealth_players", list.size ); // QA trace
	if ( list.size > 1 )
		thread coop_qa_trace();
}

// "level.player stealth_friendly_movespeed_scale_set( a, b )" / "_default()": every player
coop_movespeed_scale_set( hidden, alert, spotted )
{
	list = coop_list();
	for ( i = 0; i < list.size; i++ )
		list[ i ] maps\_stealth_logic::stealth_friendly_movespeed_scale_set( hidden, alert, spotted );
}

coop_movespeed_scale_default()
{
	list = coop_list();
	for ( i = 0; i < list.size; i++ )
		list[ i ] maps\_stealth_logic::stealth_friendly_movespeed_scale_default();
}

// "level.player._stealth_move_detection_cap = x;": every player
coop_detection_cap( cap )
{
	list = coop_list();
	for ( i = 0; i < list.size; i++ )
		list[ i ]._stealth_move_detection_cap = cap;
}

// scoutsniper graveyard_hind_spot_enemy for both players: the Hind spots a player moving in the open within 1300 for 0.5 s, or
// one in its line of sight closer than his stealth sight distance + 3500 (it sees better through camo from above); the church
// trigger hides a player
coop_hind_spot_enemy()
{
	self endon( "death" );
	if ( flag( "_stealth_spotted" ) )
		return;
	level endon( "_stealth_spotted" );
	trig = getent( "graveyard_inside_church_trig", "targetname" );
	movetime = [];
	for ( i = 0; i < level.players.size; i++ )
		movetime[ i ] = 0;
	interval = .05;
	while ( true )
	{
		wait interval;
		spotted = false;
		who = undefined;
		for ( i = 0; i < level.players.size && !spotted; i++ )
		{
			p = level.players[ i ];
			if ( p istouching( trig ) || !isdefined( p._stealth ) )
				continue;
			origin1 = p.origin;
			origin2 = ( self.origin[ 0 ], self.origin[ 1 ], origin1[ 2 ] );
			velocity = length( p getVelocity() );
			check1 = ( velocity > 10 && p._stealth.logic.stance == "crouch" );
			check2 = ( velocity > 25 && p._stealth.logic.stance == "prone" );
			if ( distance( origin1, origin2 ) < 1300 && ( check1 || check2 ) )
			{
				movetime[ i ] += interval;
				if ( movetime[ i ] < .5 )
					continue;
				if ( !coop_hind_sees( p ) )
					continue;
			}
			else if ( p._stealth.logic.stance == "prone" || p._stealth.logic.stance == "crouch" )
			{
				movetime[ i ] = 0;
				continue;
			}
			else
			{
				movetime[ i ] = 0;
				if ( distance( self.origin, p.origin ) > p.maxvisibledist + 3500 )
					continue;
				if ( !coop_hind_sees( p ) )
					continue;
			}
			spotted = true;
			who = p;
			setdvar( "coop_stealth_hind", p.coop_index ); // QA trace
		}
		if ( !spotted )
			continue;
		self notify( "enemy" );
		for ( i = 0; i < movetime.size; i++ )
			movetime[ i ] = 0;
		// a moment to hide, as the game gives (longer the farther the spotted player)
		wait .5 + distance( self.origin, who.origin ) * .0005;
	}
}

coop_hind_sees( p )
{
	trace = bullettrace( self.origin + ( 0, 0, -128 ), p.origin, true, level.price );
	return isdefined( trace[ "entity" ] ) && trace[ "entity" ] == p;
}

// QA: each player's stealth sight distance (maxVisibleDist) and stance, "<p1 dist> <p1 stance> / <p2 dist> <p2 stance>"
coop_qa_trace()
{
	while ( true )
	{
		wait 0.5;
		t = "";
		for ( i = 0; i < level.players.size; i++ )
		{
			p = level.players[ i ];
			if ( !isdefined( p._stealth ) )
				t += "none / ";
			else
				t += int( p.maxvisibledist ) + " " + p._stealth.logic.stance + " / ";
		}
		setdvar( "coop_stealth_vis", t );
	}
}
