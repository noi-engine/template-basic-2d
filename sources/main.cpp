#include <noi_engine/core/assets/asset_provider.hpp>
#include <noi_engine/core/assets/filesystem_asset_provider.hpp>
#include <noi_engine/sdl/sdl_game.hpp>
#include <unordered_map>

#include "game_config.hpp"

static const std::unordered_map<std::string,
								std::shared_ptr<noi_engine::asset_provider>>
	ASSET_PROVIDERS{ { "filesystem",
					   std::make_shared<noi_engine::filesystem_asset_provider>(
						   std::filesystem::current_path()) } };

auto main(const int argc, char** argv) -> int
{
	const auto provider = game_config::get_asset_provider(ASSET_PROVIDERS);

	noi_engine::sdl_game game(std::string(game_config::name),
							  game_config::window_width,
							  game_config::window_height, provider);

	return game.run(argc, argv);
}