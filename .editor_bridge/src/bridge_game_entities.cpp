#include "bridge_game.hpp"

#include <algorithm>
#include <glm/glm.hpp>

namespace noi_engine_editor_bridge_detail
{
    auto bridge_game::enumerate_entities(const noi_engine_editor_bridge_entity_entry_callback callback,
                                          void* const user_data) -> void
    {
        if (!callback || !this->get_current_scene())
        {
            return;
        }

        auto& world = this->get_world();

        for (const auto& e : world.get_entities())
        {
            // The editor's own reference-grid entity (added in set_scene_2d()) is a rendering
            // aid, not scene content - it has no name and isn't user-manageable, so keep it out
            // of the Hierarchy entirely rather than showing an unnamed, undeletable ghost row.
            if (m_has_grid_entity && e == m_grid_entity)
            {
                continue;
            }

            std::string name;
            if (world.has<noi_engine::name_component>(e))
            {
                name = world.get<noi_engine::name_component>(e).name;
            }

            std::string component_types;
            const auto append_type = [&component_types](const char* type_name, const bool has)
            {
                if (!has)
                {
                    return;
                }
                if (!component_types.empty())
                {
                    component_types += ",";
                }
                component_types += type_name;
            };

            append_type("camera_2d", world.has<noi_engine::camera_2d>(e));
            append_type("camera_3d", world.has<noi_engine::camera_3d>(e));
            append_type("mesh_renderer", world.has<noi_engine::mesh_renderer>(e));
            append_type("mesh_renderer_properties", world.has<noi_engine::mesh_renderer_properties>(e));
            append_type("name_component", world.has<noi_engine::name_component>(e));
            append_type("orbit_camera", world.has<noi_engine::orbit_camera>(e));
            append_type("render_layer", world.has<noi_engine::render_layer>(e));
            append_type("script_component", world.has<noi_engine::script_component>(e));
            append_type("transform", world.has<noi_engine::transform>(e));
            append_type("world_matrix", world.has<noi_engine::world_matrix>(e));

            callback(user_data, e.m_id, e.m_generation, name.c_str(), component_types.c_str());
        }
    }

