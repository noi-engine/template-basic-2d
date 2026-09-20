#include "bridge_game.hpp"

#include <glm/glm.hpp>

namespace noi_engine_editor_bridge_detail
{
    auto bridge_game::enumerate_resources(const noi_engine_editor_bridge_resource_entry_callback callback,
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

    auto bridge_game::enumerate_material_properties(const uint32_t material_id, const uint32_t material_generation,
                                                       const noi_engine_editor_bridge_property_entry_callback callback,
                                                       void* const user_data) -> void
    {
        if (!callback)
        {
            return;
        }

        const noi_engine::resource_handle<noi_engine::material> handle{material_id, material_generation};
        auto& resources_ref = this->resources();

        if (!resources_ref.materials.valid(handle))
        {
            return;
        }

        auto* mat = resources_ref.materials.get(handle);

        const auto emit_float = [&](const char* name, const float v, const bool read_only)
        {
            noi_engine_editor_bridge_property_value pv{};
            pv.type = NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_FLOAT;
            pv.number[0] = v;
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

        const std::string shader_label{resources_ref.get_label<noi_engine::shader>(mat->shader())};
        emit_resource_ref("shader", shader_label, NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_SHADER, false);

        for (const auto& [key, v] : mat->get_parameters())
        {
            emit_float(("param:" + key).c_str(), v, false);
        }
        for (const auto& [key, v] : mat->get_colors())
        {
            emit_vec4(("color:" + key).c_str(), v, false);
        }
        for (const auto& [key, texture_handle] : mat->get_textures())
        {
            const std::string label{resources_ref.get_label<noi_engine::texture>(texture_handle)};
            emit_resource_ref(("texture:" + key).c_str(), label, NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_TEXTURE, false);
        }
    }

    auto bridge_game::set_material_property(const uint32_t material_id, const uint32_t material_generation,
                                             const std::string& property_name,
                                             const noi_engine_editor_bridge_property_value& value) -> bool
    {
        const noi_engine::resource_handle<noi_engine::material> handle{material_id, material_generation};
        auto& resources_ref = this->resources();

        if (!resources_ref.materials.valid(handle))
        {
            return false;
        }

        auto* mat = resources_ref.materials.get(handle);

        if (property_name == "shader" && value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_RESOURCE_REF)
        {
            const std::string label = value.text ? value.text : "";
            mat->set_shader(resources_ref.get_handle<noi_engine::shader>(label));
            return true;
        }
        if (property_name.rfind("param:", 0) == 0 && value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_FLOAT)
        {
            mat->set_parameter(property_name.substr(6), value.number[0]);
            return true;
        }
        if (property_name.rfind("color:", 0) == 0 && value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_VEC4)
        {
            mat->set_color(property_name.substr(6),
                            glm::vec4{value.number[0], value.number[1], value.number[2], value.number[3]});
            return true;
        }
        if (property_name.rfind("texture:", 0) == 0 && value.type == NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_RESOURCE_REF)
        {
            const std::string label = value.text ? value.text : "";
            mat->set_texture(property_name.substr(8), resources_ref.get_handle<noi_engine::texture>(label));
            return true;
        }

        return false;
    }

    auto bridge_game::remove_material_property(const uint32_t material_id, const uint32_t material_generation,
                                                 const std::string& property_name) -> bool
    {
        const noi_engine::resource_handle<noi_engine::material> handle{material_id, material_generation};
        auto& resources_ref = this->resources();

        if (!resources_ref.materials.valid(handle))
        {
            return false;
        }

        auto* mat = resources_ref.materials.get(handle);

        if (property_name.rfind("param:", 0) == 0)
        {
            if (!mat->get_parameters().contains(property_name.substr(6))) return false;
            mat->remove_parameter(property_name.substr(6));
            return true;
        }
        if (property_name.rfind("color:", 0) == 0)
        {
            if (!mat->get_colors().contains(property_name.substr(6))) return false;
            mat->remove_color(property_name.substr(6));
            return true;
        }
        if (property_name.rfind("texture:", 0) == 0)
        {
            if (!mat->get_textures().contains(property_name.substr(8))) return false;
            mat->remove_texture(property_name.substr(8));
            return true;
        }

        return false;
    }
}
