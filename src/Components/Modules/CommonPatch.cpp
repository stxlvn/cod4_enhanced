#include "STDInc.hpp"

namespace Components
{
	void CommonPatch::SV_CheatsStub()
	{
		Dvars::Override::DvarBoolOverride("sv_cheats",
			Dvars::Functions::Dvar_FindVar("sv_allowCheats")->current.enabled ? true : false,
			Dvars::Functions::Dvar_FindVar("sv_allowCheats")->current.enabled ? Game::none : Game::cheat_protected);
	}

	void CommonPatch::DB_LoadCommonFastFiles()
	{
		int i = 0;
		Game::XZoneInfo XZoneInfoStack[6];

		XZoneInfoStack[i].name = "code_post_gfx";
		XZoneInfoStack[i].allocFlags = Game::XZONE_FLAGS::XZONE_POST_GFX;
		XZoneInfoStack[i].freeFlags = Game::XZONE_FLAGS::XZONE_POST_GFX_FREE;
		++i;

		const char* languageName = Game::SEH_GetLanguageName(Dvars::Functions::Dvar_FindVar("loc_language")->current.unsignedInt);
		if (FastFiles::Exists(Utils::String::VA("%s_iw3sp_mod_patch", languageName)))
		{
			XZoneInfoStack[i].name = Utils::String::VA("%s_iw3sp_mod_patch", languageName);
			XZoneInfoStack[i].allocFlags = Game::XZONE_FLAGS::XZONE_MOD;
			XZoneInfoStack[i].freeFlags = Game::XZONE_FLAGS::XZONE_MOD_FREE;
			++i;

			//override the exists the fonts from code_post_gfx
			if (Language::GetCurrentLanguage() == "russian")
			{
				Game::DB_LoadXAssets(XZoneInfoStack, i, false);
				Game::R_BeginRemoteScreenUpdate();
				WaitForSingleObject(Game::databaseCompletedEvent, 0xFFFFFFFF);
				Game::R_EndRemoteScreenUpdate();
				i = 0;

				Utils::Hook::Set<const char*>(0x42033E, "decode_characters_64px");
				Utils::Hook::Set<const char*>(0x420365, "decode_characters_64px_glow");
			}
			else
			{
				Utils::Hook::Set<const char*>(0x42033E, "decode_characters");
				Utils::Hook::Set<const char*>(0x420365, "decode_characters_glow");
			}
		}

		if (FastFiles::Exists("iw3sp_mod"))
		{
			XZoneInfoStack[i].name = "iw3sp_mod";
			XZoneInfoStack[i].allocFlags = Game::XZONE_FLAGS::XZONE_MOD;
			XZoneInfoStack[i].freeFlags = Game::XZONE_FLAGS::XZONE_MOD_FREE;
			++i;
		}

		auto mod_exists = Utils::Hook::Call<char()>(0x45CC00)(); //DB_ModFileExists 0x45C920 from 1.0
		if (mod_exists)
		{
			// Unloading our mod except "loc_mod" file when mod.ff exists in mods/modName folder
			XZoneInfoStack[2].name = nullptr;
			XZoneInfoStack[2].allocFlags = Game::XZONE_FLAGS::XZONE_MOD;
			XZoneInfoStack[2].freeFlags = Game::XZONE_FLAGS::XZONE_MOD_FREE;


			XZoneInfoStack[i].name = "mod";
			XZoneInfoStack[i].allocFlags = Game::XZONE_FLAGS::XZONE_MOD;
			XZoneInfoStack[i].freeFlags = Game::XZONE_FLAGS::XZONE_MOD_FREE;
			++i;

			Game::DB_LoadXAssets(XZoneInfoStack, i, true);

			Game::R_BeginRemoteScreenUpdate();
			WaitForSingleObject(Game::databaseCompletedEvent, 0xFFFFFFFF);
			Game::R_EndRemoteScreenUpdate();
			i = 0;
		}

		XZoneInfoStack[i].name = "ui";
		XZoneInfoStack[i].allocFlags = Game::XZONE_FLAGS::XZONE_UI;
		XZoneInfoStack[i].freeFlags = Game::XZONE_FLAGS::XZONE_UI_FREE;
		++i;

		XZoneInfoStack[i].name = "common";
		XZoneInfoStack[i].allocFlags = Game::XZONE_FLAGS::XZONE_COMMON;
		XZoneInfoStack[i].freeFlags = Game::XZONE_FLAGS::XZONE_COMMON_FREE;
		++i;

		Game::DB_LoadXAssets(XZoneInfoStack, i, true);

		Game::R_BeginRemoteScreenUpdate();
		WaitForSingleObject(Game::databaseCompletedEvent, 0xFFFFFFFF);
		Game::R_EndRemoteScreenUpdate();

		Game::Com_Printf(0, "Game [^2Success^7]: main .ff files has been loaded.\n");

		Command::Add("unlockall", [](Command::Params*)
		{
			Command::Execute("seta mis_01 50");
		});

		Command::Add("resetstats", [](Command::Params*)
		{
			Command::Execute("seta mis_01 0");
			Command::Execute("seta mis_difficulty 0000000000000000000000000000000000000000");
			Command::Execute("seta cheat_points 0");
			Command::Execute("seta cheat_items_set1 0");
		});

		Command::Add("applyCheat", [](Command::Params*)
		{
			if (Dvars::Functions::Dvar_FindVar("sv_allowCheats")->current.enabled)
			{
				Command::Execute("seta thereisacow 1337", false);

				Utils::Hook::Set<BYTE>(0x587872, 0xEB); // Read only.
				Utils::Hook::Set<BYTE>(0x58788F, 0xEB); // Write protected.
				Utils::Hook::Set<BYTE>(0x5878AD, 0xEB); // Cheat protected.
			}
			else
			{
				Command::Execute("seta thereisacow 0", false);

				Utils::Hook::Set<BYTE>(0x587872, 0x74); // Read only.
				Utils::Hook::Set<BYTE>(0x58788F, 0x74); // Write protected.
				Utils::Hook::Set<BYTE>(0x5878AD, 0x75); // Cheat protected.
			}

			Dvars::Override::DvarBoolOverride("sv_cheats",
				Dvars::Functions::Dvar_FindVar("sv_allowCheats")->current.enabled ? true : false,
				Dvars::Functions::Dvar_FindVar("sv_allowCheats")->current.enabled ? Game::none : Game::cheat_protected);
		});

		Dvars::Override::DvarBoolOverride("developer_script", Game::none);
		Dvars::Override::DvarBoolOverride("r_ignorehwgamma", Game::saved);
		Dvars::Override::DvarBoolOverride("r_gpuSync", Game::saved);

		LocalizedStrings::OverrideLocalizeStrings();

#ifdef DEBUG
		if (Flags::HasFlag("dump"))
		{
			Command::Add("dumpLocStrings", [](Command::Params* params)
			{
				if (params->size() < 2)
				{
					Game::Com_Printf(0, "dump_localize_strings <menu_name>\n");
					return;
				}

				std::vector<Game::LocalizeEntry*> localize;
				std::ofstream dumpFile;

				FastFiles::EnumAssets(Game::ASSET_TYPE_LOCALIZE_ENTRY, [&localize](Game::XAssetHeader header)
				{
					localize.push_back(header.localize);
				}, false);

				// sort localize strings
				std::sort(localize.begin(), localize.end(), [](Game::LocalizeEntry* localize_start, Game::LocalizeEntry* localize_end)
				{
					return std::string_view(localize_start->name) < std::string_view(localize_end->name);
				});

				if (const auto& fs_basepath = Dvars::Functions::Dvar_FindVar("fs_basepath"); fs_basepath)
				{
					std::string base_path = fs_basepath->current.string;
					const char* languageName = Game::SEH_GetLanguageName(Dvars::Functions::Dvar_FindVar("loc_language")->current.unsignedInt);
					base_path += Utils::String::VA("\\dump\\%s\\localizedstrings\\", languageName);

					if (std::filesystem::create_directories(base_path))
					{
						Game::Com_Printf(0, "Created directory \"dump/%s/localizedstrings\"\n", languageName);
					}

					const std::string file_name = base_path + Utils::String::VA("%s.str", params->get(1));
					dumpFile.open(file_name.c_str());
					if (dumpFile.is_open())
					{
						dumpFile << "// dumped by IW3SP_MOD " << __TIMESTAMP__ << "\n";
						dumpFile << "\n";

						dumpFile << "// Note to translators:\n";
						dumpFile << "// If a sentence is the same in your language then please change it to ""#same""\n";
						dumpFile << "//\n";
						dumpFile << "// eg:\n";
						dumpFile << "// LANG_ENGLISH  \"HALT\"\n";
						dumpFile << "// LANG_GERMAN  \"#same\"\n";
						dumpFile << "//\n";
						dumpFile << "// (This is so we can tell which strings have been signed-off as ok to be the same words for QA\n";
						dumpFile << "//  and because we do not store duplicate strings, which will then get exported again next time\n";
						dumpFile << "//  as being untranslated.)\n";
						dumpFile << "//\n";
						dumpFile << "VERSION             \"1\"\n";
						dumpFile << "CONFIG				 \"C:\\trees\\cod3\\cod3\\bin\\StringEd.cfg\"\n";
						dumpFile << "FILENOTES			 \"\"\n";
						dumpFile << "\n";

						for (std::size_t i = 0; i < localize.size(); i++)
						{
							if (Utils::String::StartsWith(localize[i]->name, Utils::String::ToUpper(params->get(1))))
							{
								//Utils::String::
								std::string localize_name = Utils::String::VA("%s", localize[i]->name);
								Utils::String::Replace(localize_name, Utils::String::ToUpper(params->get(1)) + "_", "");
								dumpFile << "REFERENCE           " << localize_name.c_str() << std::endl;
								dumpFile << "LANG_" << Utils::String::ToUpper(languageName) << "        " << Utils::String::VA("\"%s\"", localize[i]->value) << std::endl;
								dumpFile << "\n";
							}
						}

						dumpFile << "ENDMARKER\n";

						Game::Com_Printf(0, Utils::String::VA("Dumped localized strings to: %s", file_name.c_str()));
						dumpFile.close();
					}
				}
			});
		}
#endif

		TextRenderer::UpdateGameFontsAndText();
	}

