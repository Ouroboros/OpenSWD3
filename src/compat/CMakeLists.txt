add_library(openswd3_compat STATIC legacy_decimal.cpp)
target_include_directories(
    openswd3_compat
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(
    openswd3_compat
    PUBLIC openswd3_project_options openswd3_project_warnings
)
