add_library(
    openswd3_input_time_rng
    STATIC
        legacy_frame_clock.cpp
        legacy_input.cpp
        legacy_crt_rng.cpp
        legacy_secondary_rng.cpp
)
target_include_directories(
    openswd3_input_time_rng
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(
    openswd3_input_time_rng
    PUBLIC openswd3_compat openswd3_project_options openswd3_project_warnings
)
