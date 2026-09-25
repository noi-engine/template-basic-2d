//
// Created by ricka on 8/9/26.
//

#include <cmath>

#include <noi_engine/builtin/components/box_collider.hpp>
#include <noi_engine/builtin/components/gravity.hpp>
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

using noi_engine::box_collider;
using noi_engine::dirty;
using noi_engine::gravity;
using noi_engine::mesh_renderer;
using noi_engine::name_component;
using noi_engine::render_layer;
using noi_engine::script_component;
using noi_engine::transform;
using noi_engine::world_matrix;

namespace
{
    // Horizontal move speed in world units/sec.
    constexpr float MOVE_SPEED = 4.0f;
    // Upward velocity applied on jump. Combined with gravity's default -9.81 accel this
    // gives a ~1.8 unit peak height and ~1.2s of airtime - tune here if the hop feels off.
    constexpr float JUMP_VELOCITY = 6.0f;
    // physics_system zeroes gravity.velocity[axis] the instant a collision resolves it, so
    // "resting on something" and "at rest" are the same signal - no separate ground flag needed.
    constexpr float GROUNDED_EPSILON = 0.01f;
}

auto noi_engine_game::player_script::on_create(
    const noi_engine::entity self, const noi_engine::script_context& context) -> void
{
    NOI_ENGINE_ADD(gravity{});
    NOI_ENGINE_ADD((box_collider{.half_extents = {0.5f, 0.5f}, .is_static = false}));
}

auto noi_engine_game::player_script::on_update(
    const noi_engine::entity self, const noi_engine::script_context& context, const float dt) -> void
{
    const auto input = context.input;

    float move = 0.0f;
    if (input->is_key_down(noi_engine::key_code::right))
    {
        move += 1.0f;
    }
    if (input->is_key_down(noi_engine::key_code::left))
    {
        move -= 1.0f;
    }

    if (!NOI_ENGINE_HAS(transform) || !NOI_ENGINE_HAS(gravity))
    {
        return;
    }

    if (move != 0.0f)
    {
        auto& tr = NOI_ENGINE_GET(transform);
        tr.position.x += move * dt * MOVE_SPEED;
        NOI_ENGINE_ADD_DIRTY(transform);
    }

    auto& g = NOI_ENGINE_GET(gravity);
    const bool grounded = std::abs(g.velocity.y) < GROUNDED_EPSILON;
    if (grounded && input->is_key_pressed(noi_engine::key_code::space))
    {
        g.velocity.y = JUMP_VELOCITY;
    }

    // Emitted every frame regardless of movement, not just when direction != 0: camera_script's
    // follow() is a single smoothing step per invocation, not an ongoing per-frame behavior, so it
    // must be re-triggered every frame to keep converging - otherwise the camera freezes wherever
    // it was left mid-lerp the instant movement keys are released.
    NOI_ENGINE_EMIT(moved_signal{self});
}
