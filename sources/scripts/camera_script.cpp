//
// Created by ricka on 8/9/26.
//

#include <string>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <noi_engine/builtin/actions/basic.hpp>
#include <noi_engine/builtin/components/box_collider.hpp>
#include <noi_engine/core/components/mesh_renderer.hpp>
#include <noi_engine/core/components/mesh_renderer_properties.hpp>
#include <noi_engine/core/components/name_component.hpp>
#include <noi_engine/core/components/transform.hpp>
#include <noi_engine/core/resources/resources.hpp>

#include "camera_script.hpp"
#include "../signals/moved_signal.hpp"

namespace
{
    // World bounds for the level: a floor spanning [-WORLD_HALF_WIDTH, WORLD_HALF_WIDTH] with a
    // solid wall at each end, several screens wider than the camera's view - so moving the player
    // actually scrolls through a level instead of pacing back and forth on the first screen.
    constexpr float WORLD_HALF_WIDTH = 20.0f;
    constexpr float GROUND_Y = -3.0f;
    constexpr float GROUND_HALF_HEIGHT = 0.5f;
    constexpr float WALL_HALF_HEIGHT = 6.0f;
    constexpr float WALL_HALF_WIDTH = 0.5f;

    auto spawn_static_box(noi_engine::world& world, noi_engine::resources& resources,
                          const std::string& name, const glm::vec3& position, const glm::vec3& scale,
                          const glm::vec4& color) -> void
    {
        const auto e = world.create_entity();
        world.add<noi_engine::name_component>(e, {.name = name});
        world.add_transform(e, noi_engine::transform{.position = position, .scale = scale});
        world.add<noi_engine::mesh_renderer>(e, {
                                                  .m_mesh = resources.get_handle<noi_engine::mesh>("meshes:quad_2d"),
                                                  .m_material = resources.get_handle<noi_engine::material>("materials:color")
                                              });
        world.add<noi_engine::mesh_renderer_properties>(e, {.m_colors = {{"u_color", color}}});
        world.add<noi_engine::box_collider>(e, {
                                                 .half_extents = {scale.x * 0.5f, scale.y * 0.5f},
                                                 .is_static = true
                                             });
    }
}

auto noi_engine_game::camera_script::on_create(
    const noi_engine::entity self, const noi_engine::script_context& context)
    -> void
{
    const auto player = NOI_ENGINE_FIND_ENTITY("player");

    auto callback = [*this, context, self](const float dt, const moved_signal& signal) -> void
    {
        NOI_ENGINE_ACTION_FOLLOW({
                                 .target = signal.source,
                                 .follower = self,
                                 .offset = {0.0f, 0.0f, 0.0f},
                                 .smoothing = 0.3f
                                 });
    };

    NOI_ENGINE_CONNECT(moved_signal, player, callback);

    auto& world = context.get_world();
    constexpr glm::vec4 ground_color{0.25f, 0.55f, 0.18f, 1.0f};
    constexpr glm::vec4 wall_color{0.25f, 0.25f, 0.28f, 1.0f};

    spawn_static_box(world, *context.resources, "ground",
                     {0.0f, GROUND_Y, 0.0f}, {WORLD_HALF_WIDTH * 2.0f, GROUND_HALF_HEIGHT * 2.0f, 1.0f},
                     ground_color);

    const float wall_center_y = GROUND_Y + GROUND_HALF_HEIGHT + WALL_HALF_HEIGHT;
    spawn_static_box(world, *context.resources, "wall_left",
                     {-WORLD_HALF_WIDTH, wall_center_y, 0.0f}, {WALL_HALF_WIDTH * 2.0f, WALL_HALF_HEIGHT * 2.0f, 1.0f},
                     wall_color);
    spawn_static_box(world, *context.resources, "wall_right",
                     {WORLD_HALF_WIDTH, wall_center_y, 0.0f}, {WALL_HALF_WIDTH * 2.0f, WALL_HALF_HEIGHT * 2.0f, 1.0f},
                     wall_color);
}