	bool CommonPatch::CheckMilesFiles()
	{
		if (!Utils::IO::DirectoryExists("iw3sp_data/miles"))
		{
			MessageBoxA(nullptr, "The 'iw3sp_data/miles' folder does not exist.", nullptr, MB_ICONERROR);
			Utils::Library::Terminate();
			return false;
		}

		std::vector<std::string> requiredFiles = 
		{
			"milesEq.flt",
			"mssds3d.flt",
			"mssdsp.flt",
			"msseax.flt",
			"mssmp3.asi",
			"mssvoice.asi"
		};

		std::filesystem::path basePath = std::filesystem::path(Dvars::Functions::Dvar_FindVar("fs_basepath")->current.string) / "iw3sp_data" / "miles";
		const auto path = (basePath).generic_string();
		const auto files = Utils::IO::ListFiles(path);

		std::vector<std::string> unexpectedFiles;
		for (const auto& file : files)
		{
			auto fileName = std::filesystem::path(file).filename().string();
			if (std::find(requiredFiles.begin(), requiredFiles.end(), fileName) == requiredFiles.end())
			{
				unexpectedFiles.push_back(file);
			}
		}

		if (!unexpectedFiles.empty())
		{
			std::string errorMessage = "In Call of Duty 4: Modern Warfare 'iw3sp_data/miles' folder was spotted custom .asi files:\n";
			for (const auto& file : unexpectedFiles)
			{
				errorMessage += file + "\n";
			}
			errorMessage += "The 'IW3SP-MOD' client can't works property with custom .asi files. For run the 'IW3SP-MOD' client need remove the above .asi files.";
			MessageBoxA(nullptr, errorMessage.c_str(), nullptr, MB_ICONERROR);
			return false;
		}

		return true;
	}

