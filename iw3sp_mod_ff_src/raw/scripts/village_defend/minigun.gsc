#include common_scripts\utility;
#include maps\_utility;

main()
{
    ReplaceFunc(maps\village_defend_code::minigun_used, ::minigun_used);
}

minigun_used()
{
	flag_wait( "player_on_minigun" );	
	
	//Tweakable values	

	if( level.console || getdvarint("gpad_in_use") )
		overheat_time = 6;	//full usage to overheat (original 8)
	else
		overheat_time = 10;
		
	cooldown_time = 4;	//time to cool down from max heat back to 0 if not operated during this time (original 4)
	penalty_time = 7;	//hold inoperative for this amount of time
	rate = 0.02;
	slow_rate = 0.02;
	overheat_fx_rate = 0.35;
	
	adsbuttonAccumulate = 0; //check for left trigger hold down duration
	
	//Not to tweak
	
	heatrate = 1 / ( overheat_time * 20 );	//increment of the temp gauge for heating up 
	coolrate = 1 / ( cooldown_time * 20 );	//increment of the temp gauge for cooling down
	level.inuse = false;
	momentum = 0;
	self.momentum = 0;
	heat = 0;
	max = 1;
	maxed = false;
	firing = false;
	maxed_time = undefined;
	overheated = false;
	penalized_time = 0;	//if greater than gettime
	startFiringTime = undefined;
	oldheat = 0;
	level.frames = 0;
	level.normframes = 0;
	next_overheat_fx = 0;
	thread maps\village_defend_code::minigun_rumble();
	for ( ;; )
	{
		level.normframes++;
		if ( flag( "player_on_minigun" ) )
		{
			if ( !level.inuse )
			{
				if ( level.Console || getdvarint("gpad_in_use") )
				{
					//if ( level.player adsbuttonpressed() || ( level.player attackbuttonpressed() && !overheated ) )
					if ( level.player adsbuttonpressed() )
					{
						level.inuse = true;
						self thread maps\village_defend_code::minigun_sound_spinup();
					}
				}
				else
				if( level.player attackbuttonpressed() )
				{
						level.inuse = true;
						self thread maps\village_defend_code::minigun_sound_spinup();
				}
			}
			else
			{		
				if ( level.Console || getdvarint("gpad_in_use") )
				{
					//if ( !level.player adsbuttonpressed() && !level.player attackbuttonpressed() )
					if ( !level.player adsbuttonpressed() )
					{
						level.inuse = false;
						self thread maps\village_defend_code::minigun_sound_spindown();	
					}
					else
					if( !level.player adsbuttonpressed() && level.player attackbuttonpressed() && overheated )
					{
						level.inuse = false;
						self thread maps\village_defend_code::minigun_sound_spindown();
					}
				}
				else
				if( !level.player attackbuttonpressed() )
				{
					level.inuse = false;
					self thread maps\village_defend_code::minigun_sound_spindown();
				}
				else
				if( level.player attackbuttonpressed() && overheated )
				{
					level.inuse = false;
					self thread maps\village_defend_code::minigun_sound_spindown();
				}
			}
			
			if( level.Console || getdvarint("gpad_in_use"))
			{
				if( level.player adsbuttonpressed() )
				{
					adsbuttonAccumulate += 0.05;
					
					if( adsbuttonAccumulate >= 2.75 )
					{
						flag_set( "minigun_lesson_learned" );
					}
				}
				else
				{
					adsbuttonAccumulate = 0;
				}
			}
			
			if ( !firing )
			{
				if( level.player attackbuttonpressed() && !overheated && maxed )
				{
					firing = true;
					startFiringTime = gettime();
				}
				else
				if ( level.player attackbuttonpressed() && overheated )
				{
					firing = false;
					startFiringTime = undefined;
				}	
			}
			else
			{
				if( !level.player attackbuttonpressed() )
				{
					firing = false;
					startFiringTime = undefined;
				}
				
				if( level.player attackbuttonpressed() && !maxed )
				{
					firing = false;
					startFiringTime = undefined;
				}
			}
		}
		else
		{
			if( firing || level.inuse == true )
			{	
				self thread maps\village_defend_code::minigun_sound_spindown();
			}
			
			firing = false;
			level.inuse = false;
		}			
	
		if ( overheated )
		{
			if ( !(heat >= max) )
			{
				overheated = false;
				startFiringTime = undefined;
				self TurretFireEnable();
			}
		}

		if ( level.inuse )
		{
			momentum += rate;
			self.momentum = momentum;
		}
		else
		{
			momentum -= slow_rate;
			self.momentum = momentum;
		}
		
		if ( momentum > max )
		{
			momentum = max;
			self.momentum = momentum;
		}
		if ( momentum < 0 )
		{
			momentum = 0;
			self.momentum = momentum;
			self notify ( "done" );
		}

		if ( momentum == max )
		{
			maxed = true;
			maxed_time = gettime();
			self TurretFireEnable();
		}
		else
		{
			maxed = false;
			self TurretFireDisable();
		}

		if ( firing && !overheated )
		{
			level.frames++;
			heat += heatrate;
		}
		
		if( gettime() > penalized_time && !firing )
			heat -= coolrate;
		
		if ( heat > max )
				heat = max;
		if ( heat < 0 )
				heat = 0;
	
		level.heat = heat;
	
		level.turret_heat_status = int( heat * 114 );
		if ( isdefined( level.overheat_status2 ) )
			thread maps\village_defend_code::overheat_hud_update();
		
		if( (heat >= max) && (heat <= max) && ((oldheat < max) || (oldheat > max)) )
		{
			overheated = true;
			penalized_time = gettime() + penalty_time * 1000;
			next_overheat_fx = 0;
			thread maps\village_defend_code::overheat_overheated();
		}
		oldheat = heat;
		
		if ( overheated )
		{
			self TurretFireDisable();
			firing = false;
			//playfxOnTag( getfx( "turret_overheat_haze" ), self, "tag_flash");
			if ( gettime() > next_overheat_fx )
			{
				playfxOnTag( getfx( "turret_overheat_smoke" ), self, "tag_flash");
				next_overheat_fx = gettime() + overheat_fx_rate * 1000;
			}
		}

		self setanim( getanim( "spin" ), 1, 0.2, momentum );
		
		wait( 0.05 );
	}
}