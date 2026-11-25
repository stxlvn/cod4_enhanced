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
		level.players[ i ] thread fire_watch();
		level.players[ i ] thread qa_damage_watch();
	}
	for ( i = 1; i < level.players.size; i++ )
	{
		level.players[ i ] thread player_regen();
		level.players[ i ] thread weapon_watch();
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
	// the weapon in player 1's hands: _loadout's switchToWeapon has only been sent to his client yet ("none" for a few frames)
	current = p1 getcurrentweapon();
	for ( t = 0; t < 60 && current == "none" && list.size; t++ )
	{
		wait 0.05;
		current = p1 getcurrentweapon();
	}
	for ( i = 1; i < level.players.size; i++ )
	{
		p = level.players[ i ];
		// _load sets maxhealth on level.player only (level.player.maxhealth = level.player.health); 0 here broke the revive
		if ( p.maxhealth <= 0 )
			p.maxhealth = level.player.maxhealth;
		p takeallweapons();
		for ( k = 0; k < list.size; k++ )
			p giveweapon( list[ k ] );
		// ammo after all the weapons: giving an alt mode (gl_m4_grenadier) refills its base weapon's stock
		for ( k = 0; k < list.size; k++ )
		{
			p setweaponammoclip( list[ k ], p1 getweaponammoclip( list[ k ] ) );
			p setweaponammostock( list[ k ], p1 getweaponammostock( list[ k ] ) );
		}
		p setoffhandsecondaryclass( "flash" );
		if ( isdefined( current ) && current != "none" )
			p switchtoweapon( current );
	}
	// QA: the same kit (weapons, clip and stock ammo) on every player
	bad = 0;
	detail = "";
	for ( i = 1; i < level.players.size; i++ )
	{
		for ( k = 0; k < list.size; k++ )
		{
			w = list[ k ];
			if ( !level.players[ i ] hasweapon( w ) || level.players[ i ] getweaponammoclip( w ) != p1 getweaponammoclip( w ) || level.players[ i ] getweaponammostock( w ) != p1 getweaponammostock( w ) )
			{
				bad++;
				detail = w + " " + p1 getweaponammoclip( w ) + "/" + p1 getweaponammostock( w ) + " vs " + level.players[ i ] getweaponammoclip( w ) + "/" + level.players[ i ] getweaponammostock( w );
			}
		}
	}
	setdvar( "coop_qa_kit", list.size + " weapons, " + bad + " differ (" + detail + "), in hand " + current );
}

// ---- Phase 4: a weapon in the other players' hands ----
// The weapon in hand is the client's choice: switchToWeapon only sends "sw" to the player's client. After a loaded save the
// partner's client has none selected (its state is not in the save), and a level script may give weapons to level.player only.
// So once a second: a player who stands with nothing in his hands but has primaries takes the first one.
weapon_watch()
{
	while ( true )
	{
		wait 1;
		if ( isdefined( self.coop_isdown ) || self.health <= 0 )
			continue;
		if ( self getcurrentweapon() != "none" )
			continue;
		list = self getweaponslistprimaries();
		if ( list.size )
			self switchtoweapon( list[ 0 ] );
	}
}

