find_package(Threads REQUIRED)

add_library(openswd3_diagnostics STATIC log.cpp)
target_include_directories(
    openswd3_diagnostics
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(
    openswd3_diagnostics
    PUBLIC
        openswd3_project_options
        openswd3_project_warnings
        Threads::Threads
)
