#pragma once

/*
 * Stable C ABI boundary between noi_engine_editor and a noi_engine-version-specific
 * preview bridge (.so), built once per noi_engine version by noi_engine's own release
 * pipeline. The editor dlopens the bridge matching a project's declared engine version
 * and resolves these symbols by name (QLibrary::resolve) - never links against them.
 * Kept dependency-free (no Qt, no noi_engine types) so it compiles identically on both
 * sides regardless of which noi_engine version the bridge was built against.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct noi_engine_editor_bridge_game* noi_engine_editor_bridge_game_handle;
typedef void* (*noi_engine_editor_bridge_gl_proc_loader)(const char* name);

typedef enum noi_engine_editor_bridge_resource_type
{
    NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_MESH     = 0,
    NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_SCRIPT   = 1,
    NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_SHADER   = 2,
    NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_TEXTURE  = 3,
    NOI_ENGINE_EDITOR_BRIDGE_RESOURCE_MATERIAL = 4
} noi_engine_editor_bridge_resource_type;

typedef void (*noi_engine_editor_bridge_resource_entry_callback)(
    void* user_data, noi_engine_editor_bridge_resource_type resource_type,
    const char* label_utf8, uint32_t id, uint32_t generation);

typedef void (*noi_engine_editor_bridge_entity_entry_callback)(
    void* user_data, uint32_t entity_id, uint32_t entity_generation,
    const char* name_utf8, const char* component_types_csv);

/* Must be called once per process before any handle is created. */
void noi_engine_editor_bridge_init_gl(noi_engine_editor_bridge_gl_proc_loader loader);

noi_engine_editor_bridge_game_handle noi_engine_editor_bridge_create(
    int game_width, int game_height, const char* resources_path_utf8);

void noi_engine_editor_bridge_destroy(noi_engine_editor_bridge_game_handle handle);

void noi_engine_editor_bridge_run(noi_engine_editor_bridge_game_handle handle);

void noi_engine_editor_bridge_on_resize(noi_engine_editor_bridge_game_handle handle, int width, int height);

void noi_engine_editor_bridge_set_scene_2d(noi_engine_editor_bridge_game_handle handle, const char* scene_path_utf8);

/* Returns non-zero on success. */
int noi_engine_editor_bridge_load_scripts(noi_engine_editor_bridge_game_handle handle, const char* scripts_library_path_utf8);

/* The noi_engine version (e.g. "0.0.102") this bridge was built against. */
const char* noi_engine_editor_bridge_engine_version(void);

/*
 * Synchronously invokes callback once per currently-alive resource, across all 5 kinds.
 * Safe to call any time after create(), even before set_scene_2d(). The callback runs on
 * the calling thread, inline within this call; label_utf8 is only valid for the duration
 * of that one invocation - copy it if you need to keep it.
 */
void noi_engine_editor_bridge_enumerate_resources(
    noi_engine_editor_bridge_game_handle handle,
    noi_engine_editor_bridge_resource_entry_callback callback, void* user_data);

/*
 * Synchronously invokes callback once per currently-alive entity in the active scene's
 * world. name_utf8 is an empty string when the entity has no name component.
 * component_types_csv is a comma-separated list of the entity's attached component type
 * names (e.g. "transform,mesh_renderer"), empty string if none. Safe to call any time; if
 * no scene is loaded yet, callback is simply never invoked. Same lifetime rules as
 * noi_engine_editor_bridge_enumerate_resources apply to both string arguments.
 */
void noi_engine_editor_bridge_enumerate_entities(
    noi_engine_editor_bridge_game_handle handle,
    noi_engine_editor_bridge_entity_entry_callback callback, void* user_data);

#ifdef __cplusplus
}
#endif
