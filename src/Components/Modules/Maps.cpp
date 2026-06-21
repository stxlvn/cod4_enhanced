#include "STDInc.hpp"

namespace Components
{
	void Maps::DB_LoadLevelXAssets(char* zoneName, unsigned int zoneFlags)
	{
		Game::XZoneInfo zoneInfo[2];
		char fileName[256];
		int zoneCount = 0;
		vsprintf(fileName, "%s", zoneName);

		if (FastFiles::Exists(fileName))
		{
			const char* levelPatchZoneName = Utils::String::VA("%s_patch", fileName);
			if (FastFiles::Exists(levelPatchZoneName))
			{
				zoneInfo[zoneCount].name = levelPatchZoneName;
				zoneInfo[zoneCount].allocFlags = Game::XZONE_FLAGS::XZONE_LOAD;
				zoneInfo[zoneCount++].freeFlags = Game::XZONE_FLAGS::XZONE_LOAD_FREE;
			}
			else
			{
				zoneInfo[zoneCount].name = nullptr;
				zoneInfo[zoneCount].allocFlags = Game::XZONE_FLAGS::XZONE_LOAD;
				zoneInfo[zoneCount++].freeFlags = Game::XZONE_FLAGS::XZONE_LOAD_FREE;
			}

			zoneInfo[zoneCount].name = fileName;
			zoneInfo[zoneCount].allocFlags = Game::XZONE_FLAGS::XZONE_MAP;
			zoneInfo[zoneCount++].freeFlags = Game::XZONE_FLAGS::XZONE_MAP_FREE;

			Game::DB_LoadXAssets(zoneInfo, zoneCount, false);
			Game::R_BeginRemoteScreenUpdate();
			WaitForSingleObject(Game::databaseCompletedEvent, 0xFFFFFFFF);
			Game::R_EndRemoteScreenUpdate();
		}
		else
		{
			Game::Com_Error(1, "The Fastfile '%s' does not exist\n", fileName);
		}
	}

	std::string mpsp_map_name;
	char* mpsp_mapents_buffer = nullptr;

	//xoxor4d module
	//https://github.com/xoxor4d/iw3xo-dev/blob/80fe9ffd2cec4b9c683f78a0f5171aaa586de334/src/components/modules/_map.cpp#L64
	void Maps::mpsp_map_replacement_mapents(Game::clipMap_t * cm)
	{
		const auto& fs_basepath = Dvars::Functions::Dvar_FindVar("fs_basepath");
		const auto& fs_game = Dvars::Functions::Dvar_FindVar("fs_game");

		if (fs_basepath && fs_game)
		{
			std::string mod = fs_game->current.string;
			Utils::String::Replace(mod, "/", "\\"s);
			std::string base_path = fs_basepath->current.string + "\\"s + mod + "\\mapents\\"s;

			if (std::filesystem::exists(base_path))
			{
				//Reset the buffer when map restart with exists .ents file
				//mpsp_mapents_buffer = nullptr;
				std::ifstream mapents;
				mapents.open(base_path + mpsp_map_name + ".ents", std::ios::in | std::ios::binary);

				if (!mapents.is_open())
				{ 
					Game::Com_Printf(0, Utils::String::VA("Failed to open mapents file: %s.ents", mpsp_map_name.c_str()));
					return;
				}

				mapents.ignore(std::numeric_limits<std::streamsize>::max());
				const auto length = static_cast<size_t>(mapents.gcount());

				mapents.clear();
				mapents.seekg(0, std::ios_base::beg);

				if (mpsp_mapents_buffer)
				{
					mpsp_mapents_buffer = (char*)realloc(mpsp_mapents_buffer, length + 1);
				}
				else
				{
					mpsp_mapents_buffer = (char*)malloc(length + 1);
				}

				mapents.read(mpsp_mapents_buffer, length);
				mapents.close();

				mpsp_mapents_buffer[length] = 0;

				Maps::mpsp_mapents_original = cm->mapEnts->entityString;
				cm->mapEnts->entityString = mpsp_mapents_buffer;
			}
		}
	}

	void __declspec(naked) map_replacement_mapents_stub()
	{
		const static uint32_t retn_addr = 0x452048;
		__asm
		{
			pushad;
			mov		eax, [eax];
			push	eax;
			call	Maps::mpsp_map_replacement_mapents;
			add		esp, 4;
			popad;

			mov     esi, eax;
			mov     eax, [eax];
			push    eax;
			jmp		retn_addr;
		}
	}
	

	void mpsp_get_bsp_name(char* filename, int size, [[maybe_unused]] const char* original_format, const char* mapname)
	{
		mpsp_map_name = mapname;
		sprintf_s(filename, size, "maps/%s%s.d3dbsp", strstr(mapname, "mp_") ? "mp/" : "", mapname);
	}

