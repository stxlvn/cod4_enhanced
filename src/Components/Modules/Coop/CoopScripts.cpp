#include "STDInc.hpp"
#include "CoopScripts.hpp"

// Load-time patches of the game's own scripts (no game script is shipped): the text of a rawfile is changed when the script
// compiler asks for it. Scr_ReadFile_FastFile 0x54A620 looks the script up with DB_FindXAssetHeader(ASSET_TYPE_RAWFILE 0x1F, name)
// at 0x54A653 and parses rawfile->buffer in place; the hook returns a patched copy instead.

namespace Components
{
	namespace
	{
		struct RawFile
		{
			const char* name;
			int len;
			const char* buffer;
		};

		struct Patched
		{
			std::string name;
			std::string text;
			RawFile raw;
		};

		std::unordered_map<std::string, std::unique_ptr<Patched>> cache;

		bool IsSpace(char c)
		{
			return c == ' ' || c == '\t' || c == '\r' || c == '\n';
		}

		// Matches lit (a token sequence where ' ' means optional whitespace) at s[i]; returns the end or npos.
		size_t MatchLoose(const std::string& s, size_t i, const char* lit)
		{
			for (const char* p = lit; *p; p++)
			{
				if (*p == ' ')
				{
					while (i < s.size() && IsSpace(s[i]))
						i++;
					continue;
				}
				if (i >= s.size() || std::tolower(static_cast<unsigned char>(s[i])) != *p)
					return std::string::npos;
				i++;
			}
			return i;
		}

		// getent( "player", "classname" ) -> getentarray( "player", "classname" )[0]: the same for one player, and no
		// "getent used with more than one entity" error once player 2 exists.
		int PatchGetEntPlayer(std::string& text)
		{
			int n = 0;
			size_t pos = 0;
			while ((pos = text.find("etent", pos)) != std::string::npos)
			{
				const size_t start = pos - 1;
				if (pos == 0 || std::tolower(static_cast<unsigned char>(text[start])) != 'g' ||
					(start > 0 && (std::isalnum(static_cast<unsigned char>(text[start - 1])) || text[start - 1] == '_')))
				{
					pos++;
					continue;
				}
				const size_t end = MatchLoose(text, start, "getent ( \"player\" , \"classname\" )");
				if (end == std::string::npos)
				{
					pos++;
					continue;
				}
				const std::string repl = "getentarray( \"player\", \"classname\" )[0]";
				text.replace(start, end - start, repl);
				pos = start + repl.size();
				n++;
			}
			return n;
		}

		// Text patches of one script: the anchor must occur exactly once (else the patch is not applied and the log says so);
		// with all = true every occurrence is replaced (at least one).
		struct TextPatch
		{
			const char* file;
			const char* anchor;
			const char* replacement;
			bool all = false;
		};

