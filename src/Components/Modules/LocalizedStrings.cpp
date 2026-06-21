#include "STDInc.hpp"

namespace Components
{
	std::recursive_mutex LocalizedStrings::LocalizeMutex;
	std::unordered_map<std::string, Game::LocalizeEntry*> LocalizedStrings::LocalizeMap;

	std::optional<std::string> LocalizedStrings::PrefixOverride;
	std::function<void(Game::LocalizeEntry*)> LocalizedStrings::ParseCallback;

	void LocalizedStrings::OverrideLocalizeStrings()
	{
		// Clearing previous localizeMap objects when game has been initialized or restarted.
		// Otherwise, game will be replacing the localize entries even if you have you have a another localization.
		LocalizeMap.clear();

		const char* languageName = Game::SEH_GetLanguageName(Dvars::Functions::Dvar_FindVar("loc_language")->current.unsignedInt);

		LocalizedStrings::Set("IW3SP_MOD_LOC_PLATFORM_UI_SELECTBUTTON_XBOX", "^\x01\x32\x32\x0E""button_xbox_01");
		LocalizedStrings::Set("IW3SP_MOD_LOC_PLATFORM_UI_SELECTBUTTON_SONY", "^\x01\x32\x32\x0E""button_sony_01");
		LocalizedStrings::Set("IW3SP_MOD_LOC_PLATFORM_BACK_GAMEPAD_XBOX", "^\x01\x32\x32\x0E""button_xbox_02");
		LocalizedStrings::Set("IW3SP_MOD_LOC_PLATFORM_BACK_GAMEPAD_SONY", "^\x01\x32\x32\x0E""button_sony_02");

		if (!strcmp(languageName, "english"))
		{
			LocalizedStrings::Set("MENU_LAUNCH_WITHOUT_MODS", "Launch without Mods");
			LocalizedStrings::Set("SUBTITLE_AIRLIFT_MHP_MAKINGARUN31", "^2Outlaw Two-Five: ^7Command this is Outlaw Two-Five. Infantry is making a run for it. We are clear to land, over.");
			LocalizedStrings::Set("SUBTITLE_AIRLIFT_HQR_ENROUTE34", "^2Outlaw Two-Five: ^7Roger that command. Outlaw Two-Five is en route.");
			LocalizedStrings::Set("SUBTITLE_AIRLIFT_MHP_LEADTHEWAY86", "^2Outlaw Two-Five: ^7Deadly this is Outlaw. Al-Asad's got a nuke. We're buggin' out. Lead the way, over.");
			LocalizedStrings::Set("SUBTITLE_AIRLIFT_MHP_SMALLARMSFIRE93", "^2Outlaw Two-Five: ^7Command, I have a visual on the crash site. I see small arms fire coming from the cockpit. Request permission to initiate search and rescue, over.");
			LocalizedStrings::Set("SUBTITLE_AIRPLANE_SAS4_ARMEDBOMB14", "^2Charlie One-One: ^7Shit, someone's armed the bomb. We don't have much time. We've got to go - now.");
			LocalizedStrings::Set("SUBTITLE_AMBUSH_GAZ_DROPGUN82", "^2Gaz: ^7Drop the bloody gun! Now, drop it!");
			LocalizedStrings::Set("SUBTITLE_BLACKOUT_PRI_SMELLTHATGAZ231", "^2Captain Price: ^7Gaz, you smell that?");
			LocalizedStrings::Set("SUBTITLE_BLACKOUT_PRI_CUTPOWER61", "^2Captain Price: ^7Gaz, go around the back and cut the power. Soap, get ready!");
			LocalizedStrings::Set("SUBTITLE_BLACKOUT_PRI_LETSGO81", "^2Captain Price: ^7Let's go!");
			LocalizedStrings::Set("SUBTITLE_VASSAULT_RHP_WEHAVETARGET31", "^2Havoc Pilot: ^7This is 2-5, we have the target. Standby.");
			LocalizedStrings::Set("SUBTITLE_VASSAULT_RHP_ONTHEWAY32", "^2Havoc Pilot: ^72-5 here. We're on the way. Standby for air support.");
			LocalizedStrings::Set("SUBTITLE_VASSAULT_RHP_READYTOATTACK35", "^2Havoc Pilot: ^72-5 here. We are ready to attack and are standing by for new orders.");
			LocalizedStrings::Set("SUBTITLE_VILLAGEDEF_GAZ_TAKINTHEPISS7261", "^2Gaz: ^7Oh, he's gotta be takin' the piss? We just busted our arses to get to this LZ and now they want us to go all the way back down?!");
			LocalizedStrings::Set("SUBTITLE_KILLHOUSE_GAZ_RIFLEDOWNRANGE32", "^2Gaz: ^7Now aim your rifle down range.");
			LocalizedStrings::Set("SUBTITLE_KILLHOUSE_PRI_MUPPET1212", "^2Captain Price: ^7Right. What the hell kind of name is Soap, eh? How'd a muppet like you pass selection?");
			LocalizedStrings::Set("KILLHOUSE_ACCURACY_BONUS", "Accuracy bonus: -");
			LocalizedStrings::Set("KILLHOUSE_ACCURACY_BONUS_ZERO", "Accuracy bonus: -0.0");
			return;
		}
		else if (!strcmp(languageName, "german"))
		{
			LocalizedStrings::Set("MENU_RUSSIAN", "Russisch");
			LocalizedStrings::Set("SUBTITLE_CARGOSHIP_GRG_ROGERTHAT213", "^2Gaz: ^7Kapiert.");
			LocalizedStrings::Set("KILLHOUSE_ACCURACY_BONUS", "Prдzisionsbonus: -");
			LocalizedStrings::Set("KILLHOUSE_ACCURACY_BONUS_ZERO", "Prдzisionsbonus: -0.0");
			return;
		}
		else if (!strcmp(languageName, "russian"))
		{
			LocalizedStrings::Set("MENU_ENGLISH", "Английский");
			LocalizedStrings::Set("MENU_RECRUIT", "Новобранец");
			LocalizedStrings::Set("MENU_REGULAR", "Рядовой");
			LocalizedStrings::Set("MENU_HARDENED", "Закалённый");
			LocalizedStrings::Set("SUBTITLE_BLACKOUT_PRI_SWITCHTOSNIPER411", "^2Капитан Прайс: ^7Соуп, бери снайперку.");
			LocalizedStrings::Set("SUBTITLE_BLACKOUT_PRI_MAKEITQUICK4641", "^2Капитан Прайс: ^7Быстрее, Комаров. Мне нужен этот шпион...");
			LocalizedStrings::Set("SCRIPT_INTELLIGENCE_FOUND", "Вы нашли вражеские разведданные.");
			LocalizedStrings::Set("SCRIPT_INTELLIGENCE_OF_FOURTYFIVE", "Разведданных найдено &&1/45.");
			LocalizedStrings::Set("SCRIPT_INTELLIGENCE_OF_SIXTY", "Разведданных найдено &&1/60.");
			LocalizedStrings::Set("SCRIPT_INTELLIGENCE_OF_THIRTY", "Разведданных найдено &&1/30.");
			LocalizedStrings::Set("SCRIPT_INTELLIGENCE_ONEREMAINING", "Остался &&1 разведданное.");
			LocalizedStrings::Set("SCRIPT_INTELLIGENCE_PTS", "Разведданных найдено: &&1.");
			LocalizedStrings::Set("SCRIPT_INTELLIGENCE_REMAINING", "Разведданных осталось: &&1.");
			LocalizedStrings::Set("SCRIPT_PLATFORM_LAUNCHFACILITY_A_HINT_SMOKE", "Бросьте дымовую гранату ^3[{+smoke}]^7, чтобы незаметно приблизиться к вражеской технике.");
			LocalizedStrings::Set("PLATFORM_SAVE_AND_QUIT", "Выйти в главное меню");
			LocalizedStrings::Set("PLATFORM_PICKUPNEWWEAPON", "Нажмите ^3&&1^7, чтобы подобрать");
			LocalizedStrings::Set("PLATFORM_SWAPWEAPONS", "Нажмите ^3&&1^7, чтобы сменить оружие и взять");
			LocalizedStrings::Set("KILLHOUSE_ACCURACY_BONUS", "Премия за точность: -");
			LocalizedStrings::Set("KILLHOUSE_ACCURACY_BONUS_ZERO", "Премия за точность: -0.0");
			return;
		}
		else
			return;
	}