	const char* __stdcall CommonPatch::AIL_set_redist_directory_Stub(const char* directory)
	{
		if (CheckMilesFiles())
		{
			return Utils::Hook::Call<const char* (__stdcall)(const char*)>(0x63F7EA)("iw3sp_data/miles");
		}
		else
		{
			Utils::Library::Terminate();
		}
	}

	void CommonPatch::R_Cinematic_BinkOpenStub1(char* buffer, size_t size, const char* /*format*/, const char* directory, const char* fileName, const char* videoFormat)
	{
		const char* languageName = Game::SEH_GetLanguageName(Dvars::Functions::Dvar_FindVar("loc_language")->current.unsignedInt);

		if (Utils::IO::FileExists(Utils::String::VA("%s\\iw3sp_data\\video\\%s.%s", directory, fileName, videoFormat)))
		{
			_snprintf_s(buffer, size, _TRUNCATE, "%s\\iw3sp_data\\video\\%s.%s", directory, fileName, videoFormat);
			return;
		}
		else if (Utils::IO::FileExists(Utils::String::VA("%s\\main\\video\\%s.%s", directory, fileName, videoFormat)))
		{
			_snprintf_s(buffer, size, _TRUNCATE, "%s\\main\\video\\%s.%s", directory, fileName, videoFormat);
			return;
		}
		else if (Utils::IO::FileExists(Utils::String::VA("%s\\main\\%s_video\\%s.%s", directory, languageName, fileName, videoFormat)))
		{
			_snprintf_s(buffer, size, _TRUNCATE, "%s\\main\\%s_video\\%s.%s", directory, languageName, fileName, videoFormat);
			return;
		}
		else if (Utils::IO::FileExists(Utils::String::VA("%s\\%s\\main\\video\\%s.%s", directory, Dvars::Functions::Dvar_FindVar("fs_game")->current.string, fileName, videoFormat)))
		{
			_snprintf_s(buffer, size, _TRUNCATE, "%s\\%s\\main\\video\\%s.%s", directory, Dvars::Functions::Dvar_FindVar("fs_game")->current.string, fileName, videoFormat);
			return;
		}
		else
		{
			_snprintf_s(buffer, size, _TRUNCATE, "%s\\%s\\main\\video\\%s\\%s.%s", directory, Dvars::Functions::Dvar_FindVar("fs_game")->current.string, languageName, fileName, videoFormat);
			return;
		}
	}

