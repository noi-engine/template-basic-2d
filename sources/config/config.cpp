//
// Created by ricka on 8/18/26.
//

#include "config.hpp"

#include "game_config.hpp"
#define RESOURCE_DIR "resources"

const std::filesystem::path RESOURCE_PATH = std::filesystem::current_path() / RESOURCE_DIR;

static std::unordered_map<
	std::string, std::function<std::unique_ptr<noi_engine::resource_loader>()>>
	RESOURCE_LOADERS{
		{ "source", []() -> std::unique_ptr<noi_engine::resource_loader>
		  { return std::make_unique<noi_engine::source_resource_loader>(RESOURCE_PATH); } },
		{ "compiled", []() -> std::unique_ptr<noi_engine::resource_loader>
		  { return std::make_unique<noi_engine::compiled_resource_loader>(RESOURCE_PATH); } }
	};

auto noi_engine_game::get_resource_loader()
	-> std::unique_ptr<noi_engine::resource_loader>
{
	const auto resource_loader_name =
		std::string(game_config::resource_loader_name);
	return RESOURCE_LOADERS[resource_loader_name]();
}