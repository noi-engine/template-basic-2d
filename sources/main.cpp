#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <noi_engine/core/sdl/sdl_game.hpp>
#include <noi_engine/core/resources/resource_loader.hpp>

#include "game_config.hpp"

using namespace noi_engine;

static const std::unordered_map<
	std::string, std::function<std::unique_ptr<resource_loader>()>>
	RESOURCE_LOADERS{
		{ "source", []() -> std::unique_ptr<resource_loader>
		  { return std::make_unique<source_resource_loader>(); } },
		{ "compiled", []() -> std::unique_ptr<resource_loader>
		  { return std::make_unique<compiled_resource_loader>(); } }
	};

auto main(const int argc, char** argv) -> int
{
	auto provider = game_config::get_resource_loader(RESOURCE_LOADERS);

	sdl_game game(
		std::string(game_config::name),
		game_config::window_width,
		game_config::window_height, std::move(provider));

	return game.run(argc, argv);
}