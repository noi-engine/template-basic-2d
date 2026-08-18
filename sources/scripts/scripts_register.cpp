#include "player_script.hpp"
#include "camera_script.hpp"
#include "scripts_register.hpp"
#include <noi_engine/core/model/script.hpp>

using namespace noi_engine_game;
auto noi_engine_game::register_scripts(noi_engine::game &game) -> void
{
  NOI_ENGINE_REGISTER_SCRIPT(player_script);
  NOI_ENGINE_REGISTER_SCRIPT(camera_script);
}