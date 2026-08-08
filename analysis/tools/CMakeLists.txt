add_executable(
    openswd3_verify_legacy_lzo1x_tsw
    verify_legacy_lzo1x_tsw.cpp
)
target_link_libraries(
    openswd3_verify_legacy_lzo1x_tsw
    PRIVATE openswd3_resource_io
)

add_executable(
    openswd3_verify_legacy_lzo1x_save_roundtrip
    verify_legacy_lzo1x_save_roundtrip.cpp
)
target_link_libraries(
    openswd3_verify_legacy_lzo1x_save_roundtrip
    PRIVATE openswd3_resource_io
)

if(WIN32)
    add_executable(
        openswd3_probe_gdi_glyph_oracle
        probe_gdi_glyph_oracle.cpp
    )
    target_link_libraries(
        openswd3_probe_gdi_glyph_oracle
        PRIVATE
            openswd3_rendering
            openswd3_project_options
            openswd3_project_warnings
            gdi32
    )
    set_property(
        TARGET openswd3_probe_gdi_glyph_oracle
        PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()
