#include "openswd3/story_scene/legacy_dialog_text.hpp"

#include "test.hpp"

#include <array>
#include <span>
#include <string>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::story_scene::LegacyDialogTextTokenKind;
using openswd3::story_scene::LegacyDialogTextTokenStatus;
using openswd3::story_scene::LegacyDialogChoiceBackgroundRequest;
using openswd3::story_scene::LegacyDialogMessage;
using openswd3::story_scene::LegacyDialogSegmentDrawRequest;
using openswd3::story_scene::LegacyDialogTextFrameInput;
using openswd3::story_scene::LegacyDialogTextFrameStatus;
using openswd3::story_scene::LegacyDialogTextPorts;
using openswd3::story_scene::kLegacyDialogFlagClosing;
using openswd3::story_scene::kLegacyDialogFlagHasChoices;
using openswd3::story_scene::kLegacyDialogFlagPageBoundary;
using openswd3::story_scene::kLegacyDialogFlagTerminated;
using openswd3::story_scene::next_legacy_dialog_text_token;
using openswd3::story_scene::update_legacy_dialog_text;

struct DrawCall {
    int x{};
    int y{};
    std::vector<u8> bytes;
    std::uint16_t foreground{};
    std::uint16_t secondary{};
    u8 style{};
    bool selected{};
};

class RecordingPorts final : public LegacyDialogTextPorts {
public:
    [[nodiscard]] bool draw_segment(
        const LegacyDialogSegmentDrawRequest& request
    ) noexcept override {
        draws.push_back(
            DrawCall{
                .x = request.destination_x,
                .y = request.destination_y,
                .bytes =
                    std::vector<u8>{
                        request.nul_terminated_text.begin(),
                        request.nul_terminated_text.end()
                    },
                .foreground = request.foreground_index,
                .secondary = request.secondary_index,
                .style = request.style,
                .selected = request.selected_choice,
            }
        );
        return draw_success;
    }

    void draw_selected_choice_background(
        const LegacyDialogChoiceBackgroundRequest& request
    ) noexcept override {
        backgrounds.push_back(request);
    }

    void play_choice_sound() noexcept override {
        ++sound_count;
    }

    std::vector<DrawCall> draws;
    std::vector<LegacyDialogChoiceBackgroundRequest> backgrounds;
    std::uint32_t sound_count{};
    bool draw_success{true};
};

[[nodiscard]] LegacyDialogMessage message_from(const std::span<const u8> text) {
    LegacyDialogMessage message;
    message.text.assign(text.begin(), text.end());
    message.page_stop_index = text.size();
    message.record.width = 200U;
    message.record.height = 100U;
    return message;
}

template <std::size_t Size>
[[nodiscard]] constexpr std::span<const u8>
bytes(const std::array<u8, Size>& value) noexcept {
    return std::span<const u8>{value};
}

void test_fixed_and_parameter_controls(openswd3::test::Context& test) {
    constexpr std::array<u8, 17U> text{
        '%',
        'Q',
        '%',
        'N',
        '%',
        'L',
        '%',
        'P',
        '%',
        'S',
        '7',
        '%',
        'C',
        '3',
        '%',
        'G',
        '1'
    };
    const std::array expected{
        LegacyDialogTextTokenKind::terminator,
        LegacyDialogTextTokenKind::line_break,
        LegacyDialogTextTokenKind::delayed_page_break,
        LegacyDialogTextTokenKind::page_break,
        LegacyDialogTextTokenKind::character_delay,
        LegacyDialogTextTokenKind::both_colors,
        LegacyDialogTextTokenKind::text_style,
    };
    std::size_t cursor{};
    for (const auto kind : expected) {
        const auto token = next_legacy_dialog_text_token(bytes(text), cursor);
        test.expect_true(
            token.status == LegacyDialogTextTokenStatus::completed &&
                token.kind == kind && token.next_index > cursor,
            "fixed and one-byte controls retain their source widths"
        );
        cursor = token.next_index;
    }
    test.expect_equal(cursor, text.size(), "all fixed controls consumed");
}

void test_secondary_color_spelling(openswd3::test::Context& test) {
    constexpr std::array<u8, 6U> text{'D', '%', '4', '%', 'D', '4'};
    const auto legacy = next_legacy_dialog_text_token(bytes(text), 0U);
    const auto normalized = next_legacy_dialog_text_token(bytes(text), 3U);
    test.expect_true(
        legacy.kind == LegacyDialogTextTokenKind::secondary_color &&
            legacy.parameter == static_cast<u8>('4') && legacy.next_index == 3U,
        "the assembly's D%-ordered secondary-color command is recognized"
    );
    test.expect_true(
        normalized.kind == LegacyDialogTextTokenKind::text &&
            normalized.next_index == 4U,
        "the intuitive %D spelling is ordinary text in the legacy protocol"
    );
}

