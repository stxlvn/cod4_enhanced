#pragma once

namespace Components
{
	// Russian game: the text drawn with the mod's fonts (bank.ttf, default.otf, defaultBold.otf in <game dir>\cod4coop\fonts)
	// instead of the generic Cyrillic ones; a port of MW2 Enhanced's fonts.c (see Fonts.cpp)
	class Fonts : public Component
	{
	public:
		Fonts();

	private:
		static void* Find(int type, const char* name);
		static void Collect(Game::XAssetHeader header, void* userdata);
		static std::vector<std::pair<std::string, void*>> Loaded;
		static void Tick();
		static void Waiting(const std::string& why);
	};
}