	void CommonPatch::Com_Quit_f_Stub()
	{
		Utils::Library::Terminate();
	}

	void CommonPatch::LanguageSetValueConfig(int langIndex)
	{
		Config::Set<std::string>("language", Game::SEH_GetLanguageName(langIndex));
	}

	void __declspec(naked) CommonPatch::UpdateLanguageStub()
	{
		const static uint32_t retn_addr = 0x5674FD;
		__asm
		{
			push	eax; // value
			call	Dvars::Functions::Dvar_SetIntByName;
			call	LanguageSetValueConfig;
			add		esp, 4;
			jmp		retn_addr;
		}
	}

	// It may need to be redone as 'Event' module, but i don't have much dvars with latched flag :<
	int CommonPatch::R_RegisterSunDvarsStub()
	{
		Dvars::console_menu_style = Dvars::Register::Dvar_RegisterBool("console_menu_style", "Enable/Disable menu style from console version", false, Game::saved | Game::latched);
		return Utils::Hook::Call<int()>(0x61D050)();
	}

	Game::dvar_s* CommonPatch::RegisterConColor(const char* dvarName, float r, float g, float b, float a, float min, float max, unsigned __int16 flags, const char* description)
	{
		static struct
		{
			const char* name;
			float color[4];
		} patchedColors[] =
		{
			{ "con_inputBoxColor",     { 0.15f, 0.15f, 0.15f, 1.00f } },
			{ "con_inputHintBoxColor", { 0.10f, 0.10f, 0.10f, 1.00f } },
			{ "con_outputWindowColor", { 0.10f, 0.10f, 0.10f, 0.75f } },
			{ "con_outputBarColor",    { 0.10f, 0.10f, 0.10f, 1.00f } },
			{ "con_outputSliderColor", { 0.10f, 0.10f, 0.10f, 1.00f } },
		};

		for (std::size_t i = 0; i < ARRAYSIZE(patchedColors); ++i)
		{
			if (std::strcmp(dvarName, patchedColors[i].name) == 0)
			{
				r = patchedColors[i].color[0];
				g = patchedColors[i].color[1];
				b = patchedColors[i].color[2];
				a = patchedColors[i].color[3];
				break;
			}
		}

		return Dvars::Register::Dvar_RegisterVec4(dvarName, description, r, g, b, a, min, max, flags);
	}

