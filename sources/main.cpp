#include <filesystem>
#include <functional>
#include <memory>
#include <noi_engine/core/assets/asset_provider.hpp>
#include <noi_engine/core/assets/filesystem_asset_provider.hpp>
#include <noi_engine/sdl/sdl_game.hpp>
#include <string>
#include <unordered_map>

#include "game_config.hpp"

using namespace noi_engine;

static const std::unordered_map<
	std::string, std::function<std::shared_ptr<asset_provider>()>>
	ASSET_PROVIDERS{ { "filesystem", []() -> std::shared_ptr<asset_provider>
					   {
						   return std::make_shared<filesystem_asset_provider>(
							   std::filesystem::current_path());
					   } } };

auto main(const int argc, char** argv) -> int
{
	const auto provider = game_config::get_asset_provider(ASSET_PROVIDERS);

	sdl_game game(std::string(game_config::name), game_config::window_width,
				  game_config::window_height, provider);

	return game.run(argc, argv);
}