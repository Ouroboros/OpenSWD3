add_library(
    openswd3_resource_io
    STATIC
        data_directory.cpp
        legacy_file.cpp
        legacy_lzo1x.cpp
)
target_include_directories(
    openswd3_resource_io
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(
    openswd3_resource_io
    PUBLIC openswd3_compat openswd3_project_options openswd3_project_warnings
    PRIVATE tomlplusplus::tomlplusplus
)