// ---- Phase 4: health regen of the other players ----
// maps\_gameskill::playerHealthRegen runs for level.player only (checked: player 2 stayed at 50 health, player 1 went back to
// 100 in 8 s). The same rules for the others: full health after playerHealth_RegularRegenDelay when not very hurt, slow regen
// (0.1 a frame after longRegenTime) when very hurt, and the difficulty's short protection after a big enough hit
// (playerInvul: attackerAccuracy 0 + ignoreRandomBulletDamage). Not ported: the red overlay, blur and breathing, which are
// level.player's script HUD and sounds (CoD4 SP has no per-client HUD elements: both PCs would show them).
player_regen()
{
	while ( !isdefined( level.playerHealth_RegularRegenDelay ) || !isdefined( level.healthOverlayCutoff ) )
		wait 0.05;
	self thread maps\_gameskill::playerHurtcheck(); // self.hurtAgain on damage
	regenRate = 0.1;
	oldratio = 1;
	lastinvulratio = 1;
	veryHurt = false;
	hurtTime = 0;
	for ( ;; )
	{
		wait 0.05;
		waittillframeend;
		if ( isdefined( self.coop_isdown ) )
		{
			oldratio = 1;
			lastinvulratio = 1;
			veryHurt = false;
			continue;
		}
		if ( self.health >= self.maxhealth )
		{
			lastinvulratio = 1;
			veryHurt = false;
			continue;
		}
		if ( self.health <= 0 )
			return;

		wasVeryHurt = veryHurt;
		ratio = self.health / self.maxhealth;
		if ( ratio <= level.healthOverlayCutoff )
		{
			veryHurt = true;
			if ( !wasVeryHurt )
				hurtTime = gettime();
		}
		if ( self.hurtAgain )
		{
			hurtTime = gettime();
			self.hurtAgain = false;
		}

		if ( ratio >= oldratio )
		{
			if ( gettime() - hurtTime < level.playerHealth_RegularRegenDelay )
				continue;
			if ( veryHurt )
			{
				newHealth = ratio;
				if ( gettime() > hurtTime + level.longRegenTime )
					newHealth += regenRate;
			}
			else
				newHealth = 1;
			if ( newHealth > 1.0 )
				newHealth = 1.0;
			if ( newHealth <= 0 )
				return;
			self setnormalhealth( newHealth );
			oldratio = self.health / self.maxhealth;
			continue;
		}

		oldratio = lastinvulratio;
		invulWorthyHealthDrop = oldratio - ratio >= level.worthyDamageRatio;
		if ( self.health <= 1 )
		{
			self setnormalhealth( 2 / self.maxhealth );
			invulWorthyHealthDrop = true;
		}
		oldratio = self.health / self.maxhealth;
		hurtTime = gettime();
		if ( !invulWorthyHealthDrop || getdvarfloat( "scr_playerInvulTimeScale" ) <= 0.0 )
			continue;
		if ( isdefined( self.coop_invul ) )
			continue;
		self.coop_invul = true;
		if ( veryHurt && !wasVeryHurt )
			invulTime = level.invulTime_onShield;
		else if ( veryHurt )
			invulTime = level.invulTime_postShield;
		else
			invulTime = level.invulTime_preShield;
		invulTime *= getdvarfloat( "scr_playerInvulTimeScale" );
		lastinvulratio = self.health / self.maxhealth;
		self thread player_invul( invulTime );
	}
}

// maps\_gameskill::playerInvul for this player
player_invul( timer )
{
	if ( timer > 0 )
	{
		self.attackerAccuracy = 0;
		self.ignoreRandomBulletDamage = true;
		wait timer;
	}
	self.attackerAccuracy = anim.player_attacker_accuracy;
	self.ignoreRandomBulletDamage = false;
	self.coop_invul = undefined;
}

// ---- Phase 4: downed and revive = IW4 Spec Ops last stand (MW2 maps\_coop.gsc) ----
// Native side (Coop.cpp): a player whose partner is up does not die, he is marked down in his player state (coopdowned) and is
// kept alive while down (death shield); the marked player moves like IW4 last stand: eye height 22, crawling at 0.2 of the speed
// (coop_crawlscale), crouched bounds. Script side, as MW2 player_coop_set_down_attributes / _part1 / _part2:
// down -> invulnerable, ignored by the AI, weapons off; 0.3 s later his pistol (when he has one), 0.25 s later vulnerable again,
// the AI sees him again once he fires or after 4 s; the next hit puts the pistol away, shell shock, invulnerable and ignored.
// The partner revives him by holding Use within 128 units for 1.5 s (both frozen, the bleed-out timer paused) -> full health.
// Bleed-out 120 / 120 / 90 / 60 s by difficulty; bleed-out or both down -> mission failed. The dvars coop_downtime<n> /
// coop_revive<n> feed each PC's on-screen text (native overlay).
downed_watch()
{
	while ( true )
	{
		if ( self coopdowned() && !isdefined( self.coop_isdown ) )
			self downed();
		wait 0.05;
	}
}

fire_watch()
{
	while ( true )
	{
		self waittill( "weapon_fired" );
		self.coop_lastfire = gettime();
	}
}

bleedout_time()
{
	switch ( level.gameskill )
	{
		case 2:
			return 90;
		case 3:
			return 60;
	}
	return 120;
}