    auto bridge_game::enumerate_component_properties(const uint32_t entity_id, const uint32_t entity_generation,
                                                       const std::string& component_type,
                                                       const noi_engine_editor_bridge_property_entry_callback callback,
                                                       void* const user_data) -> void
    {
        if (!callback || !this->get_current_scene())
        {
            return;
        }

        auto& world = this->get_world();
        const noi_engine::entity e{entity_id, entity_generation};

        const auto emit_float = [&](const char* name, const float v, const bool read_only)
        {
            noi_engine_editor_bridge_property_value pv{};
            pv.type = NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_FLOAT;
            pv.number[0] = v;
            callback(user_data, name, pv, read_only ? 1 : 0);
        };

        const auto emit_vec3 = [&](const char* name, const glm::vec3& v, const bool read_only)
        {
            noi_engine_editor_bridge_property_value pv{};
            pv.type = NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_VEC3;
            pv.number[0] = v.x;
            pv.number[1] = v.y;
            pv.number[2] = v.z;
            callback(user_data, name, pv, read_only ? 1 : 0);
        };

        const auto emit_string = [&](const char* name, const std::string& s, const bool read_only)
        {
            noi_engine_editor_bridge_property_value pv{};
            pv.type = NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_STRING;
            pv.text = s.c_str();
            callback(user_data, name, pv, read_only ? 1 : 0);
        };

        const auto emit_bool = [&](const char* name, const bool b, const bool read_only)
        {
            noi_engine_editor_bridge_property_value pv{};
            pv.type = NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_BOOL;
            pv.flag = b ? 1 : 0;
            callback(user_data, name, pv, read_only ? 1 : 0);
        };

        const auto emit_vec4 = [&](const char* name, const glm::vec4& v, const bool read_only)
        {
            noi_engine_editor_bridge_property_value pv{};
            pv.type = NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_VEC4;
            pv.number[0] = v.x;
            pv.number[1] = v.y;
            pv.number[2] = v.z;
            pv.number[3] = v.w;
            callback(user_data, name, pv, read_only ? 1 : 0);
        };

        const auto emit_resource_ref = [&](const char* name, const std::string& label,
                                            const noi_engine_editor_bridge_resource_type resource_type,
                                            const bool read_only)
        {
            noi_engine_editor_bridge_property_value pv{};
            pv.type = NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_RESOURCE_REF;
            pv.text = label.c_str();
            pv.resource_type = resource_type;
            callback(user_data, name, pv, read_only ? 1 : 0);
        };

        if (component_type == "transform" && world.has<noi_engine::transform>(e))
        {
            const auto& t = world.get<noi_engine::transform>(e);
            emit_vec3("position", t.position, false);
            emit_vec3("rotation", t.rotation, false);
            emit_vec3("scale", t.scale, false);
        }
        else if (component_type == "camera_2d" && world.has<noi_engine::camera_2d>(e))
        {
            const auto& c = world.get<noi_engine::camera_2d>(e);
            emit_float("zoom", c.zoom, false);
            emit_float("near_plane", c.near_plane, false);
            emit_float("far_plane", c.far_plane, false);
            emit_float("aspect_ratio", c.aspect_ratio, false);
            emit_float("orthographic_size", c.orthographic_size, false);
        }
        else if (component_type == "camera_3d" && world.has<noi_engine::camera_3d>(e))
        {
            const auto& c = world.get<noi_engine::camera_3d>(e);
            emit_float("fov", c.fov, false);
            emit_float("near_plane", c.near_plane, false);
            emit_float("far_plane", c.far_plane, false);
            emit_float("aspect_ratio", c.aspect_ratio, false);
        }
        else if (component_type == "orbit_camera" && world.has<noi_engine::orbit_camera>(e))
        {
            const auto& c = world.get<noi_engine::orbit_camera>(e);
            emit_vec3("target", c.target, false);
            emit_float("distance", c.distance, false);
            emit_float("yaw", c.yaw, false);
            emit_float("pitch", c.pitch, false);
            emit_float("sensitivity", c.sensitivity, false);
            emit_float("zoom_speed", c.zoom_speed, false);
            emit_float("min_distance", c.min_distance, false);
            emit_float("max_distance", c.max_distance, false);
            emit_float("min_pitch", c.min_pitch, false);
            emit_float("max_pitch", c.max_pitch, false);
        }
        else if (component_type == "render_layer" && world.has<noi_engine::render_layer>(e))
        {
            const auto& r = world.get<noi_engine::render_layer>(e);
            emit_float("order", static_cast<float>(r.order), false);
        }
        else if (component_type == "name_component" && world.has<noi_engine::name_component>(e))
        {
            const auto& n = world.get<noi_engine::name_component>(e);
            emit_string("name", n.name, false);
        }
        else if (component_type == "mesh_renderer" && world.has<noi_engine::mesh_renderer>(e))
        {
            const auto& mr = world.get<noi_engine::mesh_renderer>(e);
            auto& resources_ref = this->resources();
            const std::string mesh_label{resources_ref.get_label<noi_engine::mesh>(mr.m_mesh)};
            const std::string material_label{resources_ref.get_label<noi_engine::material>(mr.m_material)};
            emit_resource_ref("mesh", mesh_label, NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_MESH, false);
            emit_resource_ref("material", material_label, NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_MATERIAL, false);
        }
        else if (component_type == "mesh_renderer_properties" &&
                 world.has<noi_engine::mesh_renderer_properties>(e))
        {
            const auto& mrp = world.get<noi_engine::mesh_renderer_properties>(e);
            auto& resources_ref = this->resources();

            for (const auto& [key, v] : mrp.m_parameters)
            {
                emit_float(("param:" + key).c_str(), v, false);
            }
            for (const auto& [key, v] : mrp.m_colors)
            {
                emit_vec4(("color:" + key).c_str(), v, false);
            }
            for (const auto& [key, handle] : mrp.m_textures)
            {
                const std::string label{resources_ref.get_label<noi_engine::texture>(handle)};
                emit_resource_ref(("texture:" + key).c_str(), label, NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_TEXTURE,
                                   false);
            }
        }
        else if (component_type == "script_component" && world.has<noi_engine::script_component>(e))
        {
            const auto& sc = world.get<noi_engine::script_component>(e);
            auto& resources_ref = this->resources();

            for (size_t i = 0; i < sc.scripts.size(); ++i)
            {
                const std::string label{resources_ref.get_label<noi_engine::script>(sc.scripts[i])};
                const std::string prop_name = "scripts[" + std::to_string(i) + "]";
                emit_resource_ref(prop_name.c_str(), label, NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_SCRIPT, false);
            }
            emit_bool("created", sc.created, true);
        }
        // world_matrix intentionally exposes no editable properties (fully computed each frame).
    }

