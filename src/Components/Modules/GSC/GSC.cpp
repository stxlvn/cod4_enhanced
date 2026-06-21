#include "STDInc.hpp"

namespace Components
{
	std::unordered_map<std::string, int> GSC::ScriptMainHandles;
	std::unordered_map<std::string, int> GSC::ScriptInitHandles;

	std::unordered_map<std::string, Game::scr_function_t> GSC::CustomScrFunctions;
	std::unordered_map<std::string, Game::scr_method_t> GSC::CustomScrMethods;

	void GSC::AddFunction(const char* name, Game::xfunction_t func, int type)
	{
		Game::scr_function_t toAdd;
		toAdd.function = func;
		toAdd.developer = type;

		GSC::CustomScrFunctions.insert_or_assign(Utils::String::ToLower(name), toAdd);
	}

	void GSC::AddMethod(const char* name, Game::xmethod_t method, int developer)
	{
		Game::scr_method_t toAdd;
		toAdd.function = method;
		toAdd.developer = developer;

		GSC::CustomScrMethods.insert_or_assign(Utils::String::ToLower(name), toAdd);
	}

	void GSC::AddMethods()
	{
		GSC::AddMethod("AllowFire", [](Game::scr_entref_t entref)
		{
			const auto* ent = Game::GetPlayerEntity(entref);

			if (Game::Scr_GetInt(0))
			{
				Dvars::p_allowFire->current.enabled = true;
				Dvars::p_allowFire->latched.enabled = true;
			}
			else
			{
				Dvars::p_allowFire->current.enabled = false;
				Dvars::p_allowFire->latched.enabled = false;
			}
		}, false);

		GSC::AddMethod("IsReloading", [](Game::scr_entref_t entref)
		{
			const auto* ent = Game::GetPlayerEntity(entref);

			if (ent->client->ps.weaponstate >= Game::WEAPON_RELOADING && ent->client->ps.weaponstate <= Game::WEAPON_RELOAD_END)
				return Game::Scr_AddInt(true);
			else
				return Game::Scr_AddInt(false);
		}, false);

		GSC::AddMethod("IsSprinting", [](Game::scr_entref_t entref)
		{
			const auto* ent = Game::GetPlayerEntity(entref);
			if (ent->client->ps.pm_flags & Game::PMF_SPRINTING)
				return Game::Scr_AddInt(true);
			else
				return Game::Scr_AddInt(false);
		}, false);

		GSC::AddMethod("IsUsingNVG", [](Game::scr_entref_t entref)
		{
			const auto* ent = Game::GetPlayerEntity(entref);
			if (ent->client->ps.weaponstate >= Game::WEAPON_NIGHTVISION_WEAR && ent->client->ps.weaponstate <= Game::WEAPON_NIGHTVISION_REMOVE)
				return Game::Scr_AddInt(true);
			else
				return Game::Scr_AddInt(false);
		}, false);

		GSC::AddMethod("IsMantling", [](Game::scr_entref_t entref)
		{
			const auto* ent = Game::GetPlayerEntity(entref);
			if (ent->client->ps.pm_flags & Game::PMF_MANTLE)
				return Game::Scr_AddInt(true);
			else
				return Game::Scr_AddInt(false);
		}, false);

		GSC::AddMethod("IsSwapping", [](Game::scr_entref_t entref)
		{
			const auto* ent = Game::GetPlayerEntity(entref);
			if (ent->client->ps.weaponstate >= Game::WEAPON_DROPPING && ent->client->ps.weaponstate <= Game::WEAPON_DROPPING_QUICK)
				return Game::Scr_AddInt(true);
			else
				return Game::Scr_AddInt(false);
		}, false);
	}

	void GSC::AddFunctions()
	{
		GSC::AddFunction("Exec", []
		{
			const auto str = Game::Scr_GetString(0);

			if (str == nullptr)
			{
				Game::Scr_Error("^1Exec: Illegal parameter!\n");
				return;
			}

			Command::Execute(str);
		}, false);

		GSC::AddFunction("ToUpper", []
		{
			const auto str = Game::Scr_GetString(0);

			if (str == nullptr)
			{
				Game::Scr_Error("^1Scr_GetString: Illegal parameter!\n");
				return;
			}

			return Game::Scr_AddString(Utils::String::ToUpper(str).c_str());
		}, false);

		//Should create the function for all asset type? :thinking:
		GSC::AddFunction("AssetIsExisting", []
		{
			const auto asset_type = Game::Scr_GetInt(0);
			const auto asset_name = Game::Scr_GetString(1);

			if (asset_type < 0 || asset_type >= Game::XAssetType::ASSET_TYPE_COUNT)
			{
				Game::Scr_Error(Utils::String::VA("Invalid pool passed must be between [0, %d]\n", Game::XAssetType::ASSET_TYPE_COUNT - 1));
				return;
			}

			Game::XAssetType xasset_type = static_cast<Game::XAssetType>(asset_type);

			const auto asset = Game::DB_FindXAssetHeader(xasset_type, asset_name).data;
			return Game::Scr_AddInt(asset != nullptr ? 1 : 0);
		}, false);
	}

