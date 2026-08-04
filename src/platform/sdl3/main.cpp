#include "event_translation.hpp"
#include "external_launch_sdl3.hpp"
#include "keyboard_snapshot_sdl3.hpp"
#include "legacy_command_line.hpp"
#include "mouse_sdl3.hpp"
#include "single_instance.hpp"
#include "startup_dialog_sdl3.hpp"

#include "openswd3/app/host_window_event.hpp"
#include "openswd3/app/idle_runtime.hpp"
#include "openswd3/app/frame_preparation.hpp"
#include "openswd3/app/frame_runtime.hpp"
#include "openswd3/app/initialization.hpp"
#include "openswd3/app/platform_backend_initialization.hpp"
#include "openswd3/app/process_startup.hpp"
#include "openswd3/app/startup.hpp"
#include "openswd3/app/window_events.hpp"
#include "openswd3/diagnostics/log.hpp"
#include "openswd3/input_time_rng/legacy_crt_rng.hpp"
#include "openswd3/input_time_rng/legacy_frame_clock.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/resource_io/data_directory.hpp"
#include "openswd3/resource_io/legacy_memory_manager.hpp"
#include "openswd3/resource_io/legacy_resource_databases.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdint>
#include <ctime>
#include <filesystem>
#include <source_location>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

constexpr int kFrameWidth = 640;
constexpr int kFrameHeight = 480;
constexpr int kInitialWindowWidth = kFrameWidth * 3 / 2;
constexpr int kInitialWindowHeight = kFrameHeight * 3 / 2;
constexpr int kFramePitch = kFrameWidth * static_cast<int>(sizeof(std::uint16_t));
constexpr openswd3::compat::u32 kInitialFrameIntervalMilliseconds = 35U;

openswd3::resource_io::LegacyMemoryManager legacy_memory_manager;

class LoggingShutdownGuard {
public:
    ~LoggingShutdownGuard() {
        openswd3::diagnostics::log_info("OpenSWD3 process stopped");
        openswd3::diagnostics::shutdown_logging();
    }
};

int report_error(
    const std::string_view message,
    const std::source_location location = std::source_location::current()
) {
    openswd3::diagnostics::log_error(message, location);
    return 1;
}

int report_sdl_error(
    const char* operation,
    const std::source_location location = std::source_location::current()
) {
    std::string message{operation};
    message.append(": ");
    message.append(SDL_GetError());
    return report_error(message, location);
}

[[nodiscard]] bool present_framebuffer(
    SDL_Renderer& renderer,
    SDL_Texture& texture,
    const std::vector<std::uint16_t>& framebuffer
) {
    return SDL_UpdateTexture(
               &texture,
               nullptr,
               framebuffer.data(),
               kFramePitch
           ) &&
           SDL_SetRenderDrawColor(&renderer, 0, 0, 0, 255) &&
           SDL_RenderClear(&renderer) &&
           SDL_RenderTexture(&renderer, &texture, nullptr, nullptr) &&
           SDL_RenderPresent(&renderer);
}

class SmokeWindowEventPorts final
    : public openswd3::app::WindowEventPorts {
public:
    void release_active_video() override {}

    openswd3::compat::u32 free_disk_space_mebibytes() override {
        return 0U;
    }

    void capture_legacy_screenshot() override {}
};

class SmokeCommandLinePorts final : public openswd3::app::CommandLinePorts {
public:
    void initialize_float_conversion() override {}

    void run_legacy_command(
        openswd3::compat::u8,
        std::string_view
    ) override {}
};

class SdlRngSeedPorts final : public openswd3::app::RngSeedPorts {
public:
    SdlRngSeedPorts(
        openswd3::input_time_rng::LegacyCrtRng& crt_rng,
        openswd3::input_time_rng::LegacySecondaryRng& secondary_rng
    ) : crt_rng_(crt_rng), secondary_rng_(secondary_rng) {}

    openswd3::compat::u32 read_time_seconds() override {
        return static_cast<openswd3::compat::u32>(std::time(nullptr));
    }

    void seed_crt_rng(const openswd3::compat::u32 seed) override {
        crt_rng_.seed(seed);
    }