	void LocalizedStrings::Set(const std::string& psLocalReference, const std::string& psNewString)
	{
		std::lock_guard _(LocalizeMutex);
		Utils::Memory::Allocator* allocator = Utils::Memory::GetAllocator();

		auto key = psLocalReference;
		if (PrefixOverride.has_value())
		{
			key.insert(0, PrefixOverride.value());
		}

		if (LocalizeMap.contains(key))
		{
			auto* entry = LocalizeMap[key];

			const auto* newStaticValue = allocator->duplicateString(psNewString);
			if (!newStaticValue) return;

			if (entry->value) allocator->free(entry->value);
			entry->value = newStaticValue;

			LocalizedStrings::SaveParseOutput(entry);

			return;
		}

		auto* entry = allocator->allocate<Game::LocalizeEntry>();
		if (!entry) return;

		entry->name = allocator->duplicateString(key);
		if (!entry->name)
		{
			allocator->free(entry);
			return;
		}

		entry->value = allocator->duplicateString(psNewString);
		if (!entry->value)
		{
			allocator->free(entry->name);
			allocator->free(entry);
			return;
		}

		LocalizedStrings::SaveParseOutput(entry);

		LocalizeMap[key] = entry;
	}

	void LocalizedStrings::SaveParseOutput(Game::LocalizeEntry* asset)
	{
		if (ParseCallback)
		{
			ParseCallback(asset);
		}
	}

