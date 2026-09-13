#include "bridge_game.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <memory>

#include <dlfcn.h>

#include <glm/glm.hpp>

#include <noi_engine/core/renderer/renderer.hpp>
#include <noi_engine/core/resources/resource_loader.hpp>

namespace noi_engine_editor_bridge_detail
{
    namespace
    {
        constexpr float PIXELS_PER_UNIT = 100.0f;
        constexpr auto REGISTER_SCRIPTS_SYMBOL = "noi_engine_register_scripts";
        using register_scripts_fn = void (*)(noi_engine::game&);
    }

    bridge_game::bridge_game(const int game_width, const int game_height, const std::string& resources_path) :
        game(std::make_unique<noi_engine::source_resource_loader>(std::filesystem::path(resources_path))),
        m_game_width(game_width), m_game_height(game_height)
    {
    }

    bridge_game::~bridge_game()
    {
        // Intentionally not dlclose()-ing m_scripts_library_handle here: this destructor body
        // runs *before* the base noi_engine::game destructor, which tears down the scene/world -
        // including any entities holding instances of script classes whose code lives in this
        // library. Unloading it first leaves their vtables/destructors dangling and crashes.
        // Leaking the mapping for the rest of the process's life (reclaimed at exit) is the
        // safe tradeoff here.
    }

    auto bridge_game::run() -> void
    {
        const float dt = m_timer.tick();

        this->update(dt);

        noi_engine::renderer::begin_frame();
        render();
        noi_engine::renderer::end_frame();

        // Must run after this frame's update()/render() consumed the current edge state,
        // and before the next frame's key deltas are applied (see set_key_pressed/released) -
        // otherwise is_key_pressed()/is_key_released() could never reset.
        this->input().begin_frame();
    }

    auto bridge_game::reset_timer() -> void
    {
        m_timer = noi_engine::timer{};
    }

    auto bridge_game::set_key_pressed(const int key) -> void
    {
        m_held_keys.insert(key);
        this->input().set_key_pressed(static_cast<noi_engine::key_code>(key));
    }

    auto bridge_game::set_key_released(const int key) -> void
    {
        m_held_keys.erase(key);
        this->input().set_key_released(static_cast<noi_engine::key_code>(key));
    }

    auto bridge_game::release_all_keys() -> void
    {
        for (const auto key : m_held_keys)
        {
            this->input().set_key_released(static_cast<noi_engine::key_code>(key));
        }
        m_held_keys.clear();
    }

    auto bridge_game::on_resize(const int width, const int height) -> void
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

    auto bridge_game::set_scene_2d(const std::string& path) -> void
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

    auto bridge_game::load_scripts(const std::string& scripts_library_path) -> bool
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

    auto bridge_game::get_world() -> noi_engine::world&
    {
        const auto current_scene = this->get_current_scene();
        assert(current_scene);
        return current_scene->get_world();
    }

    auto bridge_game::get_current_scene() const -> noi_engine::scene*
    {
        return this->m_scene_manager->current();
    }

    auto bridge_game::get_camera_2d_component() -> noi_engine::camera_2d&
    {
        const auto camera_entity = this->get_current_scene()->get_camera_entity();
        auto& world = this->get_world();
        assert(world.has<noi_engine::camera_2d>(camera_entity));

        return world.get<noi_engine::camera_2d>(camera_entity);
    }
}
