#include "event_translation.hpp"
#include "external_launch_sdl3.hpp"
#include "keyboard_snapshot_sdl3.hpp"
#include "legacy_command_line.hpp"
#include "legacy_sample_backend_sdl3.hpp"
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
#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/diagnostics/log.hpp"
#include "openswd3/input_time_rng/legacy_crt_rng.hpp"
#include "openswd3/input_time_rng/legacy_frame_clock.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/rendering/legacy_bmp_writer.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_glyph_atlas.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/rendering/legacy_presentation.hpp"
#include "openswd3/rendering/legacy_text_renderer_runtime.hpp"
#include "openswd3/resource_io/data_directory.hpp"
#include "openswd3/resource_io/legacy_memory_manager.hpp"
#include "openswd3/resource_io/legacy_resource_databases.hpp"
#include "openswd3/world_map/legacy_maps_world_database.hpp"
#include "openswd3/world_map/legacy_world_frame_coordinator.hpp"
#include "openswd3/world_map/legacy_world_runtime_session.hpp"
#include "openswd3/world_map/legacy_world_special_frame_loader.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <limits>
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

[[nodiscard]] std::vector<openswd3::compat::u8> read_binary_file(
    const std::filesystem::path& path
) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        return {};
    }
    const std::streamoff size = stream.tellg();
    if (size <= 0) {
        return {};
    }
    std::vector<openswd3::compat::u8> bytes(
        static_cast<std::size_t>(size)
    );
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
        openswd3::compat::u32,
        std::string_view,
        openswd3::compat::i32
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
        openswd3::audio_video::LegacyStreamHandle,
        openswd3::compat::u32
    ) override {
        return 0;
    }
    void set_stream_volume(
        openswd3::audio_video::LegacyStreamHandle,
        openswd3::compat::i32
    ) override {}
    openswd3::compat::i32 stream_volume(
        openswd3::audio_video::LegacyStreamHandle
    ) override {
        return 0;
    }
    void set_stream_loop_count(
        openswd3::audio_video::LegacyStreamHandle,
        openswd3::compat::i32
    ) override {}
    void start_stream(
        openswd3::audio_video::LegacyStreamHandle
    ) override {}
    openswd3::compat::u32 stream_status(
        openswd3::audio_video::LegacyStreamHandle
    ) override {
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

    void close_midi_output(
        openswd3::audio_video::LegacyMidiDriverHandle
    ) override {}
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
        openswd3::audio_video::LegacySequenceHandle,
        openswd3::compat::u32
    ) override {
        return 0;
    }
    void set_sequence_volume(
        openswd3::audio_video::LegacySequenceHandle,
        openswd3::compat::i32,
        openswd3::compat::i32
    ) override {}
    void set_sequence_loop_count(
        openswd3::audio_video::LegacySequenceHandle,
        openswd3::compat::i32
    ) override {}
    void start_sequence(
        openswd3::audio_video::LegacySequenceHandle
    ) override {}
    openswd3::compat::u32 sequence_status(
        openswd3::audio_video::LegacySequenceHandle
    ) override {
        return 2U;
    }
    void end_sequence(
        openswd3::audio_video::LegacySequenceHandle
    ) override {}
};

