//
// Created by ricka on 8/9/26.
//

#include <noi_engine/core/components/dirty.hpp>
#include <noi_engine/core/components/mesh_renderer.hpp>
#include <noi_engine/core/components/name_component.hpp>
#include <noi_engine/core/components/render_layer.hpp>
#include <noi_engine/core/components/script_component.hpp>
#include <noi_engine/core/components/transform.hpp>
#include <noi_engine/core/components/world_matrix.hpp>
#include <noi_engine/core/resources/resources.hpp>

#include "player_script.hpp"
#include "../signals/moved_signal.hpp"

using noi_engine::dirty;
using noi_engine::mesh_renderer;
using noi_engine::name_component;
using noi_engine::render_layer;
using noi_engine::script_component;
using noi_engine::transform;
using noi_engine::world_matrix;

auto noi_engine_game::player_script::on_update(
    const noi_engine::entity self, const noi_engine::script_context& context, const float dt) -> void
{
    const auto input = context.input;
    auto direction = glm::vec3{0.0f};
    if (input->is_key_down(noi_engine::key_code::up))
    {
        direction.y += 1.0f;
    }

    if (input->is_key_down(noi_engine::key_code::down))
    {
        direction.y -= 1.0f;
    }

    if (input->is_key_down(noi_engine::key_code::right))
    {
        direction.x += 1.0f;
    }

    if (input->is_key_down(noi_engine::key_code::left))
    {
        direction.x -= 1.0f;
    }

    if (direction != glm::vec3{0.0f})
    {
        direction = glm::normalize(direction);
    }

    if (!NOI_ENGINE_HAS(transform))
    {
        return;
    }

    if (direction != glm::vec3{0.0f})
    {
        auto& tr = NOI_ENGINE_GET(transform);
        tr.position += direction * dt * 3.0f /* TODO: Speed */ ;

        NOI_ENGINE_ADD_DIRTY(transform);
    }

    // Emitted every frame regardless of movement, not just when direction != 0: camera_script's
    // follow() is a single smoothing step per invocation, not an ongoing per-frame behavior, so it
    // must be re-triggered every frame to keep converging - otherwise the camera freezes wherever
    // it was left mid-lerp the instant movement keys are released.
    NOI_ENGINE_EMIT(moved_signal{self});
}
