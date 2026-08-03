add_library(
    openswd3_resource_io
    STATIC
        data_directory.cpp
        legacy_dbcs_text_buffer.cpp
        legacy_environment.cpp
        legacy_file.cpp
        legacy_lzo1x.cpp
        legacy_lzo1x_compressor.cpp
        legacy_memory_manager.cpp
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
