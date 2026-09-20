#pragma once

#include "noi_engine_editor_bridge/bridge_api.h"

#include <cstdint>
#include <string>
#include <unordered_set>

#include <noi_engine/core/components/camera_2d.hpp>
#include <noi_engine/core/components/camera_3d.hpp>
#include <noi_engine/core/components/dirty.hpp>
#include <noi_engine/core/components/mesh_renderer.hpp>
#include <noi_engine/core/components/mesh_renderer_properties.hpp>
#include <noi_engine/core/components/name_component.hpp>
#include <noi_engine/core/components/orbit_camera.hpp>
#include <noi_engine/core/components/render_layer.hpp>
#include <noi_engine/core/components/script_component.hpp>
#include <noi_engine/core/components/transform.hpp>
#include <noi_engine/core/components/world_matrix.hpp>
#include <noi_engine/core/ecs/entity.hpp>
#include <noi_engine/core/ecs/world.hpp>
#include <noi_engine/core/game.hpp>
#include <noi_engine/core/timer.hpp>

namespace noi_engine_editor_bridge_detail
{
    // Concrete noi_engine::game backing every noi_engine_editor_bridge_game_handle. Implementation
    // is split by responsibility:
    //   - bridge_game.cpp:           lifecycle (construction, run(), resize, scene load) and input
    //   - bridge_game_entities.cpp:  entity/component inspection & editing
    //   - bridge_game_resources.cpp: resource/material inspection & editing
    // bridge_api.cpp holds the extern "C" glue that exposes this class through bridge_api.h.
    class bridge_game final : public noi_engine::game
    {
    public:
        bridge_game(int game_width, int game_height, const std::string& resources_path);
        ~bridge_game() override;

        // Lifecycle & input - bridge_game.cpp
        auto run() -> void;
        auto reset_timer() -> void;
        auto set_key_pressed(int key) -> void;
        auto set_key_released(int key) -> void;
        auto release_all_keys() -> void;
        auto on_resize(int width, int height) -> void;
        auto set_scene_2d(const std::string& path) -> void;
        auto load_scripts(const std::string& scripts_library_path) -> bool;

        // Editor-only reference grid (fixed in world space, not attached to the camera) - a
        // visual aid so panning/following the camera visibly scrolls something underneath.
        auto set_grid_visible(bool visible) -> void;

        // Entities & components - bridge_game_entities.cpp
        auto enumerate_entities(noi_engine_editor_bridge_entity_entry_callback callback, void* user_data) -> void;
        auto enumerate_component_properties(uint32_t entity_id, uint32_t entity_generation,
                                             const std::string& component_type,
                                             noi_engine_editor_bridge_property_entry_callback callback,
                                             void* user_data) -> void;
        [[nodiscard]] auto set_component_property(uint32_t entity_id, uint32_t entity_generation,
                                                   const std::string& component_type,
                                                   const std::string& property_name,
                                                   const noi_engine_editor_bridge_property_value& value) -> bool;
        [[nodiscard]] auto add_component(uint32_t entity_id, uint32_t entity_generation,
                                          const std::string& component_type) -> bool;

        // Resources & materials - bridge_game_resources.cpp
        auto enumerate_resources(noi_engine_editor_bridge_resource_entry_callback callback,
                                  void* user_data) const -> void;
        auto enumerate_material_properties(uint32_t material_id, uint32_t material_generation,
                                            noi_engine_editor_bridge_property_entry_callback callback,
                                            void* user_data) -> void;
        [[nodiscard]] auto set_material_property(uint32_t material_id, uint32_t material_generation,
                                                  const std::string& property_name,
                                                  const noi_engine_editor_bridge_property_value& value) -> bool;

    private:
        [[nodiscard]] auto get_world() -> noi_engine::world&;
        [[nodiscard]] auto get_current_scene() const -> noi_engine::scene*;

        //TODO: handle 3d
        [[nodiscard]] auto get_camera_2d_component() -> noi_engine::camera_2d&;

        int m_game_width{};
        int m_game_height{};
        noi_engine::timer m_timer{};
        void* m_scripts_library_handle{nullptr};
        std::unordered_set<int> m_held_keys{};

        noi_engine::entity m_grid_entity{};
        bool m_has_grid_entity{false};
        noi_engine::resource_handle<noi_engine::mesh> m_grid_mesh{};
        noi_engine::resource_handle<noi_engine::material> m_grid_material{};
        bool m_grid_visible{false};
    };
}