	void __declspec(naked) mpsp_bsp_name_stub01()
	{
		const static uint32_t retn_addr = 0x420F11;
		__asm
		{
			push	esi;
			push	edi;
			call	mpsp_get_bsp_name;
			add		esp, 8;
			jmp		retn_addr;
		}
	}

	void __declspec(naked) mpsp_bsp_name_stub02()
	{
		const static uint32_t retn_addr = 0x427F6D;
		__asm
		{
			push	esi;
			push	edi;
			call	mpsp_get_bsp_name;
			add		esp, 8;
			jmp		retn_addr;
		}
	}

	void __declspec(naked) mpsp_bsp_name_stub03()
	{
		const static uint32_t retn_addr = 0x5C4709;
		__asm
		{
			push	esi; // size
			push	edi; // dest
			call	mpsp_get_bsp_name;
			add		esp, 8;
			jmp		retn_addr;
		}
	}

	void __declspec(naked) mpsp_bsp_name_stub04()
	{
		const static uint32_t retn_addr = 0x438239;
		__asm
		{
			push	esi;
			push	edi;
			call	mpsp_get_bsp_name;
			add		esp, 8;
			jmp		retn_addr;
		}
	}

	void __declspec(naked) mpsp_bsp_name_stub05()
	{
		const static uint32_t retn_addr = 0x531C58;
		__asm
		{
			push	esi;
			push	edi; 
			call	mpsp_get_bsp_name;
			add		esp, 8;
			jmp		retn_addr;
		}
	}

	void __declspec(naked) mpsp_bsp_name_stub06()
	{
		const static uint32_t retn_addr = 0x5C704E;
		__asm
		{
			push	esi;
			push	edi;
			call	mpsp_get_bsp_name;
			add		esp, 8;
			jmp		retn_addr;
		}
	}

	void __declspec(naked) mpsp_bsp_name_stub07()
	{
		const static uint32_t retn_addr = 0x5C7188;
		__asm
		{
			push	esi;
			push	edi;
			call	mpsp_get_bsp_name;
			add		esp, 8;
			jmp		retn_addr;
		}
	}

	Maps::Maps()
	{
		Events::OnMapLoad([]
		{
			// thanks to Vlad for localization the ticker tape
			// this event it was created only to replace the material image to the custom
			// bad solution, but it's works
			// P.S: this event must replaced on the assethandler module, but module still doesn't works correctly
			const char* languageName = Game::SEH_GetLanguageName(Dvars::Functions::Dvar_FindVar("loc_language")->current.unsignedInt);
			if (!strcmp(languageName, "russian"))
			{
				Game::Material* material = Game::DB_FindXAssetHeader(Game::ASSET_TYPE_MATERIAL, "minimap_tickertape").material;
				Game::GfxImage* image = Game::DB_FindXAssetHeader(Game::ASSET_TYPE_IMAGE, "minimap_tickertape_ru").image;
				material->textureTable->u.image = image;
			}

			// Replace the 'pause' key on the 'togglemenu' key for all localizations!
			std::string localizeHint = LocalizedStrings::Get("KILLHOUSE_HINT_CHECK_OBJECTIVES_PAUSED");
			Utils::String::Replace(localizeHint, "pause", "togglemenu");
			LocalizedStrings::Set("KILLHOUSE_HINT_CHECK_OBJECTIVES_PAUSED", localizeHint.data());
		});

		Utils::Hook(0x5C7089, DB_LoadLevelXAssets, HOOK_CALL).install()->quick();
		Utils::Hook(0x452043, map_replacement_mapents_stub, HOOK_JUMP).install()->quick();

		// Fix the map directory for reading the maps/mp/%s.d3dbsp
		Utils::Hook(0x420F0C, mpsp_bsp_name_stub01, HOOK_JUMP).install()->quick();
		Utils::Hook(0x427F68, mpsp_bsp_name_stub02, HOOK_JUMP).install()->quick();
		Utils::Hook(0x438234, mpsp_bsp_name_stub04, HOOK_JUMP).install()->quick();
		Utils::Hook(0x531C53, mpsp_bsp_name_stub05, HOOK_JUMP).install()->quick();
		Utils::Hook(0x5C4704, mpsp_bsp_name_stub03, HOOK_JUMP).install()->quick();
		Utils::Hook(0x5C7049, mpsp_bsp_name_stub06, HOOK_JUMP).install()->quick();
		Utils::Hook(0x5C7183, mpsp_bsp_name_stub07, HOOK_JUMP).install()->quick();
	}

	Maps::~Maps()
	{
	}
}