// CoD4 co-op: the attack-dog takedown for any player (animscripts\dog_combat's player path is written for level.player only:
// one view rig level.player_view, one hint, one bite counter). A dog whose target is player 2 runs the same sequence here with
// that player: the run-up bites (no kill while he has health), then the knock-down onto his back seen through his own view
// hands rig (shown on his PC only), the melee window (anim.dog_presstime) to snap the dog's neck, too soon / too late = the dog
// mauls him (the co-op death shield turns that into a last stand while his partner is up). His hint and HUD-off are his own
// (coophudnext / coopsetsaveddvar). On the other PC his body plays the AI's attacked-by-dog animations (on the ground, then up or
// down), also for player 1's game sequence. Started from text patches of animscripts\dog_combat and dog_init (CoopScripts.cpp).
// All of it is simulation (both PCs alike); only the HUD bits act on one PC.
#include common_scripts\utility;
#include maps\_utility;

#using_animtree( "dog" );

// animscripts\dog_combat::main for a player target other than level.player (self = the dog)
coop_dog_attack_player()
{
	attackRangeBuffer = 30;
	meleeRange = self.meleeAttackDist + attackRangeBuffer;
	for ( ;; )
	{
		if ( !isalive( self.enemy ) || !isplayer( self.enemy ) || self.enemy == level.player )
			break; // player 1 (or an AI) is the game's own path: main runs again
		p = self.enemy;
		self.coop_dog_target = p;
		if ( ( isdefined( p.syncedMeleeTarget ) && p.syncedMeleeTarget != self ) || ( isdefined( p.coop_player_view ) && isdefined( p.coop_player_view.inSeq ) ) )
		{
			if ( self animscripts\dog_combat::checkEndCombat( meleeRange ) )
				break;
			self animscripts\dog_combat::combatIdle();
			continue;
		}
		if ( self animscripts\dog_combat::shouldWaitInCombatIdle() )
		{
			self animscripts\dog_combat::combatIdle();
			continue;
		}
		self OrientMode( "face enemy" );
		self animMode( "gravity" );
		self.safeToChangeScript = false;
		self animscripts\dog_combat::prepareAttackPlayer();
		self clearanim( %root, 0.1 );
		self clearpitchorient();
		p animscripts\dog_combat::setNextDogAttackAllowTime( 500 );
		if ( coop_dog_cant_kill_in_one_hit( p ) )
		{
			p.coop_dog_last_time = getTime();
			p.coop_dog_counter++;
			self setflaggedanimrestart( "meleeanim", %german_shepherd_run_attack, 1, 0.2, 1 );
			self animscripts\shared::DoNoteTracks( "meleeanim", ::coop_bite_notetracks );
		}
		else
		{
			self thread coop_dog_melee_death( p );
			p.attacked_by_dog = true;
			self thread coop_clear_attacked_on_death( p );
			self setflaggedanimrestart( "meleeanim", %german_shepherd_attack_player, 1, 0.2, 1 );
			self setflaggedanimrestart( "meleeanim", %german_shepherd_attack_player_late, 1, 0, 1 );
			self setanimlimited( %attack_player, 1, 0, 1 );
			self setanimlimited( %attack_player_late, 0.01, 0, 1 );
			self animscripts\shared::DoNoteTracks( "meleeanim", ::coop_finish_notetracks );
			self notify( "dog_no_longer_melee_able" );
			self setcandamage( true );
			self unlink();
		}
		self.safeToChangeScript = true;
		wait 0.05;
		if ( self animscripts\dog_combat::checkEndCombat( meleeRange ) )
			break;
	}
	self.safeToChangeScript = true;
	self animMode( "none" );
}

coop_clear_attacked_on_death( p )
{
	self waittill( "death" );
	p.attacked_by_dog = undefined;
}

