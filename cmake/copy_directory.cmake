function(add_copy_directory_dependency TARGET_NAME DIR_NAME SOURCE_DIR OUTPUT_BASE_DIR)
    set(DEST_DIR "${OUTPUT_BASE_DIR}/${DIR_NAME}")
    set(CUSTOM_TARGET_NAME "${TARGET_NAME}_copy_${DIR_NAME}")

    add_custom_command(
        OUTPUT ${DEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SOURCE_DIR}
        ${DEST_DIR}
        DEPENDS ${SOURCE_DIR}
    )

    add_custom_target(${CUSTOM_TARGET_NAME} DEPENDS ${DEST_DIR})
    add_dependencies(${TARGET_NAME} ${CUSTOM_TARGET_NAME})
endfunction()