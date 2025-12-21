idf_component_register(
    INCLUDE_DIRS "."
)

add_prebuilt_library(vision_ui_lib "${CMAKE_CURRENT_LIST_DIR}/libvision_ui.a")
target_link_libraries(${COMPONENT_LIB} INTERFACE vision_ui_lib)
