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