	void __declspec(naked) CommonPatch::RegisterConColorStub1()
	{
		const static uint32_t retn_addr = 0x439948;
		__asm
		{
			push	edi;
			call	RegisterConColor;
			add		esp, 4;
			jmp		retn_addr;
		}
	}

	void __declspec(naked) CommonPatch::RegisterConColorStub2()
	{
		const static uint32_t retn_addr = 0x43998B;
		__asm
		{
			push	edi;
			call	RegisterConColor;
			add		esp, 4;
			jmp		retn_addr;
		}
	}

	void __declspec(naked) CommonPatch::RegisterConColorStub3()
	{
		const static uint32_t retn_addr = 0x4399CE;
		__asm
		{
			push	edi;
			call	RegisterConColor;
			add		esp, 4;
			jmp		retn_addr;
		}
	}

	void __declspec(naked) CommonPatch::RegisterConColorStub4()
	{
		const static uint32_t retn_addr = 0x439A17;
		__asm
		{
			push	edi;
			call	RegisterConColor;
			add		esp, 4;
			jmp		retn_addr;
		}
	}

	void __declspec(naked) CommonPatch::RegisterConColorStub5()
	{
		const static uint32_t retn_addr = 0x439A60;
		__asm
		{
			push	edi;
			call	RegisterConColor;
			add		esp, 4;
			jmp		retn_addr;
		}
	}

	float CommonPatch::CG_DrawViewpos(float previous_y, float posX, float labelWidth)
	{
		const float color[4] = { 1.00f, 1.00f, 1.00f, 1.00f };
		const char* mapname = Dvars::Functions::Dvar_FindVar("mapname")->current.string;
		const char* text = Utils::String::VA("%s (%.1f %.1f %.1f) %.1f %.1f", mapname, Game::g_clients->ps.origin[0], Game::g_clients->ps.origin[1], Game::g_clients->ps.origin[2], Game::cgs->refdefViewAngles[1], Game::cgs->refdefViewAngles[0]);
		float final_pos = Game::CG_CornerDebugPrint(color, text, Game::scrPlaceFull, posX, previous_y, labelWidth, " viewpos") + previous_y;
		return final_pos;
	}

	float CommonPatch::CG_DrawDObjInfo(float previous_y, float posX, float labelWidth)
	{
		int objFreeCount = *(int*)(0xFC1918);
		const float color[4] = { 1.00f, 1.00f, 1.00f, 1.00f };
		const char* text = Utils::String::VA("%d/%d", 2048 - objFreeCount, 2048);
		float final_pos = Game::CG_CornerDebugPrint(color, text, Game::scrPlaceFull, posX, previous_y, labelWidth, " used DObjs") + previous_y;
		return final_pos;
	}

	float CommonPatch::CG_DrawEntitiesCountInfo(float previous_y, float posX, float labelWidth)
	{
		int level_num_entities = *(int*)(0xE18E24);
		const float color[4] = { 1.00f, 1.00f, 1.00f, 1.00f };
		const char* text = Utils::String::VA("%d/%d", level_num_entities, 2174);
		float final_pos = Game::CG_CornerDebugPrint(color, text, Game::scrPlaceFull, posX, previous_y, labelWidth, " total ents") + previous_y;
		return final_pos;
	}