    auto bridge_game::set_component_property(const uint32_t entity_id, const uint32_t entity_generation,
                                              const std::string& component_type,
                                              const std::string& property_name,
                                              const noi_engine_editor_bridge_property_value& value) -> bool
    {
        if (!this->get_current_scene())
        {
            return false;
        }

        auto& world = this->get_world();
        const noi_engine::entity e{entity_id, entity_generation};

        if (component_type == "transform" && world.has<noi_engine::transform>(e) &&
            value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_VEC3)
        {
            auto& t = world.get<noi_engine::transform>(e);
            const glm::vec3 v{value.number[0], value.number[1], value.number[2]};

            if (property_name == "position") { t.position = v; }
            else if (property_name == "rotation") { t.rotation = v; }
            else if (property_name == "scale") { t.scale = v; }
            else { return false; }

            world.add<noi_engine::dirty<noi_engine::transform>>(e, noi_engine::dirty<noi_engine::transform>{});
            return true;
        }

        if (component_type == "camera_2d" && world.has<noi_engine::camera_2d>(e) &&
            value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_FLOAT)
        {
            auto& c = world.get<noi_engine::camera_2d>(e);
            const float v = value.number[0];

            if (property_name == "zoom") { c.zoom = v; }
            else if (property_name == "near_plane") { c.near_plane = v; }
            else if (property_name == "far_plane") { c.far_plane = v; }
            else if (property_name == "aspect_ratio") { c.aspect_ratio = v; }
            else if (property_name == "orthographic_size") { c.orthographic_size = v; }
            else { return false; }
            return true;
        }

        if (component_type == "camera_3d" && world.has<noi_engine::camera_3d>(e) &&
            value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_FLOAT)
        {
            auto& c = world.get<noi_engine::camera_3d>(e);
            const float v = value.number[0];

            if (property_name == "fov") { c.fov = v; }
            else if (property_name == "near_plane") { c.near_plane = v; }
            else if (property_name == "far_plane") { c.far_plane = v; }
            else if (property_name == "aspect_ratio") { c.aspect_ratio = v; }
            else { return false; }
            return true;
        }

        if (component_type == "orbit_camera" && world.has<noi_engine::orbit_camera>(e))
        {
            auto& c = world.get<noi_engine::orbit_camera>(e);

            if (property_name == "target" && value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_VEC3)
            {
                c.target = glm::vec3{value.number[0], value.number[1], value.number[2]};
                return true;
            }
            if (value.type != NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_FLOAT)
            {
                return false;
            }
            const float v = value.number[0];
            if (property_name == "distance") { c.distance = v; }
            else if (property_name == "yaw") { c.yaw = v; }
            else if (property_name == "pitch") { c.pitch = v; }
            else if (property_name == "sensitivity") { c.sensitivity = v; }
            else if (property_name == "zoom_speed") { c.zoom_speed = v; }
            else if (property_name == "min_distance") { c.min_distance = v; }
            else if (property_name == "max_distance") { c.max_distance = v; }
            else if (property_name == "min_pitch") { c.min_pitch = v; }
            else if (property_name == "max_pitch") { c.max_pitch = v; }
            else { return false; }
            return true;
        }

        if (component_type == "render_layer" && world.has<noi_engine::render_layer>(e) &&
            property_name == "order" && value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_FLOAT)
        {
            world.get<noi_engine::render_layer>(e).order = static_cast<int>(value.number[0]);
            return true;
        }

        if (component_type == "name_component" && world.has<noi_engine::name_component>(e) &&
            property_name == "name" && value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_STRING)
        {
            world.get<noi_engine::name_component>(e).name = value.text ? value.text : "";
            return true;
        }

        if (component_type == "mesh_renderer" && world.has<noi_engine::mesh_renderer>(e) &&
            value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_RESOURCE_REF)
        {
            auto& mr = world.get<noi_engine::mesh_renderer>(e);
            const std::string label = value.text ? value.text : "";

            if (property_name == "mesh")
            {
                mr.m_mesh = this->resources().get_handle<noi_engine::mesh>(label);
                return true;
            }
            if (property_name == "material")
            {
                mr.m_material = this->resources().get_handle<noi_engine::material>(label);
                return true;
            }
            return false;
        }

        if (component_type == "mesh_renderer_properties" && world.has<noi_engine::mesh_renderer_properties>(e))
        {
            auto& mrp = world.get<noi_engine::mesh_renderer_properties>(e);

            if (property_name.rfind("param:", 0) == 0 && value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_FLOAT)
            {
                mrp.m_parameters[property_name.substr(6)] = value.number[0];
                return true;
            }
            if (property_name.rfind("color:", 0) == 0 && value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_VEC4)
            {
                mrp.m_colors[property_name.substr(6)] =
                    glm::vec4{value.number[0], value.number[1], value.number[2], value.number[3]};
                return true;
            }
            if (property_name.rfind("texture:", 0) == 0 &&
                value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_RESOURCE_REF)
            {
                const std::string label = value.text ? value.text : "";
                mrp.m_textures[property_name.substr(8)] = this->resources().get_handle<noi_engine::texture>(label);
                return true;
            }
            return false;
        }

        if (component_type == "script_component" && world.has<noi_engine::script_component>(e) &&
            value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_RESOURCE_REF &&
            property_name.rfind("scripts[", 0) == 0 && !property_name.empty() && property_name.back() == ']')
        {
            auto& sc = world.get<noi_engine::script_component>(e);
            const auto index_str = property_name.substr(8, property_name.size() - 9);
            const auto index = static_cast<size_t>(std::stoul(index_str));

            if (index < sc.scripts.size())
            {
                const std::string label = value.text ? value.text : "";
                sc.scripts[index] = this->resources().get_handle<noi_engine::script>(label);
                return true;
            }
            return false;
        }

        // world_matrix exposes no editable properties (fully computed each frame).
        return false;
    }

