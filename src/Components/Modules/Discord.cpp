#include "STDInc.hpp"
#include <discord_rpc.h>

namespace Components
{
	DiscordRichPresence discord_presence;
	Discord::DiscordCustomInformation_t Discord::DiscordCustomInformation;
	bool Discord_Presence_Initialized = false;

	static const char* stock_sp_map_names[] =
	{
		"killhouse",
		"cargoship",
		"coup",
		"blackout",
		"armada",
		"bog_a",
		"hunted",
		"ac130",
		"bog_b",
		"airlift",
		"aftermath",
		"village_assault",
		"scoutsniper",
		"sniperescape",
		"village_defend",
		"ambush",
		"icbm",
		"launchfacility_a",
		"launchfacility_b",
		"jeepride",
		"simplecredits",
		"airplane",
	};

	bool InMainMenu(std::string map_name_info)
	{
		if (map_name_info == "") return true;
		else return false;
	}

	bool VanillaMap(std::string map_name_info)
	{
		for (const auto& map_name : stock_sp_map_names)
		{
			if (map_name == map_name_info)
				return true;
		}
		return false;
	}

	std::string GetDifficultyForState(std::string map_name_info)
	{
		if (!InMainMenu(map_name_info))
		{
			const auto difficulty = Dvars::Functions::Dvar_FindVar("g_gameskill")->current.integer;
			if (difficulty == 0)
				return "IW3SP_MOD_LOC_DISCORD_DIFFICULTY_EASY";
			else if (difficulty == 1)
				return "IW3SP_MOD_LOC_DISCORD_DIFFICULTY_MEDIUM";
			else if (difficulty == 2)
				return "IW3SP_MOD_LOC_DISCORD_DIFFICULTY_HARD";
			else if (difficulty == 3)
				return "IW3SP_MOD_LOC_DISCORD_DIFFICULTY_FU";
			else
				return "IW3SP_MOD_LOC_DISCORD_DIFFICULTY_UNKNOWN";
		}
		else
			return "";
	}

	std::string GetMapName(std::string map_name_info)
	{
		if (VanillaMap(map_name_info))
		{
			if(map_name_info.contains("ac130") && Dvars::Functions::Dvar_FindVar("credits_active")->current.string == "1"s)
				return "IW3SP_MOD_LOC_DISCORD_SIMPLECREDITS";
			return std::string("IW3SP_MOD_LOC_DISCORD_") + map_name_info.data();
		}
		else
		{
			if (InMainMenu(map_name_info)) 
				return "IW3SP_MOD_LOC_DISCORD_MAINMENU";
			else 
				return "custom_map";
		}
	}

	void Discord::DiscordUpdate()
	{
		Discord_RunCallbacks();

		std::string map_name_offset = reinterpret_cast<const char*>(0x1290DD8);
		std::string mapname = GetMapName(map_name_offset.data());
		std::string difficulty = GetDifficultyForState(map_name_offset.data());

		if (InMainMenu(map_name_offset.data()))
		{
			DiscordCustomInformation.details.reset();
			DiscordCustomInformation.state.reset();
			DiscordCustomInformation.button = std::make_pair("", "");
			DiscordCustomInformation.large_image_key.reset();
			discord_presence.largeImageKey = "preview_mainmenu";
		}
		else
		{
			if (DiscordCustomInformation.large_image_key.has_value())
			{
				const auto& imageUrl = DiscordCustomInformation.large_image_key.value();
				discord_presence.largeImageKey = imageUrl.data();
			}
			else
			{
				if (VanillaMap(map_name_offset.data()))
				{
					std::string imageKey = std::string("preview_") + map_name_offset.data();
					discord_presence.largeImageKey = imageKey.data();
				}
				else discord_presence.largeImageKey = nullptr;
			}
		}

		if (DiscordCustomInformation.details.has_value())
		{
			const auto& details_value = DiscordCustomInformation.details.value();
			if (details_value.starts_with("@") && details_value.size() > 1)
			{
				const auto loc_string = details_value.substr(1);
				const auto value = Game::UI_SafeTranslateString(loc_string.data());
				discord_presence.details = value;
			}
			else
			{
				discord_presence.details = details_value.data();
			}
		}
		else
		{
			if(mapname.contains("custom_map"))
				discord_presence.details = Game::UI_ReplaceConversionString(Game::UI_SafeTranslateString("IW3SP_MOD_LOC_DISCORD_MAPUNKNOWN"), map_name_offset.c_str());
			else
				discord_presence.details = Game::UI_SafeTranslateString(mapname.data());
		}

		if (DiscordCustomInformation.state.has_value())
		{
			const auto& state_value = DiscordCustomInformation.state.value();
			if (state_value.starts_with("@") && state_value.size() > 1)
			{
				const auto loc_string = state_value.substr(1);
				const auto value = Game::UI_SafeTranslateString(loc_string.data());
				discord_presence.state = value;
			}
			else discord_presence.state = state_value.data();
		}
		else
		{
			if (difficulty.data()) discord_presence.state = Game::UI_SafeTranslateString(difficulty.data());
			else discord_presence.state = nullptr;
		}

		if (!DiscordCustomInformation.button.first.empty() && !DiscordCustomInformation.button.second.empty())
		{
			const auto& buttonLabel = DiscordCustomInformation.button.first;
			const auto& buttonUrl = DiscordCustomInformation.button.second;
			if (buttonLabel.starts_with("@") && buttonLabel.size() > 1)
			{
				const auto loc_string = buttonLabel.substr(1);
				const auto value = Game::UI_SafeTranslateString(loc_string.data());
				discord_presence.button2Label = value;
			}
			else discord_presence.button2Label = buttonLabel.data();

			discord_presence.button2Url = buttonUrl.data();
		}
		else
		{
			discord_presence.button2Label = nullptr;
			discord_presence.button2Url = nullptr;
		}

		if (!discord_presence.startTimestamp)
		{
			discord_presence.startTimestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}

		Discord_UpdatePresence(&discord_presence);
	}