downed()
{
	self.coop_isdown = true;
	self.coop_down_from = self getstance();
	self.coop_preincap_weapon = self getcurrentweapon(); // before the weapons go off (MW2 takes it in part 1)
	i = self.coop_index;
	partner = level.players[ 1 - i ];

	// player_coop_set_down_attributes (setting a player's health changes his maxhealth too: MW2 puts the original back)
	if ( !isdefined( self.coop_maxhealth ) )
		self.coop_maxhealth = self.maxhealth;
	if ( self.coop_maxhealth <= 0 )
		self.coop_maxhealth = 100;
	self enableinvulnerability();
	self.health = 2;
	self.maxhealth = self.coop_maxhealth;
	self.ignoreme = true;
	self.ignoreRandomBulletDamage = true;
	self disableweapons();
	self allowstand( false );
	self allowprone( false );
	self allowcrouch( true );
	self allowjump( false );
	self allowsprint( false );
	self setstance( "crouch" );
	self thread down_part1();

	left = bleedout_time() * 20; // server frames
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
		// Use within 128 units (the native side takes it out of the partner's usercmd: coopreviveuse; usebuttonpressed covers a
		// player whose input does not go through it, player 1 in one process)
		if ( ( partner coopreviveuse() || partner usebuttonpressed() ) && distance( partner.origin, self.origin ) < 128 )
		{
			if ( !held )
				self revive_freeze( partner, true );
			held++;
		}
		else
		{
			if ( held )
				self revive_freeze( partner, false );
			held = 0;
			left--; // the bleed-out timer pauses while the partner revives (coop_pause_bleedout_timer)
		}
		setdvar( "coop_downtime" + i, int( ( left + 19 ) / 20 ) );
		setdvar( "coop_revive" + i, int( held * 100 / 30 ) );
		if ( held >= 30 ) // 1.5 s (player_coop_revive_buddy totalTime)
		{
			revived = true;
			break;
		}
		wait 0.05;
	}
	setdvar( "coop_downtime" + i, 0 );
	setdvar( "coop_revive" + i, 0 );
	if ( held )
		self revive_freeze( partner, false );
	if ( !revived )
	{
		self.coop_bled_out = true; // his body plays its death
		setdvar( "ui_deadquote", quote ); // the fail screen's line (the same text on both PCs)
		maps\_utility::missionFailedWrapper();
		return;
	}
	self notify( "coop_revived" );
	self get_up();
}

// player_coop_freeze_players (self = the downed player)
revive_freeze( partner, frozen )
{
	partner freezecontrols( frozen );
	self freezecontrols( frozen );
	if ( frozen )
	{
		partner disableweapons();
		self disableweapons();
		return;
	}
	partner enableweapons();
	if ( isdefined( self.coop_down_armed ) && !isdefined( self.coop_down_part2 ) )
		self enableweapons();
}

// player_coop_set_down_part1 / _part2
down_part1()
{
	self endon( "coop_revived" );
	wait 0.3;
	// player_coop_force_switch_to_pistol (MW2 gives a Beretta when he has none; CoD4 can only give weapons the level loaded)
	pistol = undefined;
	list = self getweaponslistprimaries();
	for ( k = 0; k < list.size; k++ )
	{
		if ( weaponclass( list[ k ] ) == "pistol" )
			pistol = list[ k ];
	}
	if ( isdefined( pistol ) )
	{
		self switchtoweapon( pistol );
		self.coop_down_armed = true;
		self enableweapons();
	}
	wait 0.25;
	self disableinvulnerability();
	self thread down_draw_attention();

	self waittill( "damage" );
	self.coop_down_part2 = true;
	self disableweapons();
	self thread down_dying_effect();
	self.ignoreme = true;
	self.ignoreRandomBulletDamage = true;
	self enableinvulnerability();
}

// player_coop_down_draw_attention: the AI ignores him until he fires, or 4 s
down_draw_attention()
{
	self endon( "coop_revived" );
	self endon( "damage" );
	self waittill_notify_or_timeout( "weapon_fired", 4 );
	self.ignoreme = false;
	self.ignoreRandomBulletDamage = false;
}

// player_dying_effect
down_dying_effect()
{
	self endon( "coop_revived" );
	while ( true )
	{
		self shellshock( "default", 60 );
		wait 60;
	}
}

