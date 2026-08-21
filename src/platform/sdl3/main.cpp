#include "event_translation.hpp"
#include "external_launch_sdl3.hpp"
#include "keyboard_snapshot_sdl3.hpp"
#include "legacy_command_line.hpp"
#include "legacy_sample_backend_sdl3.hpp"
#include "mouse_sdl3.hpp"
#include "single_instance.hpp"
#include "startup_dialog_sdl3.hpp"
#include "text_encoding_sdl3.hpp"
#include "world_role_runtime_adapter.hpp"

#include "openswd3/app/host_window_event.hpp"
#include "openswd3/app/idle_runtime.hpp"
#include "openswd3/app/frame_preparation.hpp"
#include "openswd3/app/frame_runtime.hpp"
#include "openswd3/app/initialization.hpp"
#include "openswd3/app/platform_backend_initialization.hpp"
#include "openswd3/app/process_startup.hpp"
#include "openswd3/app/screenshot.hpp"
#include "openswd3/app/startup.hpp"
#include "openswd3/app/window_events.hpp"
#include "openswd3/audio_video/legacy_audio_coordination.hpp"
#include "openswd3/audio_video/legacy_audio_output.hpp"
#include "openswd3/audio_video/legacy_sample_commands.hpp"
#include "openswd3/audio_video/legacy_sample_manager.hpp"
#include "openswd3/audio_video/legacy_sequence_manager.hpp"
#include "openswd3/audio_video/legacy_snd_archive.hpp"
#include "openswd3/audio_video/legacy_stream_commands.hpp"
#include "openswd3/audio_video/legacy_stream_manager.hpp"
#include "openswd3/audio_video/legacy_video.hpp"
#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/asset_runtime/legacy_act_runtime.hpp"
#include "openswd3/asset_runtime/legacy_asset_cache_limits.hpp"
#include "openswd3/asset_runtime/legacy_ani_activity.hpp"
#include "openswd3/asset_runtime/legacy_ani_directional_effect.hpp"
#include "openswd3/asset_runtime/legacy_ani_drift_effect.hpp"
#include "openswd3/asset_runtime/legacy_ani_follower_effect.hpp"
#include "openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp"
#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/battle/legacy_battle_assets.hpp"
#include "openswd3/battle/legacy_battle_setup.hpp"
#include "openswd3/diagnostics/log.hpp"
#include "openswd3/input_time_rng/legacy_crt_rng.hpp"
#include "openswd3/input_time_rng/legacy_frame_clock.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/input_time_rng/legacy_text_input.hpp"
#include "openswd3/rendering/legacy_bmp_writer.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_glyph_atlas.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/rendering/legacy_presentation.hpp"
#include "openswd3/rendering/legacy_rectangle_effect.hpp"
#include "openswd3/rendering/legacy_text_renderer_runtime.hpp"
#include "openswd3/resource_io/data_directory.hpp"
#include "openswd3/resource_io/legacy_memory_manager.hpp"
#include "openswd3/resource_io/legacy_resource_databases.hpp"
#include "openswd3/resource_io/window_configuration.hpp"
#include "openswd3/special_modes/legacy_initial_menu.hpp"
#include "openswd3/world_map/legacy_maps_world_database.hpp"
#include "openswd3/world_map/legacy_movement_collision.hpp"
#include "openswd3/world_map/legacy_world_collision_talk.hpp"
#include "openswd3/world_map/legacy_world_debug_hotkeys.hpp"
#include "openswd3/world_map/legacy_world_direction_adjustment.hpp"
#include "openswd3/world_map/legacy_world_direction_input.hpp"
#include "openswd3/world_map/legacy_world_facing_talk.hpp"
#include "openswd3/world_map/legacy_world_frame_coordinator.hpp"
#include "openswd3/world_map/legacy_world_dialog_runtime.hpp"
#include "openswd3/world_map/legacy_world_interaction.hpp"
#include "openswd3/world_map/legacy_world_item_lifecycle.hpp"
#include "openswd3/world_map/legacy_world_load_progress.hpp"
#include "openswd3/world_map/legacy_world_path_requests.hpp"
#include "openswd3/world_map/legacy_world_path_script.hpp"
#include "openswd3/world_map/legacy_random_encounter.hpp"
#include "openswd3/world_map/legacy_world_player_control.hpp"
#include "openswd3/world_map/legacy_world_role_lifecycle.hpp"
#include "openswd3/world_map/legacy_world_runtime_session.hpp"
#include "openswd3/world_map/legacy_world_special_frame_loader.hpp"
#include "openswd3/world_map/legacy_world_story_vm.hpp"
#include "openswd3/world_map/legacy_world_transient_reset.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <limits>
#include <list>
#include <optional>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

constexpr int kFrameWidth = 640;
constexpr int kFrameHeight = 480;
constexpr int kInitialWindowWidth = kFrameWidth * 3 / 2;
constexpr int kInitialWindowHeight = kFrameHeight * 3 / 2;
constexpr openswd3::compat::u32 kInitialFrameIntervalMilliseconds = 35U;
constexpr openswd3::compat::i32 kLegacyInitialSampleMixLevel = 6;

[[nodiscard]] std::optional<openswd3::compat::u32>
legacy_text_virtual_key(const SDL_Scancode scancode) noexcept {
    switch (scancode) {
    case SDL_SCANCODE_BACKSPACE:
        return 0x08U;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER:
        return 0x0DU;
    case SDL_SCANCODE_ESCAPE:
        return 0x1BU;
    case SDL_SCANCODE_END:
        return 0x23U;
    case SDL_SCANCODE_HOME:
        return 0x24U;
    case SDL_SCANCODE_LEFT:
        return 0x25U;
    case SDL_SCANCODE_RIGHT:
        return 0x27U;
    case SDL_SCANCODE_INSERT:
        return 0x2DU;
    case SDL_SCANCODE_DELETE:
        return 0x2EU;
    default:
        return std::nullopt;
    }
}

class SdlLegacyTextInputPorts final
    : public openswd3::input_time_rng::LegacyTextInputPorts {
public:
    bool is_ime_keyboard_layout(openswd3::compat::u32) override {
        return false;
    }

    void play_sound_effect(openswd3::compat::u32) override {}
};

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

void persist_window_configuration(
    SDL_Window& window,
    const std::filesystem::path& configuration_path,
    openswd3::resource_io::WindowSize normal_size
) {
    const bool maximized =
        (SDL_GetWindowFlags(&window) & SDL_WINDOW_MAXIMIZED) != 0U;
    if (!maximized &&
        !SDL_GetWindowSize(&window, &normal_size.width, &normal_size.height)) {
        openswd3::diagnostics::log_warning(
            std::string{"window size: SDL_GetWindowSize: "} + SDL_GetError()
        );
        return;
    }

    std::string detail;
    const auto status = openswd3::resource_io::save_window_configuration(
        configuration_path, normal_size, maximized, detail
    );
    if (status != openswd3::resource_io::WindowConfigurationStatus::ready) {
        std::string message{"window size: "};
        message.append(
            openswd3::resource_io::window_configuration_status_message(status)
        );
        if (!detail.empty()) {
            message.append(": ");
            message.append(detail);
        }
        openswd3::diagnostics::log_warning(message);
        return;
    }

    openswd3::diagnostics::log_info(
        std::string{"window configuration saved: "} +
        std::to_string(normal_size.width) + "x" +
        std::to_string(normal_size.height) +
        (maximized ? ", maximized" : ", restored")
    );
}

[[nodiscard]] std::vector<openswd3::compat::u8>
read_binary_file(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        return {};
    }
    const std::streamoff size = stream.tellg();
    if (size <= 0) {
        return {};
    }
    std::vector<openswd3::compat::u8> bytes(static_cast<std::size_t>(size));
    stream.seekg(0);
    stream.read(
        reinterpret_cast<char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size())
    );
    if (!stream) {
        return {};
    }
    return bytes;
}

[[nodiscard]] bool present_framebuffer(
    SDL_Renderer& renderer,
    SDL_Texture& texture,
    const openswd3::rendering::LegacyFramebuffer& framebuffer
) {
    const openswd3::rendering::LegacySurfaceGeometry& geometry =
        framebuffer.geometry().surface;
    return SDL_UpdateTexture(
               &texture,
               nullptr,
               framebuffer.physical_pixels().data(),
               geometry.pitch_bytes
           ) &&
        SDL_SetRenderDrawColor(&renderer, 0, 0, 0, 255) &&
        SDL_RenderClear(&renderer) &&
        SDL_RenderTexture(&renderer, &texture, nullptr, nullptr) &&
        SDL_RenderPresent(&renderer);
}

void shutdown_sample_output(
    openswd3::audio_video::LegacySampleManager& manager,
    openswd3::audio_video::LegacySndArchive& archive,
    openswd3::platform_sdl3::SdlLegacySampleBackend& backend
) {
    if (manager.initialized()) {
        static_cast<void>(manager.shutdown());
        return;
    }
    archive.close();
    backend.close_output();
}

class UnavailableLegacyStreamBackend final
    : public openswd3::audio_video::LegacyStreamBackend {
public:
    openswd3::audio_video::LegacyStreamHandle open_stream(
        openswd3::compat::u32, std::string_view, openswd3::compat::i32
    ) override {
        return 0U;
    }

    std::string_view last_error() const override {
        return "FFmpeg media backend is not available";
    }

    void close_stream(openswd3::audio_video::LegacyStreamHandle) override {}
    void set_stream_user_data(
        openswd3::audio_video::LegacyStreamHandle,
        openswd3::compat::u32,
        openswd3::compat::i32
    ) override {}
    openswd3::compat::i32 stream_user_data(
        openswd3::audio_video::LegacyStreamHandle, openswd3::compat::u32
    ) override {
        return 0;
    }
    void set_stream_volume(
        openswd3::audio_video::LegacyStreamHandle, openswd3::compat::i32
    ) override {}
    openswd3::compat::i32
    stream_volume(openswd3::audio_video::LegacyStreamHandle) override {
        return 0;
    }
    void set_stream_loop_count(
        openswd3::audio_video::LegacyStreamHandle, openswd3::compat::i32
    ) override {}
    void start_stream(openswd3::audio_video::LegacyStreamHandle) override {}
    openswd3::compat::u32
    stream_status(openswd3::audio_video::LegacyStreamHandle) override {
        return 2U;
    }
    void stream_ms_position(
        openswd3::audio_video::LegacyStreamHandle,
        openswd3::compat::i32& total_milliseconds,
        openswd3::compat::i32& current_milliseconds
    ) override {
        total_milliseconds = 0;
        current_milliseconds = 0;
    }
};

class UnavailableLegacySequenceBackend final
    : public openswd3::audio_video::LegacySequenceBackend {
public:
    bool open_midi_output(
        openswd3::compat::i32,
        openswd3::audio_video::LegacyMidiDriverHandle& driver
    ) override {
        driver = 0U;
        return false;
    }

    std::string_view last_error() const override {
        return "MIDI backend is not available";
    }

    void
    close_midi_output(openswd3::audio_video::LegacyMidiDriverHandle) override {}
    openswd3::audio_video::LegacySequenceHandle allocate_sequence_handle(
        openswd3::audio_video::LegacyMidiDriverHandle
    ) override {
        return 0U;
    }
    void release_sequence_handle(
        openswd3::audio_video::LegacySequenceHandle
    ) override {}
    openswd3::compat::i32 initialize_sequence(
        openswd3::audio_video::LegacySequenceHandle,
        std::span<const openswd3::compat::u8>,
        openswd3::compat::u32
    ) override {
        return 0;
    }
    void set_sequence_user_data(
        openswd3::audio_video::LegacySequenceHandle,
        openswd3::compat::u32,
        openswd3::compat::i32
    ) override {}
    openswd3::compat::i32 sequence_user_data(
        openswd3::audio_video::LegacySequenceHandle, openswd3::compat::u32
    ) override {
        return 0;
    }
    void set_sequence_volume(
        openswd3::audio_video::LegacySequenceHandle,
        openswd3::compat::i32,
        openswd3::compat::i32
    ) override {}
    void set_sequence_loop_count(
        openswd3::audio_video::LegacySequenceHandle, openswd3::compat::i32
    ) override {}
    void start_sequence(openswd3::audio_video::LegacySequenceHandle) override {}
    openswd3::compat::u32
    sequence_status(openswd3::audio_video::LegacySequenceHandle) override {
        return 2U;
    }
    void end_sequence(openswd3::audio_video::LegacySequenceHandle) override {}
};

class SdlLegacyAudioQueuePorts final
    : public openswd3::audio_video::LegacyAudioQueuePorts {
public:
    SdlLegacyAudioQueuePorts(
        openswd3::audio_video::LegacySequenceManager& sequence_manager,
        openswd3::audio_video::LegacyStreamManager& stream_manager
    ) noexcept
        : sequence_manager_(sequence_manager), stream_manager_(stream_manager) {
    }

    bool sequence_absent(const openswd3::compat::i32 sequence_id) override {
        return sequence_manager_.sequence_absent(sequence_id);
    }
    bool stream_absent(const openswd3::compat::i32 stream_id) override {
        return stream_manager_.stream_absent(stream_id);
    }
    void play_sequence(
        const std::string_view filename,
        const openswd3::compat::i32 sequence_id,
        const openswd3::compat::i32 volume,
        const openswd3::compat::i32 loop_count
    ) override {
        static_cast<void>(
            sequence_manager_.play(filename, sequence_id, volume, loop_count)
        );
    }
    void play_stream(
        const std::string_view filename,
        const openswd3::compat::i32 stream_id,
        const openswd3::compat::i32 volume,
        const openswd3::compat::i32 loop_count
    ) override {
        static_cast<void>(
            stream_manager_.play(filename, stream_id, volume, loop_count)
        );
    }
    void beep() override {}

private:
    openswd3::audio_video::LegacySequenceManager& sequence_manager_;
    openswd3::audio_video::LegacyStreamManager& stream_manager_;
};

class SdlLegacyAudioMaintenancePorts final
    : public openswd3::audio_video::LegacyAudioMaintenancePorts {
public:
    SdlLegacyAudioMaintenancePorts(
        openswd3::audio_video::LegacyAudioQueueCoordinator& queue,
        openswd3::audio_video::LegacyStreamManager& stream_manager,
        openswd3::audio_video::LegacySequenceManager& sequence_manager,
        openswd3::audio_video::LegacySampleManager& sample_manager
    ) noexcept
        : queue_(queue), stream_manager_(stream_manager),
          sequence_manager_(sequence_manager), sample_manager_(sample_manager) {
    }

    void service_queue() override {
        static_cast<void>(queue_.service());
    }
    void service_streams() override {
        static_cast<void>(stream_manager_.service());
    }
    void service_sequences() override {
        static_cast<void>(sequence_manager_.service());
    }
    void service_samples() override {
        static_cast<void>(sample_manager_.service_completed_samples());
    }

private:
    openswd3::audio_video::LegacyAudioQueueCoordinator& queue_;
    openswd3::audio_video::LegacyStreamManager& stream_manager_;
    openswd3::audio_video::LegacySequenceManager& sequence_manager_;
    openswd3::audio_video::LegacySampleManager& sample_manager_;
};

void service_audio(openswd3::audio_video::LegacyAudioMaintenancePorts& ports) {
    static_cast<void>(openswd3::audio_video::maintain_legacy_audio(ports));
}

void shutdown_audio_output(
    openswd3::audio_video::LegacyAudioQueueCoordinator& queue,
    openswd3::audio_video::LegacyStreamManager& stream_manager,
    openswd3::audio_video::LegacySequenceManager& sequence_manager,
    openswd3::audio_video::LegacySampleManager& sample_manager,
    openswd3::audio_video::LegacySndArchive& archive,
    openswd3::platform_sdl3::SdlLegacySampleBackend& backend
) {
    static_cast<void>(queue.shutdown());
    if (stream_manager.initialized()) {
        static_cast<void>(stream_manager.shutdown());
    }
    if (sequence_manager.initialized()) {
        static_cast<void>(sequence_manager.shutdown());
    }
    shutdown_sample_output(sample_manager, archive, backend);
}

class SmokeWindowEventPorts final
    : public openswd3::app::WindowEventPorts,
      public openswd3::app::ScreenshotPorts,
      public openswd3::rendering::LegacyBmpWriterPorts {
public:
    SmokeWindowEventPorts(
        const openswd3::rendering::LegacyFramebuffer& framebuffer,
        const openswd3::rendering::LegacyPixelConversionState& pixel_conversion,
        openswd3::audio_video::LegacyAudioMaintenancePorts& audio_maintenance,
        openswd3::audio_video::LegacyVideoPlayer& video_player
    )
        : framebuffer_(framebuffer), pixel_conversion_(pixel_conversion),
          audio_maintenance_(audio_maintenance), video_player_(video_player) {}

    void release_active_video() override {
        static_cast<void>(video_player_.close());
    }

    openswd3::compat::u32 free_disk_space_mebibytes() override {
        std::error_code error;
        const std::filesystem::path directory =
            std::filesystem::current_path(error);
        if (error) {
            return 0U;
        }
        const std::filesystem::space_info information =
            std::filesystem::space(directory, error);
        if (error) {
            return 0U;
        }
        constexpr std::uintmax_t bytes_per_mebibyte = 1024U * 1024U;
        const std::uintmax_t available =
            information.available / bytes_per_mebibyte;
        return static_cast<openswd3::compat::u32>(std::min(
            available,
            static_cast<std::uintmax_t>(
                std::numeric_limits<openswd3::compat::u32>::max()
            )
        ));
    }

    void capture_legacy_screenshot() override {
        openswd3::app::capture_legacy_screenshot(*this);
    }

    void beep() override {}

    void maintain_audio() override {
        service_audio(audio_maintenance_);
    }

    bool
    open_existing_numbered_bmp(const openswd3::compat::u32 sequence) override {
        existing_stream_.close();
        existing_stream_.clear();
        existing_stream_.open(
            numbered_screenshot_path(sequence), std::ios::binary
        );
        return static_cast<bool>(existing_stream_);
    }

    void close_existing_numbered_bmp() override {
        existing_stream_.close();
        existing_stream_.clear();
    }

    void save_framebuffer_as_numbered_bmp(
        const openswd3::compat::u32 sequence
    ) override {
        const std::string filename =
            numbered_screenshot_path(sequence).string();
        const auto result =
            openswd3::rendering::write_legacy_16bit_framebuffer_bmp(
                framebuffer_.physical_pixels(),
                kFrameWidth,
                kFrameHeight,
                filename,
                pixel_conversion_,
                *this
            );
        if (result.status !=
            openswd3::rendering::LegacyBmpWriteStatus::completed) {
            static_cast<void>(report_error(
                std::string{"legacy screenshot write failed: "} + filename
            ));
        }
    }

    bool open_or_create_without_truncation(
        const std::string_view filename
    ) override {
        output_stream_.close();
        output_stream_.clear();
        const std::filesystem::path path =
            std::filesystem::path{std::string{filename}};
        output_stream_.open(
            path, std::ios::binary | std::ios::in | std::ios::out
        );
        if (output_stream_) {
            return true;
        }

        output_stream_.clear();
        std::error_code existence_error;
        if (std::filesystem::exists(path, existence_error) || existence_error) {
            return false;
        }
        {
            std::ofstream create(path, std::ios::binary);
            if (!create) {
                return false;
            }
        }
        output_stream_.open(
            path, std::ios::binary | std::ios::in | std::ios::out
        );
        return static_cast<bool>(output_stream_);
    }

    bool seek_absolute(const openswd3::compat::u32 offset) override {
        output_stream_.seekp(
            static_cast<std::streamoff>(offset), std::ios::beg
        );
        return static_cast<bool>(output_stream_);
    }

    bool
    write_bytes(const std::span<const openswd3::compat::u8> bytes) override {
        output_stream_.write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size())
        );
        return static_cast<bool>(output_stream_);
    }

    std::optional<openswd3::compat::u32> current_position() override {
        const std::streampos position = output_stream_.tellp();
        if (position < 0 ||
            static_cast<std::uintmax_t>(position) >
                std::numeric_limits<openswd3::compat::u32>::max()) {
            return std::nullopt;
        }
        return static_cast<openswd3::compat::u32>(position);
    }

    void close() override {
        output_stream_.close();
        output_stream_.clear();
    }

private:
    [[nodiscard]] static std::filesystem::path
    numbered_screenshot_path(const openswd3::compat::u32 sequence) {
        return std::filesystem::path{"ScrnShot"} /
            openswd3::app::make_legacy_screenshot_filename(sequence);
    }

    const openswd3::rendering::LegacyFramebuffer& framebuffer_;
    const openswd3::rendering::LegacyPixelConversionState& pixel_conversion_;
    openswd3::audio_video::LegacyAudioMaintenancePorts& audio_maintenance_;
    openswd3::audio_video::LegacyVideoPlayer& video_player_;
    std::ifstream existing_stream_;
    std::fstream output_stream_;
};

class SmokeCommandLinePorts final : public openswd3::app::CommandLinePorts {
public:
    explicit SmokeCommandLinePorts(
        openswd3::input_time_rng::LegacyKeyBindingBlock& key_bindings
    )
        : key_bindings_(key_bindings) {}

    void initialize_default_key_bindings() override {
        openswd3::input_time_rng::initialize_default_key_bindings(
            key_bindings_
        );
    }

    void run_legacy_command(openswd3::compat::u8, std::string_view) override {}

private:
    openswd3::input_time_rng::LegacyKeyBindingBlock& key_bindings_;
};

