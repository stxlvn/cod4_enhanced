#include "STDInc.hpp"

namespace Components
{
	bool Loader::Pregame = true;
	bool Loader::Postgame = false;
	bool Loader::Uninitializing = false;
	std::vector<Component*> Loader::Components;

	bool Loader::IsPregame()
	{
		return Pregame;
	}

	bool Loader::IsPostgame()
	{
		return Postgame;
	}

	bool Loader::IsUninitializing()
	{
		return Uninitializing;
	}

	void Loader::Initialize()
	{
		Pregame = true;
		Postgame = false;
		Uninitializing = false;
		Utils::Memory::GetAllocator()->clear();

		Register(new AssetHandler());
		Register(new Command());
		Register(new Exception());
		Register(new ServerCommand());
		Register(new UIScript());
		Register(new Config());
		Register(new Flags());
		Register(new FastFiles());
		Register(new Branding());
		Register(new FileSystem());
		Register(new Language());
		Register(new CommonPatch());
		Register(new Mods());
		Register(new Maps());
		Register(new D3D9Ex());
		Register(new RawMouse());
		Register(new Renderer());
		Register(new Scheduler());
		Register(new GSC());
		Register(new ScriptExtension());
		Register(new Movement());
		Register(new Weapons());
		// co-op fork: no Discord presence under IW3SP-MOD's application id
		//Register(new Discord());
		Register(new TextRenderer());
		Register(new LocalizedStrings());
		Register(new Toast());
		Register(new Achievements());
		Register(new Events());
		// co-op fork: never auto-update from upstream IW3SP-MOD (it would replace this game.dll)
		//Register(new Updater());
		Register(new Window());
		Register(new Sound());
		Register(new Gamepad());
		Register(new GUI());
		Register(new Markdown());
		Register(new Changelog());
		Register(new Workshop());
		Register(new Paintball());
		Register(new PlayerStats());
		Register(new Devgui());
		Register(new Coop());
		Register(new Fonts());

		Pregame = false;
		// Make sure preDestroy is called when the game shuts down
		Scheduler::OnGameShutdown(PreDestroy);
	}

	void Loader::Uninitialize()
	{
		Uninitializing = true;
		PreDestroyNoPostGame();

		std::reverse(Components.begin(), Components.end());
		for (auto component : Components)
		{
			delete component;
		}

		Components.clear();
		Utils::Memory::GetAllocator()->clear();
		Uninitializing = false;
	}

	void Loader::PreDestroy()
	{
		if (!Postgame)
		{
			Postgame = true;

			auto components = Components;

			std::reverse(components.begin(), components.end());
			for (auto component : components)
			{
				component->preDestroy();
			}
		}
	}

	void Loader::PreDestroyNoPostGame()
	{
		if (!Postgame)
		{
			auto components = Components;

			std::reverse(components.begin(), components.end());
			for (auto component : components)
			{
				component->preDestroy();
			}

			Postgame = true;
		}
	}

	void Loader::Register(Component* component)
	{
		if (component)
		{
			Components.push_back(component);
		}
	}
}