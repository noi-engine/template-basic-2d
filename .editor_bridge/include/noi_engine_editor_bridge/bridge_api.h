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

typedef enum noi_engine_editor_bridge_property_type
{
    NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_FLOAT        = 0,
    NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_VEC3         = 1,
    NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_STRING       = 2,
    NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_BOOL         = 3,
    NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_VEC4         = 4,
    /* text holds the currently-bound resource's label (may be empty if unbound); resource_type
     * says which of the 5 loaded-resource pools the editor should offer as candidates. */
    NOI_ENGINE_EDITOR_BRIDGE_PROPERTY_RESOURCE_REF = 5
} noi_engine_editor_bridge_property_type;

/* Tagged union of a single property's value. Only the member matching `type` is valid:
 * FLOAT uses number[0]; VEC3 uses number[0..2]; VEC4 uses number[0..3]; STRING and
 * RESOURCE_REF use text (may be NULL/empty); BOOL uses flag; RESOURCE_REF also uses
 * resource_type to say which resource kind it references. */
typedef struct noi_engine_editor_bridge_property_value
{
    noi_engine_editor_bridge_property_type type;
    float number[4];
    const char* text;
    int flag;
    noi_engine_editor_bridge_resource_type resource_type;
} noi_engine_editor_bridge_property_value;

typedef void (*noi_engine_editor_bridge_property_entry_callback)(
    void* user_data, const char* property_name_utf8,
    noi_engine_editor_bridge_property_value value, int read_only);

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

/*
 * Synchronously invokes callback once per editable/viewable property of the given
 * component type attached to the given entity. No-op if the entity or component doesn't
 * exist. read_only is non-zero for properties that shouldn't be presented as editable
 * (e.g. resolved resource-handle labels, computed matrices).
 */
void noi_engine_editor_bridge_enumerate_component_properties(
    noi_engine_editor_bridge_game_handle handle, uint32_t entity_id, uint32_t entity_generation,
    const char* component_type_utf8,
    noi_engine_editor_bridge_property_entry_callback callback, void* user_data);

/*
 * Writes a single property back onto a component. Returns non-zero on success (entity,
 * component, and property all found, and value.type matches what enumerate reported for
 * that property). A no-op/failure on a read-only property.
 */
int noi_engine_editor_bridge_set_component_property(
    noi_engine_editor_bridge_game_handle handle, uint32_t entity_id, uint32_t entity_generation,
    const char* component_type_utf8, const char* property_name_utf8,
    noi_engine_editor_bridge_property_value value);

/*
 * Adds a default-constructed component of the given type to the entity. Returns non-zero
 * on success; fails (returns 0) if the entity already has that component, the component
 * type name is unrecognized, or no scene is loaded.
 */
int noi_engine_editor_bridge_add_component(
    noi_engine_editor_bridge_game_handle handle, uint32_t entity_id, uint32_t entity_generation,
    const char* component_type_utf8);

/*
 * Synchronously invokes callback once per editable property of the material identified by
 * (material_id, material_generation): always "shader" (RESOURCE_REF, SHADER), plus this
 * material's own color/parameter/texture entries (same "color:"/"param:"/"texture:" name
 * prefixing as component properties). No-op if the handle is stale/invalid.
 */
void noi_engine_editor_bridge_enumerate_material_properties(
    noi_engine_editor_bridge_game_handle handle, uint32_t material_id, uint32_t material_generation,
    noi_engine_editor_bridge_property_entry_callback callback, void* user_data);

/* Writes a single property back onto a material. Returns non-zero on success. */
int noi_engine_editor_bridge_set_material_property(
    noi_engine_editor_bridge_game_handle handle, uint32_t material_id, uint32_t material_generation,
    const char* property_name_utf8, noi_engine_editor_bridge_property_value value);

/*
 * Keyboard-only input forwarding for Play mode. key_code matches noi_engine::key_code's
 * numeric layout (SDL_Scancode-compatible - see noi_engine/core/input/input_codes.hpp).
 */
void noi_engine_editor_bridge_set_key_pressed(noi_engine_editor_bridge_game_handle handle, int key_code);
void noi_engine_editor_bridge_set_key_released(noi_engine_editor_bridge_game_handle handle, int key_code);

/* Releases every key the bridge still considers held (e.g. when Play mode stops). */
void noi_engine_editor_bridge_release_all_keys(noi_engine_editor_bridge_game_handle handle);

/*
 * Re-syncs the bridge's frame-delta timer to "now". Call this exactly once when continuous
 * Play-mode ticking starts, so the first tick's dt doesn't include time spent idle in the
 * editor since the last on-demand render.
 */
void noi_engine_editor_bridge_reset_timer(noi_engine_editor_bridge_game_handle handle);

/*
 * Shows/hides the editor-only reference grid added by set_scene_2d() (fixed in world space,
 * not attached to the camera, purely a visual aid for confirming camera movement in the
 * preview). No-op if no scene is loaded yet.
 */
void noi_engine_editor_bridge_set_grid_visible(noi_engine_editor_bridge_game_handle handle, int visible);

#ifdef __cplusplus
}
#endif