class SdlRngSeedPorts final : public openswd3::app::RngSeedPorts {
public:
    SdlRngSeedPorts(
        openswd3::input_time_rng::LegacyCrtRng& crt_rng,
        openswd3::input_time_rng::LegacySecondaryRng& secondary_rng
    )
        : crt_rng_(crt_rng), secondary_rng_(secondary_rng) {}

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
        bool& destroy_requested,
        openswd3::audio_video::LegacySndArchive& snd_archive,
        openswd3::platform_sdl3::SdlLegacySampleBackend& sample_backend,
        openswd3::audio_video::LegacySampleManager& sample_manager,
        openswd3::audio_video::LegacySequenceManager& sequence_manager,
        openswd3::audio_video::LegacyStreamManager& stream_manager
    )
        : renderer_(renderer), texture_(texture),
          destroy_requested_(destroy_requested), snd_archive_(snd_archive),
          sample_backend_(sample_backend), sample_manager_(sample_manager),
          sequence_manager_(sequence_manager), stream_manager_(stream_manager) {
    }

    bool initialize_input_backend() override {
        return true;
    }
    void report_input_initialization_failure() override {}
    void request_synchronous_destroy() override {
        destroy_requested_ = true;
    }

    void start_audio_runtime(std::string_view) override {}
    void initialize_audio_output(std::string_view) override {
        const auto archive_status = snd_archive_.open("all.snd");
        if (archive_status !=
            openswd3::audio_video::LegacySndOpenStatus::ready) {
            openswd3::diagnostics::log_error(
                "sample output: cannot open all.snd"
            );
            return;
        }

        const auto output =
            openswd3::audio_video::initialize_legacy_audio_output(
                sample_backend_
            );
        if (output.status !=
            openswd3::audio_video::LegacyAudioOutputStatus::ready) {
            std::string message{"sample output: SDL3 device open failed"};
            if (!output.last_error.empty()) {
                message.append(": ");
                message.append(output.last_error);
            }
            openswd3::diagnostics::log_error(message);
            return;
        }

        const auto manager_status =
            sample_manager_.initialize_pool(output.sample_handle_count);
        if (manager_status !=
            openswd3::audio_video::LegacySampleManagerInitializeStatus::ready) {
            openswd3::diagnostics::log_error(
                "sample output: sample handle pool initialization failed"
            );
            return;
        }

        openswd3::diagnostics::log_info(
            std::string{"sample output initialized: "} +
            std::to_string(output.selected_format.sample_rate) + " Hz, " +
            std::to_string(output.selected_format.bits_per_sample) +
            "-bit, handles=" + std::to_string(output.sample_handle_count)
        );
    }
    openswd3::app::BackendToken query_audio_driver() override {
        return sample_backend_.driver_token();
    }
    void
    initialize_midi_output(const openswd3::app::BackendToken driver) override {
        const auto status = sequence_manager_.initialize_output(
            static_cast<openswd3::compat::u32>(driver)
        );
        if (status !=
            openswd3::audio_video::LegacySequenceManagerInitializeStatus::
                ready) {
            openswd3::diagnostics::log_error(
                std::string{"sequence output: MIDI initialization failed: "} +
                std::string{sequence_manager_.last_error()}
            );
        }
    }
    void initialize_audio_stream_nodes(
        const openswd3::app::BackendToken driver
    ) override {
        const auto status = stream_manager_.initialize_pool(
            static_cast<openswd3::compat::u32>(driver)
        );
        if (status !=
            openswd3::audio_video::LegacyStreamManagerInitializeStatus::ready) {
            openswd3::diagnostics::log_error(
                "stream output: stream node pool initialization failed"
            );
        }
    }

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
        openswd3::compat::u32, openswd3::compat::u32
    ) override {
        return reinterpret_cast<openswd3::app::BackendToken>(texture_);
    }
    openswd3::app::BackendToken
    query_display_surface(openswd3::compat::u32) override {
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
    openswd3::audio_video::LegacySndArchive& snd_archive_;
    openswd3::platform_sdl3::SdlLegacySampleBackend& sample_backend_;
    openswd3::audio_video::LegacySampleManager& sample_manager_;
    openswd3::audio_video::LegacySequenceManager& sequence_manager_;
    openswd3::audio_video::LegacyStreamManager& stream_manager_;
};

