find_package(glm CONFIG REQUIRED)

add_library(noi_engine_editor_bridge SHARED
        include/noi_engine_editor_bridge/bridge_api.h
        src/bridge_game.hpp
        src/bridge_game.cpp
        src/bridge_game_entities.cpp
        src/bridge_game_resources.cpp
        src/bridge_game_persistence.cpp
        src/bridge_api.cpp
)

target_include_directories(noi_engine_editor_bridge PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
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
