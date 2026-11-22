#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
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
try
{
	noi_engine::sdl_game game(
		std::string(game_config::name), game_config::window_width,
		game_config::window_height, std::move(get_resource_loader()));

	register_scripts(game);

	// The systems a scene needs aren't part of the scene file: add them to every scene, the first
	// one and the ones scripts switch to with NOI_ENGINE_LOAD_SCENE("scenes/other.scene").
	const auto add_systems = [](noi_engine::scene& scene) -> void
	{ scene.add_system<noi_engine::physics_system>(); };
	game.on_scene_loaded(add_systems);

	auto main_scene = game.resources().load_scene(game_config::start_scene);
	add_systems(*main_scene);
	game.set_scene(std::move(main_scene));

	return game.run(argc, argv);
}
catch (const std::exception& error)
{
	std::cerr << "Fatal error: " << error.what() << std::endl;
	return EXIT_FAILURE;
}
