#include "STDInc.hpp"
#include "CoopQA.hpp"

// QA script runner for automated test runs: the file named by the environment variable COD4COOP_QA, one step per line.
//   wait <ms>        real time
//   waitlevel <ms>   until a level runs and level.time >= ms
//   anything else    a console command (e.g. "screenshotJPEG", "quit")
// Lines starting with '#' are comments. Every step is written to cod4coop\coop.log.

namespace Components
{
	namespace
	{
		std::vector<std::string> steps;
		size_t next = 0;
		ULONGLONG waitUntil = 0;
		int waitLevel = -1;
		int waitMenu = -1;
		ULONGLONG waitMenuUntil = 0;
		bool started = false;

		int LevelTimeIfRunning()
		{
			const auto* running = Dvars::Functions::Dvar_FindVar("sv_running");
			if (!running || !running->current.enabled)
				return -1;
			return *reinterpret_cast<int*>(0xE18E50);
		}

		void Step()
		{
			if (next >= steps.size())
				return;
			if (waitUntil && GetTickCount64() < waitUntil)
				return;
			waitUntil = 0;
			if (waitLevel >= 0)
			{
				if (LevelTimeIfRunning() < waitLevel)
					return;
				Coop::Log("qa: level.time %d reached", LevelTimeIfRunning());
				waitLevel = -1;
			}
			if (waitMenu >= 0)
			{
				const int type = *reinterpret_cast<int*>(0x1E209B4); // g_currentMenuType (5 = pregame / briefing)
				if (type != waitMenu && GetTickCount64() < waitMenuUntil)
					return;
				Coop::Log("qa: menu type %d %s", type, type == waitMenu ? "reached" : "NOT reached (timeout)");
				waitMenu = -1;
			}
			const std::string line = steps[next++];
			if (line.rfind("waitmenu ", 0) == 0)
			{
				waitMenu = std::atoi(line.c_str() + 9);
				waitMenuUntil = GetTickCount64() + 120000;
				Coop::Log("qa: wait for menu type %d", waitMenu);
				return;
			}
			if (line.rfind("wait ", 0) == 0)
			{
				waitUntil = GetTickCount64() + std::strtoul(line.c_str() + 5, nullptr, 10);
				return;
			}
			if (line.rfind("waitlevel ", 0) == 0)
			{
				waitLevel = std::atoi(line.c_str() + 10);
				Coop::Log("qa: wait for level.time %d", waitLevel);
				return;
			}
			Coop::Log("qa: %s", line.c_str());
			Game::Cbuf_AddText(0, (line + "\n").c_str());
			if (next >= steps.size())
				Coop::Log("qa: done");
		}
	}

	void CoopQA::Install()
	{
		char path[MAX_PATH] = {};
		if (!GetEnvironmentVariableA("COD4COOP_QA", path, sizeof(path)) || !path[0])
			return;
		std::ifstream f(path);
		std::string line;
		while (std::getline(f, line))
		{
			while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
				line.pop_back();
			if (line.empty() || line[0] == '#')
				continue;
			steps.push_back(line);
		}
		Coop::Log("qa: %zu steps from %s", steps.size(), path);
		Scheduler::Loop([]
		{
			if (!started)
			{
				started = true;
				Coop::Log("qa: start");
			}
			Step();
		}, Scheduler::Pipeline::MAIN);
	}
}