	float CommonPatch::CG_DrawAICount(float previous_y, float posX, float labelWidth)
	{
		int level_actorCount = *(int*)(0xE18E64);
		const float color[4] = { 1.00f, 1.00f, 1.00f, 1.00f };
		const char* text = Utils::String::VA("%d/%d", level_actorCount, 32);
		float final_pos = Game::CG_CornerDebugPrint(color, text, Game::scrPlaceFull, posX, previous_y, labelWidth, " total AI") + previous_y;
		return final_pos;
	}

	void CommonPatch::EnhancedDebugInformation()
	{
		float x = Game::scrPlaceFull->virtualViewableMax[0] - Game::scrPlaceFull->virtualViewableMin[0] + Dvars::Functions::Dvar_FindVar("cg_debugInfoCornerOffset")->current.value;
		float y = Dvars::Functions::Dvar_FindVar("cg_debugInfoCornerOffset")->current.vector[1];
		float labelWidth = Dvars::Functions::Dvar_FindVar("cg_small_dev_string_fontscale")->current.value * (Game::R_TextWidth(" cg ms/frame", 0, Game::cgMedia->smallDevFont) * 0.75f);

		if(Dvars::cg_drawViewpos->current.enabled)
			y = CG_DrawViewpos(y, x, labelWidth);
		if (Dvars::Functions::Dvar_FindVar("cg_drawFPS")->current.integer)
		{
			y = Utils::Hook::Call<float(float)>(0x411510)(y); //CG_DrawFPS
			if (Dvars::Functions::Dvar_FindVar("cg_drawFPS")->current.integer > 1)
			{
				y = CG_DrawDObjInfo(y, x, labelWidth);
				y = CG_DrawEntitiesCountInfo(y, x, labelWidth);
				y = CG_DrawAICount(y, x, labelWidth);
			}
		}
	}

	void __declspec(naked) CommonPatch::CG_DrawFPSStub()
	{
		const static uint32_t retn_addr = 0x411167;
		__asm
		{
			mov     dword ptr ds:[0x1628E68], 0;
			call	EnhancedDebugInformation;
			jmp		retn_addr;
		}
	}

	Game::dvar_s* CommonPatch::fs_usedevdir_fix(const char* dvarName, int type, unsigned __int16 flags, float r, float g, float b, float a, float min, float max, const char* description)
	{
		// meme: https://imgur.com/a/tczYKGR
		return Dvars::Register::Dvar_RegisterBool(dvarName, description, true, Game::saved);
	}

	void __declspec(naked) CommonPatch::fs_usedevdir_register_stub()
	{
		const static uint32_t retn_addr = 0x57AC09;
		__asm
		{
			push	edi;
			call	fs_usedevdir_fix;
			add		esp, 4;
			jmp		retn_addr;
		}
	}