    void seed_secondary_rng(const openswd3::compat::u32 seed) override {
        secondary_rng_.seed(seed);
    }

private:
    openswd3::input_time_rng::LegacyCrtRng& crt_rng_;
    openswd3::input_time_rng::LegacySecondaryRng& secondary_rng_;
};

class SdlSmokePlatformBackendPorts final
    : public openswd3::app::PlatformBackendInitializationPorts {
public:
    SdlSmokePlatformBackendPorts(
        SDL_Renderer& renderer,
        SDL_Texture*& texture,
        bool& destroy_requested
    )
        : renderer_(renderer),
          texture_(texture),
          destroy_requested_(destroy_requested) {}

    bool initialize_input_backend() override { return true; }
    void report_input_initialization_failure() override {}
    void request_synchronous_destroy() override {
        destroy_requested_ = true;
    }

    void start_audio_runtime(std::string_view) override {}
    void initialize_audio_output(std::string_view) override {}
    openswd3::app::BackendToken query_audio_driver() override { return 0U; }
    void initialize_midi_output(openswd3::app::BackendToken) override {}
    void initialize_audio_sequence_nodes(
        openswd3::app::BackendToken
    ) override {}

    bool initialize_display_backend(
        const openswd3::app::DisplayInitializationRequest request
    ) override {
        texture_ = SDL_CreateTexture(
            &renderer_,
            SDL_PIXELFORMAT_RGB565,
            SDL_TEXTUREACCESS_STREAMING,
            static_cast<int>(request.width),
            static_cast<int>(request.height)
        );
        return texture_ != nullptr;
    }
    void report_display_initialization_failure() override {
        static_cast<void>(report_sdl_error("SDL_CreateTexture"));
    }
    openswd3::app::BackendToken create_common_source_surface(
        openswd3::compat::u32,
        openswd3::compat::u32
    ) override {
        return reinterpret_cast<openswd3::app::BackendToken>(texture_);
    }
    openswd3::app::BackendToken query_display_surface(
        openswd3::compat::u32
    ) override {
        return reinterpret_cast<openswd3::app::BackendToken>(texture_);
    }
    void configure_video_audio(
        openswd3::app::BackendToken,
        openswd3::compat::u8,
        openswd3::app::BackendToken
    ) override {}

private:
    SDL_Renderer& renderer_;
    SDL_Texture*& texture_;
    bool& destroy_requested_;
};

class SdlSmokeInitializationPorts final
    : public openswd3::app::InitializationPorts {
public:
    SdlSmokeInitializationPorts(
        openswd3::app::PlatformBackendState& backend_state,
        openswd3::app::PlatformBackendInitializationPorts& backend_ports,
        openswd3::resource_io::LegacyResourceDatabases& resource_databases,
        const std::filesystem::path& data_directory,
        SDL_Renderer& renderer,
        openswd3::input_time_rng::LegacyMouseState& mouse_state,
        openswd3::platform_sdl3::SdlMouseDeviceState& mouse_device_state,
        openswd3::compat::u32& frame_interval,
        bool& destroy_requested
    )
        : backend_state_(backend_state),
          backend_ports_(backend_ports),
          resource_databases_(resource_databases),
          data_directory_(data_directory),
          renderer_(renderer),
          mouse_state_(mouse_state),
          mouse_device_state_(mouse_device_state),
          frame_interval_(frame_interval),
          destroy_requested_(destroy_requested) {}

    void hide_cursor() override {
        static_cast<void>(SDL_HideCursor());
    }

    bool initialize_platform_backends() override {
        const char* base_path = SDL_GetBasePath();
        return openswd3::app::run_platform_backend_initialization(
            base_path == nullptr ? std::string_view{} : std::string_view{base_path},
            backend_state_,
            backend_ports_
        );
    }

    void configure_input_and_audio_paths() override {
        openswd3::input_time_rng::set_mouse_sensitivity(mouse_state_, 2.0);
        const auto sample =
            openswd3::platform_sdl3::sample_sdl_mouse_state(
                renderer_,
                mouse_device_state_
            );
        openswd3::input_time_rng::rebase_mouse_coordinates(
            mouse_state_,
            sample,
            480,
            360
        );
    }
    bool initialize_software_drawing() override { return true; }
    void report_software_drawing_failure() override {}
    void check_legacy_memory_capacity() override {}
    void initialize_resource_database() override {
        const auto result = resource_databases_.initialize(data_directory_);
        using Status =
            openswd3::resource_io::LegacyResourceDatabaseStatus;
        switch (result.status) {
        case Status::ready:
            return;
        case Status::maps_open_failed:
            openswd3::diagnostics::log_error("RoleDataBase init Failed.");
            break;
        case Status::path_open_failed:
            openswd3::diagnostics::log_error("PathDataBase init Failed.");
            break;
        case Status::talk_open_failed:
            openswd3::diagnostics::log_error("StoryDataBase init Failed.");
            break;
        }

        destroy_requested_ = true;
    }
    void initialize_render_resources() override {}
    bool initialize_frame_interval_35() override {
        return openswd3::input_time_rng::set_frame_interval(
                   frame_interval_,
                   kInitialFrameIntervalMilliseconds
               ) != 0;
    }
    void report_frame_clock_failure() override {}
    void request_synchronous_destroy() override {
        destroy_requested_ = true;
    }
    void initialize_story_world_and_asset_state() override {}

private:
    openswd3::app::PlatformBackendState& backend_state_;
    openswd3::app::PlatformBackendInitializationPorts& backend_ports_;
    openswd3::resource_io::LegacyResourceDatabases& resource_databases_;
    const std::filesystem::path& data_directory_;
    SDL_Renderer& renderer_;
    openswd3::input_time_rng::LegacyMouseState& mouse_state_;
    openswd3::platform_sdl3::SdlMouseDeviceState& mouse_device_state_;
    openswd3::compat::u32& frame_interval_;
    bool& destroy_requested_;
};