void test_choice_sound_close_and_dbcs(openswd3::test::Context& test) {
    constexpr std::array<u8, 18U> text{
        '%',
        'B',
        'Y',
        'e',
        's',
        '.',
        '%',
        'a',
        '1',
        '9',
        '2',
        '.',
        '%',
        'K',
        0xA4U,
        0x40U,
        'X',
        0U
    };
    const auto choice = next_legacy_dialog_text_token(bytes(text), 0U);
    const auto sound = next_legacy_dialog_text_token(bytes(text), 6U);
    const auto close = next_legacy_dialog_text_token(bytes(text), 12U);
    const auto dbcs = next_legacy_dialog_text_token(bytes(text), 14U);
    const auto ascii = next_legacy_dialog_text_token(bytes(text), 16U);
    test.expect_true(
        choice.kind == LegacyDialogTextTokenKind::choice &&
            choice.uppercase_variant && choice.payload.size() == 3U &&
            choice.next_index == 6U,
        "uppercase choice returns its period-delimited payload"
    );
    test.expect_true(
        sound.kind == LegacyDialogTextTokenKind::sound &&
            !sound.uppercase_variant && sound.payload.size() == 3U &&
            sound.next_index == 12U,
        "lowercase sound retains its already-consumed variant"
    );
    test.expect_true(
        close.kind == LegacyDialogTextTokenKind::close &&
            close.uppercase_variant && close.next_index == 14U,
        "uppercase close is distinguished for in-place mutation"
    );
    test.expect_true(
        dbcs.kind == LegacyDialogTextTokenKind::text &&
            dbcs.bytes.size() == 2U && dbcs.next_index == 16U &&
            ascii.bytes.size() == 1U,
        "bit-7 text bytes consume the following raw byte"
    );
}

void test_checked_malformed_boundaries(openswd3::test::Context& test) {
    constexpr std::array<u8, 2U> short_control{'%', 'S'};
    constexpr std::array<u8, 4U> missing_period{'%', 'B', 'x', 'y'};
    constexpr std::array<u8, 1U> dangling{0xA4U};
    test.expect_equal(
        next_legacy_dialog_text_token(bytes(short_control), 0U).status,
        LegacyDialogTextTokenStatus::truncated_control,
        "truncated parameter control is isolated"
    );
    test.expect_equal(
        next_legacy_dialog_text_token(bytes(missing_period), 0U).status,
        LegacyDialogTextTokenStatus::missing_period,
        "unterminated period payload is isolated"
    );
    test.expect_equal(
        next_legacy_dialog_text_token(bytes(dangling), 0U).status,
        LegacyDialogTextTokenStatus::dangling_double_byte,
        "dangling DBCS lead is isolated"
    );
    test.expect_equal(
        next_legacy_dialog_text_token({}, 0U).status,
        LegacyDialogTextTokenStatus::end_of_buffer,
        "empty spans report the checked end boundary"
    );
}

void test_incremental_raw_character_reveal(openswd3::test::Context& test) {
    constexpr std::array<u8, 4U> text{'A', 'B', '%', 'Q'};
    LegacyDialogMessage message = message_from(bytes(text));
    message.page_stop_index = 0U;
    message.record.character_delay = 4U;
    message.record.character_countdown = 0xFFFFU;
    RecordingPorts ports;

    const auto result = update_legacy_dialog_text(
        message, LegacyDialogTextFrameInput{.scale = 2}, ports
    );
    test.expect_true(
        result.status == LegacyDialogTextFrameStatus::page_limit_reached &&
            result.consumed_byte_count == 1U &&
            result.next_visible_index == 1U && message.page_stop_index == 1U &&
            message.record.character_countdown == 4U &&
            ports.draws.size() == 1U &&
            ports.draws[0].bytes == std::vector<u8>{'A', 0U},
        "negative +0x2A reveals exactly one raw character then restores +0x28"
    );
}

