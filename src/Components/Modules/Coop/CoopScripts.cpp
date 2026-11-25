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

		Game::XAssetHeader __cdecl FindScriptRawfile(Game::XAssetType type, const char* name)
		{
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
			int n = PatchGetEntPlayer(text);
			if (!n)
			{
				cache.emplace(name, nullptr);
				return header;
			}
			auto p = std::make_unique<Patched>();
			p->name = name;
			p->text = std::move(text);
			p->raw = RawFile{ p->name.c_str(), static_cast<int>(p->text.size()), p->text.c_str() };
			Coop::Log("script patch %s: %d getent(player) -> getentarray(player)[0]", name, n);
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