// dog_cant_kill_in_one_hit per player (the level's dogs_dont_instant_kill is set on level.player: it covers both)
coop_dog_cant_kill_in_one_hit( p )
{
	if ( isdefined( p.dogs_dont_instant_kill ) || isdefined( level.player.dogs_dont_instant_kill ) )
		return true;
	if ( !isdefined( p.coop_dog_counter ) || !isdefined( p.coop_dog_last_time ) || getTime() - p.coop_dog_last_time > 8000 )
		p.coop_dog_counter = 0;
	return p.coop_dog_counter < anim.dog_hits_before_kill && p.health > 25;
}

coop_bite_notetracks( note )
{
	switch ( note )
	{
		case "dog_melee":
			hitEnt = self melee( anglesToForward( self.angles ) );
			if ( isdefined( hitEnt ) )
			{
				if ( isplayer( hitEnt ) )
					hitEnt shellshock( "dog_bite", 1 );
			}
			else
			{
				self animscripts\dog_combat::attackMiss();
				return true;
			}
			break;
		case "stop_tracking":
			self OrientMode( "face current" );
			break;
	}
}

coop_finish_notetracks( note )
{
	p = self.coop_dog_target;
	switch ( note )
	{
		case "dog_melee":
			healthAdded = p coop_add_safety_health();
			hitEnt = self melee( anglesToForward( self.angles ) );
			if ( healthAdded )
				p coop_remove_safety_health();
			if ( isdefined( hitEnt ) && hitEnt == p && isalive( p ) )
			{
				if ( !isdefined( p.coop_player_view ) )
					p.coop_player_view = p coop_playerview_spawn();
				if ( p.coop_player_view coop_playerview_start( self, p ) )
					self setcandamage( false );
			}
			else
			{
				self animscripts\dog_combat::attackMiss();
				return true;
			}
			break;
		case "dog_early":
			self notify( "dog_early_notetrack" );
			speed = 0.45 + 0.8 * level.dog_melee_timing_array[ level.dog_melee_index ];
			level.dog_melee_index++;
			if ( level.dog_melee_index >= level.dog_melee_timing_array.size )
			{
				level.dog_melee_index = 0;
				level.dog_melee_timing_array = maps\_utility::array_randomize( level.dog_melee_timing_array );
			}
			self setflaggedanimlimited( "meleeanim", %german_shepherd_attack_player, 1, 0.2, speed );
			self setflaggedanimlimited( "meleeanim", %german_shepherd_attack_player_late, 1, 0.2, speed );
			if ( isdefined( p.coop_player_view ) )
			{
				p.coop_player_view setflaggedanimlimited( "viewanim", animscripts\dog_combat::get_player_view_dog_knock_down_anim(), 1, 0.2, speed );
				p.coop_player_view setflaggedanimlimited( "viewanim", animscripts\dog_combat::get_player_view_dog_knock_down_late_anim(), 1, 0.2, speed );
			}
			break;
		case "dog_lunge":
			self thread coop_set_melee_timer( p );
			self setflaggedanim( "meleeanim", %german_shepherd_attack_player, 1, 0.2, 1 );
			if ( isdefined( p.coop_player_view ) )
			{
				p.coop_player_view setflaggedanim( "viewanim", animscripts\dog_combat::get_player_view_dog_knock_down_anim(), 1, 0.2, 1 );
				p.coop_player_view setflaggedanim( "viewanim", animscripts\dog_combat::get_player_view_dog_knock_down_late_anim(), 1, 0.2, 1 );
			}
			break;
		case "dogbite_damage":
			self thread coop_killplayer( p );
			break;
		case "stop_tracking":
			self OrientMode( "face current" );
			break;
	}
}

coop_add_safety_health()
{
	frac = self getnormalhealth();
	if ( frac == 0 )
		return false;
	if ( frac < 0.25 )
	{
		self setnormalhealth( frac + 0.25 );
		return true;
	}
	return false;
}

coop_remove_safety_health()
{
	frac = self getnormalhealth();
	if ( frac > 0.25 )
		self setnormalhealth( frac - 0.25 );
	else
		self setnormalhealth( 0.01 );
}