void test_color_style_speed_and_terminator(openswd3::test::Context& test) {
    constexpr std::array<u8, 18U> text{
        '%',
        'S',
        '2',
        '%',
        'C',
        '3',
        'H',
        'i',
        'D',
        '%',
        '4',
        '%',
        'G',
        '1',
        '%',
        'Q',
        0U,
        0U
    };
    LegacyDialogMessage message = message_from(bytes(text));
    message.record.foreground_index = 1U;
    message.record.secondary_index = 2U;
    message.record.text_style = 4U;
    RecordingPorts ports;

    const auto result = update_legacy_dialog_text(
        message,
        LegacyDialogTextFrameInput{
            .scale = 1,
            .base_character_delay = 5U,
            .current_tick = 1234U,
        },
        ports
    );
    test.expect_true(
        result.status == LegacyDialogTextFrameStatus::terminator_reached &&
            message.record.character_delay == 14U &&
            (message.record.flags & kLegacyDialogFlagTerminated) != 0U &&
            message.record.lifetime_started_at == 1234U &&
            ports.draws.size() == 4U,
        "speed, color/style flushes and %Q execute in source order"
    );
    test.expect_true(
        ports.draws[1].bytes == std::vector<u8>{'H', 'i', 0U} &&
            ports.draws[1].foreground == 3U && ports.draws[1].secondary == 3U &&
            ports.draws[3].style == 0x10U,
        "D% changes only the secondary color and %G1 selects style 0x10"
    );
}

void test_choices_sound_close_and_in_place_markers(
    openswd3::test::Context& test
) {
    constexpr std::array<u8, 23U> text{'%', 'B', 'Y', 'e', 's', '.', '%', 'B',
                                       'N', 'o', '.', '%', 'A', '1', '9', '2',
                                       '.', '%', 'K', '%', 'Q', 0U,  0U};
    LegacyDialogMessage message = message_from(bytes(text));
    message.record.left = 10U;
    message.record.top = 20U;
    RecordingPorts ports;

    const auto result = update_legacy_dialog_text(
        message,
        LegacyDialogTextFrameInput{
            .scale = 2,
            .selected_choice_index = 1,
        },
        ports
    );
    test.expect_true(
        result.status == LegacyDialogTextFrameStatus::terminator_reached &&
            result.choice_draw_count == 2U &&
            result.choice_hotspot_count == 2U && result.sound_count == 1U &&
            result.closing_requested && message.choices.size() == 2U &&
            ports.backgrounds.size() == 1U && ports.sound_count == 1U,
        "choice, fixed sound and close controls all execute before %Q"
    );
    test.expect_true(
        message.text[1] == static_cast<u8>('b') &&
            message.text[7] == static_cast<u8>('b') &&
            message.text[12] == static_cast<u8>('a') &&
            message.text[18] == static_cast<u8>('K') &&
            (message.record.flags & kLegacyDialogFlagHasChoices) != 0U &&
            (message.record.flags & kLegacyDialogFlagClosing) != 0U,
        "only %B and %A mutate to lowercase; %K remains unchanged"
    );
    test.expect_true(
        message.choices[0].left == 12U && message.choices[0].top == 20U &&
            message.choices[0].right == 18U &&
            message.choices[0].bottom == 24U &&
            ports.backgrounds[0].destination_x == 8 &&
            ports.backgrounds[0].width == 4,
        "choice rectangles use the offscreen position plus the dialog origin"
    );
}

void test_height_page_boundary_and_saved_style(openswd3::test::Context& test) {
    constexpr std::array<u8, 7U> text{'A', '%', 'N', 'B', '%', 'N', 'C'};
    LegacyDialogMessage message = message_from(bytes(text));
    message.record.height = 4U;
    message.record.foreground_index = 5U;
    message.record.secondary_index = 6U;
    message.record.text_style = 7U;
    message.record.saved_text_style = 0x80U;
    RecordingPorts ports;

    const auto result = update_legacy_dialog_text(
        message,
        LegacyDialogTextFrameInput{.scale = 1, .current_tick = 9U},
        ports
    );
    test.expect_true(
        result.status == LegacyDialogTextFrameStatus::page_boundary_reached &&
            result.consumed_byte_count == 6U && message.page_stop_index == 6U &&
            (message.record.flags & kLegacyDialogFlagPageBoundary) != 0U &&
            message.record.saved_foreground_index == 5U &&
            message.record.saved_secondary_index == 6U &&
            message.record.saved_text_style == 0x87U,
        "the second line break fills a four-pixel text surface and OR-saves style"
    );
}