    auto bridge_game::add_component(const uint32_t entity_id, const uint32_t entity_generation,
                                     const std::string& component_type) -> bool
    {
        if (!this->get_current_scene())
        {
            return false;
        }

        auto& world = this->get_world();
        const noi_engine::entity e{entity_id, entity_generation};

        if (component_type == "transform")
        {
            if (world.has<noi_engine::transform>(e)) return false;
            world.add<noi_engine::transform>(e, {});
            return true;
        }
        if (component_type == "camera_2d")
        {
            if (world.has<noi_engine::camera_2d>(e)) return false;
            world.add<noi_engine::camera_2d>(e, {});
            return true;
        }
        if (component_type == "camera_3d")
        {
            if (world.has<noi_engine::camera_3d>(e)) return false;
            world.add<noi_engine::camera_3d>(e, {});
            return true;
        }
        if (component_type == "orbit_camera")
        {
            if (world.has<noi_engine::orbit_camera>(e)) return false;
            world.add<noi_engine::orbit_camera>(e, {});
            return true;
        }
        if (component_type == "render_layer")
        {
            if (world.has<noi_engine::render_layer>(e)) return false;
            world.add<noi_engine::render_layer>(e, {});
            return true;
        }
        if (component_type == "name_component")
        {
            if (world.has<noi_engine::name_component>(e)) return false;
            world.add<noi_engine::name_component>(e, {});
            return true;
        }
        if (component_type == "mesh_renderer")
        {
            if (world.has<noi_engine::mesh_renderer>(e)) return false;
            // Default-constructing mesh_renderer{} leaves both handles at {id=0, generation=0},
            // which only happens to resolve to something drawable because the builtin quad mesh
            // and color material are always the very first mesh/material loaded (see
            // game::init_builtin_resources()) - relying on that coincidence is fragile. Point at
            // them explicitly instead, matching how the border gizmo above already looks up
            // "meshes:quad_2d" by label rather than assuming a handle value.
            world.add<noi_engine::mesh_renderer>(e, {
                                                      .m_mesh = this->resources().get_handle<noi_engine::mesh>("meshes:quad_2d"),
                                                      .m_material = this->resources().get_handle<noi_engine::material>("materials:color")
                                                  });
            return true;
        }
        if (component_type == "mesh_renderer_properties")
        {
            if (world.has<noi_engine::mesh_renderer_properties>(e)) return false;
            world.add<noi_engine::mesh_renderer_properties>(e, {});
            return true;
        }
        if (component_type == "script_component")
        {
            if (world.has<noi_engine::script_component>(e)) return false;
            world.add<noi_engine::script_component>(e, {});
            return true;
        }

        // world_matrix is engine-managed (recomputed from transform each frame) and is
        // intentionally not addable by hand.
        return false;
    }

