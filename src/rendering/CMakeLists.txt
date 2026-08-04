add_library(
    openswd3_rendering STATIC
    legacy_blitter.cpp
    legacy_framebuffer.cpp
    legacy_glyph_cache.cpp
    legacy_pixel_conversion.cpp
)
target_include_directories(
    openswd3_rendering
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(
    openswd3_rendering
    PUBLIC openswd3_compat openswd3_project_options openswd3_project_warnings
)