	const char* LocalizedStrings::Get(const char* key)
	{
		Game::LocalizeEntry* entry = nullptr;
		{
			std::lock_guard _(LocalizeMutex);

			if (LocalizeMap.contains(key))
			{
				entry = LocalizeMap[key];
			}
		}

		if (!entry || !entry->value)
		{
			entry = Game::DB_FindXAssetHeader(Game::XAssetType::ASSET_TYPE_LOCALIZE_ENTRY, key).localize;
		}

		if (entry && entry->value)
		{
			return entry->value;
		}

		return key;
	}

	void __stdcall LocalizedStrings::SetStringStub(const char* psLocalReference, const char* psNewString, [[maybe_unused]] int bSentenceIsEnglish)
	{
		LocalizedStrings::Set(psLocalReference, psNewString);
	}

	LocalizedStrings::LocalizedStrings()
	{
		AssetHandler::OnFind(Game::XAssetType::ASSET_TYPE_LOCALIZE_ENTRY, [](Game::XAssetType, const std::string& name)
		{
			Game::XAssetHeader header = { nullptr };
			std::lock_guard _(LocalizeMutex);

			if (const auto itr = LocalizeMap.find(name); itr != LocalizeMap.end())
			{
				header.localize = itr->second;
			}

			return header;
		});

		// Resolving hook
		Utils::Hook(0x55BEA0, LocalizedStrings::Get, HOOK_JUMP).install()->quick();
		Utils::Hook(0x55CFE9, LocalizedStrings::SetStringStub, HOOK_CALL).install()->quick();
	}

	LocalizedStrings::~LocalizedStrings()
	{
		LocalizeMap.clear();
	}
}