    auto bridge_game::remove_component(const uint32_t entity_id, const uint32_t entity_generation,
                                        const std::string& component_type) -> bool
    {
        if (!this->get_current_scene())
        {
            return false;
        }

        auto& world = this->get_world();
        const noi_engine::entity e{entity_id, entity_generation};

        if (component_type == "transform" && world.has<noi_engine::transform>(e))
        {
            world.remove<noi_engine::transform>(e);
            return true;
        }
        if (component_type == "camera_2d" && world.has<noi_engine::camera_2d>(e))
        {
            world.remove<noi_engine::camera_2d>(e);
            return true;
        }
        if (component_type == "camera_3d" && world.has<noi_engine::camera_3d>(e))
        {
            world.remove<noi_engine::camera_3d>(e);
            return true;
        }
        if (component_type == "orbit_camera" && world.has<noi_engine::orbit_camera>(e))
        {
            world.remove<noi_engine::orbit_camera>(e);
            return true;
        }
        if (component_type == "render_layer" && world.has<noi_engine::render_layer>(e))
        {
            world.remove<noi_engine::render_layer>(e);
            return true;
        }
        if (component_type == "mesh_renderer" && world.has<noi_engine::mesh_renderer>(e))
        {
            world.remove<noi_engine::mesh_renderer>(e);
            return true;
        }
        if (component_type == "mesh_renderer_properties" && world.has<noi_engine::mesh_renderer_properties>(e))
        {
            world.remove<noi_engine::mesh_renderer_properties>(e);
            return true;
        }
        if (component_type == "script_component" && world.has<noi_engine::script_component>(e))
        {
            world.remove<noi_engine::script_component>(e);
            return true;
        }

        // name_component is an entity's identity (the Hierarchy relies on every entity having
        // one) and world_matrix is engine-managed - neither is removable by hand.
        return false;
    }