		const TextPatch textPatches[] =
		{
			// level.players / level.player2 next to level.player (IW4 SP has them natively)
			{ "maps/_load.gsc", "level.player.a = spawnStruct();",
				"level.player.a = spawnStruct();\n"
				"\tlevel.players = getentarray( \"player\", \"classname\" );\n"
				"\tif ( level.players.size > 1 )\n"
				"\t{\n"
				"\t\tlevel.player2 = level.players[ 1 ];\n"
				"\t\tlevel.player2.a = spawnStruct();\n"
				"\t}\n"
				"\tsetdvar( \"coop_script_players\", level.players.size );\n"
				"\tif ( level.players.size > 1 )\n"
				"\t\tthread maps\\_coop::main();" },

			// the training level for two (maps/_coop_killhouse.gsc): Gaz's range at two stations at once
			{ "maps/killhouse.gsc", "level notify ( \"rifleTraining_start\" );",
				"level notify ( \"rifleTraining_start\" );\n"
				"\tif ( isdefined( level.players ) && level.players.size > 1 )\n"
				"\t{\n"
				"\t\tthread maps\\_coop_killhouse::rifle_training();\n"
				"\t\treturn;\n"
				"\t}" },
			// station one's targets take only player 1's hits (player 2 has his own at station two)
			{ "maps/killhouse_code.gsc", "if ( ! ( level.player istouching ( stall ) ) )",
				"if ( isdefined( level.players ) && level.players.size > 1 && ( !isdefined( attacker ) || attacker != level.player ) )\n"
				"\t\t\t{\n"
				"\t\t\t\tself.health = 1000;\n"
				"\t\t\t\tcontinue;\n"
				"\t\t\t}\n"
				"\t\t\tif ( ! ( level.player istouching ( stall ) ) )" },
			// Price's ship: one player after the other (the course loop runs each run for level.player), started from
			// report_to_price right before its "thread cargoship_training();". The loop that runs is not killhouse.gsc's:
			// IW3SP-MOD's scripts/killhouse/best_squad_record.gsc ReplaceFunc's it with its own copy (Gaz's squad record), so
			// the loop's patches target that file; the same patches on killhouse.gsc keep a build without the override right
			{ "maps/killhouse.gsc", "level.price animscripts\\shared::placeWeaponOn( level.price.weapon, \"right\" );",
				"level.price animscripts\\shared::placeWeaponOn( level.price.weapon, \"right\" );\n"
				"\tif ( isdefined( level.players ) && level.players.size > 1 )\n"
				"\t\tthread maps\\_coop_killhouse::cargoship_turns();" },
			{ "scripts/killhouse/best_squad_record.gsc", "\t\t\tthread maps\\killhouse::debrief();",
				"\t\t\tthread maps\\_coop_killhouse::debrief_gate();" },
			{ "scripts/killhouse/best_squad_record.gsc", "level.player UploadTime( 19, final_time );",
				"if ( !isdefined( level.players ) || level.player == level.players[ 0 ] )\n"
				"\t\t\t\tlevel.player UploadTime( 19, final_time );" },
			{ "maps/killhouse.gsc", "\t\t\tthread debrief();",
				"\t\t\tthread maps\\_coop_killhouse::debrief_gate();" },
			// Death from Above for two gunners (maps/_coop_ac130.gsc): each _ac130 function that is one player's (the gun, its HUD, FOV,
			// thermal, ammo, fire, impacts) hands over to the co-op version when two players play it (not in the end credits)
			{ "maps/_ac130.gsc", "level.player.ignoreme = true;",
				"level.player.ignoreme = true;\n\tif ( maps\\_coop_ac130::coop_on() )\n\t\tmaps\\_coop_ac130::coop_init();" },
			{ "maps/_ac130.gsc", "level.HUDItem = [];",
				"if ( maps\\_coop_ac130::coop_on() )\n\t{\n\t\tmaps\\_coop_ac130::coop_overlay();\n\t\treturn;\n\t}\n\tlevel.HUDItem = [];" },
			{ "maps/_ac130.gsc", "level.player playerLinkToDelta (level.ac130, \"tag_player\", 1.0, 65, 65, 40, 40 );",
				"if ( maps\\_coop_ac130::coop_on() )\n\t{\n\t\tmaps\\_coop_ac130::coop_attach();\n\t\treturn;\n\t}\n\tlevel.player playerLinkToDelta (level.ac130, \"tag_player\", 1.0, 65, 65, 40, 40 );" },
			{ "maps/_ac130.gsc", "level.ac130_weapon = [];",
				"if ( maps\\_coop_ac130::coop_on() )\n\t{\n\t\tmaps\\_coop_ac130::coop_change_weapons();\n\t\treturn;\n\t}\n\tlevel.ac130_weapon = [];" },
			{ "maps/_ac130.gsc", "level.player waittill( \"weapon_fired\" );",
				"if ( maps\\_coop_ac130::coop_on() )\n\t\t{\n\t\t\tmaps\\_coop_ac130::coop_weapon_fired();\n\t\t\treturn;\n\t\t}\n\t\tlevel.player waittill( \"weapon_fired\" );" },
			{ "maps/_ac130.gsc", "level notify( \"setting_ammo\" );",
				"if ( maps\\_coop_ac130::coop_on() )\n\t{\n\t\tmaps\\_coop_ac130::coop_set_ammo();\n\t\treturn;\n\t}\n\tlevel notify( \"setting_ammo\" );" },
			{ "maps/_ac130.gsc", "level.player notifyOnCommand( \"switch thermal\", \"+usereload\" );",
				"if ( maps\\_coop_ac130::coop_on() )\n\t{\n\t\tmaps\\_coop_ac130::coop_thermal();\n\t\treturn;\n\t}\n\tlevel.player notifyOnCommand( \"switch thermal\", \"+usereload\" );" },
			{ "maps/_ac130.gsc", "level.ac130_nofire = newHudElem();",
				"if ( maps\\_coop_ac130::coop_on() )\n\t{\n\t\tmaps\\_coop_ac130::coop_nofire();\n\t\treturn;\n\t}\n\tlevel.ac130_nofire = newHudElem();" },
			{ "maps/_ac130.gsc", "level.player waittill( \"projectile_impact\", weaponName, position, radius );",
				"if ( maps\\_coop_ac130::coop_on() )\n\t\t{\n\t\t\tmaps\\_coop_ac130::coop_shot_fired();\n\t\t\treturn;\n\t\t}\n\t\tlevel.player waittill( \"projectile_impact\", weaponName, position, radius );" },
			{ "maps/_ac130.gsc", "level.player shellshock( \"ac130\", duration );",
				"if ( maps\\_coop_ac130::coop_on() )\n\t\t\tmaps\\_coop_ac130::coop_shellshock( duration );\n\t\telse\n\t\t\tlevel.player shellshock( \"ac130\", duration );" },
			{ "maps/_ac130.gsc", "thread blink_crosshairs_stop();",
				"if ( maps\\_coop_ac130::coop_on() )\n\t\tthread maps\\_coop_ac130::coop_crosshairs_stop();\n\telse\n\t\tthread blink_crosshairs_stop();" },
			// kills, friendly fire, the church and the civilians count for both gunners
			{ "maps/_ac130.gsc", "attacker != level.player",
				"!isplayer( attacker )", true },
			{ "maps/_ac130.gsc", "attacker == level.player",
				"isplayer( attacker )", true },
			{ "maps/ac130_code.gsc", "attacker != level.player",
				"!isplayer( attacker )", true },
			{ "maps/ac130_code.gsc", "attacker == level.player",
				"isplayer( attacker )", true },
			// every level: player 2's kills and team kills count like player 1's (friendly fire fails the mission for both)
			{ "maps/_friendlyfire.gsc", "if ( attacker == level.player )",
				"if ( isplayer( attacker ) )" },
			{ "maps/_friendlyfire.gsc", "( owner == level.player )",
				"( isplayer( owner ) )" },
			// shared trigger loops that assumed level.player touched them (player 2 fires triggers now): ambience and fog follow player 1
			// (a trigger player 2 enters left "progress" undefined: script error); lookat / cansee triggers test the player who entered
			// (_load.gsc has CRLF line ends: a two-line anchor needs CR LF)
			{ "maps/_ambient.gsc", "assertEx( other == level.player, \"Non - player entity touched an ambient trigger.\" );",
				"if ( other != level.player )\n\t\t\tcontinue;", true },
			{ "maps/_load.gsc", "assertEx( other == level.player, \"Non-player entity touched a trigger_fog.\" );",
				"if ( other != level.player )\n\t\t\tcontinue;" },
			{ "maps/_load.gsc", "assertEx( other == level.player, \"trigger_lookat currently only supports looking from the player\" );\r\n\t\twhile ( level.player istouching( trigger ) )",
				"while ( other istouching( trigger ) )" },
			{ "maps/_load.gsc", "assertEx( other == level.player, \"trigger_cansee currently only supports looking from the player\" );\r\n\t\t\r\n\t\twhile ( level.player istouching( trigger ) )",
				"while ( other istouching( trigger ) )" },
			// Crew Expendable for two (maps/_coop_cargoship.gsc): the package, the explosion, the run on the sinking ship, the jump
			// the Blackhawk ride and fast-rope: player 2 rides and ropes down with player 1
			{ "maps/mo_fastrope.gsc", "playerWeaponTake();",
				"playerWeaponTake();\n\tthread maps\\_coop_cargoship::coop_fastrope_partners( self );" },
			{ "maps/cargoship.gsc", "jumptoInit();",
				"jumptoInit();\n\tmaps\\_coop_cargoship::coop_cs_init();" },
			{ "maps/cargoship.gsc", "while( distance( guy.origin, level.player.origin ) > 700 )",
				"while( distance( guy.origin, maps\\_coop_cargoship::coop_nearest( guy.origin ).origin ) > 700 )" },
			{ "maps/cargoship.gsc", "array_thread( level.heroes3, ::escape_heroes2 );",
				"maps\\_coop_cargoship::coop_heroes2();" },
			{ "maps/cargoship.gsc", "while( distance( level.player.origin, price.origin ) > 490 )",
				"while( distance( maps\\_coop_cargoship::coop_nearest( price.origin ).origin, price.origin ) > 490 )" },
			{ "maps/cargoship.gsc", "level.player freezeControls(true);",
				"if ( maps\\_coop_cargoship::coop_on() )\n\t{\n\t\tmaps\\_coop_cargoship::coop_end_rescue();\n\t\treturn;\n\t}\n\tlevel.player freezeControls(true);" },
			{ "maps/cargoship.gsc", "if( distance( level.player.origin, price.origin ) < 512 )",
				"if( maps\\_coop_cargoship::coop_last_quakes( price ) )" },
			{ "maps/cargoship.gsc", "jump = getent( \"escape_player_jump\", \"targetname\" );",
				"if ( maps\\_coop_cargoship::coop_on() )\n\t{\n\t\tmaps\\_coop_cargoship::coop_end_jump();\n\t\treturn;\n\t}\n\tjump = getent( \"escape_player_jump\", \"targetname\" );" },
			{ "maps/cargoship.gsc", "nojump = getent( \"escape_player_nojump\", \"targetname\" );",
				"if ( maps\\_coop_cargoship::coop_on() )\n\t{\n\t\tmaps\\_coop_cargoship::coop_end_nojump();\n\t\treturn;\n\t}\n\tnojump = getent( \"escape_player_nojump\", \"targetname\" );" },
			{ "maps/cargoship_code.gsc", "if( distance(level.player.origin, self.origin) < 700 )",
				"if( maps\\_coop_cargoship::coop_escape_event_players( self ) )" },
			{ "maps/cargoship_code.gsc", "earthquake(0.25, .75, level.player.origin, 1024);",
				"maps\\_coop_cargoship::coop_quake_all( 0.25, .75, 1024 );", true },
			{ "maps/cargoship_code.gsc", "level.player PlayRumbleOnEntity( \"tank_rumble\" );",
				"maps\\_coop_cargoship::coop_rumble_all( \"tank_rumble\" );", true },
			{ "maps/cargoship_code.gsc", "level.player allowsprint( false );",
				"maps\\_coop_cargoship::coop_allowsprint_all( false );" },
			{ "maps/cargoship_code.gsc", "level.player thread escape_quake();",
				"maps\\_coop_cargoship::coop_escape_quakes();" },
			{ "maps/cargoship_code.gsc", "thread escape_shellshock_vision();",
				"if ( maps\\_coop_cargoship::coop_on() )\n\t{\n\t\tmaps\\_coop_cargoship::coop_escape_shellshock();\n\t\treturn;\n\t}\n\tthread escape_shellshock_vision();" },
			{ "maps/cargoship_code.gsc", "while( level.player istouching( trig ) )",
				"while( maps\\_coop_cargoship::coop_any_touching( trig ) )" },
			// a hero in the Seaknight (escape_heroes_rescue done): Gaz catches player 2 only once he is aboard
			{ "maps/cargoship.gsc", "heli thread anim_generic_loop(self, idle, \"tag_detach\", stop );",
				"self.coop_boarded = true;\n\t\theli thread anim_generic_loop(self, idle, \"tag_detach\", stop );" },
			// the heroes hold the line by the nearest player; their single-file order needs self.position, which the level's own "end"
			// start point never sets (script error also with one player, fatal with developer 1): skipped until it is set
			{ "maps/cargoship_code.gsc", "self.dist = distance( self.origin, level.player.origin );",
				"self.dist = distance( self.origin, maps\\_coop_cargoship::coop_nearest( self.origin ).origin );" },
			{ "maps/cargoship_code.gsc", "self escape_heroes_holdtheline_run_or_walk_asone( order );",
				"if ( !isdefined( self.position ) )\n\t\t\tcontinue;\n\t\tself escape_heroes_holdtheline_run_or_walk_asone( order );" },
			// a dog after a player who is not level.player: bites him (the AI-vs-dog path broke on a player)
			{ "animscripts/dog_combat.gsc", "self meleeStruggleVsAI();",
				"{\n\t\tif ( isplayer( self.enemy ) )\n\t\t\tself maps\\_coop_stealth::coop_dog_bite_player();\n\t\telse\n\t\t\tself meleeStruggleVsAI();\n\t}" },
			// stealth (maps/_coop_stealth.gsc): both players under the same detection rules as the host
			{ "maps/_stealth_logic.gsc", "return level.player getstance();",
				"return self getstance();" },
			{ "maps/_stealth_logic.gsc", "if( other == level.player ||",
				"if( isplayer( other ) ||" },
			{ "maps/_stealth_logic.gsc", "enemy = level.player;",
				"enemy = maps\\_coop_stealth::coop_nearest( self.origin );" },
			{ "maps/_stealth_behavior.gsc", "self.favoriteenemy = level.player;",
				"self.favoriteenemy = maps\\_coop_stealth::coop_nearest( self.origin );", true },
			{ "maps/_stealth_behavior.gsc", "target = level.player;",
				"target = maps\\_coop_stealth::coop_nearest( self.origin );" },
			{ "maps/blackout_code.gsc", "level.player thread stealth_ai();",
				"maps\\_coop_stealth::coop_stealth_players();" },
			{ "maps/icbm.gsc", "level.player thread stealth_ai();",
				"maps\\_coop_stealth::coop_stealth_players();" },
			{ "maps/scoutsniper.gsc", "level.player thread stealth_ai();",
				"maps\\_coop_stealth::coop_stealth_players();" },
			{ "maps/blackout_code.gsc", "level.player._stealth_move_detection_cap = 100;",
				"maps\\_coop_stealth::coop_detection_cap( 100 );" },
			{ "maps/scoutsniper.gsc", "level.player stealth_friendly_movespeed_scale_set(",
				"maps\\_coop_stealth::coop_movespeed_scale_set(", true },
			{ "maps/scoutsniper.gsc", "level.player stealth_friendly_movespeed_scale_default()",
				"maps\\_coop_stealth::coop_movespeed_scale_default()", true },
			{ "maps/scoutsniper_code.gsc", "level.player stealth_friendly_movespeed_scale_set(",
				"maps\\_coop_stealth::coop_movespeed_scale_set(", true },
			{ "maps/scoutsniper_code.gsc", "level.player stealth_friendly_movespeed_scale_default()",
				"maps\\_coop_stealth::coop_movespeed_scale_default()", true },
			{ "maps/scoutsniper.gsc", "level.player._stealth_move_detection_cap = 0;",
				"maps\\_coop_stealth::coop_detection_cap( 0 );" },
			{ "maps/scoutsniper.gsc", "level.player._stealth_move_detection_cap = undefined;",
				"maps\\_coop_stealth::coop_detection_cap( undefined );" },
			{ "maps/scoutsniper.gsc", "trig = getent( \"graveyard_inside_church_trig\", \"targetname\" );",
				"if ( isdefined( level.players ) && level.players.size > 1 )\n\t{\n\t\tself maps\\_coop_stealth::coop_hind_spot_enemy();\n\t\treturn;\n\t}\n\ttrig = getent( \"graveyard_inside_church_trig\", \"targetname\" );" },
			{ "maps/scoutsniper.gsc", "level.player thread player_noprone_water();",
				"if ( isdefined( level.players ) )\n\t{\n\t\tfor ( coop_i = 0; coop_i < level.players.size; coop_i++ )\n\t\t\tlevel.players[ coop_i ] thread player_noprone_water();\n\t}\n\telse\n\t\tlevel.player thread player_noprone_water();" },
			// scoutsniper player_noprone_water runs on each player (threaded per player above): its body on self
			{ "maps/scoutsniper_code.gsc", "level.player allowprone( false );",
				"self allowprone( false );" },
			{ "maps/scoutsniper_code.gsc", "while( level.player istouching( water ) )",
				"while( self istouching( water ) )" },
			{ "maps/scoutsniper_code.gsc", "level.player allowprone( true );",
				"self allowprone( true );" },
			{ "maps/scoutsniper_code.gsc", "distancesquared( level.player.origin, level.price.origin )",
				"distancesquared( maps\\_coop_stealth::coop_nearest( level.price.origin ).origin, level.price.origin )", true },
			// QA helpers of the co-op training level (dvars only: idle unless a test sets coop_qa_kh)
			{ "maps/killhouse.gsc", "thread clear_hints_on_mission_fail();",
				"thread clear_hints_on_mission_fail();\n"
				"\tif ( isdefined( level.players ) && level.players.size > 1 )\n"
				"\t\tthread maps\\_coop_killhouse::coop_kh_qa();" },
			{ "maps/killhouse.gsc", "level.player UploadTime( 19, final_time );",
				"if ( !isdefined( level.players ) || level.player == level.players[ 0 ] )\n"
				"\t\t\t\tlevel.player UploadTime( 19, final_time );" },
		};

