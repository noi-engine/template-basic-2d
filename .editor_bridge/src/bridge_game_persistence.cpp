// Serializes the live world back to source-loader-compatible scene JSON. Mirrors the same
// per-component-type dispatch shape as bridge_game_entities.cpp's enumerate_component_properties
// (see noi_engine/sources/core/resources/source_loader/loader/scene_parser/component_parser.cpp
// for the exact JSON shape each component type must round-trip through).

#include "bridge_game.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>

#include <nlohmann/json.hpp>

#include <noi_engine/core/model/material.hpp>
#include <noi_engine/core/resources/resources.hpp>

namespace noi_engine_editor_bridge_detail
{
    namespace
    {
        // Inverse of noi_engine::color_parser::parse() ("#RRGGBBAA").
        auto to_hex_string(const glm::vec4& c) -> std::string
        {
            const auto byte = [](const float v) -> unsigned
            {
                return static_cast<unsigned>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
            };

            char buf[10];
            std::snprintf(buf, sizeof(buf), "#%02x%02x%02x%02x", byte(c.r), byte(c.g), byte(c.b), byte(c.a));
            return buf;
        }

        auto vec3_json(const glm::vec3& v) -> nlohmann::json
        {
            return nlohmann::json::array({v.x, v.y, v.z});
        }
    }

    auto bridge_game::save_scene(const std::string& target_path) -> bool
    {
        const auto* current_scene = this->get_current_scene();
        if (!current_scene)
        {
            return false;
        }

        auto& world = this->get_world();
        auto& resources_ref = this->resources();
        const auto camera_entity = current_scene->get_camera_entity();

        // The engine never retains a resource's original load path once it's loaded (only its
        // label), so the "resources" block and the scene's "name" are carried over unchanged
        // from whatever is already on disk rather than reconstructed from live handles.
        nlohmann::json name_json = "Untitled Scene";
        nlohmann::json resources_json = nlohmann::json::object();
        {
            std::ifstream existing_in(target_path);
            if (existing_in)
            {
                nlohmann::json existing_root;
                existing_in >> existing_root;
                if (existing_root.contains("name")) name_json = existing_root["name"];
                if (existing_root.contains("resources")) resources_json = existing_root["resources"];
            }
        }

        nlohmann::json root;
        root["name"] = name_json;
        root["resources"] = resources_json;

        auto entities = nlohmann::json::array();

        for (const auto& e : world.get_entities())
        {
            // The reference grid is an editor-only visualization entity (see set_scene_2d()) -
            // it never existed in the authored scene and must never be written back to it.
            if (m_has_grid_entity && e == m_grid_entity)
            {
                continue;
            }

            const bool is_camera = (e == camera_entity);

            std::string name = is_camera ? "main_camera" : std::string{};
            if (!is_camera && world.has<noi_engine::name_component>(e))
            {
                name = world.get<noi_engine::name_component>(e).name;
            }

            auto components = nlohmann::json::array();

            if (world.has<noi_engine::transform>(e))
            {
                const auto& t = world.get<noi_engine::transform>(e);
                nlohmann::json props;
                props["position"] = vec3_json(t.position);
                props["rotation"] = vec3_json(t.rotation);
                // set_scene_2d() overwrites the camera entity's scale to size the border gizmo -
                // never persist that, always write identity scale back for the camera.
                props["scale"] = is_camera ? vec3_json(glm::vec3{1.0f}) : vec3_json(t.scale);
                components.push_back({{"type", "transform"}, {"properties", props}});
            }

            if (world.has<noi_engine::camera_2d>(e))
            {
                const auto& c = world.get<noi_engine::camera_2d>(e);
                nlohmann::json props;
                props["zoom"] = c.zoom;
                props["near_plane"] = c.near_plane;
                props["far_plane"] = c.far_plane;
                props["orthographic_size"] = c.orthographic_size;
                // The parser only ever reads a width/height pair, not the resolved ratio -
                // reconstruct a pair that divides back to the same value.
                props["aspect_ratio_width"] = c.aspect_ratio;
                props["aspect_ratio_height"] = 1.0f;
                components.push_back({{"type", "camera_2d"}, {"properties", props}});
            }

            if (world.has<noi_engine::render_layer>(e))
            {
                const auto& r = world.get<noi_engine::render_layer>(e);
                components.push_back({{"type", "render_layer"}, {"properties", {{"order", r.order}}}});
            }

            // The camera entity's mesh_renderer is the injected red border gizmo (set_scene_2d())
            // - editor-only visualization, never part of the authored scene.
            if (!is_camera && world.has<noi_engine::mesh_renderer>(e))
            {
                const auto& mr = world.get<noi_engine::mesh_renderer>(e);
                nlohmann::json props;
                props["mesh"] = std::string(resources_ref.get_label<noi_engine::mesh>(mr.m_mesh));
                props["material"] = std::string(resources_ref.get_label<noi_engine::material>(mr.m_material));

                if (world.has<noi_engine::mesh_renderer_properties>(e))
                {
                    const auto& mrp = world.get<noi_engine::mesh_renderer_properties>(e);
                    nlohmann::json material_properties;

                    if (!mrp.m_parameters.empty())
                    {
                        auto parameters = nlohmann::json::array();
                        for (const auto& [key, v] : mrp.m_parameters)
                        {
                            parameters.push_back({{"name", key}, {"value", v}});
                        }
                        material_properties["parameters"] = parameters;
                    }

                    if (!mrp.m_colors.empty())
                    {
                        auto colors = nlohmann::json::array();
                        for (const auto& [key, v] : mrp.m_colors)
                        {
                            colors.push_back({{"name", key}, {"value", to_hex_string(v)}});
                        }
                        material_properties["colors"] = colors;
                    }

                    if (!mrp.m_textures.empty())
                    {
                        auto textures = nlohmann::json::array();
                        for (const auto& [key, handle] : mrp.m_textures)
                        {
                            textures.push_back({
                                {"name", key},
                                {"value", std::string(resources_ref.get_label<noi_engine::texture>(handle))}
                            });
                        }
                        material_properties["textures"] = textures;
                    }

                    if (!material_properties.empty())
                    {
                        props["material_properties"] = material_properties;
                    }
                }

                components.push_back({{"type", "mesh_renderer"}, {"properties", props}});
            }

            if (world.has<noi_engine::script_component>(e))
            {
                const auto& sc = world.get<noi_engine::script_component>(e);
                auto names = nlohmann::json::array();
                for (const auto& handle : sc.scripts)
                {
                    names.push_back(std::string(resources_ref.get_label<noi_engine::script>(handle)));
                }
                components.push_back({{"type", "scripts"}, {"properties", {{"names", names}}}});
            }

            entities.push_back({{"name", name}, {"components", components}});
        }

        root["entities"] = entities;

        std::ofstream out(target_path);
        if (!out)
        {
            return false;
        }
        out << root.dump(2);
        return out.good();
    }
}