class SdlSmokeInitializationPorts final
    : public openswd3::app::InitializationPorts {
public:
    SdlSmokeInitializationPorts(
        openswd3::app::PlatformBackendState& backend_state,
        openswd3::app::PlatformBackendInitializationPorts& backend_ports,
        openswd3::resource_io::LegacyResourceDatabases& resource_databases,
        openswd3::rendering::LegacyGlyphAtlasProvider& glyph_provider,
        openswd3::rendering::LegacyTextRendererRuntime& text_renderers,
        openswd3::rendering::LegacyFramebuffer& framebuffer,
        const std::filesystem::path& data_directory,
        SDL_Renderer& renderer,
        openswd3::input_time_rng::LegacyMouseState& mouse_state,
        openswd3::platform_sdl3::SdlMouseDeviceState& mouse_device_state,
        openswd3::compat::u32& frame_interval,
        bool& destroy_requested
    )
        : backend_state_(backend_state), backend_ports_(backend_ports),
          resource_databases_(resource_databases),
          glyph_provider_(glyph_provider), text_renderers_(text_renderers),
          framebuffer_(framebuffer), data_directory_(data_directory),
          renderer_(renderer), mouse_state_(mouse_state),
          mouse_device_state_(mouse_device_state),
          frame_interval_(frame_interval),
          destroy_requested_(destroy_requested) {}

    void hide_cursor() override {
        static_cast<void>(SDL_HideCursor());
    }

    bool initialize_platform_backends() override {
        const char* base_path = SDL_GetBasePath();
        return openswd3::app::run_platform_backend_initialization(
            base_path == nullptr ? std::string_view{}
                                 : std::string_view{base_path},
            backend_state_,
            backend_ports_
        );
    }

    void configure_input_and_audio_paths() override {
        openswd3::input_time_rng::set_mouse_sensitivity(mouse_state_, 2.0);
        const auto sample = openswd3::platform_sdl3::sample_sdl_mouse_state(
            renderer_, mouse_device_state_
        );
        openswd3::input_time_rng::rebase_mouse_coordinates(
            mouse_state_, sample, 480, 360
        );
    }
    bool initialize_software_drawing() override {
        return glyph_provider_.valid();
    }
    void report_software_drawing_failure() override {
        openswd3::diagnostics::log_error("legacy glyph atlas is invalid");
    }
    void check_legacy_memory_capacity() override {}
    void initialize_resource_database() override {
        const auto result = resource_databases_.initialize(data_directory_);
        using Status = openswd3::resource_io::LegacyResourceDatabaseStatus;
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
    void initialize_render_resources() override {
        for (const auto point_size :
             openswd3::rendering::kLegacyTextRendererPointSizes) {
            if (text_renderers_.rebuild(
                    point_size, framebuffer_, glyph_provider_
                ) !=
                openswd3::rendering::LegacyTextRendererRuntimeStatus::
                    completed) {
                openswd3::diagnostics::log_error(
                    "legacy text renderer initialization failed"
                );
                destroy_requested_ = true;
                return;
            }
        }
    }
    bool initialize_frame_interval_35() override {
        return openswd3::input_time_rng::set_frame_interval(
                   frame_interval_, kInitialFrameIntervalMilliseconds
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
    openswd3::rendering::LegacyGlyphAtlasProvider& glyph_provider_;
    openswd3::rendering::LegacyTextRendererRuntime& text_renderers_;
    openswd3::rendering::LegacyFramebuffer& framebuffer_;
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
        openswd3::input_time_rng::LegacyKeyBindingBlock& key_bindings,
        bool& game_initialized,
        bool& destroy_requested
    )
        : dialog_(dialog), initialization_state_(initialization_state),
          initialization_ports_(initialization_ports),
          key_bindings_(key_bindings), game_initialized_(game_initialized),
          destroy_requested_(destroy_requested) {}

    void play_startup_sound() override {}
    void initialize_default_key_bindings() override {
        openswd3::input_time_rng::initialize_default_key_bindings(
            key_bindings_
        );
    }

    void initialize_paths_and_directories() override {}
    bool scan_save_slots() override {
        return false;
    }
    openswd3::compat::i32 show_startup_dialog() override {
        return dialog_.run(false);
    }

    void initialize_game() override {
        static_cast<void>(openswd3::app::run_initialization_dialog_wrapper(
            initialization_state_, initialization_ports_
        ));
        game_initialized_ = !destroy_requested_ &&
            initialization_state_.special_mode_state ==
                openswd3::app::kInitialSpecialModeState;
    }
    void reset_result_one_game_state() override {}
    void rebuild_result_one_slot_previews() override {}
    void select_result_one_recent_save_group() override {}
    void request_synchronous_destroy() override {
        destroy_requested_ = true;
    }

private:
    openswd3::platform_sdl3::SdlStartupDialog& dialog_;
    openswd3::app::InitializationState& initialization_state_;
    openswd3::app::InitializationPorts& initialization_ports_;
    openswd3::input_time_rng::LegacyKeyBindingBlock& key_bindings_;
    bool& game_initialized_;
    bool& destroy_requested_;
};

class SdlDisplayLifecyclePorts final
    : public openswd3::app::DisplayLifecyclePorts {
public:
    SdlDisplayLifecyclePorts(
        SDL_Window& window,
        SDL_Renderer& renderer,
        SDL_Texture*& texture,
        const openswd3::rendering::LegacyFramebuffer& primary_surface,
        openswd3::rendering::LegacyFramebuffer& game_framebuffer,
        openswd3::rendering::LegacyGlyphAtlasProvider& glyph_provider,
        openswd3::rendering::LegacyTextRendererRuntime& text_renderers,
        openswd3::audio_video::LegacyStreamManager& stream_manager,
        openswd3::audio_video::LegacySampleManager& sample_manager,
        openswd3::audio_video::LegacyAudioMaintenancePorts& audio_maintenance,
        openswd3::compat::u32& frame_interval,
        const bool backend_available,
        bool& ok,
        bool& running
    )
        : window_(window), renderer_(renderer), texture_(texture),
          primary_surface_(primary_surface),
          game_framebuffer_(game_framebuffer), glyph_provider_(glyph_provider),
          text_renderers_(text_renderers), stream_manager_(stream_manager),
          sample_manager_(sample_manager),
          audio_maintenance_(audio_maintenance),
          frame_interval_(frame_interval),
          backend_available_(backend_available), ok_(ok), running_(running) {}

    bool display_backend_available() override {
        return backend_available_;
    }

    void set_frame_interval(const openswd3::compat::u32 milliseconds) override {
        if (milliseconds == 0U) {
            static_cast<void>(
                openswd3::input_time_rng::clear_frame_interval(frame_interval_)
            );
            return;
        }

        static_cast<void>(openswd3::input_time_rng::set_frame_interval(
            frame_interval_, milliseconds
        ));
    }

    void suspend_audio_output() override {
        static_cast<void>(
            openswd3::audio_video::stop_legacy_stream(stream_manager_)
        );
    }
    void suspend_audio_streams() override {
        static_cast<void>(
            openswd3::audio_video::stop_all_legacy_samples(sample_manager_)
        );
    }
    void maintain_audio() override {
        service_audio(audio_maintenance_);
    }
    void suspend_battle_display() override {}
    void release_font(const openswd3::compat::u32 point_size) override {
        static_cast<void>(text_renderers_.release(point_size));
    }

    void minimize_window() override {
        static_cast<void>(SDL_MinimizeWindow(&window_));
    }

    void show_and_position_window() override {
        if (!SDL_RestoreWindow(&window_)) {
            fail_recovery("SDL_RestoreWindow");
        }
    }

    void restore_surfaces() override {
        if (texture_ != nullptr || !running_) {
            return;
        }
        const openswd3::rendering::LegacySurfaceGeometry& geometry =
            primary_surface_.geometry().surface;
        texture_ = SDL_CreateTexture(
            &renderer_,
            SDL_PIXELFORMAT_RGB565,
            SDL_TEXTUREACCESS_STREAMING,
            geometry.width,
            geometry.height
        );
        if (texture_ == nullptr) {
            fail_recovery("SDL_CreateTexture during display recovery");
        }
    }
    void rebuild_framebuffer_binding() override {}
    void rebuild_font(const openswd3::compat::u32 point_size) override {
        if (!running_) {
            return;
        }
        if (text_renderers_.rebuild(
                point_size, game_framebuffer_, glyph_provider_
            ) !=
            openswd3::rendering::LegacyTextRendererRuntimeStatus::completed) {
            openswd3::diagnostics::log_error(
                "legacy text renderer display recovery failed"
            );
            ok_ = false;
            running_ = false;
        }
    }
    void resume_battle_display() override {}
    void finish_display_recovery() override {
        if (!running_ || texture_ == nullptr) {
            return;
        }
        if (!present_framebuffer(renderer_, *texture_, primary_surface_)) {
            fail_recovery("framebuffer presentation during display recovery");
        }
    }

private:
    void fail_recovery(const char* operation) {
        static_cast<void>(report_sdl_error(operation));
        ok_ = false;
        running_ = false;
    }

    SDL_Window& window_;
    SDL_Renderer& renderer_;
    SDL_Texture*& texture_;
    const openswd3::rendering::LegacyFramebuffer& primary_surface_;
    openswd3::rendering::LegacyFramebuffer& game_framebuffer_;
    openswd3::rendering::LegacyGlyphAtlasProvider& glyph_provider_;
    openswd3::rendering::LegacyTextRendererRuntime& text_renderers_;
    openswd3::audio_video::LegacyStreamManager& stream_manager_;
    openswd3::audio_video::LegacySampleManager& sample_manager_;
    openswd3::audio_video::LegacyAudioMaintenancePorts& audio_maintenance_;
    openswd3::compat::u32& frame_interval_;
    bool backend_available_{};
    bool& ok_;
    bool& running_;
};

class SmokeShutdownPorts final : public openswd3::app::ShutdownPorts {
public:
    SmokeShutdownPorts(
        openswd3::rendering::LegacyTextRendererRuntime& text_renderers,
        openswd3::audio_video::LegacyStreamManager& stream_manager,
        openswd3::audio_video::LegacySampleManager& sample_manager,
        openswd3::world_map::LegacyWorldItemListState& world_item_lists
    )
        : text_renderers_(text_renderers), stream_manager_(stream_manager),
          sample_manager_(sample_manager), world_item_lists_(world_item_lists) {
    }

    void bind_packed_row_effects(
        std::list<openswd3::rendering::LegacyPackedRowEffect>& effects
    ) noexcept {
        packed_row_effects_ = &effects;
    }

    void bind_picture_actions(
        openswd3::world_map::LegacyPictureActionLists& actions
    ) noexcept {
        picture_actions_ = &actions;
    }

    void bind_role_particle_effect(
        openswd3::asset_runtime::LegacyAniRoleParticleEffect& effect
    ) noexcept {
        role_particle_effect_ = &effect;
    }

    void bind_ani_drift_effect(
        openswd3::asset_runtime::LegacyAniDriftEffect& effect
    ) noexcept {
        ani_drift_effect_ = &effect;
    }

    void bind_moving_actions(
        openswd3::world_map::LegacyMovingActionList& actions
    ) noexcept {
        moving_actions_ = &actions;
    }

    void bind_role_head_actions(
        openswd3::world_map::LegacyRoleHeadActionList& actions
    ) noexcept {
        role_head_actions_ = &actions;
    }

    void bind_dialogs(
        openswd3::story_scene::LegacyDialogRuntimeState& dialogs
    ) noexcept {
        dialogs_ = &dialogs;
    }

    void perform_shutdown_operation(
        const openswd3::app::ShutdownOperation operation
    ) override {
        using Operation = openswd3::app::ShutdownOperation;
        if (operation == Operation::release_font_20) {
            static_cast<void>(text_renderers_.release(20U));
        } else if (operation == Operation::release_font_16) {
            static_cast<void>(text_renderers_.release(16U));
        } else if (operation == Operation::release_font_12) {
            static_cast<void>(text_renderers_.release(12U));
        } else if (operation == Operation::suspend_audio_output_00485710) {
            static_cast<void>(
                openswd3::audio_video::stop_legacy_stream(stream_manager_)
            );
        } else if (operation == Operation::suspend_audio_streams_00485740) {
            static_cast<void>(
                openswd3::audio_video::stop_all_legacy_samples(sample_manager_)
            );
        } else if (operation == Operation::release_0040f410) {
            const auto result =
                openswd3::world_map::release_legacy_world_item_lists(
                    world_item_lists_
                );
            if (result.status !=
                openswd3::world_map::LegacyWorldItemListReleaseStatus::ready) {
                openswd3::diagnostics::log_error(
                    "legacy world item-list shutdown rejected malformed roots"
                );
            }
        } else if (
            operation == Operation::release_0040f5e0 &&
            picture_actions_ != nullptr
        ) {
            static_cast<void>(
                openswd3::world_map::release_legacy_picture_actions(
                    *picture_actions_
                )
            );
        } else if (
            operation == Operation::release_0040f500 &&
            packed_row_effects_ != nullptr
        ) {
            static_cast<void>(
                openswd3::rendering::release_legacy_packed_row_effects(
                    *packed_row_effects_
                )
            );
        } else if (
            operation == Operation::release_0040f540 &&
            moving_actions_ != nullptr
        ) {
            static_cast<void>(
                openswd3::world_map::release_legacy_moving_actions(
                    *moving_actions_
                )
            );
        } else if (
            operation == Operation::release_0040f570 &&
            role_head_actions_ != nullptr
        ) {
            static_cast<void>(
                openswd3::world_map::release_legacy_role_head_actions(
                    *role_head_actions_
                )
            );
        } else if (
            operation == Operation::release_0040f5a0 && dialogs_ != nullptr
        ) {
            static_cast<void>(
                openswd3::story_scene::release_legacy_dialog_messages(*dialogs_)
            );
        } else if (
            operation == Operation::release_0040f630 &&
            role_particle_effect_ != nullptr
        ) {
            static_cast<void>(role_particle_effect_->release());
        } else if (
            operation == Operation::release_0040f670 &&
            ani_drift_effect_ != nullptr
        ) {
            ani_drift_effect_->reset_positions();
        }
        if (operation == openswd3::app::ShutdownOperation::show_cursor) {
            static_cast<void>(SDL_ShowCursor());
        }
    }

    bool
    perform_shutdown_close(openswd3::app::ShutdownCloseOperation) override {
        return true;
    }

    void report_shutdown_close_failure(
        openswd3::app::ShutdownCloseOperation
    ) override {}

private:
    openswd3::rendering::LegacyTextRendererRuntime& text_renderers_;
    openswd3::audio_video::LegacyStreamManager& stream_manager_;
    openswd3::audio_video::LegacySampleManager& sample_manager_;
    openswd3::world_map::LegacyWorldItemListState& world_item_lists_;
    openswd3::world_map::LegacyPictureActionLists* picture_actions_{};
    std::list<openswd3::rendering::LegacyPackedRowEffect>*
        packed_row_effects_{};
    openswd3::world_map::LegacyMovingActionList* moving_actions_{};
    openswd3::world_map::LegacyRoleHeadActionList* role_head_actions_{};
    openswd3::story_scene::LegacyDialogRuntimeState* dialogs_{};
    openswd3::asset_runtime::LegacyAniRoleParticleEffect*
        role_particle_effect_{};
    openswd3::asset_runtime::LegacyAniDriftEffect* ani_drift_effect_{};
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

class SdlDeferredWorldFramePorts final
    : public openswd3::world_map::LegacyWorldFramePorts,
      public openswd3::asset_runtime::LegacyAniActivityPorts,
      public openswd3::world_map::LegacyWorldRoleExternalPorts,
      public openswd3::world_map::LegacyWorldSpatialAudioPorts,
      public openswd3::world_map::LegacyWorldOuterFramePorts,
      public openswd3::world_map::LegacyWorldDialogExternalPorts,
      public openswd3::rendering::LegacyTimedMessageInputPorts {
public:
    SdlDeferredWorldFramePorts(
        openswd3::audio_video::LegacyAudioMaintenancePorts& audio,
        openswd3::rendering::LegacyPresentationPorts& presentation,
        openswd3::rendering::LegacyTextRendererRuntime& text_renderers,
        openswd3::world_map::LegacyWorldHeadSignActionsState*
            head_sign_actions = nullptr,
        openswd3::world_map::LegacyWorldStoryPathRuntime* story_paths = nullptr,
        const openswd3::world_map::LegacyWorldPathScriptState*
            path_script_state = nullptr,
        const openswd3::world_map::LegacyWorldStoryVmState* story_state =
            nullptr,
        openswd3::platform_sdl3::WorldRoleRuntimeAdapter* world_role_adapter =
            nullptr,
        openswd3::asset_runtime::LegacyAniActivity* ani_activity = nullptr,
        openswd3::rendering::LegacyFramebuffer* framebuffer = nullptr,
        const openswd3::rendering::LegacyPixelConversionState*
            pixel_conversion = nullptr,
        openswd3::compat::u32* process_flags = nullptr,
        openswd3::compat::u8* scene_flags = nullptr,
        openswd3::compat::u32* frame_interval = nullptr,
        const openswd3::asset_runtime::LegacyAniActivityBlockers ani_blockers =
            {},
        const std::vector<openswd3::compat::u16>* ani_scene_backup = nullptr
    ) noexcept
        : audio_(audio), presentation_(presentation),
          text_renderers_(text_renderers),
          head_sign_actions_(head_sign_actions), story_paths_(story_paths),
          path_script_state_(path_script_state), story_state_(story_state),
          world_role_adapter_(world_role_adapter), ani_activity_(ani_activity),
          framebuffer_(framebuffer), pixel_conversion_(pixel_conversion),
          process_flags_(process_flags), scene_flags_(scene_flags),
          frame_interval_(frame_interval), ani_blockers_(ani_blockers),
          ani_scene_backup_(ani_scene_backup) {}

    bool complete_role_path(
        const openswd3::compat::u32 role_index
    ) noexcept override {
        if (story_paths_ == nullptr) {
            return false;
        }
        const auto result =
            openswd3::world_map::complete_legacy_world_story_path(
                *story_paths_, role_index
            );
        return result.status ==
            openswd3::world_map::LegacyWorldStoryPathStatus::completed;
    }

    bool
    query_service(const openswd3::compat::u32 service_id) noexcept override {
        return story_state_ != nullptr &&
            service_id < openswd3::world_map::kLegacyWorldStoryFlagBytes * 8U &&
            openswd3::world_map::query_legacy_world_story_flag(
                   *story_state_, static_cast<openswd3::compat::u16>(service_id)
            );
    }

    bool query_control(openswd3::compat::u32) noexcept override {
        return false;
    }

    bool is_legacy_control_active(
        const openswd3::compat::u32 control_index
    ) noexcept override {
        return query_control(control_index);
    }

    void play_dialog_choice_sound() noexcept override {}

    bool execute_stage(
        const openswd3::world_map::LegacyWorldFrameStage stage
    ) noexcept override {
        if (stage !=
                openswd3::world_map::LegacyWorldFrameStage::
                    ani_activity_004154a0 ||
            ani_activity_ == nullptr || framebuffer_ == nullptr ||
            process_flags_ == nullptr || scene_flags_ == nullptr) {
            ++deferred_frame_stage_count_;
            return true;
        }
        ani_stage_failed_ = false;
        ani_activity_->state().scene_flags = *scene_flags_;
        try {
            const auto physical_pixels = framebuffer_->physical_pixels();
            const auto result = ani_activity_->update(
                std::span<openswd3::compat::u8>{
                    reinterpret_cast<openswd3::compat::u8*>(
                        physical_pixels.data()
                    ),
                    physical_pixels.size_bytes(),
                },
                static_cast<openswd3::compat::u32>(
                    framebuffer_->geometry().surface.pitch_bytes
                ),
                ani_blockers_,
                *this
            );
            *process_flags_ = ani_activity_->state().process_flags;
            *scene_flags_ = static_cast<openswd3::compat::u8>(
                ani_activity_->state().scene_flags
            );
            return !ani_stage_failed_ &&
                result.status ==
                openswd3::asset_runtime::LegacyAniActivityStatus::ready;
        } catch (const std::bad_alloc&) {
            return false;
        }
    }

    void redraw_scene_without_ani() override {
        if (framebuffer_ == nullptr) {
            return;
        }
        const auto destination = framebuffer_->physical_pixels();
        if (ani_scene_backup_ != nullptr &&
            ani_scene_backup_->size() == destination.size()) {
            std::ranges::copy(*ani_scene_backup_, destination.begin());
            return;
        }
        std::ranges::fill(destination, openswd3::compat::u16{0U});
    }

    void apply_ending_color_adjustment(
        const std::span<openswd3::compat::u8>,
        const openswd3::compat::u32 pixel_count,
        const openswd3::compat::i32 first,
        const openswd3::compat::i32 second,
        const openswd3::compat::i32 third
    ) override {
        if (framebuffer_ == nullptr || pixel_conversion_ == nullptr) {
            ani_stage_failed_ = true;
            return;
        }
        ani_stage_failed_ =
            openswd3::rendering::adjust_legacy_rgb_channels(
                framebuffer_->physical_pixels_with_read_guard(),
                static_cast<openswd3::compat::i32>(pixel_count),
                first,
                second,
                third,
                *pixel_conversion_
            ) != openswd3::rendering::LegacyFrameColorStatus::completed;
    }

    void finalize_service(const openswd3::compat::u32 service_id) override {
        if (frame_interval_ != nullptr) {
            *frame_interval_ = service_id;
        }
    }

    void draw_decorated_number(
        openswd3::compat::i32,
        openswd3::compat::i32,
        openswd3::compat::u32,
        openswd3::compat::u32
    ) noexcept override {}

    void play_positional_sample(
        const openswd3::compat::u16 sound_id,
        const openswd3::compat::i32 world_x,
        const openswd3::compat::i32 world_y
    ) noexcept override {
        if (world_role_adapter_ != nullptr) {
            static_cast<void>(world_role_adapter_->play_positional_sample(
                sound_id, world_x, world_y
            ));
        }
    }

    const openswd3::asset_runtime::LegacyActionRecord* resolve_overlay_action(
        const openswd3::compat::u32 token
    ) noexcept override {
        if (world_role_adapter_ != nullptr) {
            return world_role_adapter_->resolve_overlay_action(token);
        }
        if (head_sign_actions_ == nullptr) {
            return nullptr;
        }
        return openswd3::world_map::resolve_legacy_world_head_sign_action(
            *head_sign_actions_, token
        );
    }

    void emit_role_particles(
        const openswd3::compat::i32 world_x,
        const openswd3::compat::i32 world_y,
        const openswd3::compat::u16 role_selector
    ) noexcept override {
        if (world_role_adapter_ != nullptr) {
            static_cast<void>(world_role_adapter_->update_role_particles(
                world_x, world_y, role_selector
            ));
        }
    }

    std::span<const openswd3::compat::u8>
    resolve_label_bytes(const openswd3::compat::u32 token) noexcept override {
        if (world_role_adapter_ != nullptr) {
            const openswd3::platform_sdl3::WorldRoleLabelView bytes =
                world_role_adapter_->resolve_label_bytes(token);
            return {bytes.data, bytes.size};
        }
        if (path_script_state_ == nullptr) {
            return {};
        }
        return openswd3::world_map::resolve_legacy_world_path_label(
            *path_script_state_, token
        );
    }

    openswd3::compat::u16
    label_color(const openswd3::compat::u32 index) noexcept override {
        return world_role_adapter_ != nullptr
            ? world_role_adapter_->label_color(index)
            : 0U;
    }

    void draw_label(
        const std::span<const openswd3::compat::u8> bytes,
        const openswd3::compat::i32 x,
        const openswd3::compat::i32 y,
        const openswd3::compat::u16 color,
        const openswd3::compat::u32 style
    ) noexcept override {
        if (world_role_adapter_ != nullptr) {
            world_role_adapter_->draw_label(
                bytes.data(), bytes.size(), x, y, color, style
            );
        }
    }

    void play_sample(
        const openswd3::compat::u16 sound_id,
        const openswd3::compat::i32 volume,
        const openswd3::compat::i32 pan,
        const openswd3::compat::i32 loop_count
    ) noexcept override {
        if (world_role_adapter_ != nullptr) {
            world_role_adapter_->play_sample(sound_id, volume, pan, loop_count);
        }
    }

    void stop_sample(const openswd3::compat::u16 sound_id) noexcept override {
        if (world_role_adapter_ != nullptr) {
            world_role_adapter_->stop_sample(sound_id);
        }
    }

    void set_sample_volume(
        const openswd3::compat::u16 sound_id, const openswd3::compat::i32 volume
    ) noexcept override {
        if (world_role_adapter_ != nullptr) {
            world_role_adapter_->set_sample_volume(sound_id, volume);
        }
    }

    void set_sample_pan(
        const openswd3::compat::u16 sound_id, const openswd3::compat::i32 pan
    ) noexcept override {
        if (world_role_adapter_ != nullptr) {
            world_role_adapter_->set_sample_pan(sound_id, pan);
        }
    }

    void configure_debug_text(
        const openswd3::compat::u16 background_color,
        const openswd3::compat::u16 secondary_color
    ) noexcept override {
        const auto binding = text_renderers_.binding(16U);
        if (binding.ready()) {
            binding.state->background_color = background_color;
            binding.state->secondary_color = secondary_color;
        }
    }

    openswd3::rendering::LegacyTextDrawResult draw_debug_text(
        const openswd3::rendering::LegacyTextDrawRequest& request
    ) noexcept override {
        const auto binding = text_renderers_.binding(16U);
        if (!binding.ready()) {
            return {
                .status = openswd3::rendering::LegacyTextDrawStatus::
                    glyph_provider_failed,
            };
        }
        return openswd3::rendering::draw_legacy_text(
            *binding.framebuffer,
            *binding.glyph_cache,
            *binding.glyph_provider,
            *binding.state,
            request
        );
    }

    bool query_debug_flag(openswd3::compat::u32) noexcept override {
        return false;
    }

    void maintain_audio() noexcept override {
        service_audio(audio_);
    }

    void request_world_presentation() noexcept override {
        const auto result = openswd3::rendering::submit_legacy_presentation(
            openswd3::rendering::LegacyPresentationSite::steady_world,
            presentation_
        );
        presentation_failed_ = result.status !=
            openswd3::rendering::LegacyPresentationDispatchStatus::completed;
    }

    [[nodiscard]] bool presentation_failed() const noexcept {
        return presentation_failed_;
    }

    [[nodiscard]] openswd3::compat::u32
    deferred_frame_stage_count() const noexcept {
        return deferred_frame_stage_count_;
    }

private:
    openswd3::audio_video::LegacyAudioMaintenancePorts& audio_;
    openswd3::rendering::LegacyPresentationPorts& presentation_;
    openswd3::rendering::LegacyTextRendererRuntime& text_renderers_;
    openswd3::world_map::LegacyWorldHeadSignActionsState* head_sign_actions_{};
    openswd3::world_map::LegacyWorldStoryPathRuntime* story_paths_{};
    const openswd3::world_map::LegacyWorldPathScriptState* path_script_state_{};
    const openswd3::world_map::LegacyWorldStoryVmState* story_state_{};
    openswd3::platform_sdl3::WorldRoleRuntimeAdapter* world_role_adapter_{};
    openswd3::asset_runtime::LegacyAniActivity* ani_activity_{};
    openswd3::rendering::LegacyFramebuffer* framebuffer_{};
    const openswd3::rendering::LegacyPixelConversionState* pixel_conversion_{};
    openswd3::compat::u32* process_flags_{};
    openswd3::compat::u8* scene_flags_{};
    openswd3::compat::u32* frame_interval_{};
    openswd3::asset_runtime::LegacyAniActivityBlockers ani_blockers_{};
    const std::vector<openswd3::compat::u16>* ani_scene_backup_{};
    openswd3::compat::u32 deferred_frame_stage_count_{};
    bool presentation_failed_{};
    bool ani_stage_failed_{};
};

class SdlSmokeIdlePorts final
    : public openswd3::app::IdleRuntimePorts,
      public openswd3::app::FramePreparationPorts,
      public openswd3::app::FrameRuntimePorts,
      public openswd3::rendering::LegacyPresentationPorts,
      public openswd3::audio_video::LegacyVideoFramePorts,
      public openswd3::world_map::LegacyWorldLoadProgressPorts {
public:
    class WorldInteractionPorts final
        : public openswd3::world_map::LegacyWorldInteractionPorts {
    public:
        WorldInteractionPorts(
            openswd3::input_time_rng::LegacyInputNormalizationState& input,
            openswd3::world_map::LegacyWorldStoryVmState& story,
            openswd3::asset_runtime::LegacyActionUpdater& action_updater,
            openswd3::asset_runtime::LegacyTswRuntime& tsw_runtime
        ) noexcept
            : input_(input), story_(story), action_updater_(action_updater),
              tsw_runtime_(tsw_runtime) {}

        openswd3::compat::u32
        query_internal_flag(const openswd3::compat::u32 bit_index) override {
            if (bit_index == 9U) {
                return input_.mouse_inactive_flag_9 ? 1U : 0U;
            }
            if (bit_index >=
                openswd3::world_map::kLegacyWorldStoryFlagBytes * 8U) {
                return 0U;
            }
            return openswd3::world_map::query_legacy_world_story_flag(
                       story_, static_cast<openswd3::compat::u16>(bit_index)
                   )
                ? 1U
                : 0U;
        }

        bool load_role_frame_size(
            const openswd3::compat::u16 resource_id,
            const openswd3::compat::u16 frame_index,
            openswd3::compat::u16& width,
            openswd3::compat::u16& height
        ) override {
            // sub_431A20 is a cache-only lookup. A hover probe must not load
            // or evict TSW data that the original frame had not drawn yet.
            const auto loaded =
                tsw_runtime_.find_cached(resource_id, frame_index);
            if (loaded.status !=
                openswd3::asset_runtime::LegacyTswRuntimeStatus::ready) {
                width = 0U;
                height = 0U;
                return false;
            }
            width = loaded.frame.width;
            height = loaded.frame.height;
            return true;
        }

        openswd3::compat::u32 update_action(
            openswd3::asset_runtime::LegacyActionRecord& action
        ) override {
            return action_updater_.update(action).return_value;
        }

    private:
        openswd3::input_time_rng::LegacyInputNormalizationState& input_;
        openswd3::world_map::LegacyWorldStoryVmState& story_;
        openswd3::asset_runtime::LegacyActionUpdater& action_updater_;
        openswd3::asset_runtime::LegacyTswRuntime& tsw_runtime_;
    };

    class WorldDebugHotkeyPorts final
        : public openswd3::world_map::LegacyWorldDebugHotkeyPorts {
    public:
        WorldDebugHotkeyPorts(
            SdlSmokeIdlePorts& owner,
            openswd3::app::FrameCoordinatorState& frame_state
        ) noexcept
            : owner_(owner), frame_state_(frame_state) {}

        openswd3::compat::u32
        query_internal_flag(const openswd3::compat::u32 bit_index) override {
            if (bit_index >=
                openswd3::world_map::kLegacyWorldStoryFlagBytes * 8U) {
                return 0U;
            }
            return openswd3::world_map::query_legacy_world_story_flag(
                       owner_.world_story_vm_state_,
                       static_cast<openswd3::compat::u16>(bit_index)
                   )
                ? 1U
                : 0U;
        }
        void set_internal_flag(
            const openswd3::compat::u32 bit_index, const bool value
        ) override {
            if (bit_index >=
                openswd3::world_map::kLegacyWorldStoryFlagBytes * 8U) {
                return;
            }
            const auto narrowed = static_cast<openswd3::compat::u16>(bit_index);
            if (value) {
                openswd3::world_map::set_legacy_world_story_flag(
                    owner_.world_story_vm_state_, narrowed
                );
            } else {
                openswd3::world_map::clear_legacy_world_story_flag(
                    owner_.world_story_vm_state_, narrowed
                );
            }
        }
        void
        delay_milliseconds(const openswd3::compat::u32 milliseconds) override {
            SDL_Delay(milliseconds);
        }
        void request_debug_mode(
            const openswd3::world_map::LegacyWorldDebugModeRequest& request
        ) override {
            frame_state_.battle.high_priority_state = request.modal_state;
        }
        void run_debug_action(openswd3::compat::u32) override {
            frame_state_.transition_suppression = 1U;
        }
        void show_cursor() override {
            static_cast<void>(SDL_ShowCursor());
        }
        openswd3::compat::i32 show_resource_dialog() override {
            // The Win32 resource dialog has no SDL equivalent. Its nonzero
            // modal result leaves 4CC2AC clear, so the caller skips the rest
            // of this world frame as well as returning from sub_402F80.
            frame_state_.transition_suppression = 1U;
            return 1;
        }
        bool load_item_category(
            openswd3::compat::u32, openswd3::compat::u16&
        ) override {
            // The legacy party-item sentinel owner is not exposed by the SDL
            // runtime. The isolated developer feature therefore skips adds.
            return false;
        }
        void add_item(openswd3::compat::u32, openswd3::compat::u32) override {}
        void release_item_definition(openswd3::compat::u32) override {}

    private:
        SdlSmokeIdlePorts& owner_;
        openswd3::app::FrameCoordinatorState& frame_state_;
    };

    class WorldPlayerPorts final
        : public openswd3::world_map::LegacyWorldFacingTalkPorts,
          public openswd3::world_map::LegacyWorldCollisionTalkPorts {
    public:
        WorldPlayerPorts(
            const std::span<openswd3::world_map::LegacyWorldRoleRecord> roles,
            const std::span<const openswd3::compat::u8> surface_grid,
            const openswd3::compat::u32 map_width,
            openswd3::input_time_rng::LegacyInputNormalizationState& input,
            openswd3::world_map::LegacyWorldStoryVmState& story,
            openswd3::asset_runtime::LegacyActionUpdater& action_updater
        ) noexcept
            : roles_(roles), surface_grid_(surface_grid), map_width_(map_width),
              input_(input), story_(story), action_updater_(action_updater) {}

        openswd3::world_map::LegacyMovementCollisionResult query_collision(
            const openswd3::compat::u32 role_index,
            const openswd3::compat::i32 delta_x,
            const openswd3::compat::i32 delta_y
        ) override {
            return openswd3::world_map::check_legacy_movement_collision(
                roles_,
                static_cast<openswd3::compat::u32>(roles_.size()),
                role_index,
                delta_x,
                delta_y,
                surface_grid_,
                map_width_
            );
        }

        openswd3::compat::u32
        query_internal_flag(const openswd3::compat::u32 bit_index) override {
            if (bit_index == 9U) {
                return input_.mouse_inactive_flag_9 ? 1U : 0U;
            }
            if (bit_index >=
                openswd3::world_map::kLegacyWorldStoryFlagBytes * 8U) {
                return 0U;
            }
            return openswd3::world_map::query_legacy_world_story_flag(
                       story_, static_cast<openswd3::compat::u16>(bit_index)
                   )
                ? 1U
                : 0U;
        }

        openswd3::compat::u32 update_action(
            openswd3::asset_runtime::LegacyActionRecord& action
        ) override {
            return action_updater_.update(action).return_value;
        }

    private:
        std::span<openswd3::world_map::LegacyWorldRoleRecord> roles_;
        std::span<const openswd3::compat::u8> surface_grid_;
        openswd3::compat::u32 map_width_{};
        openswd3::input_time_rng::LegacyInputNormalizationState& input_;
        openswd3::world_map::LegacyWorldStoryVmState& story_;
        openswd3::asset_runtime::LegacyActionUpdater& action_updater_;
    };

    class WorldEncounterPorts final
        : public openswd3::world_map::LegacyWorldEncounterPorts {
    public:
        WorldEncounterPorts(
            SdlSmokeIdlePorts& owner,
            openswd3::world_map::LegacyWorldRuntimeSession& world,
            const openswd3::world_map::LegacyWorldRoleRecord& player
        ) noexcept
            : owner_(owner), world_(world), player_(player),
              random_(owner.secondary_rng_) {}

        openswd3::compat::u32 query_encounter_suppression() override {
            const openswd3::compat::u32 bit_index =
                world_.logical_map_id + 0x1770U;
            if (bit_index >=
                openswd3::world_map::kLegacyWorldStoryFlagBytes * 8U) {
                return 0U;
            }
            return openswd3::world_map::query_legacy_world_story_flag(
                       owner_.world_story_vm_state_,
                       static_cast<openswd3::compat::u16>(bit_index)
                   )
                ? 1U
                : 0U;
        }

        openswd3::compat::u32
        query_internal_flag(const openswd3::compat::u32 bit_index) override {
            if (bit_index >=
                openswd3::world_map::kLegacyWorldStoryFlagBytes * 8U) {
                return 0U;
            }
            return openswd3::world_map::query_legacy_world_story_flag(
                       owner_.world_story_vm_state_,
                       static_cast<openswd3::compat::u16>(bit_index)
                   )
                ? 1U
                : 0U;
        }

        openswd3::world_map::LegacyRandomEncounterResult select_encounter(
            openswd3::compat::u32& encounter_step_counter,
            const openswd3::compat::u32 force_mode
        ) override {
            return openswd3::world_map::select_legacy_random_encounter(
                {
                    .encounter_table_index =
                        world_.map_descriptor_runtime.encounter_table_index,
                    .force_mode = force_mode,
                    .player_world_x = player_.world_x,
                    .player_world_y = player_.world_y,
                },
                encounter_step_counter,
                world_.encounter_thresholds.groups,
                world_.encounter_regions.regions,
                owner_.resource_databases_.maps_payload_bytes(),
                random_
            );
        }

        void stop_legacy_stream() override {
            static_cast<void>(openswd3::audio_video::stop_legacy_stream(
                owner_.stream_manager_
            ));
        }
        void stop_all_legacy_samples() override {
            static_cast<void>(openswd3::audio_video::stop_all_legacy_samples(
                owner_.sample_manager_
            ));
        }
        void release_pre_battle_resource_433010() override {
            // The original releases a process-global DirectDraw owner. SDL
            // framebuffers are persistent RAII values and need no matching
            // destructive call at this slot.
        }
        void release_pre_battle_resource_431960() override {
            // The paired DirectDraw owner is likewise represented by a
            // persistent typed framebuffer.
        }
        void initialize_battle(const openswd3::compat::u16 battle_id) override {
            owner_.initialize_battle(battle_id);
        }
        bool close_world_map_view() override {
            // MAPS.DAT is an owned byte vector rather than a borrowed Win32
            // mapped view; the logical close succeeds without invalidating it.
            return true;
        }
        void report_world_map_view_close_failure() override {
            openswd3::diagnostics::log_warning(
                "random encounter: MAPS view close failed"
            );
        }
        void close_world_map_handle() override {
            // The LegacyFile owner is shared with later remap/reload paths and
            // closes by RAII; no raw mapping handle exists here.
        }

    private:
        SdlSmokeIdlePorts& owner_;
        openswd3::world_map::LegacyWorldRuntimeSession& world_;
        const openswd3::world_map::LegacyWorldRoleRecord& player_;
        openswd3::world_map::LegacySecondaryRandomEncounterRng random_;
    };

    SdlSmokeIdlePorts(
        SDL_Window& window,
        SDL_Renderer& renderer,
        SDL_Texture*& texture,
        openswd3::rendering::LegacyFramebuffer& game_framebuffer,
        openswd3::rendering::LegacyFramebuffer& primary_surface,
        openswd3::compat::u32& frame_interval,
        openswd3::app::WindowEventState& window_state,
        const openswd3::app::DisplayLifecycleState& display_state,
        openswd3::app::FramePreparationState& frame_preparation_state,
        openswd3::app::FrameCoordinatorState& frame_coordinator_state,
        openswd3::input_time_rng::LegacyInputNormalizationState& input_state,
        openswd3::input_time_rng::LegacyMouseState& mouse_state,
        openswd3::platform_sdl3::SdlMouseDeviceState& mouse_device_state,
        openswd3::audio_video::LegacyAudioMaintenancePorts& audio_maintenance,
        openswd3::audio_video::LegacyStreamManager& stream_manager,
        openswd3::audio_video::LegacySampleManager& sample_manager,
        openswd3::audio_video::LegacyVideoPlayer& video_player,
        openswd3::resource_io::LegacyResourceDatabases& resource_databases,
        openswd3::world_map::LegacyWorldItemListState& world_item_lists,
        std::filesystem::path data_directory,
        std::filesystem::path world_cache_directory,
        const openswd3::rendering::LegacyPixelConversionState& pixel_conversion,
        openswd3::rendering::LegacyTextRendererRuntime& text_renderers,
        openswd3::world_map::LegacyWorldRoleActionInitializer&
            world_action_initializer,
        openswd3::asset_runtime::LegacyActionUpdater& action_updater,
        openswd3::asset_runtime::LegacyTswRuntime& tsw_runtime,
        openswd3::input_time_rng::LegacyCrtRng& crt_rng,
        openswd3::input_time_rng::LegacySecondaryRng& secondary_rng,
        openswd3::app::ShutdownPorts& shutdown_ports,
        openswd3::app::ProcessExitPorts& exit_ports,
        bool& ok,
        bool& running
    )
        : window_(window), renderer_(renderer), texture_(texture),
          game_framebuffer_(game_framebuffer),
          primary_surface_(primary_surface), frame_interval_(frame_interval),
          window_state_(window_state), display_state_(display_state),
          frame_preparation_state_(frame_preparation_state),
          frame_coordinator_state_(frame_coordinator_state),
          input_state_(input_state), mouse_state_(mouse_state),
          mouse_device_state_(mouse_device_state),
          audio_maintenance_(audio_maintenance),
          stream_manager_(stream_manager), sample_manager_(sample_manager),
          video_player_(video_player), resource_databases_(resource_databases),
          world_item_lists_(world_item_lists),
          data_directory_(std::move(data_directory)),
          world_cache_directory_(std::move(world_cache_directory)),
          pixel_conversion_(pixel_conversion), text_renderers_(text_renderers),
          world_action_initializer_(world_action_initializer),
          action_updater_(action_updater), tsw_runtime_(tsw_runtime),
          crt_rng_(crt_rng), secondary_rng_(secondary_rng),
          world_raster_(game_framebuffer.geometry()),
          world_effects_{.pixel_conversion = pixel_conversion},
          shutdown_ports_(shutdown_ports), exit_ports_(exit_ports), ok_(ok),
          running_(running) {
        openswd3::asset_runtime::LegacyActionDrawRuntimePorts action_ports{
            action_updater_,
            tsw_runtime_,
            game_framebuffer_,
            world_raster_,
            world_effects_,
            world_jitter_,
        };
        if (openswd3::world_map::prime_legacy_world_cursor_state(
                world_frame_effects_.cursor, action_ports
            ) != openswd3::asset_runtime::LegacyActionUpdateStatus::completed) {
            openswd3::diagnostics::log_warning(
                "initial cursor action update failed"
            );
        }
        const auto dialog_prime =
            openswd3::world_map::prime_legacy_world_dialog_runtime(
                world_dialog_runtime_state_, action_ports
            );
        if (dialog_prime.action_update_failure_count != 0U) {
            openswd3::diagnostics::log_warning(
                "initial dialog indicator action update failed"
            );
        }
        openswd3::world_map::initialize_legacy_world_story_vm(
            world_story_vm_state_
        );
    }

    [[nodiscard]] std::list<openswd3::rendering::LegacyPackedRowEffect>&
    packed_row_effects() noexcept {
        return world_frame_effects_.packed_rows;
    }

    [[nodiscard]] openswd3::world_map::LegacyPictureActionLists&
    picture_actions() noexcept {
        return world_picture_actions_;
    }

    [[nodiscard]] openswd3::world_map::LegacyMovingActionList&
    moving_actions() noexcept {
        return world_moving_actions_;
    }

    [[nodiscard]] openswd3::world_map::LegacyRoleHeadActionList&
    role_head_actions() noexcept {
        return world_role_head_actions_;
    }

    [[nodiscard]] openswd3::story_scene::LegacyDialogRuntimeState&
    dialogs() noexcept {
        return world_dialogs_;
    }

    [[nodiscard]] openswd3::asset_runtime::LegacyAniRoleParticleEffect&
    role_particle_effect() noexcept {
        return world_role_particle_effect_;
    }

    [[nodiscard]] openswd3::asset_runtime::LegacyAniDriftEffect&
    ani_drift_effect() noexcept {
        return world_frame_effects_.drift;
    }

    void latch_keyboard_press(const SDL_Scancode scancode) noexcept {
        static_cast<void>(openswd3::platform_sdl3::latch_sdl_keyboard_press(
            pending_keyboard_presses_, scancode
        ));
    }

    void dispatch_text_input_key(const SDL_Scancode scancode) noexcept {
        if (!initial_menu_state_.name_input.has_value()) {
            return;
        }
        const auto virtual_key = legacy_text_virtual_key(scancode);
        if (!virtual_key.has_value()) {
            return;
        }
        static_cast<void>(
            openswd3::input_time_rng::filter_legacy_text_input_message(
                *initial_menu_state_.name_input,
                text_input_driver_state_,
                openswd3::input_time_rng::kLegacyKeyDownMessage,
                *virtual_key,
                0U,
                text_input_ports_
            )
        );
    }

    void dispatch_text_input_utf8(const char* const text) noexcept {
        if (!initial_menu_state_.name_input.has_value() || text == nullptr) {
            return;
        }
        const auto encoded = openswd3::platform_sdl3::utf8_to_cp950(text);
        if (!encoded.has_value()) {
            openswd3::diagnostics::log_warning(
                "name input contains a character unavailable in CP950"
            );
            return;
        }
        const auto* bytes =
            reinterpret_cast<const openswd3::compat::u8*>(encoded->c_str());
        static_cast<void>(openswd3::input_time_rng::legacy_insert_text_bytes(
            *initial_menu_state_.name_input, bytes
        ));
    }

    void latch_mouse_press(const Uint8 button) noexcept {
        if (button == SDL_BUTTON_LEFT) {
            pending_mouse_button_mask_ |= 1U;
        } else if (button == SDL_BUTTON_RIGHT) {
            pending_mouse_button_mask_ |= 2U;
        }
    }

    void clear_input_latches() noexcept {
        pending_keyboard_presses_.fill(0U);
        pending_mouse_button_mask_ = 0U;
    }

    void step_video() override {
        const auto result = video_player_.step(*this);
        if (result.status ==
            openswd3::audio_video::LegacyVideoStepStatus::completed) {
            window_state_.process_flags &= ~openswd3::app::kProcessVideoActive;
        }
    }
    void maintain_audio() override {
        service_audio(audio_maintenance_);
    }

    [[nodiscard]] openswd3::compat::u32
    next_random_bounded(const openswd3::compat::u32 upper_bound) override {
        return secondary_rng_.next_bounded(upper_bound);
    }

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
        frame_preparation_state_.world_role_count =
            active_world_session_.has_value()
            ? static_cast<openswd3::compat::u32>(
                  active_world_session_->render.map_load.session.business.state
                      .roles.size()
              )
            : 0U;
        if (openswd3::app::run_frame_preparation(
                frame_preparation_state_, *this
            ) == openswd3::app::FramePreparationOutcome::accepted) {
            frame_coordinator_state_.frame_execution_gate =
                window_state_.frame_execution_gate;
            frame_coordinator_state_.process_flags =
                window_state_.process_flags;
            frame_coordinator_state_.transition_suppression =
                display_state_.transition_suppression;
            static_cast<void>(openswd3::app::run_accepted_frame(
                frame_coordinator_state_, *this
            ));
            window_state_.process_flags =
                frame_coordinator_state_.process_flags;
        }
    }

    void present_pause() override {
        request_presentation(
            openswd3::rendering::LegacyPresentationSite::pause_overlay
        );
    }

    openswd3::compat::u32 read_seconds() override {
        return static_cast<openswd3::compat::u32>(std::time(nullptr));
    }

    openswd3::compat::u32 read_milliseconds() override {
        return static_cast<openswd3::compat::u32>(SDL_GetTicks());
    }

    bool query_internal_flag(const openswd3::compat::u32 index) override {
        if (index == 9U) {
            return input_state_.mouse_inactive_flag_9;
        }
        if (index >= openswd3::world_map::kLegacyWorldStoryFlagBytes * 8U) {
            return false;
        }
        return openswd3::world_map::query_legacy_world_story_flag(
            world_story_vm_state_, static_cast<openswd3::compat::u16>(index)
        );
    }

    void clear_internal_flag(const openswd3::compat::u32 index) override {
        if (index == 9U) {
            input_state_.mouse_inactive_flag_9 = false;
        }
        if (index < openswd3::world_map::kLegacyWorldStoryFlagBytes * 8U) {
            openswd3::world_map::clear_legacy_world_story_flag(
                world_story_vm_state_, static_cast<openswd3::compat::u16>(index)
            );
        }
    }

    void set_internal_flag(const openswd3::compat::u32 index) override {
        if (index == 9U) {
            input_state_.mouse_inactive_flag_9 = true;
        }
        if (index < openswd3::world_map::kLegacyWorldStoryFlagBytes * 8U) {
            openswd3::world_map::set_legacy_world_story_flag(
                world_story_vm_state_, static_cast<openswd3::compat::u16>(index)
            );
        }
    }

    void perform_primary_transition_operation(
        const openswd3::app::PrimaryTransitionOperation operation
    ) override {
        if (operation ==
            openswd3::app::PrimaryTransitionOperation::release_0040f5e0) {
            shutdown_ports_.perform_shutdown_operation(
                openswd3::app::ShutdownOperation::release_0040f5e0
            );
        } else if (
            operation ==
            openswd3::app::PrimaryTransitionOperation::release_0040f500
        ) {
            shutdown_ports_.perform_shutdown_operation(
                openswd3::app::ShutdownOperation::release_0040f500
            );
        } else if (
            operation ==
            openswd3::app::PrimaryTransitionOperation::release_0040f540
        ) {
            shutdown_ports_.perform_shutdown_operation(
                openswd3::app::ShutdownOperation::release_0040f540
            );
        } else if (
            operation ==
            openswd3::app::PrimaryTransitionOperation::release_0040f570
        ) {
            shutdown_ports_.perform_shutdown_operation(
                openswd3::app::ShutdownOperation::release_0040f570
            );
        } else if (
            operation ==
            openswd3::app::PrimaryTransitionOperation::release_0040f5a0
        ) {
            shutdown_ports_.perform_shutdown_operation(
                openswd3::app::ShutdownOperation::release_0040f5a0
            );
        }
    }

    void release_and_clear_world_role_transition(
        const openswd3::compat::u32 role_index
    ) override {
        if (!active_world_session_.has_value()) {
            openswd3::diagnostics::log_warning(
                "world role transition requested without an active world"
            );
            return;
        }

        auto& world = *active_world_session_;
        auto& map = world.render.map_load.session;
        auto& roles = map.business.state.roles;
        if (world.selected_role_index >= roles.size()) {
            openswd3::diagnostics::log_warning(
                "world role transition has an invalid selected role"
            );
            return;
        }

        openswd3::world_map::LegacyWorldStoryPathRuntime story_paths{
            .roles = roles,
            .active_object_slots =
                world_frame_state_.map_role_paths.active_object_slots,
            .spatial_index = &map.business.state.spatial_index,
            .role_surface =
                {
                    .map_width = map.header.width,
                    .selected_guid = roles[world.selected_role_index].guid,
                    .surface_grid = map.surface_grid.surface_grid,
                },
            .node_pool = &world_path_node_pool_,
            .movement = &world_frame_state_.movement,
            .camera = &world.camera,
            .selected_arrival_bytes =
                world_frame_state_.map_role_paths.guid_one_arrival_bytes,
            .selected_role_index = world.selected_role_index,
            .map_height = map.header.height,
            .scene_render_flags =
                &world_frame_state_.frame_runtime.frame.runtime_flags,
        };
        SdlDeferredWorldFramePorts path_ports{
            audio_maintenance_,
            *this,
            text_renderers_,
            &world_frame_state_.head_sign_actions,
            &story_paths,
            &world_path_script_state_,
            &world_story_vm_state_,
        };
        openswd3::asset_runtime::LegacyActionDrawRuntimePorts action_ports{
            action_updater_,
            tsw_runtime_,
            game_framebuffer_,
            world_raster_,
            world_effects_,
            world_jitter_,
        };
        const auto result =
            openswd3::world_map::release_legacy_world_role_transition(
                roles,
                world_frame_state_.map_role_paths.active_object_slots,
                role_index,
                world.selected_role_index,
                story_paths.role_surface,
                path_ports,
                action_ports
            );
        if (result.status !=
            openswd3::world_map::LegacyWorldRoleTransitionStatus::ready) {
            std::string message{"world role transition failed: role="};
            message.append(std::to_string(role_index));
            message.append(", status=");
            message.append(
                std::to_string(static_cast<unsigned int>(result.status))
            );
            openswd3::diagnostics::log_warning(message);
        }
    }

    void sample_input_device() override {
        static_cast<void>(openswd3::platform_sdl3::sample_sdl_keyboard_state(
            keyboard_snapshot_
        ));
        openswd3::platform_sdl3::merge_sdl_keyboard_press_latches(
            keyboard_snapshot_, pending_keyboard_presses_
        );
        pending_keyboard_presses_.fill(0U);
    }

    void normalize_input() override {
        openswd3::input_time_rng::begin_input_normalization(
            input_state_,
            frame_preparation_state_.frame_clock.sampled_milliseconds
        );
        auto sample = openswd3::platform_sdl3::sample_sdl_mouse_state(
            renderer_, mouse_device_state_
        );
        openswd3::platform_sdl3::merge_sdl_mouse_press_latches(
            sample, pending_mouse_button_mask_
        );
        pending_mouse_button_mask_ = 0U;
        openswd3::input_time_rng::normalize_input_frame(
            input_state_, mouse_state_, keyboard_snapshot_, sample
        );
    }

    void release_display_and_world_for_battle_entry() override {}
    void close_world_map_view() override {}
    void initialize_battle(const openswd3::compat::u16 battle_id) override {
        const auto loaded = openswd3::battle::load_legacy_battle_assets(
            data_directory_, battle_id, 0, battle_assets_
        );
        battle_assets_ready_ =
            loaded.status == openswd3::battle::LegacyBattleAssetStatus::ready;

        if (battle_assets_ready_) {
            std::array<openswd3::compat::u8, 4U> party_source_flags{};
            for (std::size_t index = 0U; index < party_source_flags.size();
                 ++index) {
                party_source_flags[index] =
                    openswd3::world_map::query_legacy_world_story_flag(
                        world_story_vm_state_,
                        static_cast<openswd3::compat::u16>(30U + index)
                    )
                    ? 1U
                    : 0U;
            }
            const auto prepared = openswd3::battle::prepare_legacy_battle_setup(
                battle_assets_, party_source_flags, false, battle_setup_
            );
            battle_setup_ready_ = prepared.status ==
                openswd3::battle::LegacyBattleSetupStatus::ready;
        } else {
            battle_setup_ = {};
            battle_setup_ready_ = false;
        }

        std::string message{"battle initialization assets: id="};
        message.append(std::to_string(battle_id));
        message.append(", status=");
        message.append(
            openswd3::battle::legacy_battle_asset_status_message(loaded.status)
        );
        if (battle_assets_ready_) {
            message.append(", script_bytes=");
            message.append(std::to_string(battle_assets_.figtalk_actual_size));
            message.append(", record_index=");
            message.append(std::to_string(battle_assets_.record_index));
            message.append(", enemy_count=");
            message.append(std::to_string(battle_assets_.enemy_count()));
            message.append(", party_count=");
            message.append(std::to_string(battle_setup_.party_count));
            message.append(", setup=");
            message.append(battle_setup_ready_ ? "ready" : "failed");
            if (battle_setup_ready_) {
                openswd3::diagnostics::log_info(message);
            } else {
                openswd3::diagnostics::log_error(message);
            }
        } else {
            openswd3::diagnostics::log_error(message);
        }
    }
    void clear_party_battle_entry_bits() override {}
    openswd3::compat::i32 step_battle() override {
        request_presentation(
            openswd3::rendering::LegacyPresentationSite::steady_battle
        );
        return 1;
    }
    void rebuild_display_after_result_zero() override {}
    void set_result_zero_world_state() override {}
    void reopen_world_map_after_result_zero() override {}
    void resume_audio_after_result_zero() override {}
    void prepare_result_two_internal_state() override {}
    void clear_result_two_auxiliary_state() override {}
    void finish_result_two_mode_transition() override {}
    void clear_result_three_internal_state() override {}
    void remap_world_after_result_three() override {}

    void step_high_priority(openswd3::app::FrameCoordinatorState&) override {
        request_presentation(
            openswd3::rendering::LegacyPresentationSite::steady_high_priority
        );
    }
    void
    update_background_music(openswd3::app::FrameCoordinatorState&) override {}
    void
    step_world_interaction(openswd3::app::FrameCoordinatorState&) override {
        if (!active_world_session_.has_value()) {
            return;
        }

        auto& world = *active_world_session_;
        auto& map = world.render.map_load.session;
        auto& roles = map.business.state.roles;
        std::vector<openswd3::world_map::LegacyWorldInteractionHotspot>
            hotspots;
        for (const auto& message : world_dialogs_.messages) {
            for (const auto& choice : message.choices) {
                hotspots.push_back({
                    .left = choice.left,
                    .top = choice.top,
                    .right = choice.right,
                    .bottom = choice.bottom,
                });
            }
        }

        // sub_40A6B0 resets the cursor to 13 immediately before invoking
        // sub_427300; the interaction routine only replaces that frame's
        // value when a more specific target is active.
        world_interaction_state_.cursor_variant =
            openswd3::world_map::kLegacyWorldDefaultCursorVariant;
        WorldInteractionPorts ports{
            input_state_, world_story_vm_state_, action_updater_, tsw_runtime_
        };
        const auto result =
            openswd3::world_map::coordinate_legacy_world_interaction(
                {
                    .player_index = world.selected_role_index,
                    .mouse_x = std::bit_cast<openswd3::compat::u32>(
                        input_state_.current_mouse.logical_x
                    ),
                    .mouse_y = std::bit_cast<openswd3::compat::u32>(
                        input_state_.current_mouse.logical_y
                    ),
                    .map_width = map.header.width,
                    .camera = world.camera,
                    .live_camera = &world.camera,
                    .choice_hotspots = hotspots,
                    .dialog_chain_active = !world_dialogs_.messages.empty(),
                },
                roles,
                map.business.state.events,
                map.surface_grid.surface_grid,
                input_state_.records,
                world_frame_state_.map_role_paths.talk_context,
                world_interaction_state_,
                ports,
                &world_dialogs_.close.flagged_dialog_counter
            );

        if (result.choice_chain_clear_requested) {
            openswd3::story_scene::clear_legacy_dialog_choice_chain(
                world_dialogs_
            );
        }
        world_frame_effects_.cursor.cursor_action.base_variant =
            world_interaction_state_.cursor_variant;

        if (result.status !=
            openswd3::world_map::LegacyWorldInteractionStatus::completed) {
            std::string message{"world interaction failed: status="};
            message.append(
                std::to_string(static_cast<unsigned>(result.status))
            );
            static_cast<void>(report_error(message));
            ok_ = false;
            running_ = false;
        }
    }
    void step_world_player(
        openswd3::app::FrameCoordinatorState& frame_state
    ) override {
        if (!active_world_session_.has_value()) {
            return;
        }

        auto& world = *active_world_session_;
        auto& map = world.render.map_load.session;
        auto& roles = map.business.state.roles;
        world_debug_hotkey_state_.diagnostic_text_visible =
            world_frame_state_.debug_overlay.diagnostic_text_visible;
        world_debug_hotkey_state_.collision_grid_visible =
            world_frame_state_.debug_overlay.collision_grid_visible;
        world_debug_hotkey_state_.tile_animation_interval =
            world.map_descriptor_runtime.tile_animation_interval;
        world_debug_hotkey_state_.world_frame_count =
            world_frame_state_.movement.world_frame_count;
        WorldDebugHotkeyPorts debug_ports{*this, frame_state};
        const auto debug =
            openswd3::world_map::coordinate_legacy_world_debug_hotkeys(
                {
                    .developer_tools_enabled =
                        world_frame_state_.developer_tools_enabled,
                    .talk_source_guid = world_frame_state_.map_role_paths
                                            .talk_context.source_guid,
                    .dialog_messages_active = !world_dialogs_.messages.empty(),
                },
                keyboard_snapshot_,
                world_debug_hotkey_state_,
                debug_ports
            );
        world_frame_state_.debug_overlay.diagnostic_text_visible =
            world_debug_hotkey_state_.diagnostic_text_visible;
        world_frame_state_.debug_overlay.collision_grid_visible =
            world_debug_hotkey_state_.collision_grid_visible;
        world.map_descriptor_runtime.tile_animation_interval =
            world_debug_hotkey_state_.tile_animation_interval;
        world_frame_state_.movement.world_frame_count =
            world_debug_hotkey_state_.world_frame_count;
        if (debug.status !=
            openswd3::world_map::LegacyWorldDebugHotkeyStatus::completed) {
            static_cast<void>(
                report_error("world debug hotkey snapshot is unavailable")
            );
            ok_ = false;
            running_ = false;
            return;
        }
        if (debug.outcome ==
            openswd3::world_map::LegacyWorldDebugHotkeyOutcome::
                return_from_player_control) {
            return;
        }

        const auto& movement = world_frame_state_.movement;
        const auto speed_binding = openswd3::input_time_rng::key_binding(
            input_state_.key_bindings,
            openswd3::input_time_rng::LegacyKeyBinding::configurable_16
        );
        const auto control =
            openswd3::world_map::prepare_legacy_world_player_control(
                {
                    .raw_speed_toggle_state =
                        openswd3::input_time_rng::read_raw_key(
                            keyboard_snapshot_, speed_binding
                        ),
                    .camera_x_transition = std::bit_cast<openswd3::compat::u32>(
                        movement.camera_x_transition
                    ),
                    .player_x_transition = std::bit_cast<openswd3::compat::u32>(
                        movement.player_x_transition
                    ),
                    .camera_y_transition = std::bit_cast<openswd3::compat::u32>(
                        movement.camera_y_transition
                    ),
                    .player_y_transition = std::bit_cast<openswd3::compat::u32>(
                        movement.player_y_transition
                    ),
                    .input_suppression =
                        input_state_.left_button_suppression_count,
                    .special_mode_state = frame_state.battle.special_mode_state,
                },
                input_state_.records,
                world_player_control_state_
            );
        if (control.status ==
            openswd3::world_map::LegacyWorldPlayerControlStatus::
                invalid_speed_mode) {
            static_cast<void>(
                report_error("world player control: invalid speed mode")
            );
            ok_ = false;
            running_ = false;
            return;
        }
        if (control.delay_milliseconds != 0U) {
            SDL_Delay(control.delay_milliseconds);
        }
        openswd3::world_map::prepare_legacy_world_player_motion_frame(
            world_frame_state_.movement
        );
        if (control.status !=
            openswd3::world_map::LegacyWorldPlayerControlStatus::completed) {
            std::string message{"world player control failed: status="};
            message.append(
                std::to_string(static_cast<unsigned>(control.status))
            );
            static_cast<void>(report_error(message));
            ok_ = false;
            running_ = false;
            return;
        }
        if (!control.control_allowed) {
            return;
        }
        if (world.selected_role_index >= roles.size()) {
            static_cast<void>(report_error(
                "world player input: selected role index is out of range"
            ));
            ok_ = false;
            running_ = false;
            return;
        }
        auto& player = roles[world.selected_role_index];

        WorldPlayerPorts ports{
            roles,
            map.surface_grid.surface_grid,
            map.header.width,
            input_state_,
            world_story_vm_state_,
            action_updater_,
        };
        openswd3::compat::u32 choice_count = 0U;
        for (const auto& message : world_dialogs_.messages) {
            choice_count +=
                static_cast<openswd3::compat::u32>(message.choices.size());
        }
        const bool choice_chain_active = choice_count != 0U;
        const auto arbitration =
            openswd3::world_map::arbitrate_legacy_world_control(
                control,
                {
                    .dialog_messages_active = !world_dialogs_.messages.empty(),
                    .choice_chain_active = choice_chain_active,
                    .choice_chain_flags = world_dialogs_.choice_chain_flags,
                },
                world_player_control_state_
            );
        switch (arbitration.action) {
        case openswd3::world_map::LegacyWorldControlArbitrationAction::
            continue_world_control:
            break;
        case openswd3::world_map::LegacyWorldControlArbitrationAction::
            return_from_player_control:
            return;
        case openswd3::world_map::LegacyWorldControlArbitrationAction::
            clear_choice_chain_and_return:
            openswd3::story_scene::clear_legacy_dialog_choice_chain(
                world_dialogs_
            );
            return;
        }

        if (control.primary_fresh_press) {
            constexpr std::array<openswd3::compat::i32, 8> kFacingDeltaX{
                0,
                0,
                -1,
                1,
                -1,
                1,
                -1,
                1,
            };
            constexpr std::array<openswd3::compat::i32, 8> kFacingDeltaY{
                -1,
                1,
                0,
                0,
                -1,
                1,
                1,
                -1,
            };
            if (player.action.variant_delta >= kFacingDeltaX.size()) {
                static_cast<void>(report_error(
                    "world player facing direction is out of range"
                ));
                ok_ = false;
                running_ = false;
                return;
            }
            const auto facing_talk =
                openswd3::world_map::coordinate_legacy_world_facing_talk(
                    {
                        .player_index = world.selected_role_index,
                        .delta_x = kFacingDeltaX[player.action.variant_delta],
                        .delta_y = kFacingDeltaY[player.action.variant_delta],
                        .map_width = map.header.width,
                        .map_height = map.header.height,
                    },
                    roles,
                    world_frame_state_.map_role_paths.talk_context,
                    world_player_control_state_.one_shot_interaction_state,
                    ports
                );
            if (facing_talk.status !=
                openswd3::world_map::LegacyWorldFacingTalkStatus::completed) {
                static_cast<void>(report_error("world facing Talk failed"));
                ok_ = false;
                running_ = false;
                return;
            }
            if (facing_talk.talk_created) {
                return;
            }
        }

        if (openswd3::world_map::should_request_legacy_world_menu(
                control, world_frame_state_.map_role_paths.talk_context
            )) {
            frame_state.battle.special_mode_state =
                openswd3::world_map::kLegacyWorldMenuRequest;
            return;
        }
        if (world_dialogs_.close.flagged_dialog_counter != 0U &&
            !choice_chain_active) {
            return;
        }

        auto input = openswd3::world_map::apply_legacy_world_direction_input(
            openswd3::world_map::LegacyWorldDirectionState{
                .direction = player.action.variant_delta,
                .auxiliary_selection_index =
                    world_interaction_state_.selected_choice_index,
            },
            input_state_.records,
            choice_chain_active,
            choice_count
        );
        if (input.status !=
            openswd3::world_map::LegacyWorldDirectionInputStatus::completed) {
            static_cast<void>(report_error(
                "world player input: normalized input records are unavailable"
            ));
            ok_ = false;
            running_ = false;
            return;
        }
        world_interaction_state_.selected_choice_index =
            input.state.auxiliary_selection_index;

        const openswd3::compat::i32 original_delta_x = input.delta_x;
        const openswd3::compat::i32 original_delta_y = input.delta_y;

        if ((input.delta_x != 0 || input.delta_y != 0) &&
            !openswd3::world_map::query_legacy_world_story_flag(
                world_story_vm_state_,
                static_cast<openswd3::compat::u16>(
                    openswd3::world_map::kLegacyWorldDebugCollisionFlag
                )
            )) {
            const auto adjusted = openswd3::world_map::
                adjust_legacy_world_direction_for_obstacles(
                    player,
                    input.delta_x,
                    input.delta_y,
                    map.header.width,
                    map.header.height,
                    map.surface_grid.surface_grid
                );
            if (adjusted.status !=
                openswd3::world_map::LegacyWorldDirectionProbeStatus::
                    completed) {
                std::string message{
                    "world player input: obstacle adjustment failed: status="
                };
                message.append(
                    std::to_string(static_cast<unsigned>(adjusted.status))
                );
                static_cast<void>(report_error(message));
                ok_ = false;
                running_ = false;
                return;
            }
            input.delta_x = adjusted.delta_x;
            input.delta_y = adjusted.delta_y;
        }

        const auto collision_talk =
            openswd3::world_map::coordinate_legacy_world_collision_talk(
                {
                    .player_index = world.selected_role_index,
                    .adjusted_delta_x = input.delta_x,
                    .adjusted_delta_y = input.delta_y,
                    .original_delta_x = original_delta_x,
                    .original_delta_y = original_delta_y,
                },
                roles,
                map.business.state.events,
                world_frame_state_.map_role_paths.talk_context,
                world_player_control_state_.one_shot_interaction_state,
                ports
            );
        if (collision_talk.status !=
            openswd3::world_map::LegacyWorldCollisionTalkStatus::completed) {
            std::string message{"world collision Talk failed: status="};
            message.append(
                std::to_string(static_cast<unsigned>(collision_talk.status))
            );
            static_cast<void>(report_error(message));
            ok_ = false;
            running_ = false;
            return;
        }
        input.delta_x = collision_talk.delta_x;
        input.delta_y = collision_talk.delta_y;

        const auto bounds =
            openswd3::world_map::compute_legacy_world_movement_bounds(
                player, world.camera, map.header.width, map.header.height
            );
        const openswd3::compat::u32 base_step =
            world.map_descriptor_runtime.base_movement_step;
        openswd3::world_map::apply_legacy_world_player_motion_pre_encounter(
            player,
            input,
            bounds,
            world_frame_state_.movement,
            openswd3::world_map::LegacyWorldMovementOptions{
                .base_movement_step = base_step,
                .speed_override = world_player_control_state_.speed_mode != 0U,
                .fixed_debug_speed =
                    world_debug_hotkey_state_.fixed_debug_speed != 0U,
            }
        );

        world_encounter_state_.battle_active = frame_state.battle.battle_active;
        world_encounter_state_.temporary_battle_request =
            frame_state.battle.battle_request_value;
        world_encounter_state_.movement_state_4b7920 =
            std::bit_cast<openswd3::compat::u32>(
                world_frame_state_.movement.camera_y_transition
            );
        world_encounter_state_.movement_state_4b7518 =
            std::bit_cast<openswd3::compat::u32>(
                world_frame_state_.movement.camera_x_transition
            );
        world_encounter_state_.movement_state_4a948c =
            std::bit_cast<openswd3::compat::u32>(
                world_frame_state_.movement.player_y_transition
            );
        world_encounter_state_.movement_state_4a9488 =
            std::bit_cast<openswd3::compat::u32>(
                world_frame_state_.movement.player_x_transition
            );
        WorldEncounterPorts encounter_ports{*this, world, player};
        const auto encounter =
            openswd3::world_map::coordinate_legacy_world_encounter(
                world_encounter_state_,
                world_frame_state_.map_role_paths.talk_context,
                roles,
                encounter_ports
            );
        frame_state.battle.battle_active = world_encounter_state_.battle_active;
        frame_state.battle.battle_request_value =
            world_encounter_state_.temporary_battle_request;
        world_frame_state_.movement.camera_y_transition =
            std::bit_cast<openswd3::compat::i32>(
                world_encounter_state_.movement_state_4b7920
            );
        world_frame_state_.movement.camera_x_transition =
            std::bit_cast<openswd3::compat::i32>(
                world_encounter_state_.movement_state_4b7518
            );
        world_frame_state_.movement.player_y_transition =
            std::bit_cast<openswd3::compat::i32>(
                world_encounter_state_.movement_state_4a948c
            );
        world_frame_state_.movement.player_x_transition =
            std::bit_cast<openswd3::compat::i32>(
                world_encounter_state_.movement_state_4a9488
            );
        openswd3::world_map::finish_legacy_world_player_motion_frame(
            player, world_frame_state_.movement
        );
        if (encounter.outcome ==
            openswd3::world_map::LegacyWorldEncounterOutcome::
                selection_failed) {
            std::string message{
                "world random encounter selection failed: status="
            };
            message.append(
                std::to_string(
                    static_cast<unsigned>(encounter.selection_status)
                )
            );
            static_cast<void>(report_error(message));
            ok_ = false;
            running_ = false;
        }
    }

    void begin_story_world_session_reload() noexcept {
        world_frame_state_.movement.camera_y_transition = 0;
        input_state_.records = {};
        world_frame_state_.movement.camera_x_transition = 0;
        world_frame_state_.movement.player_y_transition = 0;
        world_frame_state_.movement.player_x_transition = 0;
        window_state_.process_flags |= openswd3::app::kProcessIdleSuppression;
    }

    [[nodiscard]] bool reload_story_world_session(
        const openswd3::world_map::LegacyWorldLoadRequest& request,
        std::span<openswd3::world_map::LegacyWorldRoleRecord>& roles,
        openswd3::compat::u32& controlled_role_index,
        openswd3::world_map::LegacyWorldStoryVmRuntime& runtime
    ) {
        struct ProcessFlagReset final {
            openswd3::compat::u32& flags;
            ~ProcessFlagReset() {
                flags &= ~openswd3::app::kProcessIdleSuppression;
            }
        } process_flag_reset{window_state_.process_flags};

        auto* source_world = pending_story_world_session_.has_value()
            ? &*pending_story_world_session_
            : active_world_session_.has_value() ? &*active_world_session_
                                                : nullptr;
        if (source_world == nullptr) {
            static_cast<void>(report_error(
                "story world reload: source session is unavailable"
            ));
            return false;
        }
        auto& source_map = source_world->render.map_load.session;
        auto& source_roles = source_map.business.state.roles;

        openswd3::asset_runtime::LegacyActionDrawRuntimePorts action_ports{
            action_updater_,
            tsw_runtime_,
            game_framebuffer_,
            world_raster_,
            world_effects_,
            world_jitter_,
        };
        const auto progress_reset =
            openswd3::world_map::update_legacy_world_load_progress(
                world_load_progress_,
                world_story_vm_state_,
                game_framebuffer_,
                pixel_conversion_,
                -1,
                *this,
                action_ports,
                *this
            );
        if (progress_reset.status !=
            openswd3::world_map::LegacyWorldLoadProgressStatus::suppressed) {
            static_cast<void>(report_error(
                "story world reload: loading progress reset failed"
            ));
            return false;
        }

        std::array<
            openswd3::world_map::LegacyWorldObjectSlotPrefix,
            openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
            object_prefixes{};
        const auto read_slot_word = [](const auto& bytes,
                                       const std::size_t offset) {
            return static_cast<openswd3::compat::u16>(
                static_cast<openswd3::compat::u16>(bytes[offset]) |
                static_cast<openswd3::compat::u16>(
                    static_cast<openswd3::compat::u16>(bytes[offset + 1U]) << 8U
                )
            );
        };
        for (std::size_t index = 0U; index < object_prefixes.size(); ++index) {
            const auto& bytes =
                world_frame_state_.map_role_paths.active_object_slots[index]
                    .bytes;
            object_prefixes[index] = {
                .role_index = read_slot_word(bytes, 0U),
                .field_02 = read_slot_word(bytes, 2U),
                .world_x = read_slot_word(bytes, 4U),
                .world_y = read_slot_word(bytes, 6U),
            };
        }
        const openswd3::world_map::LegacyWorldRolePreloadContext
            preload_context{
                .path_database = resource_databases_.path_bytes(),
                .roles = source_roles,
                .object_slots = object_prefixes,
                .controlled_role_index = source_world->selected_role_index,
                .current_map_width = source_map.header.width,
                .current_map_height = source_map.header.height,
            };
        auto payload = resource_databases_.mutable_maps_payload_bytes();
        const auto role_preload =
            openswd3::world_map::preload_legacy_world_roles_before_load(
                payload, source_world->maps_database, request, preload_context
            );
        if (role_preload.status !=
            openswd3::world_map::LegacyWorldRolePreloadStatus::ready) {
            std::string message{
                "story world reload: role preload failed: status="
            };
            message.append(
                std::to_string(static_cast<unsigned>(role_preload.status))
            );
            static_cast<void>(report_error(message));
            return false;
        }

        const openswd3::world_map::LegacyWorldRolePostMaterializationContext
            post_context{
                .previous_logical_map_id = source_world->logical_map_id,
                .guid_one_action_override =
                    world_story_vm_state_.guid_one_action_override,
                .has_story_state_0x0192 = std::ranges::any_of(
                    world_item_lists_.player_inventory,
                    [](const auto& item) { return item.item_id == 0x0192U; }
                ),
                .active_object_slots =
                    world_frame_state_.map_role_paths.active_object_slots,
                .spatial_index = nullptr,
                .surface_grid = {},
                .map_width = 0U,
            };

        // From this point the source world is irreversibly torn down, as in
        // sub_40C130. A checked modern failure must stop the frame before any
        // stale outer references can observe that destroyed session.
        story_world_session_reload_fatal_ = true;
        tsw_runtime_.set_special_loader(nullptr);
        tsw_runtime_.clear_cache();
        world_special_frame_loader_.reset();
        const auto role_clear =
            openswd3::world_map::clear_legacy_world_role_table(
                source_roles, world_path_script_state_.role_label_payloads
            );
        if (role_clear.status !=
            openswd3::world_map::LegacyWorldRoleTableResetStatus::ready) {
            std::string message{
                "story world reload: previous role owner clear failed: status="
            };
            message.append(
                std::to_string(static_cast<unsigned>(role_clear.status))
            );
            static_cast<void>(report_error(message));
            return false;
        }
        world_audio_distances_.clear();
        world_audio_vertical_offsets_.clear();
        openswd3::world_map::reset_legacy_world_transient_state(
            openswd3::world_map::LegacyWorldTransientResetOwners{
                .packed_row_effects = world_frame_effects_.packed_rows,
                .moving_actions = world_moving_actions_,
                .role_head_actions = world_role_head_actions_,
                .dialogs = world_dialogs_,
                .picture_actions = world_picture_actions_,
                .role_particles = world_role_particle_effect_,
                .ani_drift = world_frame_effects_.drift,
                .frame_color = world_frame_effects_.frame_color,
                .selection_words = world_selection_words_,
                .row_copy = world_frame_effects_.row_copy,
            }
        );
        world_frame_effects_.streak.reset();
        world_frame_effects_.spark.reset_counters();
        world_frame_effects_.directional.reset_motion_block();
        world_frame_effects_.deformation.clear();
        world_frame_effects_.follower = {};
        world_frame_effects_.timed_messages.clear();
        world_interaction_state_ = {};
        world_player_control_state_ = {};
        world_encounter_state_ = {};

        auto effective_load = request;
        effective_load.load_flags = 0U;
        auto loaded = openswd3::world_map::load_legacy_world_runtime_session(
            payload,
            {
                .archive_path = data_directory_ / "huge.lmf",
                .cache_directory = world_cache_directory_,
                .load = effective_load,
                .cache_limit_megabytes = 60U,
                .pixel_conversion = pixel_conversion_,
                .post_materialization_context = &post_context,
                .random = &world_runtime_random_,
            },
            world_action_initializer_,
            [&](const openswd3::compat::i32 progress, const auto&) {
                static_cast<void>(
                    openswd3::world_map::update_legacy_world_load_progress(
                        world_load_progress_,
                        world_story_vm_state_,
                        game_framebuffer_,
                        pixel_conversion_,
                        progress,
                        *this,
                        action_ports,
                        *this
                    )
                );
            },
            [&]() { maintain_audio(); }
        );
        if (loaded.status !=
            openswd3::world_map::LegacyWorldRuntimeSessionStatus::ready) {
            std::string message{
                "story world reload: session load failed: status="
            };
            message.append(
                std::to_string(static_cast<unsigned>(loaded.status))
            );
            message.append(", maps_status=");
            message.append(
                std::to_string(
                    static_cast<unsigned>(loaded.maps_database_status)
                )
            );
            message.append(", render_status=");
            message.append(
                std::to_string(static_cast<unsigned>(loaded.render_status))
            );
            static_cast<void>(report_error(message));
            return false;
        }
        loaded.session.role_preload = role_preload;
        loaded.session.role_preload_applied = true;

        const auto progress_complete =
            openswd3::world_map::update_legacy_world_load_progress(
                world_load_progress_,
                world_story_vm_state_,
                game_framebuffer_,
                pixel_conversion_,
                100,
                *this,
                action_ports,
                *this
            );
        if (progress_complete.status !=
                openswd3::world_map::LegacyWorldLoadProgressStatus::
                    suppressed ||
            !progress_complete.suppression_flag_cleared) {
            static_cast<void>(report_error(
                "story world reload: loading progress terminal gate failed"
            ));
            return false;
        }

        pending_story_world_session_.emplace(std::move(loaded.session));
        auto& world = *pending_story_world_session_;
        auto& map = world.render.map_load.session;
        auto& reloaded_roles = map.business.state.roles;
        world_special_frame_loader_.emplace(
            data_directory_ / "huge.lmf",
            map.lookup.map_offset,
            map.referenced_records.records,
            pixel_conversion_
        );
        tsw_runtime_.set_special_loader(&*world_special_frame_loader_);
        const auto role_count = reloaded_roles.size();
        world_audio_distances_.assign(role_count, 0);
        world_audio_vertical_offsets_.assign(role_count, 0);
        constexpr std::array<std::size_t, 7U> map_service_ids{
            5U,
            6U,
            7U,
            8U,
            15U,
            19U,
            22U,
        };
        for (const std::size_t service_id : map_service_ids) {
            const auto story_flag =
                static_cast<openswd3::compat::u16>(service_id);
            if ((world.map_descriptor_runtime.enabled_service_bits &
                 (1U << service_id)) != 0U) {
                openswd3::world_map::set_legacy_world_story_flag(
                    world_story_vm_state_, story_flag
                );
            } else {
                openswd3::world_map::clear_legacy_world_story_flag(
                    world_story_vm_state_, story_flag
                );
            }
        }
        world_frame_effects_.directional_configuration = {
            .map_width_tiles =
                static_cast<openswd3::compat::i32>(map.header.width),
            .map_height_tiles =
                static_cast<openswd3::compat::i32>(map.header.height),
            .base_variant = static_cast<openswd3::compat::u16>(
                world.map_descriptor_runtime.directional_base_variant
            ),
            .variant_count = static_cast<openswd3::compat::u16>(
                world.map_descriptor_runtime.directional_variant_count
            ),
            .spawn_direction = world.map_descriptor_runtime.behavior_index,
        };
        auto& directional_state = world_frame_effects_.directional.state();
        for (std::size_t index = 0U; index < world.directional_points.size();
             ++index) {
            const auto& source = world.directional_points[index];
            auto& motion = directional_state.motion[index];
            auto& color = directional_state.color[index];
            auto& timing = directional_state.timing[index];
            motion.world_x = static_cast<openswd3::compat::i32>(source.world_x);
            motion.world_y = static_cast<openswd3::compat::i32>(source.world_y);
            motion.velocity_x = source.velocity_x;
            motion.velocity_y = source.velocity_y;
            color.target_offset = 0;
            timing.target_interval =
                static_cast<openswd3::compat::i32>(source.target_interval);
            timing.variant = static_cast<openswd3::compat::i32>(source.variant);
        }
        world_frame_state_.map_id = world.logical_map_id;
        world_frame_state_.frame_runtime.flash_red_offset =
            world.map_descriptor_runtime.role_red_offset;
        world_frame_state_.frame_runtime.flash_green_offset =
            world.map_descriptor_runtime.role_green_offset;
        world_frame_state_.frame_runtime.flash_blue_offset =
            world.map_descriptor_runtime.role_blue_offset;
        world_frame_state_.player_role_index = world.selected_role_index;
        world_frame_state_.party_role_count =
            world.role_post_materialization.party_role_count;
        world_frame_state_.party_object_slots =
            world.role_post_materialization.party_object_slots;
        world_frame_state_.selection_scroll.saved_left = world.camera.left;
        world_frame_state_.selection_scroll.saved_top = world.camera.top;
        world_frame_state_.tile_animation = {
            .cycle_counter = 1,
            .cycle_interval = static_cast<openswd3::compat::i32>(
                world.map_descriptor_runtime.tile_animation_interval
            ),
            .frame_count = map.header.layers,
            .frame_index = 0U,
            .frame_direction = 1,
            .tile_layer_stride = map.header.width * map.header.height,
            .tile_layer_offset = 0U,
        };
        world_frame_state_.frame_runtime.spatial_audio = {
            .controlled_role_index = world.selected_role_index,
            .mix_level = kLegacyInitialSampleMixLevel,
            .distance_by_role = world_audio_distances_,
            .vertical_offset_by_role = world_audio_vertical_offsets_,
        };
        openswd3::world_map::initialize_legacy_world_player_position_history(
            world_frame_state_.player_post_frame,
            reloaded_roles[world.selected_role_index]
        );
        deferred_world_stage_notice_logged_ = false;
        unsupported_world_path_opcode_notice_logged_ = false;
        unsupported_world_story_opcode_notice_logged_ = false;

        roles = std::span<openswd3::world_map::LegacyWorldRoleRecord>{
            reloaded_roles
        };
        controlled_role_index = world.selected_role_index;
        runtime.spatial_index = &map.business.state.spatial_index;
        runtime.role_surface = {
            .map_width = map.header.width,
            .selected_guid = reloaded_roles[world.selected_role_index].guid,
            .surface_grid = map.surface_grid.surface_grid,
        };
        runtime.camera = &world.camera;
        runtime.map_height = map.header.height;
        if (runtime.story_paths != nullptr) {
            runtime.story_paths->roles = roles;
            runtime.story_paths->spatial_index = runtime.spatial_index;
            runtime.story_paths->role_surface = runtime.role_surface;
            runtime.story_paths->camera = runtime.camera;
            runtime.story_paths->selected_role_index = controlled_role_index;
            runtime.story_paths->map_height = runtime.map_height;
        }

        story_world_session_reload_fatal_ = false;
        std::string message{"story world reloaded: logical_map="};
        message.append(std::to_string(world.logical_map_id));
        message.append(", roles=");
        message.append(std::to_string(role_count));
        message.append(", player_index=");
        message.append(std::to_string(world.selected_role_index));
        openswd3::diagnostics::log_info(message);
        return true;
    }

    void step_story(openswd3::app::FrameCoordinatorState&) override {
        if (!active_world_session_.has_value()) {
            return;
        }
        story_world_session_reload_fatal_ = false;
        pending_story_world_session_.reset();

        class PartyPathPorts final
            : public openswd3::world_map::LegacyWorldPartyPathPorts {
        public:
            explicit PartyPathPorts(SdlDeferredWorldFramePorts& ports) noexcept
                : ports_(ports) {}

            [[nodiscard]] bool query_collision_disabled() noexcept override {
                return ports_.query_service(0x4FU);
            }

        private:
            SdlDeferredWorldFramePorts& ports_;
        };

        class StoryVmPorts final
            : public openswd3::world_map::LegacyWorldStoryVmPorts {
        public:
            StoryVmPorts(
                SdlSmokeIdlePorts& owner,
                openswd3::resource_io::LegacyResourceDatabases& databases,
                openswd3::asset_runtime::LegacyActionUpdater& action_updater,
                openswd3::audio_video::LegacySampleManager& sample_manager,
                openswd3::audio_video::LegacyAudioMaintenancePorts&
                    audio_maintenance,
                const openswd3::compat::i32 sample_mix_level,
                openswd3::rendering::LegacyFramebuffer& framebuffer,
                openswd3::rendering::LegacyPresentationPorts& presentation,
                openswd3::audio_video::LegacyVideoPlayer& video_player,
                openswd3::asset_runtime::LegacyAniActivity& ani_activity,
                const openswd3::rendering::LegacyPixelConversionState&
                    pixel_conversion,
                const openswd3::compat::u8& scene_render_flags,
                std::vector<openswd3::compat::u16>& ani_scene_backup,
                const std::filesystem::path& data_directory,
                openswd3::compat::u32& process_flags,
                openswd3::compat::u32& frame_interval
            ) noexcept
                : owner_(owner), databases_(databases),
                  action_updater_(action_updater),
                  sample_manager_(sample_manager),
                  audio_maintenance_(audio_maintenance),
                  sample_mix_level_(sample_mix_level),
                  framebuffer_(framebuffer), presentation_(presentation),
                  video_player_(video_player), ani_activity_(ani_activity),
                  pixel_conversion_(pixel_conversion),
                  scene_render_flags_(scene_render_flags),
                  ani_scene_backup_(ani_scene_backup),
                  data_directory_(data_directory),
                  process_flags_(process_flags),
                  frame_interval_(frame_interval) {}

            openswd3::resource_io::LegacyTalkWindowLoadResult load_story_window(
                const openswd3::compat::i32 story_id,
                const std::span<
                    openswd3::compat::u8,
                    openswd3::resource_io::kLegacyTalkWindowSize> destination,
                const bool clear_before_read
            ) override {
                return databases_.load_talk_story_window(
                    story_id, destination, clear_before_read
                );
            }

            openswd3::resource_io::LegacyTalkWindowLoadResult load_data_window(
                const openswd3::compat::u32 file_number,
                const openswd3::compat::u32 data_offset,
                const std::span<
                    openswd3::compat::u8,
                    openswd3::resource_io::kLegacyTalkWindowSize> destination,
                const bool clear_before_read
            ) override {
                return databases_.load_talk_data_window(
                    file_number, data_offset, destination, clear_before_read
                );
            }

            openswd3::compat::u32 update_action(
                openswd3::asset_runtime::LegacyActionRecord& action
            ) override {
                return action_updater_.update(action).return_value;
            }

            void release_role_path_payload(
                const openswd3::compat::u32 role_index
            ) noexcept override {
                if (role_index >= owner_.world_path_script_state_
                                      .role_label_payloads.size()) {
                    return;
                }
                std::vector<openswd3::compat::u8>{}.swap(
                    owner_.world_path_script_state_
                        .role_label_payloads[role_index]
                );
            }

            void begin_world_session_reload() noexcept override {
                owner_.begin_story_world_session_reload();
            }

            bool reload_world_session(
                const openswd3::world_map::LegacyWorldLoadRequest& request,
                std::span<openswd3::world_map::LegacyWorldRoleRecord>& roles,
                openswd3::compat::u32& controlled_role_index,
                openswd3::world_map::LegacyWorldStoryVmRuntime& runtime
            ) override {
                return owner_.reload_story_world_session(
                    request, roles, controlled_role_index, runtime
                );
            }

            void patch_role_source(
                const openswd3::world_map::LegacyMapsRolePatchRequest& request
            ) noexcept override {
                auto* world = owner_.pending_story_world_session_.has_value()
                    ? &*owner_.pending_story_world_session_
                    : owner_.active_world_session_.has_value()
                    ? &*owner_.active_world_session_
                    : nullptr;
                if (world == nullptr) {
                    return;
                }
                static_cast<void>(
                    openswd3::world_map::patch_legacy_maps_role_source_record(
                        databases_.mutable_maps_payload_bytes(),
                        world->maps_database,
                        request
                    )
                );
            }

            void play_sound_effect(
                const openswd3::compat::u16 sound_id
            ) noexcept override {
                static_cast<void>(openswd3::audio_video::play_legacy_sample(
                    sample_manager_, sound_id, sample_mix_level_
                ));
            }

            void clear_story_framebuffer() noexcept override {
                std::ranges::fill(
                    framebuffer_.physical_pixels(), openswd3::compat::u16{0U}
                );
            }

            void present_story_framebuffer() noexcept override {
                static_cast<void>(
                    openswd3::rendering::submit_legacy_presentation(
                        openswd3::rendering::LegacyPresentationSite::
                            story_video_preclear,
                        presentation_
                    )
                );
            }

            [[nodiscard]] bool prepare_story_video() noexcept override {
                // The original CD-checker can request process shutdown before
                // consuming the filename. Configured data roots need no CD
                // acquisition, so the SDL adaptation is always ready.
                return true;
            }

            void begin_story_video(
                const std::span<const openswd3::compat::u8> filename
            ) override {
                const std::string scripted_filename{
                    reinterpret_cast<const char*>(filename.data()),
                    filename.size(),
                };
                const auto path =
                    openswd3::audio_video::build_legacy_video_path(
                        data_directory_, scripted_filename
                    );
                const auto status = video_player_.begin(
                    path.string(),
                    openswd3::audio_video::kLegacyVideoMaximumVolume
                );
                if (status ==
                    openswd3::audio_video::LegacyVideoBeginStatus::playing) {
                    process_flags_ |= openswd3::app::kProcessVideoActive;
                }
            }

            openswd3::compat::i32 query_story_video_progress() override {
                return video_player_.legacy_progress();
            }

            void set_story_frame_interval(
                const openswd3::compat::u32 milliseconds
            ) noexcept override {
                frame_interval_ = milliseconds;
            }

            [[nodiscard]] bool prepare_story_ani() noexcept override {
                // Configured data roots replace the original CD acquisition.
                return true;
            }

            [[nodiscard]]
            openswd3::asset_runtime::LegacyAniActivityStartResult
            begin_story_ani(
                const std::span<const openswd3::compat::u8> filename,
                const openswd3::compat::u8 flags
            ) override {
                const std::string scripted_filename{
                    reinterpret_cast<const char*>(filename.data()),
                    filename.size(),
                };
                const std::filesystem::path video_directory =
                    data_directory_ / "Video";
                std::filesystem::path archive_path =
                    video_directory / scripted_filename;
                std::error_code error;
                if (!std::filesystem::exists(archive_path, error)) {
                    error.clear();
                    for (std::filesystem::directory_iterator
                             iterator{video_directory, error},
                         end;
                         !error && iterator != end;
                         iterator.increment(error)) {
                        const std::string candidate =
                            iterator->path().filename().string();
                        if (candidate.size() != scripted_filename.size()) {
                            continue;
                        }
                        bool equal = true;
                        for (std::size_t index = 0U; index < candidate.size();
                             ++index) {
                            const auto left =
                                static_cast<unsigned char>(candidate[index]);
                            const auto right = static_cast<unsigned char>(
                                scripted_filename[index]
                            );
                            if (std::tolower(left) != std::tolower(right)) {
                                equal = false;
                                break;
                            }
                        }
                        if (equal) {
                            archive_path = iterator->path();
                            break;
                        }
                    }
                }
                error.clear();
                if (!std::filesystem::exists(archive_path, error)) {
                    return {};
                }
                ani_scene_backup_.assign(
                    framebuffer_.physical_pixels().begin(),
                    framebuffer_.physical_pixels().end()
                );
                auto result = ani_activity_.start(
                    archive_path,
                    flags,
                    process_flags_,
                    static_cast<openswd3::compat::u32>(scene_render_flags_),
                    pixel_conversion_
                );
                process_flags_ = ani_activity_.state().process_flags;
                if (result.status ==
                        openswd3::asset_runtime::LegacyAniActivityStartStatus::
                            ready &&
                    (flags &
                     openswd3::asset_runtime::kLegacyAniSkipRevealFlag) != 0U) {
                    std::ranges::fill(
                        framebuffer_.physical_pixels(),
                        openswd3::compat::u16{0U}
                    );
                }
                return result;
            }

            [[nodiscard]] bool is_story_ani_active() const noexcept override {
                return ani_activity_.is_active();
            }

            [[nodiscard]] openswd3::compat::i32
            query_story_ani_phase() const noexcept override {
                return ani_activity_.state().phase;
            }

            void beep() noexcept override {}

            void service_audio() override {
                ::service_audio(audio_maintenance_);
            }

            bool prepare_dialog_text(
                const std::span<const openswd3::compat::u8>,
                std::vector<openswd3::compat::u8>&
            ) override {
                return false;
            }

        private:
            SdlSmokeIdlePorts& owner_;
            openswd3::resource_io::LegacyResourceDatabases& databases_;
            openswd3::asset_runtime::LegacyActionUpdater& action_updater_;
            openswd3::audio_video::LegacySampleManager& sample_manager_;
            openswd3::audio_video::LegacyAudioMaintenancePorts&
                audio_maintenance_;
            openswd3::compat::i32 sample_mix_level_{};
            openswd3::rendering::LegacyFramebuffer& framebuffer_;
            openswd3::rendering::LegacyPresentationPorts& presentation_;
            openswd3::audio_video::LegacyVideoPlayer& video_player_;
            openswd3::asset_runtime::LegacyAniActivity& ani_activity_;
            const openswd3::rendering::LegacyPixelConversionState&
                pixel_conversion_;
            const openswd3::compat::u8& scene_render_flags_;
            std::vector<openswd3::compat::u16>& ani_scene_backup_;
            const std::filesystem::path& data_directory_;
            openswd3::compat::u32& process_flags_;
            openswd3::compat::u32& frame_interval_;
        };

        class PathScriptPorts final
            : public openswd3::world_map::LegacyWorldPathScriptPorts {
        public:
            PathScriptPorts(
                openswd3::asset_runtime::LegacyActionUpdater& action_updater,
                openswd3::audio_video::LegacySampleManager& sample_manager,
                const openswd3::audio_video::LegacySpatialSampleState audio
            ) noexcept
                : action_updater_(action_updater),
                  sample_manager_(sample_manager), audio_(audio) {}

            openswd3::compat::u32 update_action(
                openswd3::asset_runtime::LegacyActionRecord& action
            ) override {
                return action_updater_.update(action).return_value;
            }

            void play_positional_sample(
                const openswd3::compat::u16 sound_id,
                const openswd3::compat::i32 world_x,
                const openswd3::compat::i32 world_y
            ) noexcept override {
                static_cast<void>(
                    openswd3::audio_video::play_legacy_spatial_sample(
                        sample_manager_, sound_id, world_x, world_y, audio_
                    )
                );
            }

        private:
            openswd3::asset_runtime::LegacyActionUpdater& action_updater_;
            openswd3::audio_video::LegacySampleManager& sample_manager_;
            openswd3::audio_video::LegacySpatialSampleState audio_;
        };

        openswd3::world_map::LegacyWorldStoryVmResult story_result;
        {
            auto& world = *active_world_session_;
            auto& map = world.render.map_load.session;
            auto& roles = map.business.state.roles;
            openswd3::world_map::LegacyWorldStoryPathRuntime story_paths{
                .roles = roles,
                .active_object_slots =
                    world_frame_state_.map_role_paths.active_object_slots,
                .spatial_index = &map.business.state.spatial_index,
                .role_surface =
                    {
                        .map_width = map.header.width,
                        .selected_guid = roles[world.selected_role_index].guid,
                        .surface_grid = map.surface_grid.surface_grid,
                    },
                .node_pool = &world_path_node_pool_,
                .movement = &world_frame_state_.movement,
                .camera = &world.camera,
                .selected_arrival_bytes =
                    world_frame_state_.map_role_paths.guid_one_arrival_bytes,
                .selected_role_index = world.selected_role_index,
                .map_height = map.header.height,
                .scene_render_flags =
                    &world_frame_state_.frame_runtime.frame.runtime_flags,
            };
            SdlDeferredWorldFramePorts deferred_ports{
                audio_maintenance_,
                *this,
                text_renderers_,
                &world_frame_state_.head_sign_actions,
                &story_paths,
                &world_path_script_state_,
                &world_story_vm_state_,
            };
            StoryVmPorts story_ports{
                *this,
                resource_databases_,
                action_updater_,
                sample_manager_,
                audio_maintenance_,
                world_frame_state_.frame_runtime.spatial_audio.mix_level,
                game_framebuffer_,
                *this,
                video_player_,
                world_ani_activity_,
                pixel_conversion_,
                world_frame_state_.frame_runtime.frame.runtime_flags,
                world_ani_scene_backup_,
                data_directory_,
                window_state_.process_flags,
                frame_interval_,
            };
            const openswd3::world_map::LegacyWorldStoryVmRuntime story_runtime{
                .spatial_index = &map.business.state.spatial_index,
                .role_surface =
                    {
                        .map_width = map.header.width,
                        .selected_guid = roles[world.selected_role_index].guid,
                        .surface_grid = map.surface_grid.surface_grid,
                    },
                .mutable_maps_payload =
                    resource_databases_.mutable_maps_payload_bytes(),
                .maps_database = &world.maps_database,
                .role_storage = &roles,
                .role_transfer_state = &world.role_post_materialization,
                .live_party_role_count = &world_frame_state_.party_role_count,
                .live_party_object_slots =
                    &world_frame_state_.party_object_slots,
                .role_particles = &world_role_particle_effect_,
                .current_logical_map_id =
                    static_cast<openswd3::compat::u16>(world.logical_map_id),
                .selection_words = &world_selection_words_,
                .selection_scroll = &world_frame_state_.selection_scroll,
                .camera = &world.camera,
                .camera_pan = &world_frame_state_.camera_pan,
                .movement = &world_frame_state_.movement,
                .picture_actions = &world_picture_actions_,
                .packed_row_effects = &world_frame_effects_.packed_rows,
                .moving_actions = &world_moving_actions_,
                .role_head_actions = &world_role_head_actions_,
                .battle_request_value =
                    &frame_coordinator_state_.battle.battle_request_value,
                .frame_color = &world_frame_effects_.frame_color,
                .story_paths = &story_paths,
                .indexed_target_selector =
                    &world_interaction_state_.selected_choice_index,
                .scene_render_flags =
                    &world_frame_state_.frame_runtime.frame.runtime_flags,
                .map_height = map.header.height,
                .current_tick =
                    frame_preparation_state_.frame_clock.sampled_milliseconds,
                .secondary_rng = &secondary_rng_,
            };
            openswd3::world_map::advance_legacy_world_script_clock(
                world_story_vm_state_
            );
            story_result = openswd3::world_map::step_legacy_world_story_vm(
                world_frame_state_.map_role_paths.talk_context,
                world_story_vm_state_,
                roles,
                world.selected_role_index,
                world_frame_state_.map_role_paths.active_object_slots,
                resource_databases_.maps_payload_bytes(),
                world_dialogs_,
                world_dialog_runtime_state_,
                initial_menu_state_.first_name,
                initial_menu_state_.second_name,
                story_runtime,
                story_ports
            );
        }
        if (story_world_session_reload_fatal_) {
            pending_story_world_session_.reset();
            active_world_session_.reset();
            ok_ = false;
            running_ = false;
            return;
        }
        if (pending_story_world_session_.has_value()) {
            active_world_session_.emplace(
                std::move(*pending_story_world_session_)
            );
            pending_story_world_session_.reset();
        }
        auto& world = *active_world_session_;
        auto& map = world.render.map_load.session;
        auto& roles = map.business.state.roles;
        openswd3::world_map::LegacyWorldStoryPathRuntime story_paths{
            .roles = roles,
            .active_object_slots =
                world_frame_state_.map_role_paths.active_object_slots,
            .spatial_index = &map.business.state.spatial_index,
            .role_surface =
                {
                    .map_width = map.header.width,
                    .selected_guid = roles[world.selected_role_index].guid,
                    .surface_grid = map.surface_grid.surface_grid,
                },
            .node_pool = &world_path_node_pool_,
            .movement = &world_frame_state_.movement,
            .camera = &world.camera,
            .selected_arrival_bytes =
                world_frame_state_.map_role_paths.guid_one_arrival_bytes,
            .selected_role_index = world.selected_role_index,
            .map_height = map.header.height,
            .scene_render_flags =
                &world_frame_state_.frame_runtime.frame.runtime_flags,
        };
        SdlDeferredWorldFramePorts deferred_ports{
            audio_maintenance_,
            *this,
            text_renderers_,
            &world_frame_state_.head_sign_actions,
            &story_paths,
            &world_path_script_state_,
            &world_story_vm_state_,
        };
        using StoryStatus = openswd3::world_map::LegacyWorldStoryVmStatus;
        if (story_result.status == StoryStatus::unsupported_opcode) {
            if (!unsupported_world_story_opcode_notice_logged_) {
                std::string message{"world story deferred opcode="};
                message.append(std::to_string(story_result.opcode));
                message.append(", raw_word=");
                message.append(std::to_string(story_result.raw_word));
                openswd3::diagnostics::log_warning(message);
                unsupported_world_story_opcode_notice_logged_ = true;
            }
        } else if (
            story_result.status != StoryStatus::idle &&
            story_result.status != StoryStatus::yielded &&
            story_result.status != StoryStatus::terminated
        ) {
            std::string message{"world story failed: status="};
            message.append(
                std::to_string(static_cast<unsigned>(story_result.status))
            );
            message.append(", opcode=");
            message.append(std::to_string(story_result.opcode));
            message.append(", raw_word=");
            message.append(std::to_string(story_result.raw_word));
            message.append(", story_id=");
            message.append(
                std::to_string(world_frame_state_.map_role_paths.talk_context
                                   .talk_script_id)
            );
            message.append(", source_guid=");
            message.append(
                std::to_string(
                    world_frame_state_.map_role_paths.talk_context.source_guid
                )
            );
            message.append(", talk_data_offset=");
            message.append(
                std::to_string(world_frame_state_.map_role_paths.talk_context
                                   .talk_data_offset)
            );
            message.append(", window_file=");
            message.append(
                std::to_string(world_story_vm_state_.loaded_file_number)
            );
            message.append(", window_data_offset=");
            message.append(
                std::to_string(world_story_vm_state_.loaded_data_offset)
            );
            message.append(", ip=");
            message.append(std::to_string(story_result.instruction_offset));
            message.append(", operand0=");
            message.append(
                story_result.first_operand_available
                    ? std::to_string(story_result.first_operand_word)
                    : std::string{"unavailable"}
            );
            message.append(", executed=");
            message.append(
                std::to_string(story_result.executed_instruction_count)
            );
            message.append(", controlled_role=");
            message.append(std::to_string(world.selected_role_index));
            message.append(", role_count=");
            message.append(std::to_string(roles.size()));
            static_cast<void>(report_error(message));
            ok_ = false;
            running_ = false;
            return;
        }
        PartyPathPorts path_ports{deferred_ports};
        const openswd3::world_map::LegacyWorldRoleSurfaceContext surface{
            .map_width = map.header.width,
            .selected_guid = roles[world.selected_role_index].guid,
            .surface_grid = map.surface_grid.surface_grid,
        };
        PathScriptPorts path_script_ports{
            action_updater_,
            sample_manager_,
            openswd3::audio_video::LegacySpatialSampleState{
                .listener_x = static_cast<openswd3::compat::i32>(
                    roles[world.selected_role_index].world_x
                ),
                .listener_y = static_cast<openswd3::compat::i32>(
                    roles[world.selected_role_index].world_y
                ),
                .mix_level =
                    world_frame_state_.frame_runtime.spatial_audio.mix_level,
            },
        };
        const openswd3::world_map::LegacyWorldPathScriptRuntime path_runtime{
            .shared_script_state = &world_story_vm_state_,
            .spatial_index = &map.business.state.spatial_index,
            .crt_rng = &crt_rng_,
            .secondary_rng = &secondary_rng_,
            .controlled_role_index = world.selected_role_index,
        };
        openswd3::compat::u32 unsupported_scripts = 0U;
        for (openswd3::compat::u32 role_index = 1U; role_index < roles.size();
             ++role_index) {
            auto& role = roles[role_index];
            const auto path_action =
                openswd3::world_map::select_legacy_world_path_role_frame_action(
                    role, role_index, world.selected_role_index
                );
            using PathRoleAction =
                openswd3::world_map::LegacyWorldPathRoleFrameAction;
            if (path_action == PathRoleAction::run_party_path) {
                const auto result =
                    openswd3::world_map::prepare_legacy_world_party_path(
                        role_index,
                        roles,
                        map.business.state.spatial_index,
                        surface,
                        world.selected_role_index,
                        world.role_post_materialization.party_role_count,
                        world.role_post_materialization.party_role_indices,
                        world_frame_state_.party_object_slots,
                        world_frame_state_.player_post_frame,
                        world.camera,
                        world_path_node_pool_,
                        path_ports
                    );
                if (result.status !=
                    openswd3::world_map::LegacyWorldPartyPathPreparationStatus::
                        completed) {
                    std::string message{
                        "party path preparation failed: status="
                    };
                    message.append(
                        std::to_string(static_cast<unsigned>(result.status))
                    );
                    static_cast<void>(report_error(message));
                    ok_ = false;
                    running_ = false;
                    return;
                }
                continue;
            }
            if (path_action == PathRoleAction::mark_surface) {
                static_cast<void>(
                    openswd3::world_map::
                        mark_legacy_world_role_surface_occupancy(role, surface)
                );
                continue;
            }
            if (path_action ==
                PathRoleAction::update_action_then_mark_surface) {
                static_cast<void>(action_updater_.update(role.action));
                if (role.action.action_id != 0U) {
                    static_cast<void>(
                        openswd3::world_map::
                            mark_legacy_world_role_surface_occupancy(
                                role, surface
                            )
                    );
                }
                continue;
            }
            if (path_action != PathRoleAction::run_path_script) {
                continue;
            }

            const auto result =
                openswd3::world_map::run_legacy_world_path_script(
                    role_index,
                    resource_databases_.path_bytes(),
                    roles,
                    surface,
                    map.header.height,
                    world_frame_state_.map_role_paths.active_object_slots,
                    world_path_node_pool_,
                    world_path_script_state_,
                    path_runtime,
                    path_script_ports
                );
            if (result.status ==
                openswd3::world_map::LegacyWorldPathScriptStatus::
                    unsupported_opcode) {
                ++unsupported_scripts;
                continue;
            }
            if (result.status !=
                openswd3::world_map::LegacyWorldPathScriptStatus::completed) {
                std::string message{"world path script failed: status="};
                message.append(
                    std::to_string(static_cast<unsigned>(result.status))
                );
                static_cast<void>(report_error(message));
                ok_ = false;
                running_ = false;
                return;
            }
        }
        if (unsupported_scripts != 0U &&
            !unsupported_world_path_opcode_notice_logged_) {
            std::string message{
                "world path script deferred unsupported opcodes: "
            };
            message.append(std::to_string(unsupported_scripts));
            openswd3::diagnostics::log_warning(message);
            unsupported_world_path_opcode_notice_logged_ = true;
        }
    }
    void finish_world_frame(
        openswd3::app::FrameCoordinatorState& frame_state
    ) override {
        if (!active_world_session_.has_value()) {
            request_presentation(
                openswd3::rendering::LegacyPresentationSite::steady_world
            );
            return;
        }

        auto& world = *active_world_session_;
        auto& map = world.render.map_load.session;
        auto& roles = map.business.state.roles;
        world_frame_state_.frame_runtime.spatial_audio.controlled_role_index =
            world.selected_role_index;
        world_frame_state_.frame_runtime.frame.ani_activity_active =
            world_ani_activity_.is_active();
        openswd3::world_map::LegacyWorldStoryPathRuntime story_paths{
            .roles = roles,
            .active_object_slots =
                world_frame_state_.map_role_paths.active_object_slots,
            .spatial_index = &map.business.state.spatial_index,
            .role_surface =
                {
                    .map_width = map.header.width,
                    .selected_guid = roles[world.selected_role_index].guid,
                    .surface_grid = map.surface_grid.surface_grid,
                },
            .node_pool = &world_path_node_pool_,
            .movement = &world_frame_state_.movement,
            .camera = &world.camera,
            .selected_arrival_bytes =
                world_frame_state_.map_role_paths.guid_one_arrival_bytes,
            .selected_role_index = world.selected_role_index,
            .map_height = map.header.height,
            .scene_render_flags =
                &world_frame_state_.frame_runtime.frame.runtime_flags,
        };
        openswd3::asset_runtime::LegacyAniRoleParticleRuntimePorts
            role_particle_ports{
                action_updater_,
                tsw_runtime_,
                game_framebuffer_,
                world_raster_,
                world_effects_,
                world_jitter_,
            };
        openswd3::platform_sdl3::WorldRoleRuntimeAdapter world_role_adapter{
            sample_manager_,
            world_frame_state_.frame_runtime.spatial_audio.mix_level,
            world_role_particle_effect_,
            std::bit_cast<openswd3::compat::i32>(world_frame_state_.map_id),
            world.camera,
            secondary_rng_,
            roles.data(),
            roles.size(),
            world.selected_role_index,
            world_frame_effects_.directional_action,
            role_particle_ports,
            &world_frame_state_.head_sign_actions,
            &world_path_script_state_,
            &world_story_vm_state_,
            text_renderers_,
            pixel_conversion_,
        };
        SdlDeferredWorldFramePorts deferred_ports{
            audio_maintenance_,
            *this,
            text_renderers_,
            &world_frame_state_.head_sign_actions,
            &story_paths,
            &world_path_script_state_,
            &world_story_vm_state_,
            &world_role_adapter,
            &world_ani_activity_,
            &game_framebuffer_,
            &pixel_conversion_,
            &window_state_.process_flags,
            &world_frame_state_.frame_runtime.frame.runtime_flags,
            &frame_interval_,
            openswd3::asset_runtime::LegacyAniActivityBlockers{
                .first = world_dialogs_.messages.empty() ? 0U : 1U,
                .second = world_frame_effects_.packed_rows.empty() ? 0U : 1U,
                .third = world_role_head_actions_.empty() ? 0U : 1U,
            },
            &world_ani_scene_backup_,
        };
        // sub_40A570 0x0040AA6C..0x0040AA8B applies the gameplay advances
        // after the renderers were initially built as 24/18/16 by
        // sub_40F340.  Dialogue therefore uses a 20x20 mask with a 22-pixel
        // advance; retaining the construction-time 24 stretches each line.
        static_cast<void>(text_renderers_.set_horizontal_advance(20U, 0x16));
        static_cast<void>(text_renderers_.set_horizontal_advance(16U, 0x12));
        static_cast<void>(text_renderers_.set_horizontal_advance(12U, 0x10));
        const auto timed_message_binding = text_renderers_.binding(12U);
        const auto dialog_text_20 = text_renderers_.binding(20U);
        const auto dialog_text_16 = text_renderers_.binding(16U);
        if (!timed_message_binding.ready() || !dialog_text_20.ready() ||
            !dialog_text_16.ready()) {
            static_cast<void>(report_error(
                "ordinary world frame: legacy text renderer is unavailable"
            ));
            ok_ = false;
            running_ = false;
            return;
        }
        openswd3::rendering::LegacyBoundTimedMessageRuntimePorts
            timed_message_ports{
                deferred_ports,
                *timed_message_binding.framebuffer,
                *timed_message_binding.glyph_cache,
                *timed_message_binding.glyph_provider,
                *timed_message_binding.state,
            };
        openswd3::asset_runtime::LegacyActionDrawRuntimePorts action_ports{
            action_updater_,
            tsw_runtime_,
            game_framebuffer_,
            world_raster_,
            world_effects_,
            world_jitter_,
        };
        openswd3::world_map::LegacyWorldDialogRuntimePorts dialog_ports{
            world_dialog_runtime_state_,
            game_framebuffer_,
            world_raster_,
            pixel_conversion_,
            world_effects_,
            world_jitter_,
            roles,
            action_ports,
            dialog_text_20,
            dialog_text_16,
            &deferred_ports,
            &world_frame_state_.map_role_paths.talk_context,
        };
        openswd3::world_map::LegacyWorldRoleRenderRuntimePorts role_ports{
            tsw_runtime_,
            game_framebuffer_,
            world_raster_,
            world_effects_,
            deferred_ports,
        };
        openswd3::asset_runtime::LegacyAniDriftRuntimePorts ani_drift_ports{
            action_updater_,
            tsw_runtime_,
            game_framebuffer_,
            world_raster_,
            world_effects_,
            world_jitter_,
        };
        openswd3::asset_runtime::LegacyAniDirectionalRuntimePorts
            ani_directional_ports{
                action_updater_,
                tsw_runtime_,
                game_framebuffer_,
                world_raster_,
                world_effects_,
                world_jitter_,
            };
        openswd3::asset_runtime::LegacyAniFollowerRuntimePorts
            ani_follower_ports{
                action_updater_,
                tsw_runtime_,
                game_framebuffer_,
                world_raster_,
                world_effects_,
                world_jitter_,
            };
        const auto result = openswd3::world_map::run_legacy_world_frame(
            game_framebuffer_,
            world_raster_,
            world.render.background_source(),
            map.business.state.events,
            map.business.state.spatial_index,
            roles,
            openswd3::world_map::LegacyWorldRoleSurfaceContext{
                .map_width = map.header.width,
                .selected_guid = roles[world.selected_role_index].guid,
                .surface_grid = map.surface_grid.surface_grid,
            },
            world_selection_words_,
            world.camera,
            world_frame_state_,
            world_jitter_,
            world_effects_,
            openswd3::world_map::LegacyWorldFrameRuntimePorts{
                .remaining_stages = deferred_ports,
                .indexed_objects =
                    world.render.prepared_indexed_objects.objects,
                .picture_actions = world_picture_actions_,
                .moving_actions = world_moving_actions_,
                .role_head_actions = world_role_head_actions_,
                .environment_effects = world_frame_effects_,
                .secondary_rng = secondary_rng_,
                .pixel_conversion = pixel_conversion_,
                .blit_effects = &world_effects_,
                .cursor_delete_key_pressed =
                    openswd3::input_time_rng::read_raw_key(
                        keyboard_snapshot_, 0x2EU
                    ) != 0U,
                .cursor_mouse_x = input_state_.current_mouse.logical_x,
                .cursor_mouse_y = input_state_.current_mouse.logical_y,
                .cursor_left_press_multiplicity =
                    input_state_.records[15U].rapid_press_multiplicity,
                .special_mode_state = &frame_state.battle.special_mode_state,
                .ani_drift = ani_drift_ports,
                .ani_directional = ani_directional_ports,
                .ani_follower = ani_follower_ports,
                .timed_message_runtime = timed_message_ports,
                .flagged_roles = action_ports,
                .world_roles = role_ports,
                .spatial_audio = deferred_ports,
                .dialogs = &world_dialogs_,
                .dialog_runtime = &dialog_ports,
                .dialog_input =
                    {
                        .current_tick = frame_preparation_state_.frame_clock
                                            .sampled_milliseconds,
                        .primary_press_state =
                            input_state_.records[1U].rapid_press_multiplicity,
                        .selected_choice_index =
                            std::bit_cast<openswd3::compat::i32>(
                                world_interaction_state_.selected_choice_index
                            ),
                        .camera_left = std::bit_cast<openswd3::compat::i32>(
                            world.camera.left
                        ),
                        .camera_top = std::bit_cast<openswd3::compat::i32>(
                            world.camera.top
                        ),
                        .choice_chain_active =
                            [&]() {
                                for (const auto& message :
                                     world_dialogs_.messages) {
                                    if (!message.choices.empty()) {
                                        return true;
                                    }
                                }
                                return false;
                            }(),
                    },
            },
            deferred_ports
        );
        if (result.status !=
                openswd3::world_map::LegacyWorldFrameCoordinatorStatus::
                    completed ||
            deferred_ports.presentation_failed()) {
            std::string message{"ordinary world frame failed: status="};
            message.append(
                std::to_string(static_cast<unsigned>(result.status))
            );
            message.append(", inner_status=");
            message.append(
                std::to_string(static_cast<unsigned>(result.frame.status))
            );
            static_cast<void>(report_error(message));
            ok_ = false;
            running_ = false;
            return;
        }
        if (!deferred_world_stage_notice_logged_) {
            std::string message{
                "ordinary world frame is live with deferred stages: inner="
            };
            message.append(
                std::to_string(deferred_ports.deferred_frame_stage_count())
            );
            openswd3::diagnostics::log_info(message);
            deferred_world_stage_notice_logged_ = true;
        }
    }
    void prepare_special_mode_objects(
        openswd3::app::FrameCoordinatorState&
    ) override {}
    openswd3::app::StandardSpecialModeEvent step_standard_special_mode(
        openswd3::app::FrameCoordinatorState& state
    ) override {
        constexpr openswd3::compat::u32 kSpecialModeValueMask = 0x0FFFFFFFU;
        if ((state.battle.special_mode_state & kSpecialModeValueMask) == 3U) {
            openswd3::asset_runtime::LegacyActionDrawRuntimePorts action_ports{
                action_updater_,
                tsw_runtime_,
                game_framebuffer_,
                world_raster_,
                world_effects_,
                world_jitter_,
            };
            const auto& records = input_state_.records;
            const auto previous_phase = initial_menu_state_.phase;
            const auto previous_counter = initial_menu_state_.counter;
            const auto previous_choice = initial_menu_state_.selected_choice;
            const auto result =
                openswd3::special_modes::run_legacy_initial_menu_frame(
                    initial_menu_state_,
                    openswd3::special_modes::LegacyInitialMenuInput{
                        .mouse_x = input_state_.current_mouse.logical_x,
                        .mouse_y = input_state_.current_mouse.logical_y,
                        .mouse_button_mask =
                            input_state_.current_mouse.button_mask,
                        .cancel = &records[0U],
                        .primary = &records[1U],
                        .alternate_primary = &records[12U],
                        .left = &records[3U],
                        .up = &records[4U],
                        .right = &records[5U],
                        .down = &records[6U],
                        .page_up = &records[7U],
                        .page_down = &records[8U],
                        .mouse_left = &records[15U],
                        .mouse_right = &records[14U],
                    },
                    action_ports,
                    world_effects_
                );
            synchronize_text_input();
            if (initial_menu_state_.phase != previous_phase ||
                initial_menu_state_.selected_choice != previous_choice ||
                (initial_menu_state_.counter != previous_counter &&
                 initial_menu_state_.counter >=
                     openswd3::special_modes::
                         kLegacyInitialMenuNameOneCounter)) {
                std::string message{"initial menu state: phase="};
                message.append(std::to_string(initial_menu_state_.phase));
                message.append(", counter=");
                message.append(std::to_string(initial_menu_state_.counter));
                message.append(", choice=");
                message.append(
                    std::to_string(initial_menu_state_.selected_choice)
                );
                openswd3::diagnostics::log_info(message);
            }
            if (result.draw_status !=
                openswd3::special_modes::LegacyInitialMenuDrawStatus::
                    completed) {
                std::string message{"initial menu draw failed: status="};
                message.append(
                    std::to_string(static_cast<unsigned>(result.draw_status))
                );
                message.append(", blit_failures=");
                message.append(std::to_string(result.blit_failure_count));
                static_cast<void>(report_error(message));
                ok_ = false;
                running_ = false;
                return openswd3::app::StandardSpecialModeEvent::none;
            }
            if (!draw_initial_menu_name_input()) {
                return openswd3::app::StandardSpecialModeEvent::none;
            }
            if (result.event ==
                openswd3::special_modes::LegacyInitialMenuEvent::
                    commit_new_game_004492ba) {
                openswd3::diagnostics::log_info(
                    "initial menu committed new game"
                );
                return openswd3::app::StandardSpecialModeEvent::
                    commit_new_game_004492ba;
            }
            if (result.event ==
                openswd3::special_modes::LegacyInitialMenuEvent::
                    commit_choice_3_00449320) {
                openswd3::diagnostics::log_info(
                    "initial menu requested process close"
                );
                return openswd3::app::StandardSpecialModeEvent::
                    request_close_00449320;
            }
            static_cast<void>(request_presentation(
                openswd3::rendering::LegacyPresentationSite::
                    steady_special_modes_1_3_4_5_6
            ));
            return openswd3::app::StandardSpecialModeEvent::none;
        }
        request_presentation(
            openswd3::rendering::LegacyPresentationSite::
                steady_special_modes_1_3_4_5_6
        );
        return openswd3::app::StandardSpecialModeEvent::none;
    }
    void step_shop_mode(openswd3::app::FrameCoordinatorState&) override {
        request_presentation(
            openswd3::rendering::LegacyPresentationSite::steady_shop_mode_2
        );
    }

    void clear_accumulated_play_time() override {
        accumulated_play_time_ = 0U;
    }

    openswd3::compat::u32 sample_epoch_seconds() override {
        return static_cast<openswd3::compat::u32>(std::time(nullptr));
    }

    void set_play_time_origin(const openswd3::compat::u32 seconds) override {
        play_time_origin_ = seconds;
    }

    void set_initial_menu_phase(const openswd3::compat::u32 phase) override {
        initial_menu_state_.phase = std::bit_cast<openswd3::compat::i32>(phase);
    }

    void clear_game_framebuffer() override {
        std::ranges::fill(
            game_framebuffer_.physical_pixels().first(
                openswd3::rendering::kLegacyFixedCanvasPixels
            ),
            openswd3::compat::u16{}
        );
    }

    bool initialize_new_game_state_and_world() override {
        tsw_runtime_.set_special_loader(nullptr);
        tsw_runtime_.clear_cache();
        world_special_frame_loader_.reset();
        if (active_world_session_.has_value()) {
            auto& roles = active_world_session_->render.map_load.session
                              .business.state.roles;
            const auto role_clear =
                openswd3::world_map::clear_legacy_world_role_table(
                    roles, world_path_script_state_.role_label_payloads
                );
            if (role_clear.status !=
                openswd3::world_map::LegacyWorldRoleTableResetStatus::ready) {
                std::string message{
                    "initial world: previous role owner clear failed: status="
                };
                message.append(
                    std::to_string(static_cast<unsigned int>(role_clear.status))
                );
                static_cast<void>(report_error(message));
                ok_ = false;
                running_ = false;
                return false;
            }
        }
        active_world_session_.reset();
        world_audio_distances_.clear();
        world_audio_vertical_offsets_.clear();

        const auto payload_load = resource_databases_.reload_maps_payload();
        if (payload_load.status !=
            openswd3::resource_io::LegacyMapsPayloadStatus::ready) {
            std::string message{"initial world: MAPS payload reload failed: "};
            message.append(
                std::to_string(static_cast<unsigned>(payload_load.status))
            );
            static_cast<void>(report_error(message));
            ok_ = false;
            running_ = false;
            return false;
        }

        auto payload = resource_databases_.mutable_maps_payload_bytes();
        const auto decoded =
            openswd3::world_map::decode_legacy_maps_world_database(payload);
        if (decoded.status !=
            openswd3::world_map::LegacyMapsWorldDatabaseStatus::ready) {
            std::string message{"initial world: MAPS decode failed: "};
            message.append(
                std::to_string(static_cast<unsigned>(decoded.status))
            );
            static_cast<void>(report_error(message));
            ok_ = false;
            running_ = false;
            return false;
        }

        // sub_40F160 calls sub_40E0B0 after replacing MAPS and before the
        // initial sub_40C130 map load. Rebuild the same persistent world
        // owners before role materialization starts consuming action state.
        world_raster_ = game_framebuffer_.geometry();
        world_jitter_ = {};
        world_frame_state_ = {};
        openswd3::world_map::reset_legacy_world_transient_state(
            openswd3::world_map::LegacyWorldTransientResetOwners{
                .packed_row_effects = world_frame_effects_.packed_rows,
                .moving_actions = world_moving_actions_,
                .role_head_actions = world_role_head_actions_,
                .dialogs = world_dialogs_,
                .picture_actions = world_picture_actions_,
                .role_particles = world_role_particle_effect_,
                .ani_drift = world_frame_effects_.drift,
                .frame_color = world_frame_effects_.frame_color,
                .selection_words = world_selection_words_,
                .row_copy = world_frame_effects_.row_copy,
            }
        );
        world_frame_effects_.frame_color = {};
        world_frame_effects_.streak.reset();
        world_frame_effects_.spark.reset_counters();
        world_frame_effects_.directional.reset_motion_block();
        world_frame_effects_.deformation.clear();
        world_frame_effects_.follower = {};
        world_frame_effects_.timed_messages.clear();
        world_frame_effects_.cursor = {};
        world_frame_effects_.initialize_action_records();
        world_path_script_state_ = {};
        openswd3::asset_runtime::LegacyActionDrawRuntimePorts action_ports{
            action_updater_,
            tsw_runtime_,
            game_framebuffer_,
            world_raster_,
            world_effects_,
            world_jitter_,
        };
        openswd3::world_map::initialize_legacy_world_story_vm(
            world_story_vm_state_
        );
        const auto progress_reset =
            openswd3::world_map::update_legacy_world_load_progress(
                world_load_progress_,
                world_story_vm_state_,
                game_framebuffer_,
                pixel_conversion_,
                -1,
                *this,
                action_ports,
                *this
            );
        if (progress_reset.status !=
            openswd3::world_map::LegacyWorldLoadProgressStatus::suppressed) {
            static_cast<void>(report_error(
                "initial world: loading progress reset was not suppressed"
            ));
            ok_ = false;
            running_ = false;
            return false;
        }
        world_interaction_state_ = {};
        world_player_control_state_ = {};
        world_encounter_state_ = {};
        if (openswd3::world_map::prime_legacy_world_cursor_state(
                world_frame_effects_.cursor, action_ports
            ) != openswd3::asset_runtime::LegacyActionUpdateStatus::completed) {
            openswd3::diagnostics::log_warning(
                "new-game cursor action update failed"
            );
        }
        const auto dialog_prime =
            openswd3::world_map::prime_legacy_world_dialog_runtime(
                world_dialog_runtime_state_, action_ports
            );
        if (dialog_prime.action_update_failure_count != 0U) {
            openswd3::diagnostics::log_warning(
                "new-game dialog indicator action update failed"
            );
        }

        auto loaded = openswd3::world_map::load_legacy_world_runtime_session(
            payload,
            {
                .archive_path = data_directory_ / "huge.lmf",
                .cache_directory = world_cache_directory_,
                .load = decoded.database.initial_load,
                .cache_limit_megabytes = 60U,
                .pixel_conversion = pixel_conversion_,
                .random = &world_runtime_random_,
            },
            world_action_initializer_,
            [&](const openswd3::compat::i32 progress, const auto&) {
                static_cast<void>(
                    openswd3::world_map::update_legacy_world_load_progress(
                        world_load_progress_,
                        world_story_vm_state_,
                        game_framebuffer_,
                        pixel_conversion_,
                        progress,
                        *this,
                        action_ports,
                        *this
                    )
                );
            },
            [&]() { maintain_audio(); }
        );
        if (loaded.status !=
            openswd3::world_map::LegacyWorldRuntimeSessionStatus::ready) {
            std::string message{"initial world: session load failed: status="};
            message.append(
                std::to_string(static_cast<unsigned>(loaded.status))
            );
            message.append(", maps_status=");
            message.append(
                std::to_string(
                    static_cast<unsigned>(loaded.maps_database_status)
                )
            );
            message.append(", render_status=");
            message.append(
                std::to_string(static_cast<unsigned>(loaded.render_status))
            );
            static_cast<void>(report_error(message));
            ok_ = false;
            running_ = false;
            return false;
        }

        const auto progress_complete =
            openswd3::world_map::update_legacy_world_load_progress(
                world_load_progress_,
                world_story_vm_state_,
                game_framebuffer_,
                pixel_conversion_,
                100,
                *this,
                action_ports,
                *this
            );
        if (progress_complete.status !=
                openswd3::world_map::LegacyWorldLoadProgressStatus::
                    suppressed ||
            !progress_complete.suppression_flag_cleared) {
            static_cast<void>(report_error(
                "initial world: loading progress terminal gate failed"
            ));
            ok_ = false;
            running_ = false;
            return false;
        }

        active_world_session_.emplace(std::move(loaded.session));
        auto& world = *active_world_session_;
        const auto& map = world.render.map_load.session;
        world_special_frame_loader_.emplace(
            data_directory_ / "huge.lmf",
            map.lookup.map_offset,
            map.referenced_records.records,
            pixel_conversion_
        );
        tsw_runtime_.set_special_loader(&*world_special_frame_loader_);
        const auto role_count = map.business.state.roles.size();
        world_audio_distances_.assign(role_count, 0);
        world_audio_vertical_offsets_.assign(role_count, 0);
        constexpr std::array<std::size_t, 7U> map_service_ids{
            5U,
            6U,
            7U,
            8U,
            15U,
            19U,
            22U,
        };
        for (const std::size_t service_id : map_service_ids) {
            const auto story_flag =
                static_cast<openswd3::compat::u16>(service_id);
            if ((world.map_descriptor_runtime.enabled_service_bits &
                 (1U << service_id)) != 0U) {
                openswd3::world_map::set_legacy_world_story_flag(
                    world_story_vm_state_, story_flag
                );
            } else {
                openswd3::world_map::clear_legacy_world_story_flag(
                    world_story_vm_state_, story_flag
                );
            }
        }
        world_frame_effects_.directional_configuration = {
            .map_width_tiles =
                static_cast<openswd3::compat::i32>(map.header.width),
            .map_height_tiles =
                static_cast<openswd3::compat::i32>(map.header.height),
            .base_variant = static_cast<openswd3::compat::u16>(
                world.map_descriptor_runtime.directional_base_variant
            ),
            .variant_count = static_cast<openswd3::compat::u16>(
                world.map_descriptor_runtime.directional_variant_count
            ),
            .spawn_direction = world.map_descriptor_runtime.behavior_index,
        };
        auto& directional_state = world_frame_effects_.directional.state();
        for (std::size_t index = 0U; index < world.directional_points.size();
             ++index) {
            const auto& source = world.directional_points[index];
            auto& motion = directional_state.motion[index];
            auto& color = directional_state.color[index];
            auto& timing = directional_state.timing[index];
            motion.world_x = static_cast<openswd3::compat::i32>(source.world_x);
            motion.world_y = static_cast<openswd3::compat::i32>(source.world_y);
            motion.velocity_x = source.velocity_x;
            motion.velocity_y = source.velocity_y;
            color.target_offset = 0;
            timing.target_interval =
                static_cast<openswd3::compat::i32>(source.target_interval);
            timing.variant = static_cast<openswd3::compat::i32>(source.variant);
        }
        world_frame_state_.map_id = world.logical_map_id;
        world_frame_state_.frame_runtime.flash_red_offset =
            world.map_descriptor_runtime.role_red_offset;
        world_frame_state_.frame_runtime.flash_green_offset =
            world.map_descriptor_runtime.role_green_offset;
        world_frame_state_.frame_runtime.flash_blue_offset =
            world.map_descriptor_runtime.role_blue_offset;
        world_frame_state_.player_role_index = world.selected_role_index;
        world_frame_state_.party_role_count =
            world.role_post_materialization.party_role_count;
        world_frame_state_.party_object_slots =
            world.role_post_materialization.party_object_slots;
        // sub_40F160 seeds the initial story owner from MAPS +0x0C and uses
        // TALK entry 100 for a new game.
        world_frame_state_.map_role_paths.talk_context.source_guid =
            decoded.database.initial_load.selected_guid;
        world_frame_state_.map_role_paths.talk_context.talk_script_id = 100U;
        world_frame_state_.selection_scroll.saved_left = world.camera.left;
        world_frame_state_.selection_scroll.saved_top = world.camera.top;
        world_frame_state_.tile_animation = {
            .cycle_counter = 1,
            .cycle_interval = static_cast<openswd3::compat::i32>(
                world.map_descriptor_runtime.tile_animation_interval
            ),
            .frame_count = map.header.layers,
            .frame_index = 0U,
            .frame_direction = 1,
            .tile_layer_stride = map.header.width * map.header.height,
            .tile_layer_offset = 0U,
        };
        world_frame_state_.frame_runtime.spatial_audio = {
            .controlled_role_index = world.selected_role_index,
            .mix_level = kLegacyInitialSampleMixLevel,
            .distance_by_role = world_audio_distances_,
            .vertical_offset_by_role = world_audio_vertical_offsets_,
        };
        openswd3::world_map::initialize_legacy_world_player_position_history(
            world_frame_state_.player_post_frame,
            map.business.state.roles[world.selected_role_index]
        );
        deferred_world_stage_notice_logged_ = false;
        unsupported_world_path_opcode_notice_logged_ = false;
        unsupported_world_story_opcode_notice_logged_ = false;

        std::string message{"initial world ready: logical_map="};
        message.append(std::to_string(world.logical_map_id));
        message.append(", archive_map=");
        message.append(std::to_string(world.map_descriptor.archive_map_id));
        message.append(", roles=");
        message.append(std::to_string(role_count));
        message.append(", player_index=");
        message.append(std::to_string(world.selected_role_index));
        openswd3::diagnostics::log_info(message);
        return true;
    }

    void set_high_priority_submode(const openswd3::compat::u32 value) override {
        high_priority_submode_ = value;
    }

    void
    set_high_priority_auxiliary(const openswd3::compat::u32 value) override {
        high_priority_auxiliary_ = value;
    }

    void reset_input_menu_and_save_previews() override {}
    void apply_new_game_name_overrides() override {}
    void load_fame_table() override {}

    [[nodiscard]] bool present_legacy_frame(
        const openswd3::rendering::LegacyPresentationRequest& request
    ) override {
        const auto composition_status =
            openswd3::rendering::compose_legacy_primary_surface(
                primary_surface_,
                request,
                openswd3::rendering::LegacyPresentationSources{
                    .game_framebuffer = &game_framebuffer_,
                }
            );
        if (composition_status !=
            openswd3::rendering::LegacyPrimaryCompositionStatus::completed) {
            static_cast<void>(report_error(
                "framebuffer presentation: primary composition failed"
            ));
            ok_ = false;
            running_ = false;
            return false;
        }
        if (texture_ == nullptr) {
            static_cast<void>(
                report_error("framebuffer presentation: missing texture")
            );
            ok_ = false;
            running_ = false;
            return false;
        }
        if (!present_framebuffer(renderer_, *texture_, primary_surface_)) {
            static_cast<void>(report_sdl_error("framebuffer presentation"));
            ok_ = false;
            running_ = false;
            return false;
        }
        return true;
    }

    std::span<openswd3::compat::u16> video_destination_pixels() override {
        return game_framebuffer_.physical_pixels();
    }

    openswd3::compat::i32 video_destination_pitch_bytes() override {
        return game_framebuffer_.geometry().surface.pitch_bytes;
    }

    openswd3::audio_video::LegacyVideoPixelFormat
    video_pixel_format() override {
        return openswd3::audio_video::LegacyVideoPixelFormat::rgb565;
    }

    void report_video_copy_failure() override {
        openswd3::diagnostics::log_error("legacy video frame copy failed");
    }

    bool present_video_frame() override {
        return request_presentation(
            openswd3::rendering::LegacyPresentationSite::bink_video
        );
    }

    void request_synchronous_close() override {
        window_state_.process_flags = frame_coordinator_state_.process_flags;
        openswd3::app::handle_window_destroy(
            window_state_, shutdown_ports_, exit_ports_
        );
    }

private:
    void synchronize_text_input() noexcept {
        const bool should_be_active =
            initial_menu_state_.name_input.has_value();
        if (should_be_active == text_input_active_) {
            return;
        }
        if (should_be_active) {
            const bool started = SDL_StartTextInput(&window_);
            text_input_active_ = true;
            if (!started) {
                openswd3::diagnostics::log_warning(
                    std::string{"SDL_StartTextInput: "} + SDL_GetError()
                );
            }
            return;
        }
        static_cast<void>(SDL_StopTextInput(&window_));
        text_input_active_ = false;
    }

    [[nodiscard]] bool draw_initial_menu_name_input() {
        if (!initial_menu_state_.name_input.has_value()) {
            return true;
        }
        const auto binding = text_renderers_.binding(20U);
        if (!binding.ready()) {
            static_cast<void>(report_error(
                "initial menu name input: 20px text renderer unavailable"
            ));
            ok_ = false;
            running_ = false;
            return false;
        }

        std::array<openswd3::compat::u8, 0x40U> text{};
        static_cast<void>(initial_menu_state_.name_input->copy_to(
            text.data(), static_cast<openswd3::compat::i32>(text.size())
        ));
        const auto color_pair = openswd3::rendering::legacy_pack_color_pair(
            pixel_conversion_, 0x15, 0x0F, 0x08
        );
        const auto draw = openswd3::rendering::draw_legacy_text(
            *binding.framebuffer,
            *binding.glyph_cache,
            *binding.glyph_provider,
            *binding.state,
            openswd3::rendering::LegacyTextDrawRequest{
                .destination_x = initial_menu_state_.name_input->x(),
                .destination_y = initial_menu_state_.name_input->y(),
                .nul_terminated_text = text,
                .foreground_color =
                    static_cast<openswd3::compat::u16>(color_pair),
                .flags = 4U,
            }
        );
        if (draw.status !=
            openswd3::rendering::LegacyTextDrawStatus::completed) {
            static_cast<void>(
                report_error("initial menu name input: text draw failed")
            );
            ok_ = false;
            running_ = false;
            return false;
        }

        const auto cursor = openswd3::rendering::apply_legacy_rectangle_effect(
            game_framebuffer_,
            world_raster_,
            pixel_conversion_,
            openswd3::rendering::LegacyRectangleEffectRequest{
                .x = initial_menu_state_.name_input->x() +
                    initial_menu_state_.name_input->cursor_byte_offset() * 11,
                .y = initial_menu_state_.name_input->y(),
                .width = 0x0B,
                .height = 0x16,
                .red = 0x14,
                .green = 0x0D,
                .blue = 0,
                .mode = 5U,
            }
        );
        if (cursor !=
                openswd3::rendering::LegacyRectangleEffectStatus::completed &&
            cursor !=
                openswd3::rendering::LegacyRectangleEffectStatus::clipped_out) {
            static_cast<void>(
                report_error("initial menu name input: cursor draw failed")
            );
            ok_ = false;
            running_ = false;
            return false;
        }
        return true;
    }

    bool request_presentation(
        const openswd3::rendering::LegacyPresentationSite site
    ) {
        const auto result =
            openswd3::rendering::submit_legacy_presentation(site, *this);
        if (result.status !=
                openswd3::rendering::LegacyPresentationDispatchStatus::
                    completed &&
            result.status !=
                openswd3::rendering::LegacyPresentationDispatchStatus::
                    backend_failed) {
            static_cast<void>(
                report_error("framebuffer presentation: invalid legacy request")
            );
            ok_ = false;
            running_ = false;
        }
        return result.status ==
            openswd3::rendering::LegacyPresentationDispatchStatus::completed;
    }

    SDL_Window& window_;

    SDL_Renderer& renderer_;
    SDL_Texture*& texture_;
    openswd3::rendering::LegacyFramebuffer& game_framebuffer_;
    openswd3::rendering::LegacyFramebuffer& primary_surface_;
    openswd3::compat::u32& frame_interval_;
    openswd3::app::WindowEventState& window_state_;
    const openswd3::app::DisplayLifecycleState& display_state_;
    openswd3::app::FramePreparationState& frame_preparation_state_;
    openswd3::app::FrameCoordinatorState& frame_coordinator_state_;
    openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard_snapshot_{};
    openswd3::input_time_rng::LegacyKeyboardSnapshot
        pending_keyboard_presses_{};
    openswd3::compat::u32 pending_mouse_button_mask_{};
    openswd3::input_time_rng::LegacyInputNormalizationState& input_state_;
    openswd3::input_time_rng::LegacyMouseState& mouse_state_;
    openswd3::platform_sdl3::SdlMouseDeviceState& mouse_device_state_;
    openswd3::audio_video::LegacyAudioMaintenancePorts& audio_maintenance_;
    openswd3::audio_video::LegacyStreamManager& stream_manager_;
    openswd3::audio_video::LegacySampleManager& sample_manager_;
    openswd3::audio_video::LegacyVideoPlayer& video_player_;
    openswd3::resource_io::LegacyResourceDatabases& resource_databases_;
    openswd3::world_map::LegacyWorldItemListState& world_item_lists_;
    std::filesystem::path data_directory_;
    std::filesystem::path world_cache_directory_;
    openswd3::rendering::LegacyPixelConversionState pixel_conversion_;
    openswd3::rendering::LegacyTextRendererRuntime& text_renderers_;
    openswd3::world_map::LegacyWorldRoleActionInitializer&
        world_action_initializer_;
    openswd3::asset_runtime::LegacyActionUpdater& action_updater_;
    openswd3::asset_runtime::LegacyTswRuntime& tsw_runtime_;
    openswd3::input_time_rng::LegacyCrtRng& crt_rng_;
    openswd3::input_time_rng::LegacySecondaryRng& secondary_rng_;
    class SecondaryWorldRuntimeRandom final
        : public openswd3::world_map::LegacyWorldRuntimeRandom {
    public:
        explicit SecondaryWorldRuntimeRandom(
            openswd3::input_time_rng::LegacySecondaryRng& random
        ) noexcept
            : random_{random} {}

        openswd3::compat::u32
        next_bounded(const openswd3::compat::u32 upper_bound) override {
            return random_.next_bounded(upper_bound);
        }

    private:
        openswd3::input_time_rng::LegacySecondaryRng& random_;
    } world_runtime_random_{secondary_rng_};
    openswd3::battle::LegacyBattleAssets battle_assets_;
    bool battle_assets_ready_{};
    openswd3::battle::LegacyBattleSetupState battle_setup_;
    bool battle_setup_ready_{};
    openswd3::rendering::LegacyRasterGeometryState world_raster_;
    openswd3::rendering::LegacyBlitEffectState world_effects_;
    openswd3::rendering::LegacyRleRowJitterState world_jitter_;
    std::optional<openswd3::world_map::LegacyWorldRuntimeSession>
        active_world_session_;
    std::optional<openswd3::world_map::LegacyWorldRuntimeSession>
        pending_story_world_session_;
    bool story_world_session_reload_fatal_{};
    std::optional<openswd3::world_map::LegacyWorldSpecialFrameLoader>
        world_special_frame_loader_;
    openswd3::world_map::LegacyWorldFrameCoordinatorState world_frame_state_;
    openswd3::world_map::LegacyWorldPathNodePool world_path_node_pool_;
    openswd3::world_map::LegacyWorldPathScriptState world_path_script_state_;
    openswd3::world_map::LegacyPictureActionLists world_picture_actions_;
    openswd3::world_map::LegacyMovingActionList world_moving_actions_;
    openswd3::world_map::LegacyRoleHeadActionList world_role_head_actions_;
    openswd3::asset_runtime::LegacyAniRoleParticleEffect
        world_role_particle_effect_;
    openswd3::asset_runtime::LegacyAniActivity world_ani_activity_;
    std::vector<openswd3::compat::u16> world_ani_scene_backup_;
    openswd3::world_map::LegacyWorldFrameEffectState world_frame_effects_;
    openswd3::story_scene::LegacyDialogRuntimeState world_dialogs_;
    openswd3::world_map::LegacyWorldStoryVmState world_story_vm_state_;
    openswd3::world_map::LegacyWorldLoadProgressState world_load_progress_;
    openswd3::world_map::LegacyWorldInteractionState world_interaction_state_;
    openswd3::world_map::LegacyWorldDebugHotkeyState world_debug_hotkey_state_;
    openswd3::world_map::LegacyWorldPlayerControlState
        world_player_control_state_;
    openswd3::world_map::LegacyWorldEncounterState world_encounter_state_;
    openswd3::world_map::LegacyWorldDialogRuntimeState
        world_dialog_runtime_state_;
    openswd3::special_modes::LegacyInitialMenuState initial_menu_state_;
    openswd3::input_time_rng::LegacyTextInputDriverState
        text_input_driver_state_{};
    SdlLegacyTextInputPorts text_input_ports_{};
    bool text_input_active_{};
    std::vector<openswd3::compat::i16> world_audio_distances_;
    std::vector<openswd3::compat::i16> world_audio_vertical_offsets_;
    std::array<
        openswd3::compat::i16,
        openswd3::world_map::kLegacyWorldSelectionWordCount>
        world_selection_words_ = [] {
            std::array<
                openswd3::compat::i16,
                openswd3::world_map::kLegacyWorldSelectionWordCount>
                words{};
            words.fill(
                std::bit_cast<openswd3::compat::i16>(
                    openswd3::world_map::kLegacyWorldSelectionSentinel
                )
            );
            return words;
        }();
    bool deferred_world_stage_notice_logged_{};
    bool unsupported_world_path_opcode_notice_logged_{};
    bool unsupported_world_story_opcode_notice_logged_{};
    openswd3::app::ShutdownPorts& shutdown_ports_;
    openswd3::app::ProcessExitPorts& exit_ports_;
    openswd3::compat::u32 accumulated_play_time_{};
    openswd3::compat::u32 play_time_origin_{};
    openswd3::compat::u32 high_priority_submode_{};
    openswd3::compat::u32 high_priority_auxiliary_{};
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
    openswd3::input_time_rng::LegacyInputNormalizationState input_state{};

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
            command_arguments, executable_directory, launch_directory
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
            data_directory.directory, directory_error
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
    SmokeCommandLinePorts command_line_ports(input_state.key_bindings);
    const std::string legacy_command_line =
        openswd3::platform_sdl3::reconstruct_legacy_command_line_tail(
            argument_count,
            arguments,
            1 + static_cast<int>(data_directory.consumed_argument_count)
        );
    const openswd3::app::ProcessStartupGateResult startup_gate_result =
        openswd3::app::run_process_startup_gates(
            legacy_command_line, single_instance, command_line_ports
        );
    if (startup_gate_result !=
        openswd3::app::ProcessStartupGateResult::continue_normal_startup) {
        openswd3::diagnostics::log_info(
            "process startup exited before SDL initialization"
        );
        return 0;
    }

    const std::filesystem::path glyph_atlas_path =
        executable_directory / "assets" / "fonts" / "legacy-glyph-atlas.bin";
    const std::vector<openswd3::compat::u8> glyph_atlas_bytes =
        read_binary_file(glyph_atlas_path);
    openswd3::rendering::LegacyGlyphAtlasProvider glyph_provider(
        glyph_atlas_bytes
    );
    if (!glyph_provider.valid()) {
        return report_error(
            std::string{"legacy glyph atlas: cannot load valid asset: "} +
            glyph_atlas_path.string()
        );
    }
    openswd3::diagnostics::log_info(
        std::string{"legacy glyph atlas loaded: "} +
        std::to_string(glyph_atlas_bytes.size()) + " bytes"
    );

    const std::filesystem::path configuration_path =
        executable_directory / openswd3::resource_io::kConfigurationFilename;
    const openswd3::resource_io::WindowConfigurationLoadResult window_config =
        openswd3::resource_io::load_window_configuration(
            configuration_path, {kInitialWindowWidth, kInitialWindowHeight}
        );
    if (window_config.status !=
        openswd3::resource_io::WindowConfigurationStatus::ready) {
        std::string message{"window size: "};
        message.append(
            openswd3::resource_io::window_configuration_status_message(
                window_config.status
            )
        );
        if (!window_config.detail.empty()) {
            message.append(": ");
            message.append(window_config.detail);
        }
        message.append("; using ");
        message.append(std::to_string(window_config.size.width));
        message.push_back('x');
        message.append(std::to_string(window_config.size.height));
        openswd3::diagnostics::log_warning(message);
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return report_sdl_error("SDL_Init");
    }
    openswd3::diagnostics::log_debug("SDL video subsystem initialized");

    openswd3::resource_io::WindowSize normal_window_size = window_config.size;
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;
    if (window_config.maximized) {
        window_flags |= SDL_WINDOW_MAXIMIZED;
    }
    SDL_Window* window = SDL_CreateWindow(
        "OpenSWD3",
        window_config.size.width,
        window_config.size.height,
        window_flags
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
        persist_window_configuration(
            *window, configuration_path, normal_window_size
        );
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
        persist_window_configuration(
            *window, configuration_path, normal_window_size
        );
        SDL_DestroyWindow(window);
        SDL_Quit();
        return result;
    }

    bool game_initialized = false;
    bool startup_destroy_requested = false;
    SDL_Texture* texture = nullptr;
    openswd3::compat::u32 frame_interval = kInitialFrameIntervalMilliseconds;
    openswd3::app::InitializationState initialization_state{};
    openswd3::app::PlatformBackendState backend_state{};
    openswd3::resource_io::LegacyResourceDatabases resource_databases;
    openswd3::rendering::LegacyFramebuffer framebuffer;
    openswd3::rendering::LegacyTextRendererRuntime text_renderers;
    openswd3::input_time_rng::LegacyMouseState mouse_state{};
    openswd3::platform_sdl3::SdlMouseDeviceState mouse_device_state{};
    openswd3::platform_sdl3::SdlLegacySampleBackend sample_backend;
    openswd3::audio_video::LegacySndArchive snd_archive;
    openswd3::audio_video::LegacySampleManager sample_manager(
        sample_backend, snd_archive
    );
    UnavailableLegacySequenceBackend sequence_backend;
    openswd3::audio_video::LegacySequenceManager sequence_manager(
        sequence_backend
    );
    UnavailableLegacyStreamBackend stream_backend;
    openswd3::audio_video::LegacyStreamManager stream_manager(stream_backend);
    openswd3::audio_video::ImmediateCompleteLegacyVideoBackend video_backend;
    openswd3::audio_video::LegacyVideoPlayer video_player(video_backend);
    SdlLegacyAudioQueuePorts audio_queue_ports(
        sequence_manager, stream_manager
    );
    openswd3::audio_video::LegacyAudioQueueCoordinator audio_queue(
        audio_queue_ports
    );
    SdlLegacyAudioMaintenancePorts audio_maintenance(
        audio_queue, stream_manager, sequence_manager, sample_manager
    );
    SdlSmokePlatformBackendPorts backend_ports(
        *renderer,
        texture,
        startup_destroy_requested,
        snd_archive,
        sample_backend,
        sample_manager,
        sequence_manager,
        stream_manager
    );
    SdlSmokeInitializationPorts initialization_ports(
        backend_state,
        backend_ports,
        resource_databases,
        glyph_provider,
        text_renderers,
        framebuffer,
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
            *renderer, external_launch_ports, [] {}
        );
        SdlSmokeStartupPorts startup_ports(
            startup_dialog,
            initialization_state,
            initialization_ports,
            input_state.key_bindings,
            game_initialized,
            startup_destroy_requested
        );
        openswd3::app::StartupState startup_state{};
        static_cast<void>(openswd3::app::run_startup_custom_message(
            startup_state, startup_ports
        ));
    }

    openswd3::input_time_rng::LegacyCrtRng crt_rng;
    openswd3::input_time_rng::LegacySecondaryRng secondary_rng;
    SdlRngSeedPorts rng_seed_ports(crt_rng, secondary_rng);
    if (startup_destroy_requested) {
        openswd3::app::seed_two_rng_streams(rng_seed_ports);
        shutdown_audio_output(
            audio_queue,
            stream_manager,
            sequence_manager,
            sample_manager,
            snd_archive,
            sample_backend
        );
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
        }
        SDL_DestroyRenderer(renderer);
        persist_window_configuration(
            *window, configuration_path, normal_window_size
        );
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    openswd3::rendering::LegacyFramebuffer primary_surface(
        framebuffer.geometry().surface
    );
    openswd3::rendering::LegacyPixelConversionState pixel_conversion;
    openswd3::rendering::select_legacy_pixel_conversion(
        pixel_conversion, {0xF800U, 0x07E0U, 0x001FU}
    );
    openswd3::asset_runtime::LegacyActRuntime act_runtime{
        data_directory.directory
    };
    act_runtime.set_cache_limit(openswd3::asset_runtime::kLegacyActCacheBytes);
    openswd3::asset_runtime::LegacyActActionStreamProvider action_provider{
        act_runtime
    };
    openswd3::asset_runtime::LegacyActionUpdater action_updater{
        action_provider
    };
    action_updater.set_stream_cache_mode(1U);
    openswd3::world_map::LegacyWorldActionUpdaterInitializer
        world_action_initializer{action_updater};
    openswd3::asset_runtime::LegacyTswRuntime tsw_runtime{
        data_directory.directory, pixel_conversion
    };
    tsw_runtime.set_cache_limit(
        openswd3::asset_runtime::kLegacyMaximumTswCacheBytes
    );

    openswd3::app::seed_two_rng_streams(rng_seed_ports);

    const bool runtime_ready = game_initialized &&
        backend_state.display_active != 0U && texture != nullptr;
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
    bool ok = true;
    bool running = true;
    SmokeWindowEventPorts window_ports(
        framebuffer, pixel_conversion, audio_maintenance, video_player
    );
    SdlDisplayLifecyclePorts display_ports(
        *window,
        *renderer,
        texture,
        primary_surface,
        framebuffer,
        glyph_provider,
        text_renderers,
        stream_manager,
        sample_manager,
        audio_maintenance,
        frame_interval,
        runtime_ready,
        ok,
        running
    );
    openswd3::world_map::LegacyWorldItemListState world_item_lists;
    SmokeShutdownPorts shutdown_ports(
        text_renderers, stream_manager, sample_manager, world_item_lists
    );

    SdlProcessExitPorts exit_ports(running);
    if (runtime_ready &&
        !present_framebuffer(*renderer, *texture, primary_surface)) {
        static_cast<void>(report_sdl_error("initial framebuffer presentation"));
        ok = false;
        openswd3::app::handle_window_destroy(
            window_state, shutdown_ports, exit_ports
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
        *window,
        *renderer,
        texture,
        framebuffer,
        primary_surface,
        frame_interval,
        window_state,
        display_state,
        frame_preparation_state,
        frame_coordinator_state,
        input_state,
        mouse_state,
        mouse_device_state,
        audio_maintenance,
        stream_manager,
        sample_manager,
        video_player,
        resource_databases,
        world_item_lists,
        data_directory.directory,
        executable_directory / "cache" / "maps",
        pixel_conversion,
        text_renderers,
        world_action_initializer,
        action_updater,
        tsw_runtime,
        crt_rng,
        secondary_rng,
        shutdown_ports,
        exit_ports,
        ok,
        running
    );
    shutdown_ports.bind_picture_actions(idle_ports.picture_actions());
    shutdown_ports.bind_role_particle_effect(idle_ports.role_particle_effect());
    shutdown_ports.bind_ani_drift_effect(idle_ports.ani_drift_effect());
    shutdown_ports.bind_packed_row_effects(idle_ports.packed_row_effects());
    shutdown_ports.bind_moving_actions(idle_ports.moving_actions());
    shutdown_ports.bind_role_head_actions(idle_ports.role_head_actions());
    shutdown_ports.bind_dialogs(idle_ports.dialogs());
    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_WINDOW_RESIZED &&
                (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) == 0U &&
                event.window.data1 > 0 && event.window.data2 > 0) {
                normal_window_size = {
                    event.window.data1,
                    event.window.data2,
                };
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                idle_ports.latch_keyboard_press(event.key.scancode);
                idle_ports.dispatch_text_input_key(event.key.scancode);
            } else if (event.type == SDL_EVENT_TEXT_INPUT) {
                idle_ports.dispatch_text_input_utf8(event.text.text);
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                idle_ports.latch_mouse_press(event.button.button);
            } else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
                idle_ports.clear_input_latches();
            }
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
                    window_state, shutdown_ports, exit_ports
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
        SDL_DelayNS(100U);
    }

    shutdown_audio_output(
        audio_queue,
        stream_manager,
        sequence_manager,
        sample_manager,
        snd_archive,
        sample_backend
    );
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
    }
    SDL_DestroyRenderer(renderer);
    persist_window_configuration(
        *window, configuration_path, normal_window_size
    );
    SDL_DestroyWindow(window);
    SDL_Quit();
    return ok ? 0 : 1;
}
