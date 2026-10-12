#include "openswd3/battle/legacy_battle_selection_hint_frame.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <ranges>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using Call = openswd3::battle::LegacyBattleSelectionHintFrameCall;
using Reply = openswd3::battle::LegacyBattleSelectionHintFrameCallReply;
using Request = openswd3::battle::LegacyBattleSelectionHintFrameCallRequest;

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(const u32 action_id, const u32, const bool) override {
        constexpr std::array<u16, 8> kWords{
            0x5246U, 0x0077U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
        };
        bytes.clear();
        for (const u16 word : kWords) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
        if (action_id == failing_action_id) {
            return {};
        }
        return {
            openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            bytes,
            false,
        };
    }

    std::vector<u8> bytes;
    u32 failing_action_id{0xFFFFFFFFU};
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    FrameProvider() {
        for (std::size_t index = 0U; index < storage.size(); ++index) {
            const u16 color = static_cast<u16>(0x6200U + index);
            storage[index] = {
                static_cast<u8>(color), static_cast<u8>(color >> 8U)
            };
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        if (fail || piece_index >= storage.size()) {
            piece = {};
            return false;
        }
        piece = {
            .source =
                {
                    .bytes = storage[piece_index],
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    std::array<std::vector<u8>, 9> storage;
    std::vector<u32> resource_ids;
    bool fail{};
};

class HintPort final
    : public openswd3::battle::LegacyBattleSelectionHintFramePort {
public:
    void reply(const Call call, const Reply& value) {
        replies[call].push_back(value);
    }

    [[nodiscard]] Reply
    invoke_selection_hint_frame(const Request& request) override {
        calls.push_back(request);
        if (on_call) {
            on_call(request);
        }

        auto& index = reply_indices[request.call];
        const auto found = replies.find(request.call);
        if (found == replies.end() || index >= found->second.size()) {
            return {};
        }
        return found->second[index++];
    }

    [[nodiscard]] std::vector<Request> calls_of(const Call call) const {
        std::vector<Request> selected;
        std::ranges::copy_if(
            calls,
            std::back_inserter(selected),
            [call](const Request& request) { return request.call == call; }
        );
        return selected;
    }

    std::vector<Request> calls;
    std::function<void(const Request&)> on_call;
    std::map<Call, std::vector<Reply>> replies;
    std::map<Call, std::size_t> reply_indices;
};

struct Fixture {
    Fixture() : raster(framebuffer.geometry()), action_updater(action_streams) {
        party_source.fill(0U);
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            8>>();
        shared_request.target_height = 3;
        shared_request.opacity_step = 15;
    }

    [[nodiscard]]
    openswd3::battle::LegacyBattleSelectionHintFrameBindings bindings() {
        return {
            .state = state,
            .startup = startup,
            .queued_actor_code = queued_actor_code,
            .party_source_words = party_source,
            .target_selection_block = target_selection_block,
            .published_actor_code = published_actor_code,
            .group_b_count = group_b_count,
            .mirror_mode = mirror_mode,
            .panel_action_record = panel_action_record,
            .framebuffer = framebuffer,
            .clip = clip,
            .raster = raster,
            .shared_request = shared_request,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
        };
    }

    u32 queued_actor_code{8U};
    std::array<u32, 0x32> party_source{};
    u32 target_selection_block{};
    u32 published_actor_code{2U};
    u32 group_b_count{8U};
    u32 mirror_mode{};
    openswd3::battle::LegacyBattleSelectionHintFrameState state;
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::asset_runtime::LegacyActionRecord panel_action_record;
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    openswd3::rendering::LegacyBlitClipRectangle clip{0, 0, 640, 480};
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitRequest shared_request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    ActionStreamProvider action_streams;
    openswd3::asset_runtime::LegacyActionUpdater action_updater;
    FrameProvider frame_provider;
    HintPort port;
};

[[nodiscard]] openswd3::battle::LegacyBattleSelectionHintFrameRequest
request() {
    return {
        .origin_x = 12U,
        .origin_y = 14U,
        .entry_eax = 0x11111111U,
        .entry_ecx = 0x22222222U,
        .entry_edx = 0x33333333U,
        .local_text_token = 0x70001000U,
        .local_current_token = 0x70001014U,
        .local_limit_token = 0x70001018U,
        .panel_rectangle_return_registers =
            {
                .eax = 0xAAAA0001U,
                .ecx = 0xBBBB0002U,
                .edx = 0xCCCC0003U,
            },
        .tiled_frame_return_registers =
            {
                .eax = 0xDDDD0001U,
                .ecx = 0xDDDD0002U,
                .edx = 0xDDDD0003U,
            },
        .color_fade_return_registers = {
            .eax = 0xEEEE0001U,
            .ecx = 0xEEEE0002U,
            .edx = 0xEEEE0003U,
        },
    };
}

void prepare_label_and_metric(Fixture& fixture, const u32 metric) {
    fixture.port.reply(
        Call::query_actor_label, {.eax = 0x00501000U, .text_length = 4U}
    );
    fixture.port.reply(Call::configure_font_width, {});
    fixture.port.reply(Call::draw_text, {});
    fixture.port.reply(Call::configure_font_width, {});
    (*fixture.startup.group_b_lifecycle)[fixture.published_actor_code - 1U]
        .action_execution.profile_value = 0x900U;
    fixture.port.object_words[0U][1U] = (metric << 16U) | 0x900U;
}

void test_enemy_profile_reads(openswd3::test::Context& test) {
    using Status = openswd3::battle::LegacyBattleSelectionHintFrameStatus;
    using Path = openswd3::battle::LegacyBattleFixedCountPath;
    constexpr std::array<u16, 7> profiles{
        0U, 1U, 0x001CU, 0x7FFFU, 0x8000U, 0xBEEFU, 0xFFFFU
    };

    for (u32 selected = 1U; selected <= 8U; ++selected) {
        for (const u16 profile : profiles) {
            Fixture fixture;
            fixture.published_actor_code = selected;
            prepare_label_and_metric(fixture, 9U);
            for (u32 index = 0U; index < 8U; ++index) {
                auto& actor = (*fixture.startup.group_b_lifecycle)[index]
                                  .action_execution;
                actor.profile_value = static_cast<u16>(0x3300U + index);
                actor.profile_value_read_accessible = index == selected - 1U;
            }

            (*fixture.startup.group_b_lifecycle)[selected - 1U]
                .action_execution.profile_value = profile;
            fixture.port.object_words[0U][1U] = (9U << 16U) | profile;
            const auto root_before = fixture.port.object_words;
            const auto result =
                openswd3::battle::draw_legacy_battle_selection_hint_frame(
                    fixture.bindings(), fixture.port, request()
                );
            bool actors_unchanged = true;
            for (u32 index = 0U; index < 8U; ++index) {
                const auto& actor = (*fixture.startup.group_b_lifecycle)[index]
                                        .action_execution;
                actors_unchanged = actors_unchanged &&
                    actor.profile_value ==
                        (index == selected - 1U
                             ? profile
                             : static_cast<u16>(0x3300U + index)) &&
                    actor.profile_value_read_accessible ==
                        (index == selected - 1U);
            }

            test.expect_true(
                result.status == Status::completed && result.label_drawn &&
                    result.actor_code == selected &&
                    result.metric_value == 9U &&
                    result.fixed_count_lookup.path == Path::existing_root &&
                    result.return_eax == 9U &&
                    result.return_ecx ==
                        openswd3::battle::kLegacyBattleFixedCountOwnerToken &&
                    result.return_edx == profile && !result.metric_text_drawn &&
                    !result.fade_drawn &&
                    fixture.port.object_words == root_before &&
                    fixture.port.fixed_count_nodes.empty() && actors_unchanged,
                "enemy hint reads the selected actual unsigned profile WORD after the name prefix without changing actors or count storage"
            );
        }
    }

    constexpr std::array<u16, 6> quantities{0U, 9U, 10U, 14U, 15U, 0xFFFFU};
    for (const u16 quantity : quantities) {
        for (u32 storage = 0U; storage < 3U; ++storage) {
            Fixture fixture;
            prepare_label_and_metric(fixture, quantity);
            auto& actor =
                (*fixture.startup.group_b_lifecycle)[1U].action_execution;
            actor.profile_value = 0xBEEFU;
            if (storage == 0U) {
                fixture.port.object_words[0U][1U] =
                    (static_cast<u32>(quantity) << 16U) | 0xBEEFU;
            } else {
                fixture.port.object_words[0U][1U] = 0U;
                const auto inserted =
                    openswd3::battle::accumulate_legacy_battle_fixed_count(
                        fixture.port, storage == 1U ? 0xBEEFU : 0x1234U, 1U
                    );
                test.expect_true(
                    inserted.path == Path::allocated_node,
                    "hint threshold fixture owns an actual allocated count node"
                );
                fixture.port.fixed_count_nodes.back().words[1U] =
                    (static_cast<u32>(quantity) << 16U) |
                    (storage == 1U ? 0xBEEFU : 0x1234U);
            }

            fixture.port.reply(
                Call::query_metric_pair,
                {.metric_current = 12U, .metric_limit = 34U}
            );
            fixture.port.reply(Call::draw_text, {});
            fixture.startup.enemies[1U].progress.progress = 291U;
            fixture.port.reply(Call::query_fade_color, {.eax = 0xA5A51234U});
            const auto root_before = fixture.port.object_words;
            const auto result =
                openswd3::battle::draw_legacy_battle_selection_hint_frame(
                    fixture.bindings(), fixture.port, request()
                );
            const u16 observed = storage == 2U ? 0U : quantity;
            test.expect_true(
                result.status == Status::completed &&
                    result.metric_value == observed &&
                    result.fixed_count_lookup.path ==
                        (storage == 0U       ? Path::existing_root
                             : storage == 1U ? Path::existing_node
                                             : Path::none) &&
                    result.metric_text_drawn == (observed >= 10U) &&
                    result.fade_drawn == (observed >= 15U) &&
                    result.fade_width == (observed >= 15U ? 20U : 0U) &&
                    fixture.port.object_words == root_before &&
                    fixture.port.fixed_count_nodes.size() ==
                        (storage == 0U ? 0U : 1U),
                "hint actual root, owned node and missing key preserve unsigned ten and fifteen thresholds including FFFF"
            );
        }
    }

    for (u32 predecessor = 0U; predecessor < 2U; ++predecessor) {
        Fixture fixture;
        prepare_label_and_metric(fixture, 0U);
        fixture.port.object_words[0U][1U] = (9U << 16U) | 0x8001U;
        fixture.port.on_call = [&](const Request& call) {
            if ((predecessor == 0U && call.call == Call::draw_text) ||
                (predecessor == 1U && call.call == Call::configure_font_width &&
                 call.arguments[0U] == 16U)) {
                fixture.published_actor_code = 8U;
                (*fixture.startup.group_b_lifecycle)[7U]
                    .action_execution.profile_value = 0x8001U;
                (*fixture.startup.group_b_lifecycle)[1U]
                    .action_execution.profile_value_read_accessible = false;
            }
        };

        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status == Status::completed && result.actor_code == 2U &&
                result.actor_token == 0x00528030U && result.label_drawn &&
                result.font_width_calls == 2U && result.metric_value == 9U &&
                result.return_edx == 0x8001U &&
                fixture.published_actor_code == 8U,
            "name drawing and font restoration can change the live selection and profile while the original name panel remains committed"
        );
    }

    for (u32 gate = 0U; gate < 6U; ++gate) {
        Fixture fixture;
        fixture.startup.group_b_lifecycle.reset();
        if (gate == 0U) {
            fixture.party_source[0U] = 1U;
        } else if (gate == 1U) {
            fixture.target_selection_block = 1U;
        } else if (gate == 2U) {
            fixture.published_actor_code = 0U;
        } else if (gate == 3U) {
            fixture.published_actor_code = 0x80000000U;
        } else if (gate == 4U) {
            fixture.group_b_count = 1U;
        } else {
            fixture.group_b_count = 0x80000000U;
        }

        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status == Status::completed && fixture.port.calls.empty() &&
                !result.label_drawn &&
                result.fixed_count_lookup.path == Path::none,
            "full-value entry gates and signed selection guards return before any profile storage access"
        );
    }

    for (const u32 accessible : std::array<u32, 2>{4U, 6U}) {
        Fixture fixture;
        prepare_label_and_metric(fixture, 0U);
        (*fixture.startup.group_b_lifecycle)[1U]
            .action_execution.profile_value = 0xBEEFU;
        fixture.port.object_words[0U][1U] = 0U;
        const auto inserted =
            openswd3::battle::accumulate_legacy_battle_fixed_count(
                fixture.port, 0xBEEFU, 1U
            );
        fixture.port.fixed_count_nodes.back().accessible_bytes = accessible;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status == Status::fixed_count_typed_stop &&
                result.fixed_count_lookup.stopped_token ==
                    inserted.matched_token &&
                result.fixed_count_lookup.stopped_offset == accessible &&
                result.label_drawn && result.font_width_calls == 2U &&
                result.metric_pair_calls == 0U && !result.fade_drawn &&
                fixture.port.fixed_count_nodes.size() == 1U,
            "actual count node key and quantity access failures keep the name prefix and owned node while blocking information suffixes"
        );
    }

    for (u32 failure = 0U; failure < 5U; ++failure) {
        Fixture fixture;
        prepare_label_and_metric(fixture, 15U);
        fixture.port.on_call = [&](const Request& call) {
            if (call.call == Call::configure_font_width &&
                call.arguments[0U] == 16U) {
                if (failure == 0U) {
                    fixture.startup.group_b_lifecycle.reset();
                } else if (failure == 1U) {
                    (*fixture.startup.group_b_lifecycle)[1U]
                        .action_execution.profile_value_read_accessible = false;
                } else {
                    fixture.published_actor_code = failure == 2U ? 0U
                        : failure == 3U                          ? 9U
                                                                 : 0xFFFFFFFFU;
                }
            }
        };

        const auto root_before = fixture.port.object_words;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status == Status::group_b_actor_typed_stop &&
                result.label_drawn && result.font_width_calls == 2U &&
                result.text_draw_calls == 1U &&
                result.fixed_count_lookup.path == Path::none &&
                result.metric_pair_calls == 0U && !result.fade_drawn &&
                fixture.port.object_words == root_before,
            "profile storage, access and unmapped selection failures occur after the full name and font prefix and block count and display suffixes"
        );
    }

    for (const u32 alias :
         std::array<u32, 3>{0x20000001U, 0x80000001U, 0xE0000001U}) {
        Fixture fixture;
        prepare_label_and_metric(fixture, 0U);
        (*fixture.startup.group_b_lifecycle)[0U]
            .action_execution.profile_value = 0xBEEFU;
        fixture.port.object_words[0U][1U] = (9U << 16U) | 0xBEEFU;
        fixture.port.on_call = [&](const Request& call) {
            if (call.call == Call::configure_font_width &&
                call.arguments[0U] == 16U) {
                fixture.published_actor_code = alias;
            }
        };

        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status == Status::completed && result.actor_code == 2U &&
                result.label_drawn && result.metric_value == 9U &&
                result.return_edx == ((alias & 0xFFFF0000U) | 0xBEEFU),
            "the profile access resolves the actual enemy address after DWORD selector multiplication wraps"
        );
    }

    {
        Fixture fixture;
        prepare_label_and_metric(fixture, 0U);
        fixture.port.object_words[0U][1U] = (15U << 16U) | 0xBEEFU;
        (*fixture.startup.group_b_lifecycle)[6U]
            .action_execution.profile_value = 0xBEEFU;
        fixture.startup.enemies[2U].progress.progress = 291U;
        fixture.port.reply(
            Call::query_metric_pair,
            {.metric_current = 12U, .metric_limit = 34U}
        );
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::query_fade_color, {.eax = 0xA5A51234U});
        fixture.port.on_call = [&](const Request& call) {
            if (call.call == Call::configure_font_width &&
                call.arguments[0U] == 16U) {
                fixture.published_actor_code = 7U;
            } else if (call.call == Call::query_metric_pair) {
                test.expect_true(
                    call.object_token == 0x005357F8U,
                    "the metric pair rereads the actual selection after count lookup"
                );
                fixture.published_actor_code = 3U;
            }
        };

        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status == Status::completed && result.actor_code == 2U &&
                result.metric_value == 15U && result.metric_text_drawn &&
                result.fade_drawn && result.fade_width == 20U &&
                fixture.published_actor_code == 3U,
            "the hint retains its count threshold and original name while later pair and progress reads consume the live selected enemy"
        );
    }
}

}  // namespace