    auto bridge_game::remove_component_property(const uint32_t entity_id, const uint32_t entity_generation,
                                                  const std::string& component_type,
                                                  const std::string& property_name) -> bool
    {
        if (!this->get_current_scene())
        {
            return false;
        }

        auto& world = this->get_world();
        const noi_engine::entity e{entity_id, entity_generation};

        if (component_type != "mesh_renderer_properties" || !world.has<noi_engine::mesh_renderer_properties>(e))
        {
            return false;
        }

        auto& mrp = world.get<noi_engine::mesh_renderer_properties>(e);

        if (property_name.rfind("param:", 0) == 0)
        {
            return mrp.m_parameters.erase(property_name.substr(6)) > 0;
        }
        if (property_name.rfind("color:", 0) == 0)
        {
            return mrp.m_colors.erase(property_name.substr(6)) > 0;
        }
        if (property_name.rfind("texture:", 0) == 0)
        {
            return mrp.m_textures.erase(property_name.substr(8)) > 0;
        }

        return false;
    }

    auto bridge_game::create_entity(const std::string& name) -> noi_engine::entity
    {
        if (!this->get_current_scene())
        {
            return {};
        }

        auto& world = this->get_world();
        const auto e = world.create_entity();
        noi_engine::name_component nc{};
        nc.name = name;
        world.add<noi_engine::name_component>(e, nc);
        return e;
    }

    auto bridge_game::destroy_entity(const uint32_t entity_id, const uint32_t entity_generation) -> bool
    {
        if (!this->get_current_scene())
        {
            return false;
        }

        auto& world = this->get_world();
        const noi_engine::entity e{entity_id, entity_generation};

        // Neither the scene's camera entity nor the editor's own reference-grid entity is
        // user-deletable - both are structural, not authored scene content.
        if (e == this->get_current_scene()->get_camera_entity())
        {
            return false;
        }
        if (m_has_grid_entity && e == m_grid_entity)
        {
            return false;
        }
        if (std::ranges::find(world.get_entities(), e) == world.get_entities().end())
        {
            return false;
        }

        world.destroy_entity(e);
        return true;
    }

    auto bridge_game::duplicate_entity(const uint32_t entity_id, const uint32_t entity_generation,
                                        const std::string& new_name) -> noi_engine::entity
    {
        if (!this->get_current_scene())
        {
            return {};
        }

        auto& world = this->get_world();
        const noi_engine::entity e{entity_id, entity_generation};

        if (std::ranges::find(world.get_entities(), e) == world.get_entities().end())
        {
            return {};
        }

        const auto ne = world.create_entity();

        // Deep-copy every component type an entity can carry (same list as add_component/
        // enumerate_entities), except name_component (replaced with new_name) and world_matrix
        // (engine-recomputed from transform, never copied by hand).
        if (world.has<noi_engine::transform>(e))
        {
            world.add<noi_engine::transform>(ne, world.get<noi_engine::transform>(e));
        }
        if (world.has<noi_engine::camera_2d>(e))
        {
            world.add<noi_engine::camera_2d>(ne, world.get<noi_engine::camera_2d>(e));
        }
        if (world.has<noi_engine::camera_3d>(e))
        {
            world.add<noi_engine::camera_3d>(ne, world.get<noi_engine::camera_3d>(e));
        }
        if (world.has<noi_engine::orbit_camera>(e))
        {
            world.add<noi_engine::orbit_camera>(ne, world.get<noi_engine::orbit_camera>(e));
        }
        if (world.has<noi_engine::render_layer>(e))
        {
            world.add<noi_engine::render_layer>(ne, world.get<noi_engine::render_layer>(e));
        }
        if (world.has<noi_engine::mesh_renderer>(e))
        {
            world.add<noi_engine::mesh_renderer>(ne, world.get<noi_engine::mesh_renderer>(e));
        }
        if (world.has<noi_engine::mesh_renderer_properties>(e))
        {
            world.add<noi_engine::mesh_renderer_properties>(ne, world.get<noi_engine::mesh_renderer_properties>(e));
        }
        if (world.has<noi_engine::script_component>(e))
        {
            world.add<noi_engine::script_component>(ne, world.get<noi_engine::script_component>(e));
        }

        noi_engine::name_component nc{};
        nc.name = new_name;
        world.add<noi_engine::name_component>(ne, nc);
        return ne;
    }
}
