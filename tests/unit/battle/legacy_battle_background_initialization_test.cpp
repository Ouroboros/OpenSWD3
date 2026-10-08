#include "openswd3/battle/legacy_battle_background_initialization.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"

#include <array>
#include <filesystem>
#include <utility>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActionRotationCacheState;
using openswd3::battle::LegacyBattleActionRotationReleasePort;
using openswd3::battle::LegacyBattleActionRotationUpdatePort;
using openswd3::battle::LegacyBattleActionRotationUpdateSnapshot;
using openswd3::battle::LegacyBattleBackgroundImageLoadPort;
using openswd3::battle::LegacyBattleBackgroundImageLoadResult;
using openswd3::battle::LegacyBattleBackgroundInitializationRequest;
using openswd3::battle::LegacyBattleBackgroundInitializationStatus;
using openswd3::battle::LegacyBattleBackgroundState;
using openswd3::battle::LegacyBattleMutableFrameImage;
using openswd3::battle::LegacyBattleMutableFrameImagePort;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

[[nodiscard]] u16
read_u16(const std::vector<u8>& bytes, const std::size_t offset) {
    return static_cast<u16>(
        static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U)
    );
}

[[nodiscard]] LegacyBattleBackgroundImageLoadResult make_loaded_image() {
    const std::array<u8, 4> pixels{0x11U, 0x00U, 0x22U, 0x00U};
    auto encoded = openswd3::rendering::encode_legacy_image_command_stream(
        pixels, 2U, 1U, 16U
    );
    return LegacyBattleBackgroundImageLoadResult{
        .ready = encoded.status ==
            openswd3::rendering::LegacyImageCommandStreamStatus::completed,
        .width = 2U,
        .height = 1U,
        .image_size = static_cast<u32>(encoded.bytes.size()),
        .command_stream = std::move(encoded.bytes),
    };
}

class TrackingBackgroundLoadPort final
    : public LegacyBattleBackgroundImageLoadPort {
public:
    [[nodiscard]] LegacyBattleBackgroundImageLoadResult load_image(
        const std::filesystem::path& archive_path,
        const u32 one_based_resource,
        const u32 variant_index
    ) override {
        ++calls;
        last_path = archive_path;
        last_resource = one_based_resource;
        last_variant = variant_index;
        image_was_released_before_load = observed_background != nullptr &&
            observed_background->image.empty();
        if (events != nullptr) {
            events->push_back(3U);
        }
        return std::move(next);
    }

    LegacyBattleBackgroundState* observed_background{};
    std::vector<u32>* events{};
    LegacyBattleBackgroundImageLoadResult next{};
    std::filesystem::path last_path;
    u32 last_resource{};
    u32 last_variant{};
    u32 calls{};
    bool image_was_released_before_load{};
};

class TrackingRotationReleasePort final
    : public LegacyBattleActionRotationReleasePort {
public:
    void release_image(const u32 image_token) noexcept override {
        released_images.push_back(image_token);
        if (events != nullptr) {
            events->push_back(1U);
        }
    }

    void release_owner(const u32 owner_token) noexcept override {
        released_owners.push_back(owner_token);
        if (events != nullptr) {
            events->push_back(2U);
        }
    }

    std::vector<u32>* events{};
    std::vector<u32> released_images;
    std::vector<u32> released_owners;
};

class TrackingActionUpdatePort final
    : public LegacyBattleActionRotationUpdatePort {
public:
    [[nodiscard]] LegacyBattleActionRotationUpdateSnapshot update_action(
        openswd3::asset_runtime::LegacyActionRecord& record
    ) override {
        ++calls;
        last_action_id = record.action_id;
        if (events != nullptr) {
            events->push_back(4U);
        }
        return snapshot;
    }

    std::vector<u32>* events{};
    LegacyBattleActionRotationUpdateSnapshot snapshot{};
    u32 calls{};
    u32 last_action_id{};
};

