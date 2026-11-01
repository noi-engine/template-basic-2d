find_package(glm CONFIG REQUIRED)

add_library(noi_engine_editor_bridge SHARED
        include/noi_engine_editor_bridge/bridge_api.h
        src/bridge_api.cpp
        src/bridge_game.hpp
        src/bridge_game.cpp
        src/bridge_game_overlay.cpp
        src/bridge_game_entities.cpp
        src/bridge_game_components.cpp
        src/bridge_game_resources.cpp
        src/bridge_game_persistence.cpp

        src/properties/property_emitter.hpp
        src/properties/property_value.hpp
        src/persistence/json_utils.hpp

        src/components/component_handler.hpp
        src/components/component_registry.hpp
        src/components/component_registry.cpp
        src/components/box_collider.cpp
        src/components/camera_2d.cpp
        src/components/camera_3d.cpp
        src/components/gravity.cpp
        src/components/mesh_renderer.cpp
        src/components/mesh_renderer_properties.hpp
        src/components/mesh_renderer_properties.cpp
        src/components/name_component.cpp
        src/components/orbit_camera.cpp
        src/components/render_layer.cpp
        src/components/script_component.cpp
        src/components/transform.cpp
        src/components/world_matrix.cpp
)

target_include_directories(noi_engine_editor_bridge PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
        ${CMAKE_SOURCE_DIR}/.noi_engine/include
)

target_link_directories(noi_engine_editor_bridge PRIVATE ${CMAKE_SOURCE_DIR}/.noi_engine/libs)
target_link_libraries(noi_engine_editor_bridge PRIVATE noi_engine glm::glm)

set_target_properties(noi_engine_editor_bridge PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/libs"
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
)
