#pragma once
#include <rapidjson/document.h>

namespace Components
{
	class Workshop : public Component
	{
	public:
		struct Workshop_t
		{
			std::string mod_title;
			std::string mod_author;
			std::string mod_image_preview;
			std::string mod_description;
			std::string mod_external_download_link;
			std::string mod_type;
		};

		static void CreateWorkshop(Game::gui_menus_t& menu);
		Workshop();
		~Workshop();

	private:
		static std::optional<std::string>modsData;
		static Workshop_t workshopDataInfo;

		static void LoadModData();
	};
}