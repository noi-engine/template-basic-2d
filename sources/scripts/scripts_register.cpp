#include "scripts_register.hpp"

auto noi_engine_game::register_scripts([[maybe_unused]] noi_engine::game &game) -> void
{
}

// Called by the editor after it loads the scripts library.
auto noi_engine_register_scripts(noi_engine::game &game) -> void
{
  noi_engine_game::register_scripts(game);
}