		// The mod's own scripts: cod4coop/scripts/<name> in the game folder (e.g. maps/_coop.gsc), served before the fastfiles.
		std::unordered_map<std::string, std::unique_ptr<Patched>> ownScripts;

		const RawFile* OwnScript(const char* name)
		{
			auto it = ownScripts.find(name);
			if (it != ownScripts.end())
				return it->second ? &it->second->raw : nullptr;
			std::ifstream f(std::string("cod4coop/scripts/") + name, std::ios::binary);
			if (!f)
			{
				ownScripts.emplace(name, nullptr);
				return nullptr;
			}
			auto p = std::make_unique<Patched>();
			p->name = name;
			p->text.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
			p->raw = RawFile{ p->name.c_str(), static_cast<int>(p->text.size()), p->text.c_str() };
			Coop::Log("own script %s (%zu bytes)", name, p->text.size());
			const RawFile* r = &p->raw;
			ownScripts.emplace(name, std::move(p));
			return r;
		}

		int ApplyTextPatches(const char* name, std::string& text)
		{
			int n = 0;
			for (const auto& p : textPatches)
			{
				if (std::strcmp(p.file, name))
					continue;
				const size_t at = text.find(p.anchor);
				if (p.all && at != std::string::npos)
				{
					for (size_t pos = at; pos != std::string::npos; pos = text.find(p.anchor, pos + std::strlen(p.replacement)))
						text.replace(pos, std::strlen(p.anchor), p.replacement);
					n++;
					continue;
				}
				if (at == std::string::npos || text.find(p.anchor, at + 1) != std::string::npos)
				{
					Coop::Log("script patch %s: anchor \"%s\" %s, patch NOT applied", name, p.anchor, at == std::string::npos ? "not found" : "found twice");
					continue;
				}
				text.replace(at, std::strlen(p.anchor), p.replacement);
				n++;
			}
			return n;
		}

