#include "STDInc.hpp"

namespace Components
{
	std::vector<std::string> Mods::ModList;

	void Mods::UI_LoadModsListStub()
	{
		Game::sharedUiInfo->modCount = 0;
		auto folder = Dvars::Functions::Dvar_FindVar("fs_basepath")->current.string + "\\mods"s;
		ModList = FileSystem::GetSysFileList(folder, "", true);
		Game::sharedUiInfo->modCount = ModList.size();
	}

	void Mods::RunModStub()
	{
		if ((unsigned int)Game::sharedUiInfo->modIndex < ModList.size())
		{
			if (Dvars::Functions::Dvar_FindVar("useFastFile")->current.enabled)
			{
				Game::R_BeginRemoteScreenUpdate();
				WaitForSingleObject(Game::databaseCompletedEvent, 0xFFFFFFFF);
				Game::R_EndRemoteScreenUpdate();
			}

			Dvars::Functions::Dvar_SetStringByName("fs_game", Utils::String::Format("mods/{}", ModList[Game::sharedUiInfo->modIndex].c_str()));
			Language::VideoSubtitlesStub();
			Config::GameWithMod = true;
			Config::CallExecFromCFG(true);
		}
	}

	void Mods::ClearModStub()
	{
		Dvars::Functions::Dvar_Reset(Game::DVAR_SOURCE_INTERNAL, Dvars::Functions::Dvar_FindVar("fs_game"));
		Language::VideoSubtitlesStub();
		Config::GameWithMod = false;
		Config::CallExecFromCFG(false);
	}

	const char* Mods::GetItemTextMods(unsigned int index/*ecx*/)
	{		
		return ModList[index].data();
	}

	void __declspec(naked) Mods::UI_FeederItemTextStub()
	{
		const static uint32_t savegame_jmp = 0x567769;

		__asm
		{
			push	eax;
			pushad;
			push	ecx;
			call	GetItemTextMods;
			add		esp, 4;
			mov		[esp + 0x20], eax
			popad;
			pop		eax

			retn;
		}
	}

	Mods::Mods()
	{
		// Change the original objects to owns
		Utils::Hook(0x567200, UI_LoadModsListStub, HOOK_CALL).install()->quick();
		Utils::Hook(0x567238, RunModStub, HOOK_CALL).install()->quick();
		Utils::Hook(0x56727E, ClearModStub, HOOK_CALL).install()->quick();

		Utils::Hook::Nop(0x56774B, 7);
		Utils::Hook(0x56774B, UI_FeederItemTextStub, HOOK_CALL).install()->quick();
	}

	Mods::~Mods()
	{
	}
}