	CommonPatch::CommonPatch()
	{
		Events::OnDvarInit([]
		{
			Dvars::external_console = Dvars::Register::Dvar_RegisterBool("external_console", "Turn on/off the external console", false, Game::saved);

			// New fov control dvars
			Dvars::cg_fov = Dvars::Register::Dvar_RegisterFloat("cg_fov", "The field of view angle in degrees", 65.0f, 1.0f, 160.0f, Game::saved_flag);
			Dvars::cg_fovScale = Dvars::Register::Dvar_RegisterFloat("cg_fovScale", "Scale applied to the field of view", 1.0f, 0.2f, 2.0f, Game::saved);
			Dvars::cg_fovMin = Dvars::Register::Dvar_RegisterFloat("cg_fovMin", "The minimum possible field of view", 10.0f, 1.0f, 160.0f, Game::saved);

			// UI Debug without (developer 1 and CTRL+F11)
			Dvars::ui_debugMode = Dvars::Register::Dvar_RegisterBool("ui_debugMode", "Shows ui debug information on screen.", 0, 4u);
			// Misc
			Game::dvar_s* ui_skipMainLockout = Dvars::Register::Dvar_RegisterBool("ui_skipMainLockout", "", false, Game::none);
			Game::dvar_s* profile_unlock_all = Dvars::Register::Dvar_RegisterBool("profile_unlock_all", "", false, Game::saved);
			Game::dvar_s* cg_drawBrandingInfo = Dvars::Register::Dvar_RegisterBool("cg_drawBrandingInfo", "Show/Hide the branding text on the top-left screen", true, Game::saved);
			Game::dvar_s* cg_drawDamageFeedbackOption = Dvars::Register::Dvar_RegisterBool("cg_drawDamageFeedbackOption", "Display a marker when hitting enemies", false, Game::saved);
			Game::dvar_s* snd_hitsoundDisabled = Dvars::Register::Dvar_RegisterBool("snd_hitsoundDisabled", "Disable the hit indicator sound", true, Game::saved);
			Game::dvar_s* sv_allowCheats = Dvars::Register::Dvar_RegisterBool("sv_allowCheats", "Enable/Disable the game cheats", false, Game::saved);
			Dvars::allowCompactFolderForMod = Dvars::Register::Dvar_RegisterBool("allowCompactFolderForMod", "Enable/Disable compact loading of the mod files.", false, Game::saved);
			
			if (Game::DebugModeEnabled())
			{
				Dvars::cg_drawViewpos = Dvars::Register::Dvar_RegisterBool("cg_drawViewpos", "Draw viewpos", false, Game::saved);
			}
		});

		// Change the folder for loading files from miles
		Utils::Hook(0x591D15, AIL_set_redist_directory_Stub, HOOK_CALL).install()->quick();

		// Hook for correct works the latched dvars
 		Utils::Hook(0x5F68DB, R_RegisterSunDvarsStub, HOOK_CALL).install()->quick();

		Utils::Hook(0x4B83CF, SV_CheatsStub, HOOK_CALL).install()->quick();
		Utils::Hook::Nop(0x5899E8, 5);

		Utils::Hook::Set<const char*>(0x445667, "iw3sp_mod.exe"); //-startSingleplayer

		// console logo
		if (Utils::IO::FileExists("iw3sp_data/assets/console_logo.bmp"))
			Utils::Hook::Set<const char*>(0x59647A, "iw3sp_data/assets/console_logo.bmp");

		// splash logo
		if (Utils::IO::FileExists("iw3sp_data/assets/splash_logo.bmp"))
			Utils::Hook::Set<const char*>(0x595EC4, "iw3sp_data/assets/splash_logo.bmp");

		// g_mem limit expansion. Thanks to Nikolai for offsets.
		Utils::Hook::Set<DWORD>(0x534B3F, 0x20000000); //528mb
		Utils::Hook::Set<DWORD>(0x534B70, 0x20000000); //528mb

		// Increase hunkTotal from 10mb to 15mb (xoxor4d)
		Utils::Hook::Set<BYTE>(0x5800D9, 0xF0);

		// Increase g_copyInfo limit.
		Utils::Hook::Set<BYTE>(0x45B5C4, 0x16);

		// Fastfiles Hooking The Function
		Utils::Hook(0x5D94D0, DB_LoadCommonFastFiles, HOOK_CALL).install()->quick();

		Utils::Hook::Set<BYTE>(0x57AB0C, Game::none);
		Utils::Hook::Set<const char*>(0x567241, "vid_restart\n");
		Utils::Hook::Set<const char*>(0x567284, "vid_restart\n"); //for ClearMods

		// Making the separate video folder
		Utils::Hook(0x5D70DA, R_Cinematic_BinkOpenStub1, HOOK_CALL).install()->quick();

		Utils::Hook(0x5674F8, UpdateLanguageStub, HOOK_JUMP).install()->quick();

		// Com_Quit_f_stub for auto-update.
		Utils::Hook(0x595324, Com_Quit_f_Stub, HOOK_CALL).install()->quick();

		// Increase fps cap to 250 for menus.
		Utils::Hook::Set<BYTE>(0x535881, 0xEB);
		Utils::Hook::Set<BYTE>(0x535884, 0xEB);

		// Fix fps on windows 10 (stuck at 500) :: sleep(1) to sleep(0) (xoxor4d)
		Utils::Hook::Set<BYTE>(0x53571F, 0x0);

		// Alt + Enter for switching from fullscreen2window mode or window2fullscreen without developer mode.
		Utils::Hook::Nop(0x596BBE, 2);

		// Changed function for notice that player activate godmode/notarget and etc.
		Utils::Hook::Set<const char*>(0x4AD009, "gm \"%s\""); // 'godmode ON/OFF'
		Utils::Hook::Set<const char*>(0x4AD099, "gm \"%s\""); // 'demigod ON/OFF'
		Utils::Hook::Set<const char*>(0x4AD129, "gm \"%s\""); // 'notarget ON/OFF'
		Utils::Hook::Set<const char*>(0x4AD1C4, "gm \"%s\""); // 'noclip ON/OFF'
		Utils::Hook::Set<const char*>(0x4AD264, "gm \"%s\""); // 'ufo ON/OFF'
		Utils::Hook::Set<const char*>(0x4AC66B, "gm \"GAME_MUSTBEALIVECOMMAND\""); // 'You must be alive to use this command.'
		Utils::Hook::Set<const char*>(0x4AC610, "gm \"GAME_CHEATSNOTENABLED\"");   // 'Cheats are not enabled on this server.'

		// Replaced the original .cfg name file.
		Utils::Hook::Set<const char*>(0x5309CE, CLIENT_CONFIG);
		Utils::Hook::Set<const char*>(0x532778, CLIENT_CONFIG);
		Utils::Hook::Set<const char*>(0x53544A, CLIENT_CONFIG);
		Utils::Hook::Set<const char*>(0x57B2BD, CLIENT_CONFIG);

		static float BuildVersionColor[] = { 0.00f, 0.80f, 0.00f, 1.00f };
		Utils::Hook::Set<float*>(0x447661, BuildVersionColor);
		Utils::Hook::Set<float*>(0x43A8D2, BuildVersionColor);

		Utils::Hook::Nop(0x4477A1, 4);

		Utils::Hook::Nop(0x44045E, 5); // MAX_PACKET_USERCMDS\n
		Utils::Hook::Nop(0x53A334, 5); // Ragdoll allocation failed, out of ragdoll bodies (obj %d)

		// ui_showList: changing the font.
		Utils::Hook::Set<DWORD>(0x57164F, 0x129ADB8);

		// Patch in-game console dvars
		Utils::Hook(0x439943, RegisterConColorStub1, HOOK_JUMP).install()->quick();
		Utils::Hook(0x439986, RegisterConColorStub2, HOOK_JUMP).install()->quick();
		Utils::Hook(0x4399C9, RegisterConColorStub3, HOOK_JUMP).install()->quick();
		Utils::Hook(0x439A12, RegisterConColorStub4, HOOK_JUMP).install()->quick();
		Utils::Hook(0x439A5B, RegisterConColorStub5, HOOK_JUMP).install()->quick();

		// Fix the 'fs_usedevdir' skill issue
		/*
		This hook can cause crashes for some users.
		*/
		//Utils::Hook(0x57AC04, fs_usedevdir_register_stub, HOOK_JUMP).install()->quick();

		if (Game::DebugModeEnabled())
		{
			// Additional debug information
			Utils::Hook::Nop(0x411140, 10);
			Utils::Hook(0x411140, CG_DrawFPSStub, HOOK_JUMP).install()->quick();
		}

		// Mouse fix
		Utils::Hook::Nop(0x59681C, 8);
		Scheduler::Loop([]
		{
			SetThreadExecutionState(ES_DISPLAY_REQUIRED);
		}, Scheduler::Pipeline::MAIN);

		Scheduler::Loop([]
		{
			Utils::Hook::Set<bool>(0x1E209D4, (Dvars::ui_debugMode->current.enabled) ? 1 : 0);
		}, Scheduler::Pipeline::MAIN, 50ms);
	}
}