		Game::XAssetHeader __cdecl FindScriptRawfile(Game::XAssetType type, const char* name)
		{
			if (name)
			{
				if (const RawFile* own = OwnScript(name))
				{
					Game::XAssetHeader h;
					h.data = const_cast<RawFile*>(own);
					return h;
				}
			}
			Game::XAssetHeader header = Game::DB_FindXAssetHeader(type, name);
			if (!header.data || !name)
				return header;
			auto it = cache.find(name);
			if (it != cache.end())
			{
				if (it->second)
					header.data = &it->second->raw;
				return header;
			}

			const auto* raw = static_cast<const RawFile*>(header.data);
			std::string text(raw->buffer ? raw->buffer : "");
			const int n = PatchGetEntPlayer(text);
			const int t = ApplyTextPatches(name, text);
			if (!n && !t)
			{
				cache.emplace(name, nullptr);
				return header;
			}
			auto p = std::make_unique<Patched>();
			p->name = name;
			p->text = std::move(text);
			p->raw = RawFile{ p->name.c_str(), static_cast<int>(p->text.size()), p->text.c_str() };
			Coop::Log("script patch %s: %d getent(player) -> getentarray(player)[0], %d text patch(es)", name, n, t);
			// QA: the patched text in cod4coop/patched/<name> when COD4COOP_DUMP is set
			char dump[8] = {};
			if (GetEnvironmentVariableA("COD4COOP_DUMP", dump, sizeof(dump)) && dump[0] == '1')
			{
				std::string out = std::string("cod4coop/patched/") + name;
				std::filesystem::create_directories(std::filesystem::path(out).parent_path());
				std::ofstream(out, std::ios::binary) << p->text;
			}
			header.data = &p->raw;
			cache.emplace(name, std::move(p));
			return header;
		}
	}

	void CoopScripts::Install()
	{
		Utils::Hook(0x54A653, FindScriptRawfile, HOOK_CALL).install()->quick();
	}
}