coop_set_melee_timer( p )
{
	wait 0.15;
	self.melee_able_timer = gettime();
	setdvar( "coop_dog_window" + p.coop_index, "open" ); // QA trace: the melee window
	p thread coop_dog_hint();
	wait anim.dog_presstime / 1000;
	setdvar( "coop_dog_window" + p.coop_index, "shut" );
}

// the "press melee" hint on this player's screen only
coop_dog_hint()
{
	self notify( "coop_dog_hint" );
	self endon( "coop_dog_hint" );
	if ( isdefined( self.coop_dog_hint_elem ) )
		self.coop_dog_hint_elem maps\_hud_util::destroyElem();
	self coophudnext();
	h = maps\_hud_util::createFontString( "default", 2 );
	h.color = ( 1, 1, 1 );
	h setText( &"SCRIPT_PLATFORM_DOG_HINT" );
	h.x = 0;
	h.y = 20;
	h.alignX = "center";
	h.alignY = "middle";
	h.horzAlign = "center";
	h.vertAlign = "middle";
	h.foreground = true;
	h.alpha = 1;
	self.coop_dog_hint_elem = h;
	wait anim.dog_presstime / 1000;
	if ( isdefined( self.coop_dog_hint_elem ) )
		self.coop_dog_hint_elem maps\_hud_util::destroyElem();
}

// dog_melee_death for player p: one press, inside the window = the neck snap
coop_dog_melee_death( p )
{
	self endon( "killanimscript" );
	self endon( "dog_no_longer_melee_able" );
	pressed = false;
	press_time = anim.dog_presstime;
	self waittill( "dog_early_notetrack" );
	while ( isalive( p ) && p MeleeButtonPressed() )
		wait 0.05;
	for ( ;; )
	{
		if ( !pressed )
		{
			if ( isalive( p ) && p MeleeButtonPressed() )
			{
				pressed = true;
				if ( isdefined( self.melee_able_timer ) && isalive( p ) )
				{
					if ( gettime() - self.melee_able_timer <= press_time )
					{
						setdvar( "coop_dog_result" + p.coop_index, "neck_snap" ); // QA trace
						if ( isdefined( p.coop_player_view ) )
							p.coop_player_view.custom_dog_save = "neck_snap";
						self notify( "melee_stop" );
						self setflaggedanimknobrestart( "dog_death_anim", %german_shepherd_player_neck_snap, 1, 0.2, 1 );
						self waittillmatch( "dog_death_anim", "dog_death" );
						self thread play_sound_in_space( "dog_neckbreak", self gettagorigin( "tag_eye" ) );
						self setcandamage( true );
						self.a.nodeath = true;
						dif = p.origin - self.origin;
						dif = ( dif[ 0 ], dif[ 1 ], 0 );
						self dodamage( self.health + 503, self geteye() - dif, p );
						self notify( "killanimscript" );
					}
					else
					{
						setdvar( "coop_dog_result" + p.coop_index, "too_late" ); // QA trace
						if ( isdefined( p.coop_player_view ) )
						{
							p.coop_player_view setanimlimited( animscripts\dog_combat::get_player_knockdown_knob(), 0.01, 0.2, 1 );
							p.coop_player_view setanimlimited( animscripts\dog_combat::get_player_knockdown_late_knob(), 1, 0.2, 1 );
						}
						self setanimlimited( %attack_player, 0.01, 0.2, 1 );
						self setanimlimited( %attack_player_late, 1, 0.2, 1 );
					}
					return;
				}
				setdvar( "coop_dog_result" + p.coop_index, "too_soon" ); // QA trace
				self setflaggedanimknobrestart( "meleeanim", %german_shepherd_player_neck_miss, 1, 0.2, 1 );
				if ( isdefined( p.coop_player_view ) )
					p.coop_player_view setflaggedanimknobrestart( "viewanim", animscripts\dog_combat::get_player_dog_neck_miss_anim(), 1, 0.2, 1 );
				return;
			}
		}
		else if ( !( isalive( p ) && p MeleeButtonPressed() ) )
			pressed = false;
		wait 0.05;
	}
}

