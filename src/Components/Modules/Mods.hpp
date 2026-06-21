#pragma once

namespace Components
{
	class Mods : public Component
	{
	public:
		Mods();
		~Mods();
	private:
		static std::vector<std::string> ModList;

		static void RunModStub();
		static void ClearModStub();
		static void UI_LoadModsListStub();
		static void UI_FeederItemTextStub();
		static const char* GetItemTextMods(unsigned int index/*ecx*/);
	};
}