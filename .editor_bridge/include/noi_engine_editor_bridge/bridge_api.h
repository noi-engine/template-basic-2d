#pragma once

/*
 * Stable C ABI boundary between noi_engine_editor and a project-specific
 * preview bridge (.so), built as part of this project's own Debug build.
 * The editor dlopens the bridge produced under build/debug and resolves
 * these symbols by name (QLibrary::resolve) - never links against them.
 * Kept dependency-free (no Qt) so it compiles identically on both sides.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct noi_engine_editor_bridge_game* noi_engine_editor_bridge_game_handle;
typedef void* (*noi_engine_editor_bridge_gl_proc_loader)(const char* name);

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

#ifdef __cplusplus
}
#endif