void test_battle_selection_hint_frame(openswd3::test::Context& test) {
    test_enemy_profile_reads(test);

    {
        Fixture fixture;
        fixture.party_source[0U] = 1U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        completed &&
                result.return_eax == 1U && result.return_ecx == 0U &&
                result.return_edx == 0U,
            "selection hint skips party-source mode one with post-read registers"
        );
    }

    {
        Fixture fixture;
        fixture.target_selection_block = 1U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        completed &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0U,
            "selection hint skips the complete target-selection block value one"
        );
    }

    {
        Fixture zero;
        zero.published_actor_code = 0U;
        const auto zero_result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                zero.bindings(), zero.port, request()
            );
        Fixture too_large;
        too_large.group_b_count = 1U;
        const auto too_large_result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                too_large.bindings(), too_large.port, request()
            );
        test.expect_true(
            zero_result.return_ecx == 0U && too_large_result.return_ecx == 2U,
            "selection hint preserves signed one-based published-code guards"
        );
    }

    {
        Fixture fixture;
        fixture.queued_actor_code = 0U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        party_source_typed_stop &&
                result.return_eax == 0x11111111U && result.return_ecx == 0U &&
                result.return_edx == 0xFFFFFFD8U,
            "selection hint stops at the first invalid five-dword party-source access"
        );
    }

    {
        Fixture fixture;
        fixture.published_actor_code = 9U;
        fixture.group_b_count = 9U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        group_b_actor_typed_stop &&
                result.actor_label_query_calls == 0U &&
                result.return_eax == 3105U && result.return_edx == 0U,
            "selection hint permits count nine through the legacy guard then stops at the first actor call"
        );
    }

    {
        Fixture fixture;
        prepare_label_and_metric(fixture, 9U);
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        const auto labels = fixture.port.calls_of(Call::query_actor_label);
        const auto widths = fixture.port.calls_of(Call::configure_font_width);
        const auto draws = fixture.port.calls_of(Call::draw_text);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        completed &&
                result.panel_x == 12U && result.panel_y == 14U &&
                result.label_character_count == 2U && result.label_drawn &&
                !result.metric_text_drawn && !result.fade_drawn &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U &&
                fixture.panel_action_record.action_id == 0x233BU &&
                fixture.panel_action_record.base_variant == 0U &&
                !fixture.frame_provider.resource_ids.empty() &&
                fixture.frame_provider.resource_ids.front() == 0xBBBB0077U,
            "selection hint draws the nonmirrored name panel and stops below metric threshold ten"
        );
        test.expect_true(
            labels.size() == 1U && labels[0U].eax == 690U &&
                labels[0U].edx == 0U && widths.size() == 2U &&
                widths[0U].arguments[0U] == 20U &&
                widths[1U].arguments[0U] == 16U && draws.size() == 1U &&
                draws[0U].arguments[1U] == 14U &&
                draws[0U].arguments[2U] == 14U &&
                draws[0U].eax ==
                    openswd3::battle::kLegacyBattleSelectionHintFontToken &&
                draws[0U].edx == 14U,
            "selection hint preserves actor scaling, width twenty then sixteen and label draw registers"
        );
        test.expect_true(
            result.metric_value == 9U &&
                result.fixed_count_lookup.quantity == 9U &&
                result.fixed_count_lookup.path ==
                    openswd3::battle::LegacyBattleFixedCountPath::existing_root,
            "selection hint below ten retains the actual quantity without drawing metric text"
        );
    }

    {
        Fixture fixture;
        fixture.mirror_mode = 1U;
        fixture.port.reply(
            Call::query_actor_label, {.eax = 0x00501000U, .text_length = 6U}
        );
        fixture.port.reply(Call::configure_font_width, {});
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::configure_font_width, {});
        (*fixture.startup.group_b_lifecycle)[1U]
            .action_execution.profile_value = 0x900U;
        fixture.port.object_words[0U][1U] = (10U << 16U) | 0x900U;
        fixture.port.reply(
            Call::query_metric_pair,
            {
                .publish_metric_pair = true,
                .metric_current = 123U,
                .metric_limit = 456U,
            }
        );
        fixture.port.reply(
            Call::draw_text,
            {.eax = 0xFA000001U, .ecx = 0xFA000002U, .edx = 0xFA000003U}
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        const auto draws = fixture.port.calls_of(Call::draw_text);
        const std::array<u8, 12> expected{
            0xA5U, 0xCDU, 0xA9U, 0x52U, 0x3AU, '1', '2', '3', '/', '4', '5', '6'
        };
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        completed &&
                result.panel_x == 558U && result.metric_text_drawn &&
                !result.fade_drawn && result.formatted_text_length == 12U &&
                std::equal(
                    expected.begin(),
                    expected.end(),
                    result.formatted_text.begin()
                ) &&
                draws.size() == 2U && draws[1U].arguments[1U] == 418U &&
                draws[1U].arguments[2U] == 12U &&
                draws[1U].text_token == 0x70001000U &&
                draws[1U].eax ==
                    openswd3::battle::kLegacyBattleSelectionHintFontToken &&
                draws[1U].edx == 418U && result.return_eax == 0xFA000001U,
            "selection hint mirror one centers the name and draws signed vitality text on the mirrored side"
        );
    }

    {
        Fixture fixture;
        fixture.mirror_mode = 2U;
        prepare_label_and_metric(fixture, 10U);
        fixture.port.reply(
            Call::query_metric_pair, {.metric_current = 1U, .metric_limit = 2U}
        );
        fixture.port.reply(Call::draw_text, {});
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        const auto draws = fixture.port.calls_of(Call::draw_text);
        test.expect_true(
            result.panel_x == 12U && draws.size() == 2U &&
                draws[1U].arguments[1U] == 0xFFFFFF80U,
            "selection hint preserves mirror-equals-one name placement but mirror-nonzero metric placement"
        );
    }

    {
        Fixture fixture;
        prepare_label_and_metric(fixture, 15U);
        fixture.port.reply(
            Call::query_metric_pair,
            {.metric_current = 12U, .metric_limit = 34U}
        );
        fixture.port.reply(Call::draw_text, {});
        fixture.startup.enemies[1U].progress.progress = 291U;
        fixture.port.reply(Call::query_fade_color, {.eax = 0xA5A51234U});
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        const auto colors = fixture.port.calls_of(Call::query_fade_color);
        const auto reserved_widths =
            fixture.port.calls_of(Call::reserved_query_fade_width);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        completed &&
                result.metric_text_drawn && result.fade_drawn &&
                result.fade_width == 20U && result.fade_color == 0xA5A51234U &&
                result.color_fade_calls == 1U && colors.size() == 1U &&
                reserved_widths.empty() &&
                result.actor_progress_width.scaled_width == 20 &&
                colors[0U].arguments[0U] == 0U &&
                colors[0U].arguments[1U] == 0U &&
                colors[0U].arguments[2U] == 24U &&
                fixture.state.color_fade.source_argument_slot ==
                    std::array<u8, 4>{0x34U, 0x12U, 0xA5U, 0xA5U} &&
                result.return_eax == 0xEEEE0001U &&
                result.return_ecx == 0xEEEE0002U &&
                result.return_edx == 0xEEEE0003U,
            "selection hint threshold fifteen draws the queried-width color fade and returns its configured registers"
        );
    }

    {
        Fixture fixture;
        prepare_label_and_metric(fixture, 15U);
        fixture.port.reply(
            Call::query_metric_pair, {.metric_current = 1U, .metric_limit = 2U}
        );
        fixture.port.reply(Call::draw_text, {});
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            !result.fade_drawn && result.fade_color_calls == 0U &&
                result.return_eax == 0U && result.return_ecx == 0x00528030U &&
                result.return_edx == 0U &&
                result.actor_progress_width.scaled_width == 0,
            "selection hint zero fade width skips color and fade while retaining width-query registers"
        );
    }

    {
        Fixture fixture;
        prepare_label_and_metric(fixture, 15U);
        fixture.port.reply(
            Call::query_metric_pair, {.metric_current = 1U, .metric_limit = 2U}
        );
        fixture.port.reply(Call::draw_text, {});
        fixture.startup.enemies[1U].progress.progress = 60U;
        fixture.startup.enemies[1U].progress.progress_read_accessible = false;

        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );

        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        actor_progress_width_typed_stop &&
                result.metric_text_drawn && !result.fade_drawn &&
                result.fade_color_calls == 0U &&
                fixture.port.calls_of(Call::reserved_query_fade_width).empty(),
            "selection hint stops at the typed group-B progress read after metric text"
        );
    }

    {
        Fixture fixture;
        prepare_label_and_metric(fixture, 10U);
        fixture.port.reply(
            Call::query_metric_pair,
            {.metric_current = 0x80000000U, .metric_limit = 0x80000000U}
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        format_buffer_typed_stop &&
                result.formatted_text_length == 20U &&
                result.text_draw_calls == 1U && !result.fade_drawn,
            "selection hint formats signed decimals and stops at the first write beyond its twenty-byte stack buffer"
        );
    }

    {
        Fixture fixture;
        fixture.port.reply(
            Call::query_actor_label, {.eax = 0x00501000U, .text_length = 4U}
        );
        fixture.port.reply(Call::configure_font_width, {});
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::configure_font_width, {});
        (*fixture.startup.group_b_lifecycle)[1U]
            .action_execution.profile_value = 0x900U;
        fixture.port.object_words[0U][0U] = 0x78001234U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        fixed_count_typed_stop &&
                result.label_drawn && result.font_width_calls == 2U &&
                result.fixed_count_lookup.stopped_token == 0x78001234U &&
                result.metric_pair_calls == 0U && !result.fade_drawn &&
                result.fixed_count_lookup.stopped_offset == 4U,
            "selection hint blocks metric text and fade when the fixed-count successor is unmapped"
        );
    }

    {
        Fixture fixture;
        fixture.raster.surface.pitch_bytes = 1;
        prepare_label_and_metric(fixture, 9U);
        fixture.startup.group_b_lifecycle.reset();
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        panel_rectangle_typed_stop &&
                result.panel_action_update_calls == 1U &&
                result.tiled_frame_calls == 0U && result.font_width_calls == 0U,
            "selection hint panel rectangle stop preserves only the actor and action-update prefix"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.fail = true;
        prepare_label_and_metric(fixture, 9U);
        fixture.startup.group_b_lifecycle.reset();
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        tiled_frame_typed_stop &&
                result.tiled_frame_calls == 1U && result.font_width_calls == 0U,
            "selection hint tiled frame stop blocks all text and metrics"
        );
    }

    {
        Fixture fixture;
        fixture.action_streams.failing_action_id = 0x233BU;
        prepare_label_and_metric(fixture, 9U);
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        completed &&
                result.panel_action_update.return_value == 0U &&
                result.tiled_frame_calls == 1U && result.label_drawn,
            "selection hint action update failure remains non-branching"
        );
    }

    {
        Fixture fixture;
        prepare_label_and_metric(fixture, 15U);
        fixture.port.reply(
            Call::query_metric_pair, {.metric_current = 1U, .metric_limit = 2U}
        );
        fixture.port.reply(Call::draw_text, {});
        fixture.startup.enemies[1U].progress.progress = 291U;
        fixture.port.reply(Call::query_fade_color, {.eax = 0x0000FFFFU});
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        color_fade_typed_stop &&
                result.color_fade_calls == 1U && !result.fade_drawn &&
                result.return_eax == 0xEEEE0001U,
            "selection hint propagates the closed color-fade unsupported-source stop after all prior calls"
        );
    }
}