	static void ready([[maybe_unused]] const DiscordUser* request)
	{
		ZeroMemory(&discord_presence, sizeof(discord_presence));

		discord_presence.instance = 1;
		discord_presence.state = "";
		discord_presence.button1Label = "About IW3SP-MOD";
		discord_presence.button1Url = "https://gitea.com/JerryALT/iw3sp_mod";

		Discord_UpdatePresence(&discord_presence);
	}

	static void Errored(const int error_code, const char* message)
	{
		Game::Com_Printf(0, "Discord: (%i) %s", error_code, message);
	}

	void Discord::DiscordInit()
	{
		if (EnableDiscord_RPC && !Discord_Presence_Initialized)
		{
			DiscordEventHandlers handlers;
			ZeroMemory(&handlers, sizeof(handlers));
			handlers.ready = ready;
			handlers.errored = Errored;
			handlers.disconnected = Errored;
			handlers.joinGame = nullptr;
			handlers.spectateGame = nullptr;
			handlers.joinRequest = nullptr;

			Discord_Initialize("1088866597940248626", &handlers, 1, nullptr);

			Discord_Presence_Initialized = true;

			Scheduler::Loop(Discord::DiscordUpdate, Scheduler::Pipeline::MAIN, 5s);
		}
	}

	void Discord::DiscordShutdown()
	{
		if (Discord_Presence_Initialized)
		{
			Discord_Shutdown();
			Discord_Presence_Initialized = false;
		}
	}

	Discord::Discord()
	{
		Discord::DiscordInit();

		GSC::AddFunction("setdiscorddetails", []
		{
			if (Game::Scr_GetNumParam() != 1)
			{
				Game::Scr_Error("setdiscorddetails ( <string> detail ) requires 1 string parameter");
				return;
			}

			const char* details = Game::Scr_GetString(0);
			DiscordCustomInformation.details.emplace(details);
			Scheduler::Once(Discord::DiscordUpdate, Scheduler::Pipeline::ASYNC);
			Game::Scr_AddInt(true);
		}, false);

		GSC::AddFunction("setdiscordstate", []
		{
			if (Game::Scr_GetNumParam() != 1)
			{
				Game::Scr_Error("setdiscordstate ( <string> state ) requires 1 string parameter");
				return;
			}

			const char* state = Game::Scr_GetString(0);
			DiscordCustomInformation.state.emplace(state);
			Scheduler::Once(Discord::DiscordUpdate, Scheduler::Pipeline::ASYNC);
			Game::Scr_AddInt(true);
		}, false);

		GSC::AddFunction("setbuttoninformation", []
		{
			const char* label = Game::Scr_GetString(0);
			const char* url = Game::Scr_GetString(1);

			if (strlen(label) > 32)
			{
				Game::Scr_Error("URL label maximum is 32 characters");
				return;
			}

			DiscordCustomInformation.button.first = label;
			DiscordCustomInformation.button.second = url;
			Scheduler::Once(Discord::DiscordUpdate, Scheduler::Pipeline::ASYNC);
			Game::Scr_AddInt(true);
		}, false);

		GSC::AddFunction("setdiscordimage", []
		{
			if (Game::Scr_GetNumParam() != 1)
			{
				Game::Scr_Error("setdiscordimage ( <string> url ) requires 1 string parameter");
				return;
			}

			const char* imageLink = Game::Scr_GetString(0);
			DiscordCustomInformation.large_image_key.emplace(imageLink);
			Scheduler::Once(Discord::DiscordUpdate, Scheduler::Pipeline::ASYNC);
			Game::Scr_AddInt(true);
		}, false);
	}

	Discord::~Discord()
	{
		Discord::DiscordShutdown();
	}
}