// player_coop_set_original_attributes + player_coop_getup
get_up()
{
	self coopsetdowned( 0 ); // native: normal movement again
	self.coop_isdown = undefined;
	self.coop_down_armed = undefined;
	self.coop_down_part2 = undefined;
	self stopshellshock();
	self disableweapons();
	if ( isdefined( self.coop_preincap_weapon ) && self.coop_preincap_weapon != "none" && self hasweapon( self.coop_preincap_weapon ) )
		self switchtoweapon( self.coop_preincap_weapon );
	self.coop_preincap_weapon = undefined;
	self.maxhealth = self.coop_maxhealth;
	self.health = self.coop_maxhealth;
	self.ignoreme = false;
	self.ignoreRandomBulletDamage = false;
	self allowstand( true );
	self allowprone( true );
	self allowjump( true );
	self allowsprint( true );
	self setstance( "stand" );
	self enableweapons();
	wait 1.0;
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
	fall_end = undefined;
	fired = 0;
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

		if ( isdefined( self.coop_isdown ) )
		{
			// last stand: the CoD4 AI's wounded-on-the-back set (animscripts\pain.gsc dying crawl) for IW4's pb_laststand_*:
			// fall from his stance onto the back, then idle / crawl / fire with the pistol, death when he bleeds out
			if ( !isdefined( fall_end ) )
			{
				fall = body_fall_anim( self.coop_down_from );
				body setanimknoballrestart( fall, %root, 1, 0.2, 1 );
				key = "down_fall";
				fall_end = gettime() + int( getanimlength( fall ) * 1000 );
			}
			else if ( gettime() >= fall_end )
			{
				want = "down_idle";
				if ( isdefined( self.coop_bled_out ) )
					want = "down_death";
				else if ( isdefined( self.coop_lastfire ) && gettime() - self.coop_lastfire < 400 )
					want = "down_fire";
				else if ( speed > 5 )
					want = "down_crawl";
				if ( want != key || ( want == "down_fire" && self.coop_lastfire != fired ) )
				{
					key = want;
					if ( want == "down_fire" )
						fired = self.coop_lastfire;
					body setanimknoballrestart( body_anim( key ), %root, 1, 0.2, 1 );
				}
			}
		}
		else
		{
			fall_end = undefined;
			want = body_anim_key( self, speed );
			if ( want != key )
			{
				key = want;
				body setanimknoball( body_anim( key ), %root, 1, 0.2, 1 ); // the whole branch up to the root gets weight 1
			}
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
		case "down_idle":
			return %dying_back_idle;
		case "down_crawl":
			return %dying_crawl_back;
		case "down_fire":
			return %dying_back_fire;
		case "down_death":
			return %dying_back_death_v1;
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

// the fall onto the back from the stance he went down in (animscripts\pain.gsc stand_2_back / crouch_2_back / prone_2_back)
body_fall_anim( stance )
{
	if ( stance == "prone" )
		return %dying_crawl_2_back;
	if ( stance == "crouch" )
		return %dying_crouch_2_back;
	return %dying_stand_2_back_v1;
}

// QA: hits per player in the dvars coop_hits<n> (count, the last attacker kind, damage and time); coop_qa_keephealth 1 (set on
// both PCs) puts a standing player back to full health every frame, so a long fight can be counted without anybody going down.
qa_damage_watch()
{
	hits = 0;
	ai_hits = 0;
	self thread qa_keep_health();
	while ( true )
	{
		self waittill( "damage", amount, attacker );
		hits++;
		kind = "other";
		if ( isdefined( attacker ) && isai( attacker ) )
		{
			kind = "ai";
			ai_hits++;
		}
		down = 0;
		if ( isdefined( self.coop_isdown ) )
			down = 1;
		setdvar( "coop_hits" + self.coop_index, hits + " ai " + ai_hits + " last " + kind + " " + amount + " down " + down + " at " + gettime() );
	}
}

qa_keep_health()
{
	while ( true )
	{
		wait 0.05;
		if ( getdvar( "coop_qa_keephealth" ) == "1" && !isdefined( self.coop_isdown ) && self.health > 0 && self.health < self.maxhealth )
			self.health = self.maxhealth;
	}
}
