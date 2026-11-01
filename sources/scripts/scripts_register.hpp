#pragma once

#include <noi_engine/core/game.hpp>

namespace noi_engine_game
{
  // Scripts register themselves (NOI_ENGINE_SCRIPT(<name>) in their class): nothing to add
  // here for a new script. Use this only for custom registrations, e.g.
  // NOI_ENGINE_REGISTER_SCRIPT_NAMED(my_script, "other_name").
  auto register_scripts(noi_engine::game &game) -> void;
}

extern "C" auto noi_engine_register_scripts(noi_engine::game &game) -> void;