// the dog's bite for p: in co-op the death shield turns it into a last stand while his partner is up
coop_killplayer( p )
{
	self endon( "pvd_melee_interrupted" );
	p.specialDeath = true;
	p setcandamage( true );
	if ( isdefined( p.coop_player_view ) )
	{
		tagPos = p.coop_player_view gettagorigin( "tag_torso" );
		playfx( level._effect[ "dog_bite_blood" ], tagPos + ( 0, 0, 20 ), anglestoforward( p.angles ), anglestoup( p.angles ) );
	}
	wait 1;
	p enableHealthShield( false );
	damage = 10 * p.health / getdvarfloat( "player_DamageMultiplier" );
	if ( !isalive( p ) )
		return;
	setdvar( "coop_dog_result" + p.coop_index, "mauled" ); // QA trace
	p dodamage( damage, p.origin );
	p shellshock( "default", 5 );
	p enableHealthShield( true );
	p.specialDeath = undefined;
}

#using_animtree( "player" );

coop_playerview_spawn()
{
	view = spawn( "script_model", self.origin );
	view.angles = self.angles;
	view setModel( level.player_viewhand_model );
	view useAnimTree( #animtree );
	view hide();
	view coopshowonlyto( self.coop_index );
	return view;
}

// PlayerView_StartSequence for p (self = his view rig)
coop_playerview_start( dog, p )
{
	if ( isdefined( self.inSeq ) )
		return false;
	p notify( "dog_attacks_player" );
	self.inSeq = true;
	self.coop_player = p;
	if ( isalive( p ) )
		p coopsetsaveddvar( "hud_drawhud", 0 );
	p setstance( "stand" );
	p.syncedMeleeTarget = dog;
	self show();
	p hideViewModel();
	p disableweapons();
	direction = dog.origin - p.origin;
	self.angles = ( 0, vectortoangles( direction )[ 1 ], 0 );
	playerpos = p.origin;
	self.origin = playerphysicstrace( ( playerpos[ 0 ], playerpos[ 1 ], playerpos[ 2 ] + 50 ), ( playerpos[ 0 ], playerpos[ 1 ], playerpos[ 2 ] - 200 ) );
	if ( self.origin[ 2 ] > playerpos[ 2 ] + 50 - 5 )
		self.origin = playerphysicstrace( ( playerpos[ 0 ], playerpos[ 1 ], playerpos[ 2 ] + 2 ), ( playerpos[ 0 ], playerpos[ 1 ], playerpos[ 2 ] - 200 ) );
	self thread coop_playerview_knockdown( dog );
	self dontInterpolate();
	p playerLinkToAbsolute( self, "tag_player" );
	dog linkto( self, "tag_sync", ( 0, 0, 0 ), ( 0, 0, 0 ) );
	syncTagAngles = self gettagangles( "tag_sync" );
	dog orientmode( "face angle", syncTagAngles[ 1 ] );
	dog orientmode( "face default" );
	p allowLean( false );
	p allowCrouch( false );
	p allowProne( false );
	p freezeControls( true );
	p setcandamage( false );
	p thread coop_dog_body( self );
	setdvar( "coop_dog_seq" + p.coop_index, "start" ); // QA trace
	return true;
}

coop_playerview_knockdown( dog )
{
	self endon( "pvd_melee_interrupted" );
	self.dog = dog;
	self thread coop_playerview_check_interrupted();
	self setflaggedanimrestart( "viewanim", %player_view_dog_knockdown );
	self setflaggedanimrestart( "viewanim", %player_view_dog_knockdown_late );
	self setanimlimited( animscripts\dog_combat::get_player_knockdown_knob(), 1, 0, 1 );
	self setanimlimited( animscripts\dog_combat::get_player_knockdown_late_knob(), 0.01, 0, 1 );
	self animscripts\shared::DoNoteTracks( "viewanim", animscripts\dog_combat::handlePlayerKnockDownNotetracks );
	self dontInterpolate();
	self.dog = undefined;
	self coop_playerview_end();
	self notify( "pvd_melee_done" );
}

coop_playerview_check_interrupted()
{
	self endon( "pvd_melee_done" );
	self.dog waittill_any( "death", "pain", "melee_stop" );
	p = self.coop_player;
	if ( !isdefined( p.specialDeath ) && isAlive( p ) )
	{
		self notify( "pvd_melee_interrupted" );
		self.dog notify( "pvd_melee_interrupted" );
		self coop_playerview_end();
	}
}

// PlayerView_EndSequence for p: up again (saved / neck snap), or off the rig where he lies when the bite put him down
coop_playerview_end()
{
	if ( isdefined( self.coop_ending ) )
		return;
	self.coop_ending = true;
	p = self.coop_player;
	p coopsetsaveddvar( "hud_drawhud", 1 );
	if ( isalive( p ) && !p coopdowned() )
	{
		self clearanim( %player_view_dog_knockdown, 0.1 );
		if ( isdefined( self.custom_dog_save ) )
			self setflaggedanimrestart( "viewanim", %player_view_dog_knockdown_neck_snap, 1, 0.2, 1 );
		else
		{
			p thread coop_saved_notify();
			self setflaggedanimrestart( "viewanim", %player_view_dog_knockdown_saved );
		}
		p delaythread( 3.0, ::coop_weapons_back );
		self animscripts\shared::DoNoteTracks( "viewanim" );
		p setcandamage( true );
		p notify( "player_saved_from_dog" );
		p unlink();
		p setOrigin( self.origin );
		angles = p getplayerangles();
		p setplayerangles( ( 0, angles[ 1 ], 0 ) );
		setdvar( "coop_dog_seq" + p.coop_index, "up" ); // QA trace
	}
	else
	{
		p setcandamage( true );
		p unlink();
		p showViewModel();
		setdvar( "coop_dog_seq" + p.coop_index, "down" ); // QA trace
	}
	self.inSeq = undefined;
	p.syncedMeleeTarget = undefined;
	p allowLean( true );
	if ( !p coopdowned() ) // the last stand set its own stance limits
	{
		p allowCrouch( true );
		p allowProne( true );
	}
	p freezeControls( false );
	p.attacked_by_dog = undefined;
	p.coop_player_view = undefined;
	self delete();
}

coop_saved_notify()
{
	wait 0.5;
	self playsound( "saved_from_dog" );
}

coop_weapons_back()
{
	self endon( "death" );
	if ( self coopdowned() )
		return; // the last stand gives him his pistol
	self showViewModel();
	self enableweapons();
}

#using_animtree( "generic_human" );

// the victim's body for the other PC: the AI's attacked-by-dog animations (falls back with the dog on him, struggles on the
// ground, gets up), while his view rig is in the sequence (view = his rig: p.coop_player_view or the game's level.player_view)
coop_dog_body( view )
{
	self notify( "coop_dog_body" );
	self endon( "coop_dog_body" );
	start = %AI_attacked_german_shepherd_01_start_a;
	self.coop_scene_anim = start;
	setdvar( "coop_dog_body" + self.coop_index, "fall" ); // QA trace
	wait getanimlength( start );
	if ( isdefined( view ) && isdefined( view.inSeq ) )
	{
		self.coop_scene_anim = %AI_attacked_german_shepherd_02_idle_a;
		setdvar( "coop_dog_body" + self.coop_index, "ground" );
	}
	while ( isdefined( view ) && isdefined( view.inSeq ) )
		wait 0.05;
	if ( isalive( self ) && !self coopdowned() )
	{
		getup = %AI_attacked_german_shepherd_04_getup_a;
		self.coop_scene_anim = getup;
		setdvar( "coop_dog_body" + self.coop_index, "getup" );
		wait getanimlength( getup );
	}
	self.coop_scene_anim = undefined;
	setdvar( "coop_dog_body" + self.coop_index, "done" );
}
