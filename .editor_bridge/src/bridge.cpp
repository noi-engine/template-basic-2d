#include "noi_engine_editor_bridge/bridge_api.h"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>

#include <dlfcn.h>

#include <glm/glm.hpp>

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
#include <noi_engine/core/renderer/renderer.hpp>
#include <noi_engine/core/resources/resource_loader.hpp>
#include <noi_engine/core/timer.hpp>
#include <noi_engine/noi_engineConfig.hpp>

namespace
{
    constexpr float PIXELS_PER_UNIT = 100.0f;
    constexpr auto REGISTER_SCRIPTS_SYMBOL = "noi_engine_register_scripts";
    using register_scripts_fn = void (*)(noi_engine::game&);

    class bridge_game final : public noi_engine::game
    {
    public:
        bridge_game(const int game_width, const int game_height, const std::string& resources_path) :
            game(std::make_unique<noi_engine::source_resource_loader>(std::filesystem::path(resources_path))),
            m_game_width(game_width), m_game_height(game_height)
        {
        }

        ~bridge_game() override
        {
            // Intentionally not dlclose()-ing m_scripts_library_handle here: this destructor body
            // runs *before* the base noi_engine::game destructor, which tears down the scene/world -
            // including any entities holding instances of script classes whose code lives in this
            // library. Unloading it first leaves their vtables/destructors dangling and crashes.
            // Leaking the mapping for the rest of the process's life (reclaimed at exit) is the
            // safe tradeoff here.
        }

        auto run() -> void
        {
            const float dt = m_timer.tick();

            this->update(dt);

            noi_engine::renderer::begin_frame();
            render();
            noi_engine::renderer::end_frame();
        }

        auto on_resize(const int width, const int height) -> void
        {
            if (!this->get_current_scene())
            {
                return;
            }

            auto& cam = this->get_camera_2d_component();
            cam.orthographic_size = (static_cast<float>(height) / PIXELS_PER_UNIT) * 0.5f;
            cam.aspect_ratio = static_cast<float>(width) / static_cast<float>(height);

            this->m_scene_manager->on_resize(width, height);
        }

        auto set_scene_2d(const std::string& path) -> void
        {
            this->set_scene(path);
            auto& resources_ref = this->resources();

            const float world_width = static_cast<float>(this->m_game_width) / PIXELS_PER_UNIT;
            const float world_height = static_cast<float>(this->m_game_height) / PIXELS_PER_UNIT;

            constexpr float GIZMO_BORDER_PIXELS = 2.0f;
            const float border_thickness_x = (GIZMO_BORDER_PIXELS / PIXELS_PER_UNIT) / world_width;
            const float border_thickness_y = (GIZMO_BORDER_PIXELS / PIXELS_PER_UNIT) / world_height;

            const auto shader_handle = resources_ref.load_from_path<noi_engine::shader>(
                "camera_gizmo_shader", "shaders/camera_gizmo_shader/index.shader");

            auto material = std::make_unique<noi_engine::material>();
            material->set_color("u_color", glm::vec4{1, 0, 0, 1});
            material->set_parameter("u_border_thickness_x", border_thickness_x);
            material->set_parameter("u_border_thickness_y", border_thickness_y);
            material->set_shader(shader_handle);
            const auto material_handle = resources_ref.load_instance(std::move(material));
            const auto mesh_handle = resources_ref.get_handle<noi_engine::mesh>("meshes:quad_2d");

            const auto camera_entity = this->get_current_scene()->get_camera_entity();
            auto& world = this->get_current_scene()->get_world();

            world.add<noi_engine::mesh_renderer>(camera_entity, {
                                                     .m_mesh = mesh_handle,
                                                     .m_material = material_handle
                                                 });

            auto& transform = world.get<noi_engine::transform>(camera_entity);
            transform.scale = glm::vec3{world_width, world_height, 1.0};

            world.add<noi_engine::dirty<noi_engine::transform>>(camera_entity, noi_engine::dirty<noi_engine::transform>{});
        }