	Game::xmethod_t GSC::Player_GetMethod_Stub(const char** name)
	{
		auto got = GSC::CustomScrMethods.find(*name);

		if (got == GSC::CustomScrMethods.end())
			return Game::Player_GetMethod(name);

		return got->second.function;
	}

	Game::xfunction_t GSC::Scr_GetFunction_Stub(const char** name, int* isDev)
	{
		auto got = GSC::CustomScrFunctions.find(*name);

		if (got == GSC::CustomScrFunctions.end())
			return Game::Scr_GetFunction(name, isDev);

		*isDev = got->second.developer;
		return got->second.function;
	}

	void GSC::LoadScript(const std::string& path)
	{
		if (!Game::Scr_LoadScript(path.data()))
		{
			Game::Com_Printf(0, Utils::String::VA("The script %s encountered an error during the loading process.\n", path.data()));
			return;
		}

		Game::Com_Printf(0, Utils::String::VA("The script %s has been loaded successfully.\n", path.data()));

		const auto initHandle = Game::Scr_GetFunctionHandle(path.data(), "init");
		if (initHandle != 0)
		{
			ScriptInitHandles.insert_or_assign(path, initHandle);
		}

		const auto mainHandle = Game::Scr_GetFunctionHandle(path.data(), "main");
		if (mainHandle != 0)
		{
			ScriptMainHandles.insert_or_assign(path, mainHandle);
		}
	}

	void GSC::LoadScriptsFromFastFiles()
	{
		// Clear handles (from previous GSC loading session)
		ScriptMainHandles.clear();
		ScriptInitHandles.clear();

		//Support exclusively loading scripts from fast files.
		FastFiles::EnumAssets(Game::ASSET_TYPE_RAWFILE, [](Game::XAssetHeader header)
		{
			std::string name = header.rawfile->name;

			if (name.ends_with(".gsc") && name.starts_with("scripts/"))
			{
				// Scr_LoadScriptInternal will add the '.gsc' suffix so we remove it
				const std::string path = name.substr(0, name.size() - 4);
				std::string middlePath = path.substr(path.rfind('/', path.find_last_of('/') - 1) + 1, path.find_last_of('/') - path.rfind('/', path.find_last_of('/') - 1) - 1);

				// Scripts folder:
				// 'scripts/[name_script].gsc' - global scripts
				// 'scripts/[level_name]/[name_script].gsc' - scripts for the specific level.
				if (name.contains("scripts/"s + middlePath + "/"))
				{
					if (middlePath == Dvars::Functions::Dvar_FindVar("mapname")->current.string)
					{
						LoadScript(path);
					}
				}
				else
				{
					LoadScript(path);
				}
			}

		}, false);
	}

	void __declspec(naked) GSC::GScr_LoadScripts_Stub()
	{
		const static uint32_t GScr_LoadScriptsForEntities_func = 0x4C6680;
		const static uint32_t retn_addr = 0x4C6AF8;
		__asm
		{
			pushad;
			call	GSC::LoadScriptsFromFastFiles;
			popad;

			call	GScr_LoadScriptsForEntities_func;
			jmp		retn_addr;
		}
	}

	void GSC::G_RunFrame_Stub(int extent, int timeCap)
	{
		for (const auto& handle : ScriptInitHandles)
		{
			const auto thread = Game::Scr_ExecThread(handle.second);
			Game::Scr_FreeThread(static_cast<std::uint16_t>(thread));
		}

		Utils::Hook::Call<int(int, int)>(0x4BAB40)(extent, timeCap); //G_RunFrame
	}

	void GSC::Scr_LoadLevel_Stub()
	{
		for (const auto& handle : ScriptMainHandles)
		{
			const auto thread = Game::Scr_ExecThread(handle.second);
			Game::Scr_FreeThread(static_cast<std::uint16_t>(thread));
		}

		Utils::Hook::Call<void()>(0x4F2F50)(); //Path_AutoDisconnectPaths
	}

	GSC::GSC()
	{
		Utils::Hook(0x4DB7D7, Player_GetMethod_Stub, HOOK_CALL).install()->quick(); // Player_GetMethod
		Utils::Hook(0x5435F1, Scr_GetFunction_Stub, HOOK_CALL).install()->quick();  // Scr_GetFunction

		// Additional the folder 'scripts' for .GSC
		// Load handles
		Utils::Hook(0x4C6AF3, GScr_LoadScripts_Stub, HOOK_JUMP).install()->quick();
		// Exec handles
		Utils::Hook(0x4BA564, Scr_LoadLevel_Stub, HOOK_CALL).install()->quick();
		Utils::Hook(0x4BA58A, G_RunFrame_Stub, HOOK_CALL).install()->quick();

		GSC::AddFunctions();
		GSC::AddMethods();
	}


	GSC::~GSC()
	{
		CustomScrMethods.clear();
		CustomScrFunctions.clear();
	}
}