class RejectingFrameImagePort final : public LegacyBattleMutableFrameImagePort {
public:
    [[nodiscard]] LegacyBattleMutableFrameImage
    query_frame_image(const u32, const u32) override {
        ++calls;
        return {.typed_stop = typed_stop};
    }

    u32 calls{};
    bool typed_stop{};
};

struct Ports {
    std::vector<u32> events;
    TrackingBackgroundLoadPort loader;
    TrackingRotationReleasePort releaser;
    TrackingActionUpdatePort updater;
    RejectingFrameImagePort images;

    explicit Ports(LegacyBattleBackgroundState& background) {
        loader.observed_background = &background;
        loader.events = &events;
        releaser.events = &events;
        updater.events = &events;
    }
};

}  // namespace

void test_battle_background_initialization(openswd3::test::Context& test) {
    const openswd3::rendering::LegacyPixelConversionState pixel_conversion;

    for (const bool indexed : {false, true}) {
        LegacyBattleBackgroundState background;
        LegacyBattleActionRotationCacheState rotation_cache;
        Ports ports{background};
        ports.loader.next = make_loaded_image();
        if (indexed) {
            const std::array<u8, 2> indices{4U, 5U};
            auto encoded =
                openswd3::rendering::encode_legacy_image_command_stream(
                    indices, 2U, 1U, 8U
                );
            ports.loader.next.command_stream = std::move(encoded.bytes);
            ports.loader.next.image_size =
                static_cast<u32>(ports.loader.next.command_stream.size());
            ports.loader.next.has_palette = true;
            ports.loader.next.palette[4U] = 0x0011U;
            ports.loader.next.palette[5U] = 0x0022U;
        }

        ports.loader.next.width = 9U;
        ports.loader.next.height = 7U;
        const auto result =
            openswd3::battle::initialize_legacy_battle_background(
                background,
                rotation_cache,
                ports.loader,
                ports.releaser,
                ports.updater,
                ports.images,
                pixel_conversion,
                {.data_root = "game-data",
                 .one_based_resource = 1U,
                 .rotation_divisor = 0}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleBackgroundInitializationStatus::
                        rotation_division_by_zero &&
                background.image_record[0U] != 0U &&
                background.image_record[0U] ==
                    background.image_allocation_token &&
                background.image_record[1U] == 0U &&
                background.image_record[2U] == 0U &&
                background.image_record[3U] ==
                    (indexed ? 0x00070009U : 0x00010002U) &&
                background.image_record[4U] == background.image.size() &&
                background.palette_allocation_token == 0U &&
                read_u16(background.image, 2U) == 2U &&
                read_u16(background.image, 4U) == 1U &&
                read_u16(background.image, 12U) == 0x0011U &&
                !result.completion_words_published,
            "word conversion rewrites the two record dimensions while indexed " "conversion retains file dimensions and replaces its image identity"
        );
        if (indexed) {
            const u32 final_allocation_size =
                (static_cast<u32>(background.image.size()) + 15U) & ~u32{15U};
            const auto adjacent_range =
                openswd3::asset_runtime::register_legacy_external_guest_bytes(
                    background.image_record[0U] + final_allocation_size, 16U
                );
            test.expect_true(
                adjacent_range != nullptr,
                "indexed conversion publishes a final-sized allocation " "rather than its temporary conversion buffer"
            );
        }
    }

    {
        LegacyBattleBackgroundState background;
        LegacyBattleActionRotationCacheState rotation_cache;
        Ports ports{background};
        ports.loader.next = make_loaded_image();
        ports.loader.next.command_stream.resize(
            ports.loader.next.command_stream.size() - 2U
        );
        ports.loader.next.image_size =
            static_cast<u32>(ports.loader.next.command_stream.size());
        ports.loader.next.width = 9U;
        ports.loader.next.height = 7U;
        auto conversion = pixel_conversion;
        conversion.forward =
            openswd3::rendering::LegacyPixelTransform::shift_whole_word_left;
        const auto result =
            openswd3::battle::initialize_legacy_battle_background(
                background,
                rotation_cache,
                ports.loader,
                ports.releaser,
                ports.updater,
                ports.images,
                conversion,
                {.data_root = "game-data",
                 .one_based_resource = 1U,
                 .rotation_divisor = 640}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleBackgroundInitializationStatus::
                        image_conversion_typed_stop &&
                result.conversion.status ==
                    openswd3::rendering::LegacyImageCommandStreamStatus::
                        source_exhausted &&
                background.image_record[0U] ==
                    background.image_allocation_token &&
                background.image_record[0U] != 0U &&
                background.image_record[3U] == 0x00010002U &&
                background.image_record[4U] == background.image.size() &&
                read_u16(background.image, 12U) == 0x0022U &&
                read_u16(background.image, 14U) == 0x0044U &&
                !result.completion_words_published && ports.updater.calls == 0U,
            "a later missing row header preserves the published image record " "and the literal pixels already converted in its actual allocation"
        );
    }

    for (const bool stopped : {false, true}) {
        LegacyBattleBackgroundState background;
        background.completion_words = {1U, 2U, 3U};
        LegacyBattleActionRotationCacheState rotation_cache;
        Ports ports{background};
        ports.loader.next = make_loaded_image();
        ports.updater.snapshot.typed_stop = stopped;
        const auto result =
            openswd3::battle::initialize_legacy_battle_background(
                background, rotation_cache, ports.loader, ports.releaser,
                ports.updater, ports.images, pixel_conversion,
                LegacyBattleBackgroundInitializationRequest{
                    .data_root = "game-data",
                    .one_based_resource = 1U,
                    .initial_action_id = 1U,
                    .rotation_divisor = 640,
                    .background_action_gate = 1U,
                }
            );
        test.expect_true(
            result.status == (stopped
                ? LegacyBattleBackgroundInitializationStatus::
                      action_rotation_cache_typed_stop
                : LegacyBattleBackgroundInitializationStatus::completed) &&
                result.completion_words_published == !stopped &&
                background.completion_words == (stopped
                    ? std::array<u16, 3>{1U, 2U, 3U}
                    : std::array<u16, 3>{0xFFFFU, 0xFFFFU, 0xFFFFU}) &&
                !background.image.empty() && ports.images.calls == 0U &&
                ports.updater.calls == 1U &&
                rotation_cache.action_record.action_id == 1U,
            "background distinguishes a stopped action call from normal zero"
        );
    }

    for (const bool unknown_edx : {false, true}) {
        LegacyBattleBackgroundState background;
        background.completion_words = {1U, 2U, 3U};
        LegacyBattleActionRotationCacheState rotation_cache;
        Ports ports{background};
        ports.loader.next = make_loaded_image();
        ports.updater.snapshot.eax = 1U;
        ports.updater.snapshot.edx = unknown_edx
            ? std::nullopt : std::optional<u32>{0U};
        ports.images.typed_stop = true;
        const auto result =
            openswd3::battle::initialize_legacy_battle_background(
                background,
                rotation_cache,
                ports.loader,
                ports.releaser,
                ports.updater,
                ports.images,
                pixel_conversion,
                LegacyBattleBackgroundInitializationRequest{
                    .data_root = "game-data",
                    .one_based_resource = 1U,
                    .initial_action_id = 1U,
                    .rotation_divisor = 640,
                    .background_action_gate = 1U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleBackgroundInitializationStatus::
                        action_rotation_cache_typed_stop &&
                result.action_rotation.status == (unknown_edx
                    ? openswd3::battle::LegacyBattleActionRotationCacheStatus::
                          action_update_edx_unavailable
                    : openswd3::battle::LegacyBattleActionRotationCacheStatus::
                          frame_query_typed_stop) &&
                ports.images.calls == (unknown_edx ? 0U : 1U) &&
                !result.completion_words_published &&
                background.completion_words == std::array<u16, 3>{1U, 2U, 3U} &&
                !background.image.empty() &&
                rotation_cache.frame_owner_tokens[0U] == 0U,
            "background retains its loaded image but stops before completion"
        );
    }

    {
        LegacyBattleBackgroundState background{
            .image_record = {0x9000U, 0U, 0U, 0x00010001U, 2U},
            .image_allocation_token = 0x9000U,
            .image = {0xAAU, 0xBBU},
            .completion_words = {1U, 2U, 3U},
        };
        LegacyBattleActionRotationCacheState rotation_cache;
        rotation_cache.frame_owner_tokens[0] = 0x11111111U;
        rotation_cache.cached_image_tokens[0] = 0x22222222U;
        Ports ports{background};
        ports.loader.next = make_loaded_image();
        ports.updater.snapshot.eax = 0U;

        const auto result =
            openswd3::battle::initialize_legacy_battle_background(
                background,
                rotation_cache,
                ports.loader,
                ports.releaser,
                ports.updater,
                ports.images,
                pixel_conversion,
                LegacyBattleBackgroundInitializationRequest{
                    .data_root = "battle-data",
                    .one_based_resource = 4U,
                    .initial_action_id = 0xABCD1234U,
                    .field_b4 = 0x10203040U,
                    .field_b8 = 0x50607080U,
                    .rotation_divisor = 640,
                    .background_action_gate = 1U,
                }
            );

        test.expect_true(
            result.status ==
                    LegacyBattleBackgroundInitializationStatus::completed &&
                result.archive_path ==
                    std::filesystem::path{"battle-data"} /
                        openswd3::battle::kLegacyBattleBackgroundArchiveName &&
                result.cache_release.image_release_calls == 1U &&
                result.cache_release.owner_release_calls == 1U &&
                result.previous_image_released &&
                result.image_load_calls == 1U && ports.loader.calls == 1U &&
                ports.loader.last_resource == 4U &&
                ports.loader.last_variant == 0U &&
                ports.loader.image_was_released_before_load &&
                result.rotation_shift == 1 &&
                result.image_rotation.status ==
                    openswd3::battle::LegacyBattleImageRotationStatus::
                        completed &&
                result.action_rotation_requested &&
                result.action_rotation.status ==
                    openswd3::battle::LegacyBattleActionRotationCacheStatus::
                        initial_action_update_stopped &&
                ports.updater.calls == 1U &&
                ports.updater.last_action_id == 0x1234U &&
                ports.images.calls == 0U &&
                ports.events == std::vector<u32>{1U, 2U, 3U, 4U} &&
                result.previous_image_release_token == 0x9000U &&
                ports.releaser.released_images ==
                    std::vector<u32>{0x22222222U} &&
                read_u16(background.image, 12U) == 0x0022U &&
                read_u16(background.image, 14U) == 0x0011U &&
                background.completion_words ==
                    std::array<u16, 3>{0xFFFFU, 0xFFFFU, 0xFFFFU} &&
                result.completion_words_published &&
                result.completion_write_order ==
                    std::array<u8, 3>{2U, 1U, 0U} &&
                result.return_value == 0xFFFFFFFFU,
            "battle background releases loads converts rotates and initializes cache in order"
        );
    }

    {
        LegacyBattleBackgroundState background{
            .image_record = {0x9001U, 0U, 0U, 0x12345678U, 1U},
            .image_allocation_token = 0x9001U,
            .image = {0xCCU},
            .completion_words = {4U, 5U, 6U},
        };
        LegacyBattleActionRotationCacheState rotation_cache;
        Ports ports{background};
        const auto result =
            openswd3::battle::initialize_legacy_battle_background(
                background,
                rotation_cache,
                ports.loader,
                ports.releaser,
                ports.updater,
                ports.images,
                pixel_conversion,
                LegacyBattleBackgroundInitializationRequest{
                    .data_root = "missing",
                    .one_based_resource = 9U,
                    .rotation_divisor = 1,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleBackgroundInitializationStatus::
                        image_load_failed &&
                result.return_value == 0U && background.image.empty() &&
                background.image_record ==
                    std::array<u32, 5>{0U, 0U, 0U, 0x12345678U, 1U} &&
                result.previous_image_release_token == 0x9001U &&
                background.image_allocation_token == 0U &&
                background.completion_words == std::array<u16, 3>{4U, 5U, 6U} &&
                !result.completion_words_published && ports.updater.calls == 0U,
            "battle background load failure returns zero before completion words"
        );
    }

    {
        LegacyBattleBackgroundState background{
            .image = {},
            .completion_words = {7U, 8U, 9U},
        };
        LegacyBattleActionRotationCacheState rotation_cache;
        Ports ports{background};
        ports.loader.next = make_loaded_image();
        const auto result =
            openswd3::battle::initialize_legacy_battle_background(
                background,
                rotation_cache,
                ports.loader,
                ports.releaser,
                ports.updater,
                ports.images,
                pixel_conversion,
                LegacyBattleBackgroundInitializationRequest{
                    .data_root = {},
                    .one_based_resource = 1U,
                    .initial_action_id = 1U,
                    .rotation_divisor = 0,
                    .background_action_gate = 1U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleBackgroundInitializationStatus::
                        rotation_division_by_zero &&
                !background.image.empty() &&
                background.completion_words == std::array<u16, 3>{7U, 8U, 9U} &&
                ports.updater.calls == 0U && !result.completion_words_published,
            "battle background zero divisor stops after conversion and before rotation"
        );
    }

    {
        LegacyBattleBackgroundState background;
        LegacyBattleActionRotationCacheState rotation_cache;
        Ports ports{background};
        ports.loader.next = make_loaded_image();
        const auto result =
            openswd3::battle::initialize_legacy_battle_background(
                background,
                rotation_cache,
                ports.loader,
                ports.releaser,
                ports.updater,
                ports.images,
                pixel_conversion,
                LegacyBattleBackgroundInitializationRequest{
                    .data_root = {},
                    .one_based_resource = 1U,
                    .initial_action_id = 0x12340000U,
                    .rotation_divisor = 640,
                    .background_action_gate = 1U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleBackgroundInitializationStatus::completed &&
                !result.action_rotation_requested &&
                ports.updater.calls == 0U &&
                result.return_value == 0xFFFFFFFFU &&
                background.completion_words ==
                    std::array<u16, 3>{0xFFFFU, 0xFFFFU, 0xFFFFU},
            "battle background action cache gate checks only action id low word"
        );
    }

#ifdef OPENSWD3_GAME_DATA_ROOT
    {
        LegacyBattleBackgroundState background;
        LegacyBattleActionRotationCacheState rotation_cache;
        Ports ports{background};
        const auto result =
            openswd3::battle::initialize_legacy_battle_background(
                background,
                rotation_cache,
                ports.releaser,
                ports.updater,
                ports.images,
                pixel_conversion,
                LegacyBattleBackgroundInitializationRequest{
                    .data_root = OPENSWD3_GAME_DATA_ROOT,
                    .one_based_resource = 1U,
                    .initial_action_id = 0U,
                    .field_b4 = 0U,
                    .field_b8 = 0U,
                    .rotation_divisor = 4,
                    .background_action_gate = 0U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleBackgroundInitializationStatus::completed &&
                result.image_load_calls == 1U &&
                result.conversion.status ==
                    openswd3::rendering::LegacyImageCommandStreamStatus::
                        completed &&
                result.rotation_shift == 160 &&
                result.image_rotation.status ==
                    openswd3::battle::LegacyBattleImageRotationStatus::
                        completed &&
                result.image_rotation.width == 640U &&
                result.image_rotation.height == 400U &&
                !background.image.empty() &&
                background.completion_words ==
                    std::array<u16, 3>{0xFFFFU, 0xFFFFU, 0xFFFFU},
            "real all map two first frame completes battle background conversion and rotation"
        );
    }
#endif
}
