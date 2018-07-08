#include "stdafx.h"

#include <Windows.h>
#include <ShellAPI.h>

#include <SADXModLoader.h>
#include <Trampoline.h>

#include "globals.h"
#include "input.h"
#include "player.h"

static sws::Address address { "localhost", 21790 };
static bool is_server = false;
static bool do_the_things = false;

static void __cdecl LoadLevel_r();
static Trampoline LoadLevel_t(0x004159A0, 0x004159A7, LoadLevel_r);
static void __cdecl LoadLevel_r()
{
	auto original = reinterpret_cast<decltype(LoadLevel_r)*>(LoadLevel_t.Target());
	original();

	if (globals::broker || !do_the_things)
	{
		return;
	}

	globals::broker = std::make_unique<PacketBroker>();

	if (is_server)
	{
		globals::broker->listen(address);
	}
	else
	{
		globals::broker->connect(address);
	}

	events::input_register();
	events::player_register();
}

void tick_start()
{
	if (!globals::broker)
	{
		return;
	}

	globals::broker->read();
}

void tick_end()
{
	if (!globals::broker)
	{
		return;
	}

	events::player_update();

	globals::broker->finalize();
}

static short OnTick();
static Trampoline OnTick_t(0x00413D40, 0x00413D45, OnTick);
static short OnTick()
{
	// tick start
	tick_start();

	const auto original = reinterpret_cast<decltype(OnTick)*>(OnTick_t.Target());
	const auto result = original();

	// tick end
	tick_end();

	return result;
}

extern "C"
{
	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer, nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0 };

	void __declspec(dllexport) __cdecl Init()
	{
		// Allow multi-instance
		WriteData<2>(reinterpret_cast<void*>(0x00789D18), static_cast<uint8_t>(0x90));

		// Enables WriteAnalogs for controllers >= 2 (3)
		uint8_t patch[3] = { 0x83u, 0xFFu, 0x04u };
		WriteData(reinterpret_cast<void*>(0x0040F180), static_cast<void*>(patch), 3);

		// Object patches
		WriteData<uint8_t>(reinterpret_cast<uint8_t*>(0x007A4DC4), 8); // Spring_Main
		WriteData<uint8_t>(reinterpret_cast<uint8_t*>(0x007A4FF7), 8); // SpringB_Main
		WriteData<uint8_t>(reinterpret_cast<uint8_t*>(0x0079F77C), 8); // SpringH_Main
		WriteData<uint8_t>(reinterpret_cast<uint8_t*>(0x004418B8), 8); // IsPlayerInsideSphere (could probably use a better name!)

		// Disables AI Tails
		WriteData<uint8_t>(reinterpret_cast<uint8_t*>(0x0047ED60), 0xC3);

		int argc = 0;
		const auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);

		for (int i = 1; i < argc; i++)
		{
			if ((!wcscmp(argv[i], L"--host") || !wcscmp(argv[i], L"-h")) && i + 1 < argc)
			{
				do_the_things = true;

				address.port = std::stoi(argv[++i]);
				is_server = true;
			}
			else if ((!wcscmp(argv[i], L"--connect") || !wcscmp(argv[i], L"-c")) && i + 1 < argc)
			{
				do_the_things = true;

				std::wstring ip_w = argv[++i];
				std::string ip(ip_w.begin(), ip_w.end());

				if (ip.empty())
				{
					continue;
				}

				const auto   npos  = std::string::npos;
				const size_t colon = ip.find_first_of(':');

				address.address = ip.substr(0, colon);

				if (colon != npos)
				{
					address.port = static_cast<sws::port_t>(std::stoi(ip.substr(colon + 1)));
				}

				is_server = false;
			}
		}

		LocalFree(argv);

		if ((is_server && !address.port) || (!is_server && address.address.empty()) || argc < 2)
		{
			do_the_things = false;
			return;
		}

		sws::Socket::initialize();

		auto addresses = sws::Address::get_addresses(address.address.c_str(), address.port);
		address = addresses[0];
	}

	void __declspec(dllexport) __cdecl OnFrame()
	{
		tick_end();
		tick_start();
	}

	void __declspec(dllexport) __cdecl OnExit()
	{
		sws::Socket::cleanup();
	}
}