void test_width_boundary_preserves_page_stop(openswd3::test::Context& test) {
    constexpr std::array<u8, 3U> text{'A', '%', 'Q'};
    LegacyDialogMessage message = message_from(bytes(text));
    message.record.width = 0U;
    message.record.height = 2U;
    RecordingPorts ports;

    const auto result = update_legacy_dialog_text(
        message, LegacyDialogTextFrameInput{.scale = 1}, ports
    );
    test.expect_true(
        result.status == LegacyDialogTextFrameStatus::page_boundary_reached &&
            result.consumed_byte_count == 0U &&
            message.page_stop_index == text.size(),
        "a width-only full-page boundary does not replace +0x40 without fast mode"
    );
}

void test_delayed_page_break_does_not_reload_countdown(
    openswd3::test::Context& test
) {
    constexpr std::array<u8, 4U> text{'%', 'L', '%', 'Q'};
    LegacyDialogMessage message = message_from(bytes(text));
    message.page_stop_index = 0U;
    message.record.character_delay = 9U;
    message.record.character_countdown = 0xFFFFU;
    RecordingPorts ports;

    const auto result =
        update_legacy_dialog_text(message, LegacyDialogTextFrameInput{}, ports);
    test.expect_true(
        result.status == LegacyDialogTextFrameStatus::page_limit_reached &&
            message.page_stop_index == 2U &&
            message.record.character_countdown == 0xFFFFU,
        "%L advances the delayed boundary without reloading +0x2A"
    );
}

void test_fixed_control_buffer_boundaries(openswd3::test::Context& test) {
    std::vector<u8> oversized_choice{'%', 'B'};
    oversized_choice.insert(oversized_choice.end(), 256U, static_cast<u8>('X'));
    oversized_choice.push_back(static_cast<u8>('.'));
    LegacyDialogMessage choice_message = message_from(oversized_choice);
    RecordingPorts ports;
    const auto choice = update_legacy_dialog_text(
        choice_message, LegacyDialogTextFrameInput{}, ports
    );
    test.expect_equal(
        choice.status,
        LegacyDialogTextFrameStatus::segment_buffer_overflow,
        "the original 256-byte choice stack buffer has a checked boundary"
    );

    std::vector<u8> oversized_sound{'%', 'a'};
    oversized_sound.insert(oversized_sound.end(), 16U, static_cast<u8>('1'));
    oversized_sound.push_back(static_cast<u8>('.'));
    LegacyDialogMessage sound_message = message_from(oversized_sound);
    const auto sound = update_legacy_dialog_text(
        sound_message, LegacyDialogTextFrameInput{}, ports
    );
    test.expect_equal(
        sound.status,
        LegacyDialogTextFrameStatus::segment_buffer_overflow,
        "the original 16-byte sound stack buffer has a checked boundary"
    );
}

void test_prefix_replacement(openswd3::test::Context& test) {
    using openswd3::story_scene::LegacyDialogTextReplaceStatus;
    using openswd3::story_scene::replace_legacy_dialog_text_prefix;
    using Status = LegacyDialogTextReplaceStatus;
    constexpr std::array<u8, 8U> initial{'A', 'B', 'C', 'D', 0U, 'x', 'y', 'z'};
    constexpr std::array<u8, 3U> pattern{'A', 'B', 0U};
    struct Vector {
        std::array<u8, 4U> replacement;
        std::array<u8, 8U> expected;
    };
    constexpr std::array<Vector, 4U> vectors{{
        {{'X', 'Y', 'Z', 0U}, {'X', 'Y', 'Z', 'C', 'D', 0U, 'y', 'z'}},
        {{'Q', 'R', 0U, 0U}, {'Q', 'R', 'C', 'D', 0U, 'x', 'y', 'z'}},
        {{'Q', 0U, 0U, 0U}, {'Q', 'C', 'D', 0U, 'x', 'y', 'y', 'z'}},
        {{0U, 0U, 0U, 0U}, {'C', 'D', 0U, 'x', 'y', 'z', 'y', 'z'}},
    }};
    for (const auto& vector : vectors) {
        auto bytes = initial;
        const auto result = replace_legacy_dialog_text_prefix(
            bytes, pattern, vector.replacement, 6U
        );
        test.expect_true(
            result == Status::replaced && bytes == vector.expected,
            "40BAA0 moves the fixed extent including bytes after NUL in either overlap direction"
        );
    }

    auto bytes = initial;
    constexpr std::array<u8, 1U> empty{0U};
    constexpr std::array<u8, 2U> mismatch{'Z', 0U};
    test.expect_true(
        replace_legacy_dialog_text_prefix(bytes, empty, {}, 0U) ==
                Status::no_match &&
            replace_legacy_dialog_text_prefix(bytes, mismatch, {}, 0U) ==
                Status::no_match &&
            bytes == initial,
        "empty or mismatched pattern does not inspect the replacement"
    );
    constexpr std::array<u8, 4U> longer{'X', 'Y', 'Z', 0U};
    test.expect_true(
        replace_legacy_dialog_text_prefix(bytes, pattern, longer, 2U) ==
                Status::memory_access_typed_stop &&
            bytes == initial,
        "replacement length exceeding limit wraps the original move size and stops at the mapped boundary"
    );
    test.expect_true(
        replace_legacy_dialog_text_prefix(bytes, pattern, empty, 7U) ==
                Status::memory_access_typed_stop &&
            bytes == initial,
        "source extent is checked independently of the destination extent"
    );
    constexpr std::array<u8, 2U> unterminated{'X', 'Y'};
    test.expect_true(
        replace_legacy_dialog_text_prefix(bytes, pattern, unterminated, 6U) ==
                Status::memory_access_typed_stop &&
            replace_legacy_dialog_text_prefix(bytes, {}, empty, 6U) ==
                Status::memory_access_typed_stop &&
            replace_legacy_dialog_text_prefix(
                std::span<u8>{bytes}.first(1U), pattern, empty, 0U
            ) == Status::memory_access_typed_stop &&
            bytes == initial,
        "unterminated replacement and unmapped pattern or destination reads stop before writes"
    );
    test.expect_true(
        replace_legacy_dialog_text_prefix(bytes, pattern, empty, 0U) ==
                Status::replaced &&
            bytes == initial,
        "a matched prefix with zero move and replacement lengths succeeds without writes"
    );
}

