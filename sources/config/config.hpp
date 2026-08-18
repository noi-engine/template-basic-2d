//
// Created by ricka on 8/18/26.
//

#pragma once

#include <memory>
#include <noi_engine/core/resources/resource_loader.hpp>

namespace noi_engine_game
{
	auto get_resource_loader() -> std::unique_ptr<noi_engine::resource_loader>;
}