class SdlSmokeStartupPorts final : public openswd3::app::StartupPorts {
public:
    SdlSmokeStartupPorts(
        openswd3::platform_sdl3::SdlStartupDialog& dialog,
        openswd3::app::InitializationState& initialization_state,
        openswd3::app::InitializationPorts& initialization_ports,
        bool& game_initialized,
        bool& destroy_requested
    )
        : dialog_(dialog),
          initialization_state_(initialization_state),
          initialization_ports_(initialization_ports),
          game_initialized_(game_initialized),
          destroy_requested_(destroy_requested) {}

    void play_startup_sound() override {}
    void initialize_float_conversion() override {}
    void initialize_paths_and_directories() override {}
    bool scan_save_slots() override { return false; }
    openswd3::compat::i32 show_startup_dialog() override {
        return dialog_.run(false);
    }

    void initialize_game() override {
        static_cast<void>(openswd3::app::run_initialization_dialog_wrapper(
            initialization_state_,
            initialization_ports_
        ));
        game_initialized_ =
            !destroy_requested_ &&
            initialization_state_.special_mode_state ==
                openswd3::app::kInitialSpecialModeState;
    }
    void reset_result_one_game_state() override {}
    void rebuild_result_one_slot_previews() override {}
    void select_result_one_recent_save_group() override {}
    void request_synchronous_destroy() override { destroy_requested_ = true; }

private:
    openswd3::platform_sdl3::SdlStartupDialog& dialog_;
    openswd3::app::InitializationState& initialization_state_;
    openswd3::app::InitializationPorts& initialization_ports_;
    bool& game_initialized_;
    bool& destroy_requested_;
};

class SdlDisplayLifecyclePorts final
    : public openswd3::app::DisplayLifecyclePorts {
public:
    SdlDisplayLifecyclePorts(
        SDL_Window& window,
        openswd3::compat::u32& frame_interval,
        const bool backend_available
    )
        : window_(window),
          frame_interval_(frame_interval),
          backend_available_(backend_available) {}

    bool display_backend_available() override {
        return backend_available_;
    }

    void set_frame_interval(
        const openswd3::compat::u32 milliseconds
    ) override {
        if (milliseconds == 0U) {
            static_cast<void>(
                openswd3::input_time_rng::clear_frame_interval(frame_interval_)
            );
            return;
        }

        static_cast<void>(openswd3::input_time_rng::set_frame_interval(
            frame_interval_,
            milliseconds
        ));
    }

    void suspend_audio_output() override {}
    void suspend_audio_streams() override {}
    void maintain_audio() override {}
    void suspend_battle_display() override {}
    void release_font(openswd3::compat::u32) override {}

    void minimize_window() override {
        static_cast<void>(SDL_MinimizeWindow(&window_));
    }

    void show_and_position_window() override {
        static_cast<void>(SDL_RestoreWindow(&window_));
        static_cast<void>(SDL_SetWindowSize(&window_, kFrameWidth, kFrameHeight));
    }

    void restore_surfaces() override {}
    void rebuild_framebuffer_binding() override {}
    void rebuild_font(openswd3::compat::u32) override {}
    void resume_battle_display() override {}
    void finish_display_recovery() override {}

private:
    SDL_Window& window_;
    openswd3::compat::u32& frame_interval_;
    bool backend_available_{};
};