class SdlLegacyAudioQueuePorts final
    : public openswd3::audio_video::LegacyAudioQueuePorts {
public:
    SdlLegacyAudioQueuePorts(
        openswd3::audio_video::LegacySequenceManager& sequence_manager,
        openswd3::audio_video::LegacyStreamManager& stream_manager
    ) noexcept
        : sequence_manager_(sequence_manager),
          stream_manager_(stream_manager) {}

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
        static_cast<void>(sequence_manager_.play(
            filename,
            sequence_id,
            volume,
            loop_count
        ));
    }
    void play_stream(
        const std::string_view filename,
        const openswd3::compat::i32 stream_id,
        const openswd3::compat::i32 volume,
        const openswd3::compat::i32 loop_count
    ) override {
        static_cast<void>(stream_manager_.play(
            filename,
            stream_id,
            volume,
            loop_count
        ));
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
        : queue_(queue),
          stream_manager_(stream_manager),
          sequence_manager_(sequence_manager),
          sample_manager_(sample_manager) {}

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

void service_audio(
    openswd3::audio_video::LegacyAudioMaintenancePorts& ports
) {
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
        : framebuffer_(framebuffer),
          pixel_conversion_(pixel_conversion),
          audio_maintenance_(audio_maintenance),
          video_player_(video_player) {}

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

    bool open_existing_numbered_bmp(
        const openswd3::compat::u32 sequence
    ) override {
        existing_stream_.close();
        existing_stream_.clear();
        existing_stream_.open(
            numbered_screenshot_path(sequence),
            std::ios::binary
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
            path,
            std::ios::binary | std::ios::in | std::ios::out
        );
        if (output_stream_) {
            return true;
        }

        output_stream_.clear();
        std::error_code existence_error;
        if (std::filesystem::exists(path, existence_error) ||
            existence_error) {
            return false;
        }
        {
            std::ofstream create(path, std::ios::binary);
            if (!create) {
                return false;
            }
        }
        output_stream_.open(
            path,
            std::ios::binary | std::ios::in | std::ios::out
        );
        return static_cast<bool>(output_stream_);
    }

    bool seek_absolute(const openswd3::compat::u32 offset) override {
        output_stream_.seekp(
            static_cast<std::streamoff>(offset),
            std::ios::beg
        );
        return static_cast<bool>(output_stream_);
    }

    bool write_bytes(
        const std::span<const openswd3::compat::u8> bytes
    ) override {
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
    [[nodiscard]] static std::filesystem::path numbered_screenshot_path(
        const openswd3::compat::u32 sequence
    ) {
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
    ) : key_bindings_(key_bindings) {}

    void initialize_default_key_bindings() override {
        openswd3::input_time_rng::initialize_default_key_bindings(
            key_bindings_
        );
    }

    void run_legacy_command(
        openswd3::compat::u8,
        std::string_view
    ) override {}

private:
    openswd3::input_time_rng::LegacyKeyBindingBlock& key_bindings_;
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
        bool& destroy_requested,
        openswd3::audio_video::LegacySndArchive& snd_archive,
        openswd3::platform_sdl3::SdlLegacySampleBackend& sample_backend,
        openswd3::audio_video::LegacySampleManager& sample_manager,
        openswd3::audio_video::LegacySequenceManager& sequence_manager,
        openswd3::audio_video::LegacyStreamManager& stream_manager
    )
        : renderer_(renderer),
          texture_(texture),
          destroy_requested_(destroy_requested),
          snd_archive_(snd_archive),
          sample_backend_(sample_backend),
          sample_manager_(sample_manager),
          sequence_manager_(sequence_manager),
          stream_manager_(stream_manager) {}

    bool initialize_input_backend() override { return true; }
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

        const auto manager_status = sample_manager_.initialize_pool(
            output.sample_handle_count
        );
        if (manager_status != openswd3::audio_video::
                LegacySampleManagerInitializeStatus::ready) {
            openswd3::diagnostics::log_error(
                "sample output: sample handle pool initialization failed"
            );
            return;
        }

        openswd3::diagnostics::log_info(
            std::string{"sample output initialized: "} +
            std::to_string(output.selected_format.sample_rate) + " Hz, " +
            std::to_string(output.selected_format.bits_per_sample) +
            "-bit, handles=" +
            std::to_string(output.sample_handle_count)
        );
    }
    openswd3::app::BackendToken query_audio_driver() override {
        return sample_backend_.driver_token();
    }
    void initialize_midi_output(
        const openswd3::app::BackendToken driver
    ) override {
        const auto status = sequence_manager_.initialize_output(
            static_cast<openswd3::compat::u32>(driver)
        );
        if (status != openswd3::audio_video::
                LegacySequenceManagerInitializeStatus::ready) {
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
        if (status != openswd3::audio_video::
                LegacyStreamManagerInitializeStatus::ready) {
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
        : backend_state_(backend_state),
          backend_ports_(backend_ports),
          resource_databases_(resource_databases),
          glyph_provider_(glyph_provider),
          text_renderers_(text_renderers),
          framebuffer_(framebuffer),
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
    bool initialize_software_drawing() override {
        return glyph_provider_.valid();
    }
    void report_software_drawing_failure() override {
        openswd3::diagnostics::log_error("legacy glyph atlas is invalid");
    }
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
    void initialize_render_resources() override {
        for (const auto point_size :
             openswd3::rendering::kLegacyTextRendererPointSizes) {
            if (text_renderers_.rebuild(
                    point_size,
                    framebuffer_,
                    glyph_provider_
                ) != openswd3::rendering::
                         LegacyTextRendererRuntimeStatus::completed) {
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
        : dialog_(dialog),
          initialization_state_(initialization_state),
          initialization_ports_(initialization_ports),
          key_bindings_(key_bindings),
          game_initialized_(game_initialized),
          destroy_requested_(destroy_requested) {}

    void play_startup_sound() override {}
    void initialize_default_key_bindings() override {
        openswd3::input_time_rng::initialize_default_key_bindings(
            key_bindings_
        );
    }

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
        : window_(window),
          renderer_(renderer),
          texture_(texture),
          primary_surface_(primary_surface),
          game_framebuffer_(game_framebuffer),
          glyph_provider_(glyph_provider),
          text_renderers_(text_renderers),
          stream_manager_(stream_manager),
          sample_manager_(sample_manager),
          audio_maintenance_(audio_maintenance),
          frame_interval_(frame_interval),
          backend_available_(backend_available),
          ok_(ok),
          running_(running) {}

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
                point_size,
                game_framebuffer_,
                glyph_provider_
            ) != openswd3::rendering::
                     LegacyTextRendererRuntimeStatus::completed) {
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
        openswd3::audio_video::LegacySampleManager& sample_manager
    )
        : text_renderers_(text_renderers),
          stream_manager_(stream_manager),
          sample_manager_(sample_manager) {}

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
        } else if (
            operation == Operation::suspend_audio_output_00485710
        ) {
            static_cast<void>(
                openswd3::audio_video::stop_legacy_stream(stream_manager_)
            );
        } else if (
            operation == Operation::suspend_audio_streams_00485740
        ) {
            static_cast<void>(
                openswd3::audio_video::stop_all_legacy_samples(
                    sample_manager_
                )
            );
        }
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

private:
    openswd3::rendering::LegacyTextRendererRuntime& text_renderers_;
    openswd3::audio_video::LegacyStreamManager& stream_manager_;
    openswd3::audio_video::LegacySampleManager& sample_manager_;
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
      public openswd3::world_map::LegacyWorldRoleExternalPorts,
      public openswd3::world_map::LegacyWorldSpatialAudioPorts,
      public openswd3::world_map::LegacyWorldOuterFramePorts {
public:
    SdlDeferredWorldFramePorts(
        openswd3::audio_video::LegacyAudioMaintenancePorts& audio,
        openswd3::rendering::LegacyPresentationPorts& presentation
    ) noexcept : audio_(audio), presentation_(presentation) {}

    bool complete_role_path(openswd3::compat::u32) noexcept override {
        return false;
    }

    bool query_service(openswd3::compat::u32) noexcept override {
        return false;
    }

    bool query_control(openswd3::compat::u32) noexcept override {
        return false;
    }

    bool execute_stage(
        openswd3::world_map::LegacyWorldFrameStage
    ) noexcept override {
        ++deferred_frame_stage_count_;
        return true;
    }

    void draw_decorated_number(
        openswd3::compat::i32,
        openswd3::compat::i32,
        openswd3::compat::u32,
        openswd3::compat::u32
    ) noexcept override {}

    void play_positional_sample(
        openswd3::compat::u16,
        openswd3::compat::i32,
        openswd3::compat::i32
    ) noexcept override {}

    const openswd3::asset_runtime::LegacyActionRecord* resolve_overlay_action(
        openswd3::compat::u32
    ) noexcept override {
        return nullptr;
    }

    void emit_role_particles(
        openswd3::compat::i32,
        openswd3::compat::i32,
        openswd3::compat::u16
    ) noexcept override {}

    std::span<const openswd3::compat::u8> resolve_label_bytes(
        openswd3::compat::u32
    ) noexcept override {
        return {};
    }

    openswd3::compat::u16 label_color(
        openswd3::compat::u32
    ) noexcept override {
        return 0U;
    }

    void draw_label(
        std::span<const openswd3::compat::u8>,
        openswd3::compat::i32,
        openswd3::compat::i32,
        openswd3::compat::u16,
        openswd3::compat::u32
    ) noexcept override {}

    void play_sample(
        openswd3::compat::u16,
        openswd3::compat::i32,
        openswd3::compat::i32,
        openswd3::compat::i32
    ) noexcept override {}

    void stop_sample(openswd3::compat::u16) noexcept override {}

    void set_sample_volume(
        openswd3::compat::u16,
        openswd3::compat::i32
    ) noexcept override {}

    void set_sample_pan(
        openswd3::compat::u16,
        openswd3::compat::i32
    ) noexcept override {}

    bool execute_stage(
        const openswd3::world_map::LegacyWorldOuterFrameStageRequest&
    ) noexcept override {
        ++deferred_outer_stage_count_;
        return true;
    }

    void maintain_audio() noexcept override {
        service_audio(audio_);
    }

    void request_world_presentation() noexcept override {
        const auto result = openswd3::rendering::submit_legacy_presentation(
            openswd3::rendering::LegacyPresentationSite::steady_world,
            presentation_
        );
        presentation_failed_ =
            result.status != openswd3::rendering::
                                 LegacyPresentationDispatchStatus::completed;
    }

    [[nodiscard]] bool presentation_failed() const noexcept {
        return presentation_failed_;
    }

    [[nodiscard]] openswd3::compat::u32 deferred_frame_stage_count()
        const noexcept {
        return deferred_frame_stage_count_;
    }

    [[nodiscard]] openswd3::compat::u32 deferred_outer_stage_count()
        const noexcept {
        return deferred_outer_stage_count_;
    }

private:
    openswd3::audio_video::LegacyAudioMaintenancePorts& audio_;
    openswd3::rendering::LegacyPresentationPorts& presentation_;
    openswd3::compat::u32 deferred_frame_stage_count_{};
    openswd3::compat::u32 deferred_outer_stage_count_{};
    bool presentation_failed_{};
};

class SdlSmokeIdlePorts final
    : public openswd3::app::IdleRuntimePorts,
      public openswd3::app::FramePreparationPorts,
      public openswd3::app::FrameRuntimePorts,
      public openswd3::rendering::LegacyPresentationPorts,
      public openswd3::audio_video::LegacyVideoFramePorts {
public:
    SdlSmokeIdlePorts(
        SDL_Renderer& renderer,
        SDL_Texture*& texture,
        openswd3::rendering::LegacyFramebuffer& game_framebuffer,
        openswd3::rendering::LegacyFramebuffer& primary_surface,
        const openswd3::compat::u32& frame_interval,
        openswd3::app::WindowEventState& window_state,
        const openswd3::app::DisplayLifecycleState& display_state,
        openswd3::app::FramePreparationState& frame_preparation_state,
        openswd3::app::FrameCoordinatorState& frame_coordinator_state,
        openswd3::input_time_rng::LegacyInputNormalizationState& input_state,
        openswd3::input_time_rng::LegacyMouseState& mouse_state,
        openswd3::platform_sdl3::SdlMouseDeviceState& mouse_device_state,
        openswd3::audio_video::LegacyAudioMaintenancePorts& audio_maintenance,
        openswd3::audio_video::LegacyVideoPlayer& video_player,
        openswd3::resource_io::LegacyResourceDatabases& resource_databases,
        std::filesystem::path data_directory,
        std::filesystem::path world_cache_directory,
        const openswd3::rendering::LegacyPixelConversionState&
            pixel_conversion,
        openswd3::world_map::LegacyWorldRoleActionInitializer&
            world_action_initializer,
        openswd3::asset_runtime::LegacyActionUpdater& action_updater,
        openswd3::asset_runtime::LegacyTswRuntime& tsw_runtime,
        openswd3::app::ShutdownPorts& shutdown_ports,
        openswd3::app::ProcessExitPorts& exit_ports,
        bool& ok,
        bool& running
    )
        : renderer_(renderer),
          texture_(texture),
          game_framebuffer_(game_framebuffer),
          primary_surface_(primary_surface),
          frame_interval_(frame_interval),
          window_state_(window_state),
          display_state_(display_state),
          frame_preparation_state_(frame_preparation_state),
          frame_coordinator_state_(frame_coordinator_state),
          input_state_(input_state),
          mouse_state_(mouse_state),
          mouse_device_state_(mouse_device_state),
          audio_maintenance_(audio_maintenance),
          video_player_(video_player),
          resource_databases_(resource_databases),
          data_directory_(std::move(data_directory)),
          world_cache_directory_(std::move(world_cache_directory)),
          pixel_conversion_(pixel_conversion),
          world_action_initializer_(world_action_initializer),
          action_updater_(action_updater),
          tsw_runtime_(tsw_runtime),
          world_raster_(game_framebuffer.geometry()),
          world_effects_{.pixel_conversion = pixel_conversion},
          shutdown_ports_(shutdown_ports),
          exit_ports_(exit_ports),
          ok_(ok),
          running_(running) {}

    void step_video() override {
        const auto result = video_player_.step(*this);
        if (result.status ==
            openswd3::audio_video::LegacyVideoStepStatus::completed) {
            window_state_.process_flags &=
                ~openswd3::app::kProcessVideoActive;
        }
    }
    void maintain_audio() override {
        service_audio(audio_maintenance_);
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
        return index == 9U && input_state_.mouse_inactive_flag_9;
    }

    void clear_internal_flag(const openswd3::compat::u32 index) override {
        if (index == 9U) {
            input_state_.mouse_inactive_flag_9 = false;
        }
    }

    void set_internal_flag(const openswd3::compat::u32 index) override {
        if (index == 9U) {
            input_state_.mouse_inactive_flag_9 = true;
        }
    }

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
        openswd3::input_time_rng::begin_input_normalization(
            input_state_,
            frame_preparation_state_.frame_clock.sampled_milliseconds
        );
        const auto sample =
            openswd3::platform_sdl3::sample_sdl_mouse_state(
                renderer_,
                mouse_device_state_
            );
        openswd3::input_time_rng::normalize_input_frame(
            input_state_,
            mouse_state_,
            keyboard_snapshot_,
            sample
        );
    }

    void release_display_and_world_for_battle_entry() override {}
    void close_world_map_view() override {}
    void initialize_battle(openswd3::compat::u16) override {}
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
    void update_background_music(
        openswd3::app::FrameCoordinatorState&
    ) override {}
    void step_world_interaction(
        openswd3::app::FrameCoordinatorState&
    ) override {}
    void step_world_player(openswd3::app::FrameCoordinatorState&) override {}
    void step_story(openswd3::app::FrameCoordinatorState&) override {}
    void finish_world_frame(openswd3::app::FrameCoordinatorState&) override {
        if (!active_world_session_.has_value()) {
            request_presentation(
                openswd3::rendering::LegacyPresentationSite::steady_world
            );
            return;
        }

        auto& world = *active_world_session_;
        auto& map = world.render.map_load.session;
        auto& roles = map.business.state.roles;
        SdlDeferredWorldFramePorts deferred_ports{
            audio_maintenance_,
            *this,
        };
        openswd3::asset_runtime::LegacyActionDrawRuntimePorts action_ports{
            action_updater_,
            tsw_runtime_,
            game_framebuffer_,
            world_raster_,
            world_effects_,
            world_jitter_,
        };
        openswd3::world_map::LegacyWorldRoleRenderRuntimePorts role_ports{
            tsw_runtime_,
            game_framebuffer_,
            world_raster_,
            world_effects_,
            deferred_ports,
        };
        const auto result = openswd3::world_map::run_legacy_world_frame(
            game_framebuffer_,
            world_raster_,
            world.render.background_source(),
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
            {
                deferred_ports,
                action_ports,
                role_ports,
                deferred_ports,
            },
            deferred_ports
        );
        if (result.status != openswd3::world_map::
                                 LegacyWorldFrameCoordinatorStatus::completed ||
            deferred_ports.presentation_failed()) {
            std::string message{"ordinary world frame failed: status="};
            message.append(std::to_string(static_cast<unsigned>(result.status)));
            message.append(", inner_status=");
            message.append(std::to_string(
                static_cast<unsigned>(result.frame.status)
            ));
            static_cast<void>(report_error(message));
            ok_ = false;
            running_ = false;
            return;
        }
        if (!deferred_world_stage_notice_logged_) {
            std::string message{
                "ordinary world frame is live with deferred stages: inner="
            };
            message.append(std::to_string(
                deferred_ports.deferred_frame_stage_count()
            ));
            message.append(", outer=");
            message.append(std::to_string(
                deferred_ports.deferred_outer_stage_count()
            ));
            openswd3::diagnostics::log_info(message);
            deferred_world_stage_notice_logged_ = true;
        }
    }
    void prepare_special_mode_objects(
        openswd3::app::FrameCoordinatorState&
    ) override {}
    openswd3::app::StandardSpecialModeEvent step_standard_special_mode(
        openswd3::app::FrameCoordinatorState&
    ) override {
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
        initial_menu_phase_ = phase;
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
        active_world_session_.reset();
        world_audio_distances_.clear();
        world_audio_vertical_offsets_.clear();

        const auto payload_load = resource_databases_.reload_maps_payload();
        if (payload_load.status !=
            openswd3::resource_io::LegacyMapsPayloadStatus::ready) {
            std::string message{"initial world: MAPS payload reload failed: "};
            message.append(std::to_string(
                static_cast<unsigned>(payload_load.status)
            ));
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
            message.append(std::to_string(
                static_cast<unsigned>(decoded.status)
            ));
            static_cast<void>(report_error(message));
            ok_ = false;
            running_ = false;
            return false;
        }

        auto loaded = openswd3::world_map::load_legacy_world_runtime_session(
            payload,
            {
                .archive_path = data_directory_ / "huge.lmf",
                .cache_directory = world_cache_directory_,
                .load = decoded.database.initial_load,
                .cache_limit_megabytes = 60U,
                .pixel_conversion = pixel_conversion_,
            },
            world_action_initializer_
        );
        if (loaded.status != openswd3::world_map::
                                 LegacyWorldRuntimeSessionStatus::ready) {
            std::string message{"initial world: session load failed: status="};
            message.append(std::to_string(
                static_cast<unsigned>(loaded.status)
            ));
            message.append(", maps_status=");
            message.append(std::to_string(
                static_cast<unsigned>(loaded.maps_database_status)
            ));
            message.append(", render_status=");
            message.append(std::to_string(
                static_cast<unsigned>(loaded.render_status)
            ));
            static_cast<void>(report_error(message));
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
        world_raster_ = game_framebuffer_.geometry();
        world_jitter_ = {};
        world_frame_state_ = {};
        world_frame_state_.map_id = world.logical_map_id;
        world_frame_state_.player_role_index = world.selected_role_index;
        world_frame_state_.company_role_count = 1U;
        world_frame_state_.selection_scroll.saved_left = world.camera.left;
        world_frame_state_.selection_scroll.saved_top = world.camera.top;
        world_frame_state_.tile_animation = {
            .cycle_counter = 1,
            .cycle_interval = std::max(
                static_cast<openswd3::compat::i32>(world.map_descriptor.field_08),
                1
            ),
            .frame_count = map.header.layers,
            .frame_index = 0U,
            .frame_direction = 1,
            .tile_layer_stride = map.header.width * map.header.height,
            .tile_layer_offset = 0U,
        };
        world_frame_state_.frame_runtime.spatial_audio = {
            .controlled_role_index = world.selected_role_index,
            .mix_level = 0,
            .distance_by_role = world_audio_distances_,
            .vertical_offset_by_role = world_audio_vertical_offsets_,
        };
        openswd3::world_map::initialize_legacy_world_player_position_history(
            world_frame_state_.player_post_frame,
            map.business.state.roles[world.selected_role_index]
        );
        deferred_world_stage_notice_logged_ = false;

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

    void set_high_priority_submode(
        const openswd3::compat::u32 value
    ) override {
        high_priority_submode_ = value;
    }

    void set_high_priority_auxiliary(
        const openswd3::compat::u32 value
    ) override {
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
            static_cast<void>(report_error(
                "framebuffer presentation: missing texture"
            ));
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

    std::span<openswd3::compat::u16>
    video_destination_pixels() override {
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
            window_state_,
            shutdown_ports_,
            exit_ports_
        );
    }

private:
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
            static_cast<void>(report_error(
                "framebuffer presentation: invalid legacy request"
            ));
            ok_ = false;
            running_ = false;
        }
        return result.status ==
            openswd3::rendering::LegacyPresentationDispatchStatus::completed;
    }

    SDL_Renderer& renderer_;
    SDL_Texture*& texture_;
    openswd3::rendering::LegacyFramebuffer& game_framebuffer_;
    openswd3::rendering::LegacyFramebuffer& primary_surface_;
    const openswd3::compat::u32& frame_interval_;
    openswd3::app::WindowEventState& window_state_;
    const openswd3::app::DisplayLifecycleState& display_state_;
    openswd3::app::FramePreparationState& frame_preparation_state_;
    openswd3::app::FrameCoordinatorState& frame_coordinator_state_;
    openswd3::input_time_rng::LegacyKeyboardSnapshot keyboard_snapshot_{};
    openswd3::input_time_rng::LegacyInputNormalizationState& input_state_;
    openswd3::input_time_rng::LegacyMouseState& mouse_state_;
    openswd3::platform_sdl3::SdlMouseDeviceState& mouse_device_state_;
    openswd3::audio_video::LegacyAudioMaintenancePorts& audio_maintenance_;
    openswd3::audio_video::LegacyVideoPlayer& video_player_;
    openswd3::resource_io::LegacyResourceDatabases& resource_databases_;
    std::filesystem::path data_directory_;
    std::filesystem::path world_cache_directory_;
    openswd3::rendering::LegacyPixelConversionState pixel_conversion_;
    openswd3::world_map::LegacyWorldRoleActionInitializer&
        world_action_initializer_;
    openswd3::asset_runtime::LegacyActionUpdater& action_updater_;
    openswd3::asset_runtime::LegacyTswRuntime& tsw_runtime_;
    openswd3::rendering::LegacyRasterGeometryState world_raster_;
    openswd3::rendering::LegacyBlitEffectState world_effects_;
    openswd3::rendering::LegacyRleRowJitterState world_jitter_;
    std::optional<openswd3::world_map::LegacyWorldRuntimeSession>
        active_world_session_;
    std::optional<openswd3::world_map::LegacyWorldSpecialFrameLoader>
        world_special_frame_loader_;
    openswd3::world_map::LegacyWorldFrameCoordinatorState world_frame_state_;
    std::vector<openswd3::compat::i16> world_audio_distances_;
    std::vector<openswd3::compat::i16> world_audio_vertical_offsets_;
    std::array<openswd3::compat::i16, 1U> world_selection_words_{
        std::bit_cast<openswd3::compat::i16>(
            openswd3::world_map::kLegacyWorldSelectionSentinel
        )
    };
    bool deferred_world_stage_notice_logged_{};
    openswd3::app::ShutdownPorts& shutdown_ports_;
    openswd3::app::ProcessExitPorts& exit_ports_;
    openswd3::compat::u32 accumulated_play_time_{};
    openswd3::compat::u32 play_time_origin_{};
    openswd3::compat::u32 initial_menu_phase_{};
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
    SmokeCommandLinePorts command_line_ports(input_state.key_bindings);
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

    const std::filesystem::path glyph_atlas_path = executable_directory
        / "assets" / "fonts" / "legacy-glyph-atlas.bin";
    const std::vector<openswd3::compat::u8> glyph_atlas_bytes =
        read_binary_file(glyph_atlas_path);
    openswd3::rendering::LegacyGlyphAtlasProvider glyph_provider(
        glyph_atlas_bytes
    );
    if (!glyph_provider.valid()) {
        return report_error(
            std::string{"legacy glyph atlas: cannot load valid asset: "}
            + glyph_atlas_path.string()
        );
    }
    openswd3::diagnostics::log_info(
        std::string{"legacy glyph atlas loaded: "}
        + std::to_string(glyph_atlas_bytes.size()) + " bytes"
    );

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
    openswd3::rendering::LegacyFramebuffer framebuffer;
    openswd3::rendering::LegacyTextRendererRuntime text_renderers;
    openswd3::input_time_rng::LegacyMouseState mouse_state{};
    openswd3::platform_sdl3::SdlMouseDeviceState mouse_device_state{};
    openswd3::platform_sdl3::SdlLegacySampleBackend sample_backend;
    openswd3::audio_video::LegacySndArchive snd_archive;
    openswd3::audio_video::LegacySampleManager sample_manager(
        sample_backend,
        snd_archive
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
        sequence_manager,
        stream_manager
    );
    openswd3::audio_video::LegacyAudioQueueCoordinator audio_queue(
        audio_queue_ports
    );
    SdlLegacyAudioMaintenancePorts audio_maintenance(
        audio_queue,
        stream_manager,
        sequence_manager,
        sample_manager
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
            *renderer,
            external_launch_ports,
            [] {}
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
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    openswd3::rendering::LegacyFramebuffer primary_surface(
        framebuffer.geometry().surface
    );
    openswd3::rendering::LegacyPixelConversionState pixel_conversion;
    openswd3::rendering::select_legacy_pixel_conversion(
        pixel_conversion,
        {0xF800U, 0x07E0U, 0x001FU}
    );
    openswd3::asset_runtime::LegacyActRuntime act_runtime{
        data_directory.directory
    };
    act_runtime.set_cache_limit(
        openswd3::asset_runtime::kLegacyActCacheBytes
    );
    openswd3::asset_runtime::LegacyActActionStreamProvider action_provider{
        act_runtime
    };
    openswd3::asset_runtime::LegacyActionUpdater action_updater{
        action_provider
    };
    openswd3::world_map::LegacyWorldActionUpdaterInitializer
        world_action_initializer{action_updater};
    openswd3::asset_runtime::LegacyTswRuntime tsw_runtime{
        data_directory.directory,
        pixel_conversion
    };
    tsw_runtime.set_cache_limit(
        openswd3::asset_runtime::kLegacyMaximumTswCacheBytes
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
    bool ok = true;
    bool running = true;
    SmokeWindowEventPorts window_ports(
        framebuffer,
        pixel_conversion,
        audio_maintenance,
        video_player
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
    SmokeShutdownPorts shutdown_ports(
        text_renderers,
        stream_manager,
        sample_manager
    );

    SdlProcessExitPorts exit_ports(running);
    if (runtime_ready &&
        !present_framebuffer(*renderer, *texture, primary_surface)) {
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
        video_player,
        resource_databases,
        data_directory.directory,
        executable_directory / "cache" / "maps",
        pixel_conversion,
        world_action_initializer,
        action_updater,
        tsw_runtime,
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
    SDL_DestroyWindow(window);
    SDL_Quit();
    return ok ? 0 : 1;
}
