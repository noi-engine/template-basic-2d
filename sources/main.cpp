#include <filesystem>
#include <functional>
#include <memory>
#include <noi_engine/builtin/systems/physics_system.hpp>
#include <noi_engine/core/sdl/sdl_game.hpp>
#include <string>

#include "config/config.hpp"
#include "game_config.hpp"
#include "scripts/scripts_register.hpp"

using namespace noi_engine_game;

auto main(const int argc, char** argv) -> int
{
	noi_engine::sdl_game game(
		std::string(game_config::name), game_config::window_width,
		game_config::window_height, std::move(get_resource_loader()));

	register_scripts(game);

	auto main_scene = game.resources().load_scene(game_config::start_scene);
	main_scene->add_system<noi_engine::physics_system>();
	game.set_scene(std::move(main_scene));

	return game.run(argc, argv);
}