void test_preparation_metrics(openswd3::test::Context& test) {
    using openswd3::story_scene::measure_legacy_dialog_prepared_text;
    constexpr std::array<u8, 2U> terminator{'%', 'Q'};
    constexpr std::array<u8, 2U> empty{0U, 0U};
    constexpr std::array<u8, 10U> breaks{
        '%', 'N', '%', 'L', '%', 'K', '%', 'P', '%', 'Q'
    };
    constexpr std::array<u8, 15U> controls{
        '%',
        'S',
        '4',
        '%',
        'C',
        '2',
        '%',
        'D',
        '1',
        '%',
        'B',
        '%',
        'A',
        '%',
        'Q'
    };
    constexpr std::array<u8, 5U> renderer_spelling{'D', '%', '1', '%', 'Q'};
    test.expect_true(
        measure_legacy_dialog_prepared_text(terminator) == 0x00010000U &&
            measure_legacy_dialog_prepared_text(empty) == 0x00010001U &&
            measure_legacy_dialog_prepared_text(breaks) == 0x00050000U &&
            measure_legacy_dialog_prepared_text(controls) == 0x00010000U &&
            measure_legacy_dialog_prepared_text(renderer_spelling) ==
                0x00010003U,
        "preparation retains the initial NUL counting quirk and its distinct percent-D spelling"
    );
    std::array<u8, 24U> wrapped{};
    wrapped.fill('A');
    wrapped[20U] = '%';
    wrapped[21U] = 'Q';
    test.expect_true(
        measure_legacy_dialog_prepared_text(wrapped) == 0x00010014U,
        "twenty visible bytes still occupy the first measured line"
    );
    wrapped[20U] = 'A';
    wrapped[21U] = '%';
    wrapped[22U] = 'Q';
    test.expect_true(
        measure_legacy_dialog_prepared_text(wrapped) == 0x00020015U,
        "the twenty-first byte increments the measured line count"
    );
    constexpr std::array<u8, 3U> truncated{'%', 'S', '4'};
    test.expect_true(
        !measure_legacy_dialog_prepared_text({}).has_value() &&
            !measure_legacy_dialog_prepared_text(truncated).has_value() &&
            !measure_legacy_dialog_prepared_text(
                 std::span<const u8>{empty}.first(1U)
            )
                 .has_value(),
        "counter stops on missing initial WORD or post-control byte reads"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_fixed_and_parameter_controls(test);
    test_secondary_color_spelling(test);
    test_choice_sound_close_and_dbcs(test);
    test_checked_malformed_boundaries(test);
    test_incremental_raw_character_reveal(test);
    test_color_style_speed_and_terminator(test);
    test_choices_sound_close_and_in_place_markers(test);
    test_height_page_boundary_and_saved_style(test);
    test_width_boundary_preserves_page_stop(test);
    test_delayed_page_break_does_not_reload_countdown(test);
    test_fixed_control_buffer_boundaries(test);
    test_prefix_replacement(test);
    test_preparation_metrics(test);
    return test.exit_code();
}