class SmokeShutdownPorts final : public openswd3::app::ShutdownPorts {
public:
    void perform_shutdown_operation(
        const openswd3::app::ShutdownOperation operation
    ) override {
        if (operation == openswd3::app::ShutdownOperation::show_cursor) {
            static_cast<void>(SDL_ShowCursor());
        }
    }

    bool perform_shutdown_close(
        openswd3::app::ShutdownCloseOperation
    ) override {
        return true;
    }

    void report_shutdown_close_failure(
        openswd3::app::ShutdownCloseOperation
    ) override {}
};

class SdlProcessExitPorts final : public openswd3::app::ProcessExitPorts {
public:
    explicit SdlProcessExitPorts(bool& running) : running_(running) {}

    void uninitialize_com() override {}

    void post_quit_message_zero() override {
        running_ = false;
    }

private:
    bool& running_;
};

class SdlSmokeIdlePorts final
    : public openswd3::app::IdleRuntimePorts,
      public openswd3::app::FramePreparationPorts,
      public openswd3::app::FrameRuntimePorts {
public:
    SdlSmokeIdlePorts(
        SDL_Renderer& renderer,
        SDL_Texture* texture,
        const std::vector<std::uint16_t>& framebuffer,
        const openswd3::compat::u32& frame_interval,
        openswd3::app::WindowEventState& window_state,
        const openswd3::app::DisplayLifecycleState& display_state,
        openswd3::app::FramePreparationState& frame_preparation_state,
        openswd3::app::FrameCoordinatorState& frame_coordinator_state,
        openswd3::input_time_rng::LegacyMouseState& mouse_state,
        openswd3::platform_sdl3::SdlMouseDeviceState& mouse_device_state,
        openswd3::app::ShutdownPorts& shutdown_ports,
        openswd3::app::ProcessExitPorts& exit_ports,
        bool& ok,
        bool& running
    )
        : renderer_(renderer),
          texture_(texture),
          framebuffer_(framebuffer),
          frame_interval_(frame_interval),
          window_state_(window_state),
          display_state_(display_state),
          frame_preparation_state_(frame_preparation_state),
          frame_coordinator_state_(frame_coordinator_state),
          mouse_state_(mouse_state),
          mouse_device_state_(mouse_device_state),
          shutdown_ports_(shutdown_ports),
          exit_ports_(exit_ports),
          ok_(ok),
          running_(running) {}

    void step_video() override {}
    void maintain_audio() override {}

    void yield() override {
        SDL_Delay(0U);
    }

    void step_game_frame() override {
        frame_preparation_state_.process_flags = window_state_.process_flags;
        frame_preparation_state_.display_active = display_state_.display_active;
        frame_preparation_state_.frame_clock.frame_interval_milliseconds =
            frame_interval_;
        frame_preparation_state_.special_mode_state =
            frame_coordinator_state_.battle.special_mode_state;
        frame_preparation_state_.high_priority_state =
            frame_coordinator_state_.battle.high_priority_state;
        if (openswd3::app::run_frame_preparation(
                frame_preparation_state_,
                *this
            ) == openswd3::app::FramePreparationOutcome::accepted) {
            frame_coordinator_state_.frame_execution_gate =
                window_state_.frame_execution_gate;
            frame_coordinator_state_.process_flags = window_state_.process_flags;
            frame_coordinator_state_.transition_suppression =
                display_state_.transition_suppression;
            static_cast<void>(openswd3::app::run_accepted_frame(
                frame_coordinator_state_,
                *this
            ));
            window_state_.process_flags = frame_coordinator_state_.process_flags;
            if (running_) {
                present();
            }
        }
    }

    void present_pause() override {
        present();
    }

    openswd3::compat::u32 read_seconds() override {
        return static_cast<openswd3::compat::u32>(std::time(nullptr));
    }

    openswd3::compat::u32 read_milliseconds() override {
        return static_cast<openswd3::compat::u32>(SDL_GetTicks());
    }

    bool query_internal_flag(openswd3::compat::u32) override {
        return false;
    }

    void clear_internal_flag(openswd3::compat::u32) override {}
    void set_internal_flag(openswd3::compat::u32) override {}

    void perform_primary_transition_operation(
        openswd3::app::PrimaryTransitionOperation
    ) override {}

    void release_and_clear_party_member_transition(
        openswd3::compat::u32
    ) override {}

    void sample_input_device() override {
        static_cast<void>(openswd3::platform_sdl3::sample_sdl_keyboard_state(
            keyboard_snapshot_
        ));
    }
    void normalize_input() override {
        const auto sample =
            openswd3::platform_sdl3::sample_sdl_mouse_state(
                renderer_,
                mouse_device_state_
            );
        mouse_frame_ = openswd3::input_time_rng::normalize_mouse_sample(
            mouse_state_,
            sample
        );
    }

    void release_display_and_world_for_battle_entry() override {}
    void close_world_map_view() override {}
    void initialize_battle(openswd3::compat::u16) override {}
    void clear_party_battle_entry_bits() override {}
    openswd3::compat::i32 step_battle() override { return 1; }
    void rebuild_display_after_result_zero() override {}
    void set_result_zero_world_state() override {}
    void reopen_world_map_after_result_zero() override {}
    void resume_audio_after_result_zero() override {}
    void prepare_result_two_internal_state() override {}
    void clear_result_two_auxiliary_state() override {}
    void finish_result_two_mode_transition() override {}
    void clear_result_three_internal_state() override {}
    void remap_world_after_result_three() override {}

    void step_high_priority(openswd3::app::FrameCoordinatorState&) override {}
    void update_background_music(
        openswd3::app::FrameCoordinatorState&
    ) override {}
    void step_world_interaction(
        openswd3::app::FrameCoordinatorState&
    ) override {}
    void step_world_player(openswd3::app::FrameCoordinatorState&) override {}
    void step_story(openswd3::app::FrameCoordinatorState&) override {}
    void finish_world_frame(openswd3::app::FrameCoordinatorState&) override {}
    void prepare_special_mode_objects(
        openswd3::app::FrameCoordinatorState&
    ) override {}
    void step_standard_special_mode(
        openswd3::app::FrameCoordinatorState&
    ) override {}
    void step_shop_mode(openswd3::app::FrameCoordinatorState&) override {}

    void request_synchronous_close() override {
        window_state_.process_flags = frame_coordinator_state_.process_flags;
        openswd3::app::handle_window_destroy(
            window_state_,
            shutdown_ports_,
            exit_ports_
        );
    }

private:
    void present() {
        if (texture_ == nullptr) {
            static_cast<void>(report_error(
                "framebuffer presentation: missing texture"
            ));
            ok_ = false;
            running_ = false;
            return;
        }
        if (!present_framebuffer(renderer_, *texture_, framebuffer_)) {
            report_sdl_error("framebuffer presentation");
            ok_ = false;
            running_ = false;
        }
    }

    SDL_Renderer& renderer_;
    SDL_Texture* texture_{};
    const std::vector<std::uint16_t>& framebuffer_;
    const openswd3::compat::u32& frame_interval_;
    openswd3::app::WindowEventState& window_state_;
    const openswd3::app::DisplayLifecycleState& display_state_;
    openswd3::app::FramePreparationState& frame_preparation_state_;
    openswd3::app::FrameCoordinatorState& frame_coordinator_state_;
    openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard_snapshot_{};
    openswd3::input_time_rng::LegacyMouseState& mouse_state_;
    openswd3::platform_sdl3::SdlMouseDeviceState& mouse_device_state_;
    openswd3::input_time_rng::LegacyMouseFrame mouse_frame_{};
    openswd3::app::ShutdownPorts& shutdown_ports_;
    openswd3::app::ProcessExitPorts& exit_ports_;
    bool& ok_;
    bool& running_;
};

}  // namespace

