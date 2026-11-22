# Packaging: `cmake --build --preset release --target package` produces, in
# build/release/packages, a relocatable game folder in an archive:
#
#   <name>/
#     <name executable>      the game
#     libs/                  noi_engine + the game's scripts (next to the executable on Windows)
#     resources/             the compiled resources
#
# Unzip it anywhere and run the executable: resources are found next to it, whatever the
# current directory is.

if(WIN32)
    # Windows has no RPATH: the DLLs must sit next to the executable.
    set(GAME_LIBS_DESTINATION ".")
else()
    set(GAME_LIBS_DESTINATION "libs")
endif()

install(TARGETS noi_engine_game RUNTIME DESTINATION ".")
install(TARGETS game_scripts LIBRARY DESTINATION "${GAME_LIBS_DESTINATION}" RUNTIME DESTINATION "${GAME_LIBS_DESTINATION}")

if(WIN32)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/.noi_engine/libs/" DESTINATION "${GAME_LIBS_DESTINATION}"
            FILES_MATCHING PATTERN "*.dll")
else()
    # USE_SOURCE_PERMISSIONS keeps the symlinks of versioned libraries (libnoi_engine.so.0 -> ...).
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/.noi_engine/libs/" DESTINATION "${GAME_LIBS_DESTINATION}"
            USE_SOURCE_PERMISSIONS)
endif()

install(DIRECTORY "${CMAKE_BINARY_DIR}/bin/resources" DESTINATION ".")

string(TOLOWER "${CMAKE_SYSTEM_NAME}" GAME_PACKAGE_OS)
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" GAME_PACKAGE_ARCH)
if(GAME_PACKAGE_ARCH STREQUAL "amd64")
    set(GAME_PACKAGE_ARCH "x86_64")
endif()

string(REGEX REPLACE "[^A-Za-z0-9_.-]" "_" GAME_PACKAGE_SAFE_NAME "${GAME_NAME}")

set(CPACK_PACKAGE_NAME "${GAME_PACKAGE_SAFE_NAME}")
set(CPACK_PACKAGE_VERSION "${GAME_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${GAME_PACKAGE_SAFE_NAME}-${GAME_VERSION}-${GAME_PACKAGE_OS}-${GAME_PACKAGE_ARCH}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_STRIP_FILES ON)
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    # tar keeps the executable bit, zip may not.
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
