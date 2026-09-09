cmake_minimum_required(VERSION 3.18)

project(noi_engine_game VERSION 0.0.1 DESCRIPTION "noi_engine_game" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/project_config.cmake)
include(cmake/copy_directory.cmake)

set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set(NOI_ENGINE_GAME_SCRIPTS_SOURCES
        sources/signals/moved_signal.hpp
        sources/scripts/camera_script.hpp
        sources/scripts/camera_script.cpp
        sources/scripts/player_script.hpp
        sources/scripts/player_script.cpp
        sources/scripts/scripts_register.hpp
        sources/scripts/scripts_register.cpp
)

add_library(game_scripts SHARED ${NOI_ENGINE_GAME_SCRIPTS_SOURCES})

target_include_directories(game_scripts PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.noi_engine/include)
target_link_directories(game_scripts PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.noi_engine/libs)
target_link_libraries(game_scripts PRIVATE noi_engine)

set_target_properties(game_scripts PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/libs"
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)

set(NOI_ENGINE_GAME_HEADERS)

set(NOI_ENGINE_GAME_SOURCES
        sources/main.cpp
        sources/config/config.hpp
        sources/config/config.cpp
)

add_executable(noi_engine_game ${NOI_ENGINE_GAME_SOURCES} ${NOI_ENGINE_GAME_HEADERS})

# Engine(begin)
target_include_directories(noi_engine_game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.noi_engine/include)
target_link_directories(noi_engine_game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.noi_engine/libs)
target_link_libraries(noi_engine_game PRIVATE noi_engine)
# Engine(end)

target_link_libraries(noi_engine_game PRIVATE game_scripts)

set_target_properties(noi_engine_game PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set_target_properties(noi_engine_game PROPERTIES
    BUILD_RPATH "$ORIGIN/libs"
    INSTALL_RPATH "$ORIGIN/libs"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_copy_directory_dependency(noi_engine_game "resources" "${CMAKE_CURRENT_SOURCE_DIR}/resources" "${CMAKE_BINARY_DIR}/bin")

    find_package(glm CONFIG REQUIRED)

    add_library(noi_engine_editor_bridge SHARED .editor_bridge/src/bridge.cpp)

    target_include_directories(noi_engine_editor_bridge PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/.editor_bridge/include
        ${CMAKE_CURRENT_SOURCE_DIR}/.noi_engine/include
    )
    target_link_directories(noi_engine_editor_bridge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.noi_engine/libs)
    target_link_libraries(noi_engine_editor_bridge PRIVATE noi_engine glm::glm)

    set_target_properties(noi_engine_editor_bridge PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/libs"
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
else()
    include(.resources_compiler/ResourcesCompiler.cmake)

    compile_resources(
        "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        "${CMAKE_BINARY_DIR}/bin/resources"
    )
endif()

add_copy_directory_dependency(noi_engine_game "libs" "${CMAKE_CURRENT_SOURCE_DIR}/.noi_engine/libs" "${CMAKE_BINARY_DIR}/bin")

if(DEFINED GAME_GENERATED_DIR)
    target_include_directories(noi_engine_game PRIVATE ${GAME_GENERATED_DIR})
endif()