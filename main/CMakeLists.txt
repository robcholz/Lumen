file(GLOB SRC_FILES "src/*.cpp")

idf_component_register(
        SRCS
        main.cpp
        ${SRC_FILES}
        INCLUDE_DIRS
        .
        REQUIRES
        vision-ui
        u8g2
        esp_driver_ledc
        esp_driver_i2c
        esp_driver_spi
        esp_lcd
        ina226
)

find_package(Git QUIET)

if (GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --always
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_DESCRIBE
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else ()
    set(GIT_COMMIT_HASH "unknown")
    set(GIT_DESCRIBE "unknown")
endif ()

add_compile_definitions(
        LUMEN_GIT_COMMIT="${GIT_COMMIT_HASH}"
        LUMEN_GIT_TAG="${GIT_DESCRIBE}"
)

message(STATUS "Git commit: ${GIT_COMMIT_HASH}")
message(STATUS "Git tag: ${GIT_DESCRIBE}")

add_subdirectory(main_app)
target_link_libraries(${COMPONENT_LIB} PRIVATE main_app_lib)