int main(const int argument_count, char** arguments) {
    const char* base_path = SDL_GetBasePath();
    if (base_path == nullptr) {
        return report_sdl_error("SDL_GetBasePath");
    }

    const std::filesystem::path executable_directory =
        openswd3::resource_io::path_from_utf8(base_path);
    static_cast<void>(openswd3::diagnostics::initialize_logging(
        executable_directory / "logs" / "openswd3.log"
    ));
    LoggingShutdownGuard logging_shutdown_guard;
    openswd3::diagnostics::log_info("OpenSWD3 process started");

    std::error_code directory_error;
    const std::filesystem::path launch_directory =
        std::filesystem::current_path(directory_error);
    if (directory_error) {
        return report_error(
            std::string{"game data directory: cannot read launch directory: "} +
            directory_error.message()
        );
    }

    std::vector<std::string_view> command_arguments;
    command_arguments.reserve(
        argument_count > 1 ? static_cast<std::size_t>(argument_count - 1) : 0U
    );
    for (int index = 1; index < argument_count; ++index) {
        command_arguments.emplace_back(arguments[index]);
    }

    const openswd3::resource_io::DataDirectoryResolution data_directory =
        openswd3::resource_io::resolve_data_directory(
            command_arguments,
            executable_directory,
            launch_directory
        );
    if (data_directory.status !=
        openswd3::resource_io::DataDirectoryStatus::ready) {
        const std::string_view message =
            openswd3::resource_io::data_directory_status_message(
                data_directory.status
            );
        std::string error_message{"game data directory: "};
        error_message.append(message);
        if (!data_directory.detail.empty()) {
            error_message.append(": ");
            error_message.append(data_directory.detail);
        }

        return report_error(error_message);
    }

    if (!openswd3::resource_io::activate_data_directory(
            data_directory.directory,
            directory_error
        )) {
        return report_error(
            std::string{"game data directory: cannot activate directory: "} +
            directory_error.message()
        );
    }
    openswd3::diagnostics::log_info(
        std::string{"game data directory activated: "} +
        data_directory.directory.string()
    );

    openswd3::platform_sdl3::SingleInstanceGuard single_instance;
    SmokeCommandLinePorts command_line_ports;
    const std::string legacy_command_line =
        openswd3::platform_sdl3::reconstruct_legacy_command_line_tail(
            argument_count,
            arguments,
            1 + static_cast<int>(data_directory.consumed_argument_count)
        );
    const openswd3::app::ProcessStartupGateResult startup_gate_result =
        openswd3::app::run_process_startup_gates(
            legacy_command_line,
            single_instance,
            command_line_ports
        );
    if (startup_gate_result !=
        openswd3::app::ProcessStartupGateResult::continue_normal_startup) {
        openswd3::diagnostics::log_info(
            "process startup exited before SDL initialization"
        );
        return 0;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return report_sdl_error("SDL_Init");
    }
    openswd3::diagnostics::log_debug("SDL video subsystem initialized");

    SDL_Window* window = SDL_CreateWindow(
        "OpenSWD3",
        kInitialWindowWidth,
        kInitialWindowHeight,
        SDL_WINDOW_RESIZABLE
    );
    if (window == nullptr) {
        const int result = report_sdl_error("SDL_CreateWindow");
        SDL_Quit();
        return result;
    }
    openswd3::diagnostics::log_info("main window created");

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        const int result = report_sdl_error("SDL_CreateRenderer");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return result;
    }

    if (!SDL_SetRenderLogicalPresentation(
            renderer,
            kFrameWidth,
            kFrameHeight,
            SDL_LOGICAL_PRESENTATION_LETTERBOX
        )) {
        const int result = report_sdl_error("SDL_SetRenderLogicalPresentation");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return result;
    }

    bool game_initialized = false;
    bool startup_destroy_requested = false;
    SDL_Texture* texture = nullptr;
    openswd3::compat::u32 frame_interval =
        kInitialFrameIntervalMilliseconds;
    openswd3::app::InitializationState initialization_state{};
    openswd3::app::PlatformBackendState backend_state{};
    openswd3::resource_io::LegacyResourceDatabases resource_databases;
    openswd3::input_time_rng::LegacyMouseState mouse_state{};
    openswd3::platform_sdl3::SdlMouseDeviceState mouse_device_state{};
    SdlSmokePlatformBackendPorts backend_ports(
        *renderer,
        texture,
        startup_destroy_requested
    );
    SdlSmokeInitializationPorts initialization_ports(
        backend_state,
        backend_ports,
        resource_databases,
        data_directory.directory,
        *renderer,
        mouse_state,
        mouse_device_state,
        frame_interval,
        startup_destroy_requested
    );
    {
        openswd3::platform_sdl3::SdlExternalLaunchPorts external_launch_ports;
        openswd3::platform_sdl3::SdlStartupDialog startup_dialog(
            *renderer,
            external_launch_ports,
            [] {}
        );
        SdlSmokeStartupPorts startup_ports(
            startup_dialog,
            initialization_state,
            initialization_ports,
            game_initialized,
            startup_destroy_requested
        );
        openswd3::app::StartupState startup_state{};
        static_cast<void>(
            openswd3::app::run_startup_custom_message(
                startup_state,
                startup_ports
            )
        );
    }

    openswd3::input_time_rng::LegacyCrtRng crt_rng;
    openswd3::input_time_rng::LegacySecondaryRng secondary_rng;
    SdlRngSeedPorts rng_seed_ports(crt_rng, secondary_rng);
    if (startup_destroy_requested) {
        openswd3::app::seed_two_rng_streams(rng_seed_ports);
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    std::vector<std::uint16_t> framebuffer(
        static_cast<std::size_t>(kFrameWidth * kFrameHeight)
    );

    openswd3::app::seed_two_rng_streams(rng_seed_ports);

    const bool runtime_ready =
        game_initialized && backend_state.display_active != 0U &&
        texture != nullptr;
    openswd3::app::WindowEventState window_state{
        runtime_ready ? 1U : 0U,
        backend_state.process_flags,
        1U,
    };
    openswd3::app::DisplayLifecycleState display_state{
        runtime_ready ? 1U : 0U,
        initialization_state.transition_suppression,
        0U,
    };
    SmokeWindowEventPorts window_ports;
    SdlDisplayLifecyclePorts display_ports(
        *window,
        frame_interval,
        runtime_ready
    );
    SmokeShutdownPorts shutdown_ports;

    bool ok = true;
    bool running = true;
    SdlProcessExitPorts exit_ports(running);
    if (runtime_ready &&
        !present_framebuffer(*renderer, *texture, framebuffer)) {
        static_cast<void>(report_sdl_error("initial framebuffer presentation"));
        ok = false;
        openswd3::app::handle_window_destroy(
            window_state,
            shutdown_ports,
            exit_ports
        );
    }
    openswd3::app::FramePreparationState frame_preparation_state{};
    openswd3::app::FrameCoordinatorState frame_coordinator_state{
        window_state.frame_execution_gate,
        window_state.process_flags,
        display_state.transition_suppression,
        false,
        {
            0U,
            0U,
            runtime_ready ? initialization_state.special_mode_state : 0U,
            0U,
        },
    };
    SdlSmokeIdlePorts idle_ports(
        *renderer,
        texture,
        framebuffer,
        frame_interval,
        window_state,
        display_state,
        frame_preparation_state,
        frame_coordinator_state,
        mouse_state,
        mouse_device_state,
        shutdown_ports,
        exit_ports,
        ok,
        running
    );
    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            const auto translated =
                openswd3::platform_sdl3::translate_sdl_event(event);
            if (!translated.has_value()) {
                continue;
            }

            const auto result = openswd3::app::dispatch_host_window_event(
                *translated,
                window_state,
                window_ports,
                display_state,
                display_ports
            );
            if (result == openswd3::app::HostWindowEventResult::request_close) {
                openswd3::app::handle_window_destroy(
                    window_state,
                    shutdown_ports,
                    exit_ports
                );
                break;
            }
        }
        if (!running) {
            break;
        }
        openswd3::app::run_idle_iteration(
            {
                window_state.frame_execution_gate,
                window_state.process_flags,
                display_state.transition_suppression,
                display_state.display_active,
            },
            idle_ports
        );
    }

    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return ok ? 0 : 1;
}
