set(PROJECT_JSON "${CMAKE_SOURCE_DIR}/noi_engine.proj.json")

set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROJECT_JSON}"
)

file(READ "${PROJECT_JSON}" PROJECT_JSON_CONTENT)

string(JSON GAME_NAME GET "${PROJECT_JSON_CONTENT}" name)
string(JSON GAME_VERSION GET "${PROJECT_JSON_CONTENT}" version)
string(JSON GAME_CATEGORY GET "${PROJECT_JSON_CONTENT}" category)
string(JSON GAME_START_SCENE GET "${PROJECT_JSON_CONTENT}" start_scene)
string(JSON GAME_WINDOW_WIDTH GET "${PROJECT_JSON_CONTENT}" window width)
string(JSON GAME_WINDOW_HEIGHT GET "${PROJECT_JSON_CONTENT}" window height)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(GAME_ASSET_PROVIDER_CLASS "compiled") # TODO
else()
    set(GAME_ASSET_PROVIDER_CLASS "source")
endif()

set(GAME_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")

file(MAKE_DIRECTORY "${GAME_GENERATED_DIR}")

configure_file(
    "${CMAKE_SOURCE_DIR}/.templates/game_config.hpp.in"
    "${GAME_GENERATED_DIR}/game_config.hpp"
    @ONLY
)