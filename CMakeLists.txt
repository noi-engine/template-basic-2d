cmake_minimum_required(VERSION 3.18)

project(noi_engine_game VERSION 0.0.1 DESCRIPTION "noi_engine_game" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/project_config.cmake)
include(cmake/copy_directory.cmake)

set(NOI_ENGINE_GAME_HEADERS)

set(NOI_ENGINE_GAME_SOURCES
        sources/main.cpp
)

add_executable(noi_engine_game ${NOI_ENGINE_GAME_SOURCES} ${NOI_ENGINE_GAME_HEADERS})

# Engine(begin)
target_include_directories(noi_engine_game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.noi_engine/include)
target_link_directories(noi_engine_game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.noi_engine/libs)
target_link_libraries(noi_engine_game PRIVATE noi_engine)
# Engine(end)

set_target_properties(noi_engine_game PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set_target_properties(noi_engine_game PROPERTIES
    BUILD_RPATH "$ORIGIN/libs"
    INSTALL_RPATH "$ORIGIN/libs"
)

add_copy_directory_dependency(noi_engine_game "resources" "${CMAKE_CURRENT_SOURCE_DIR}/resources" "${CMAKE_BINARY_DIR}/bin")
add_copy_directory_dependency(noi_engine_game "libs" "${CMAKE_CURRENT_SOURCE_DIR}/.noi_engine/libs" "${CMAKE_BINARY_DIR}/bin")

if(DEFINED GAME_GENERATED_DIR)
    target_include_directories(noi_engine_game PRIVATE ${GAME_GENERATED_DIR})
endif()