        auto enumerate_resources(const noi_engine_editor_bridge_resource_entry_callback callback,
                                  void* const user_data) const -> void
        {
            if (!callback)
            {
                return;
            }

            auto& resources_ref = this->resources();

            for (const auto& [label, handle] : resources_ref.meshes.entries())
                callback(user_data, NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_MESH, label.c_str(), handle.m_id, handle.m_generation);

            for (const auto& [label, handle] : resources_ref.scripts.entries())
                callback(user_data, NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_SCRIPT, label.c_str(), handle.m_id, handle.m_generation);

            for (const auto& [label, handle] : resources_ref.shaders.entries())
                callback(user_data, NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_SHADER, label.c_str(), handle.m_id, handle.m_generation);

            for (const auto& [label, handle] : resources_ref.textures.entries())
                callback(user_data, NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_TEXTURE, label.c_str(), handle.m_id, handle.m_generation);

            for (const auto& [label, handle] : resources_ref.materials.entries())
                callback(user_data, NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_MATERIAL, label.c_str(), handle.m_id, handle.m_generation);
        }

        auto enumerate_entities(const noi_engine_editor_bridge_entity_entry_callback callback,
                                 void* const user_data) -> void
        {
            if (!callback || !this->get_current_scene())
            {
                return;
            }

            auto& world = this->get_world();

            for (const auto& e : world.get_entities())
            {
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

        auto enumerate_component_properties(const uint32_t entity_id, const uint32_t entity_generation,
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

        [[nodiscard]] auto set_component_property(const uint32_t entity_id, const uint32_t entity_generation,
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

        [[nodiscard]] auto add_component(const uint32_t entity_id, const uint32_t entity_generation,
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
                world.add<noi_engine::mesh_renderer>(e, {});
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

        auto load_scripts(const std::string& scripts_library_path) -> bool
        {
            if (m_scripts_library_handle)
            {
                return true;
            }

            if (!std::filesystem::exists(scripts_library_path))
            {
                std::fprintf(stderr,
                             "[noi_engine_editor_bridge] Game scripts library not found (build the project first): %s\n",
                             scripts_library_path.c_str());
                return false;
            }

            m_scripts_library_handle = dlopen(scripts_library_path.c_str(), RTLD_NOW | RTLD_LOCAL);
            if (!m_scripts_library_handle)
            {
                std::fprintf(stderr, "[noi_engine_editor_bridge] Failed to load game scripts library %s: %s\n",
                             scripts_library_path.c_str(), dlerror());
                return false;
            }

            const auto register_scripts = reinterpret_cast<register_scripts_fn>(
                dlsym(m_scripts_library_handle, REGISTER_SCRIPTS_SYMBOL));

            if (!register_scripts)
            {
                std::fprintf(stderr, "[noi_engine_editor_bridge] Game scripts library %s has no %s entry point\n",
                             scripts_library_path.c_str(), REGISTER_SCRIPTS_SYMBOL);
                dlclose(m_scripts_library_handle);
                m_scripts_library_handle = nullptr;
                return false;
            }

            register_scripts(*this);
            return true;
        }

    private:
        [[nodiscard]] auto get_world() -> noi_engine::world&
        {
            const auto current_scene = this->get_current_scene();
            assert(current_scene);
            return current_scene->get_world();
        }

        [[nodiscard]] auto get_current_scene() const -> noi_engine::scene*
        {
            return this->m_scene_manager->current();
        }

        //TODO: handle 3d
        [[nodiscard]] auto get_camera_2d_component() -> noi_engine::camera_2d&
        {
            const auto camera_entity = this->get_current_scene()->get_camera_entity();
            auto& world = this->get_world();
            assert(world.has<noi_engine::camera_2d>(camera_entity));

            return world.get<noi_engine::camera_2d>(camera_entity);
        }

        int m_game_width{};
        int m_game_height{};
        noi_engine::timer m_timer{};
        void* m_scripts_library_handle{nullptr};
    };
}

void noi_engine_editor_bridge_init_gl(const noi_engine_editor_bridge_gl_proc_loader loader)
{
    noi_engine::game::init(loader);
}

noi_engine_editor_bridge_game_handle noi_engine_editor_bridge_create(
    const int game_width, const int game_height, const char* resources_path_utf8)
{
    auto* game = new bridge_game(game_width, game_height, std::string(resources_path_utf8));
    return reinterpret_cast<noi_engine_editor_bridge_game_handle>(game);
}

void noi_engine_editor_bridge_destroy(const noi_engine_editor_bridge_game_handle handle)
{
    delete reinterpret_cast<bridge_game*>(handle);
}

void noi_engine_editor_bridge_run(const noi_engine_editor_bridge_game_handle handle)
{
    reinterpret_cast<bridge_game*>(handle)->run();
}

void noi_engine_editor_bridge_on_resize(const noi_engine_editor_bridge_game_handle handle, const int width, const int height)
{
    reinterpret_cast<bridge_game*>(handle)->on_resize(width, height);
}

void noi_engine_editor_bridge_set_scene_2d(const noi_engine_editor_bridge_game_handle handle, const char* scene_path_utf8)
{
    reinterpret_cast<bridge_game*>(handle)->set_scene_2d(scene_path_utf8);
}

int noi_engine_editor_bridge_load_scripts(const noi_engine_editor_bridge_game_handle handle,
                                           const char* scripts_library_path_utf8)
{
    return reinterpret_cast<bridge_game*>(handle)->load_scripts(scripts_library_path_utf8) ? 1 : 0;
}

const char* noi_engine_editor_bridge_engine_version(void)
{
    return NOI_ENGINE_VERSION;
}

void noi_engine_editor_bridge_enumerate_resources(const noi_engine_editor_bridge_game_handle handle,
                                                   const noi_engine_editor_bridge_resource_entry_callback callback,
                                                   void* const user_data)
{
    reinterpret_cast<bridge_game*>(handle)->enumerate_resources(callback, user_data);
}

void noi_engine_editor_bridge_enumerate_entities(const noi_engine_editor_bridge_game_handle handle,
                                                  const noi_engine_editor_bridge_entity_entry_callback callback,
                                                  void* const user_data)
{
    reinterpret_cast<bridge_game*>(handle)->enumerate_entities(callback, user_data);
}

void noi_engine_editor_bridge_enumerate_component_properties(
    const noi_engine_editor_bridge_game_handle handle, const uint32_t entity_id, const uint32_t entity_generation,
    const char* component_type_utf8, const noi_engine_editor_bridge_property_entry_callback callback,
    void* const user_data)
{
    reinterpret_cast<bridge_game*>(handle)->enumerate_component_properties(
        entity_id, entity_generation, component_type_utf8 ? component_type_utf8 : "", callback, user_data);
}

int noi_engine_editor_bridge_set_component_property(
    const noi_engine_editor_bridge_game_handle handle, const uint32_t entity_id, const uint32_t entity_generation,
    const char* component_type_utf8, const char* property_name_utf8,
    const noi_engine_editor_bridge_property_value value)
{
    return reinterpret_cast<bridge_game*>(handle)->set_component_property(
               entity_id, entity_generation, component_type_utf8 ? component_type_utf8 : "",
               property_name_utf8 ? property_name_utf8 : "", value)
               ? 1
               : 0;
}

int noi_engine_editor_bridge_add_component(const noi_engine_editor_bridge_game_handle handle,
                                            const uint32_t entity_id, const uint32_t entity_generation,
                                            const char* component_type_utf8)
{
    return reinterpret_cast<bridge_game*>(handle)->add_component(
               entity_id, entity_generation, component_type_utf8 ? component_type_utf8 : "")
               ? 1
               : 0;
}
