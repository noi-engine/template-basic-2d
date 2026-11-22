//
// Created by ricka on 8/18/26.
//

#include "config.hpp"

#include <filesystem>

#include "game_config.hpp"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace
{
	constexpr auto RESOURCE_DIR = "resources";

	// The folder the game was launched from is the install folder, not the working directory:
	// a desktop launcher or a shortcut starts the game from anywhere.
	auto executable_directory() -> std::filesystem::path
	{
		try
		{
#if defined(_WIN32)
			std::wstring buffer(MAX_PATH, L'\0');
			while (true)
			{
				const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
				if (length == 0)
				{
					break;
				}
				if (length < buffer.size())
				{
					buffer.resize(length);
					return std::filesystem::path(buffer).parent_path();
				}
				buffer.resize(buffer.size() * 2);
			}
#elif defined(__APPLE__)
			std::uint32_t size = 0;
			_NSGetExecutablePath(nullptr, &size);
			std::string buffer(size, '\0');
			if (_NSGetExecutablePath(buffer.data(), &size) == 0)
			{
				return std::filesystem::weakly_canonical(buffer.c_str()).parent_path();
			}
#else
			return std::filesystem::read_symlink("/proc/self/exe").parent_path();
#endif
		}
		catch (const std::exception&)
		{
		}

		return std::filesystem::current_path();
	}

	auto resource_path() -> std::filesystem::path
	{
		return executable_directory() / RESOURCE_DIR;
	}

	auto make_source_loader() -> std::unique_ptr<noi_engine::resource_loader>
	{
		return std::make_unique<noi_engine::source_resource_loader>(resource_path());
	}

	auto make_compiled_loader() -> std::unique_ptr<noi_engine::resource_loader>
	{
		return std::make_unique<noi_engine::compiled_resource_loader>(resource_path());
	}
}

auto noi_engine_game::get_resource_loader()
	-> std::unique_ptr<noi_engine::resource_loader>
{
	if (game_config::resource_loader_name == "compiled")
	{
		return make_compiled_loader();
	}
	return make_source_loader();
}
