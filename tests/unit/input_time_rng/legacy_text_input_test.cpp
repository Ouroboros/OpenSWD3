#include "test.hpp"

#include "openswd3/input_time_rng/legacy_text_input.hpp"
#include "openswd3/resource_io/legacy_dbcs_text_buffer.hpp"

#include <array>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u32;
using openswd3::input_time_rng::LegacyTextInputDriverState;
using openswd3::resource_io::LegacyDbcsTextBuffer;

class RecordingPorts final
    : public openswd3::input_time_rng::LegacyTextInputPorts {
public:
    bool is_ime_keyboard_layout(const u32 keyboard_layout) override {
        queried_layouts.push_back(keyboard_layout);
        return ime_layout_result;
    }

    void play_sound_effect(const u32 sound_id) override {
        sound_ids.push_back(sound_id);
    }

    bool ime_layout_result{};
    std::vector<u32> queried_layouts;
    std::vector<u32> sound_ids;
};

template <std::size_t Size>
[[nodiscard]] std::array<u8, Size> copy_bytes(LegacyDbcsTextBuffer& buffer) {
    std::array<u8, Size> bytes{};
    bytes.fill(0xCCU);
    static_cast<void>(buffer.copy_to(bytes.data(), static_cast<i32>(Size)));
    return bytes;
}

[[nodiscard]] u32 dispatch(
    LegacyDbcsTextBuffer& buffer,
    LegacyTextInputDriverState& state,
    RecordingPorts& ports,
    const u32 message,
    const u32 first_parameter = 0U,
    const u32 second_parameter = 0U
) {
    return openswd3::input_time_rng::filter_legacy_text_input_message(
        buffer, state, message, first_parameter, second_parameter, ports
    );
}

void test_cursor_helpers_and_enable_bug(openswd3::test::Context& test) {
    constexpr std::array<u8, 6> kText{
        0xA9U,
        0x67U,
        0x41U,
        0xA5U,
        0x69U,
        0x00U,
    };
    LegacyDbcsTextBuffer buffer{kText.data(), 8, 0, 0};

    test.expect_equal(
        openswd3::input_time_rng::legacy_move_text_cursor_previous(buffer),
        0U,
        "cursor previous returns zero at the beginning"
    );
    test.expect_equal(
        openswd3::input_time_rng::legacy_move_text_cursor_next(buffer),
        1U,
        "cursor next crosses a CP950 pair"
    );
    test.expect_equal(
        buffer.cursor_byte_offset(), 2, "cursor reaches byte two"
    );
    static_cast<void>(
        openswd3::input_time_rng::legacy_move_text_cursor_next(buffer)
    );
    test.expect_equal(buffer.cursor_byte_offset(), 3, "cursor crosses ASCII");
    test.expect_equal(
        openswd3::input_time_rng::legacy_move_text_cursor_previous(buffer),
        1U,
        "cursor previous returns one away from byte zero"
    );
    test.expect_equal(
        buffer.cursor_byte_offset(), 2, "cursor returns to byte two"
    );
    test.expect_equal(
        openswd3::input_time_rng::legacy_move_text_cursor_previous(buffer),
        0U,
        "moving onto byte zero returns zero despite changing the cursor"
    );
    test.expect_equal(
        buffer.cursor_byte_offset(), 0, "cursor reaches byte zero"
    );

    test.expect_equal(
        openswd3::input_time_rng::legacy_set_text_input_enabled(buffer, 0),
        1,
        "enable setter returns one"
    );
    test.expect_equal(
        openswd3::input_time_rng::legacy_text_input_enabled(buffer),
        1,
        "enable setter ignores zero and keeps the original always-on bug"
    );
}

void test_key_dispatch(openswd3::test::Context& test) {
    constexpr std::array<u8, 6> kText{
        0xA9U,
        0x67U,
        0x41U,
        0xA5U,
        0x69U,
        0x00U,
    };
    LegacyDbcsTextBuffer buffer{kText.data(), 8, 0, 0};
    LegacyTextInputDriverState state;
    RecordingPorts ports;

    static_cast<void>(dispatch(
        buffer,
        state,
        ports,
        openswd3::input_time_rng::kLegacyKeyDownMessage,
        0x23U
    ));
    test.expect_equal(
        buffer.cursor_byte_offset(), 5, "End reaches the NUL byte"
    );
    static_cast<void>(dispatch(
        buffer,
        state,
        ports,
        openswd3::input_time_rng::kLegacyKeyDownMessage,
        0x25U
    ));
    test.expect_equal(
        buffer.cursor_byte_offset(), 3, "Left crosses one CP950 pair"
    );
    static_cast<void>(dispatch(
        buffer,
        state,
        ports,
        openswd3::input_time_rng::kLegacyKeyDownMessage,
        0x27U
    ));
    test.expect_equal(
        buffer.cursor_byte_offset(), 5, "Right restores the end offset"
    );
    static_cast<void>(dispatch(
        buffer,
        state,
        ports,
        openswd3::input_time_rng::kLegacyKeyDownMessage,
        0x24U
    ));
    test.expect_equal(buffer.cursor_byte_offset(), 0, "Home reaches byte zero");

    static_cast<void>(dispatch(
        buffer,
        state,
        ports,
        openswd3::input_time_rng::kLegacyKeyDownMessage,
        0x0DU
    ));
    test.expect_equal(buffer.result(), 1, "Enter stores result one");
    static_cast<void>(dispatch(
        buffer,
        state,
        ports,
        openswd3::input_time_rng::kLegacyKeyDownMessage,
        0x1BU
    ));
    test.expect_equal(buffer.result(), 2, "Escape stores result two");

    static_cast<void>(dispatch(
        buffer,
        state,
        ports,
        openswd3::input_time_rng::kLegacyKeyDownMessage,
        0x2DU
    ));
    test.expect_equal(
        openswd3::input_time_rng::legacy_text_input_enabled(buffer),
        1,
        "Insert cannot toggle the always-on field"
    );
    test.expect_equal(
        dispatch(buffer, state, ports, 0x9999U, 1U, 2U),
        1U,
        "unknown messages return one"
    );
}

void test_delete_and_backspace_bug(openswd3::test::Context& test) {
    constexpr std::array<u8, 5> kTwoCharacters{
        0xA9U,
        0x67U,
        0xA5U,
        0x69U,
        0x00U,
    };
    LegacyTextInputDriverState state;
    RecordingPorts ports;

    LegacyDbcsTextBuffer backspace{kTwoCharacters.data(), 8, 0, 0};
    static_cast<void>(dispatch(
        backspace,
        state,
        ports,
        openswd3::input_time_rng::kLegacyKeyDownMessage,
        0x08U
    ));
    constexpr std::array<u8, 9> kSecondOnly{
        0xA5U,
        0x69U,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U,
    };
    test.expect_equal(
        copy_bytes<9>(backspace),
        kSecondOnly,
        "Backspace at byte zero deletes the first character"
    );
    test.expect_equal(
        backspace.cursor_byte_offset(),
        0,
        "Backspace bug leaves the cursor at zero"
    );

    LegacyDbcsTextBuffer deletion{kTwoCharacters.data(), 8, 0, 0};
    static_cast<void>(
        openswd3::input_time_rng::legacy_move_text_cursor_next(deletion)
    );
    static_cast<void>(
        openswd3::input_time_rng::legacy_delete_text_at_cursor(deletion)
    );
    constexpr std::array<u8, 9> kFirstOnly{
        0xA9U,
        0x67U,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U,
    };
    test.expect_equal(
        copy_bytes<9>(deletion),
        kFirstOnly,
        "Delete shifts the overlapping suffix including the terminator"
    );
    static_cast<void>(
        openswd3::input_time_rng::legacy_move_text_cursor_next(deletion)
    );
    static_cast<void>(
        openswd3::input_time_rng::legacy_delete_text_at_cursor(deletion)
    );
    test.expect_equal(
        copy_bytes<9>(deletion),
        kFirstOnly,
        "Delete at NUL copies the same region and changes nothing"
    );
}

void test_insertion_and_capacity(openswd3::test::Context& test) {
    constexpr std::array<u8, 5> kTwoCharacters{
        0xA9U,
        0x67U,
        0xA5U,
        0x69U,
        0x00U,
    };
    constexpr std::array<u8, 2> kAscii{0x58U, 0x00U};
    LegacyDbcsTextBuffer insertion{kTwoCharacters.data(), 8, 0, 0};
    static_cast<void>(
        openswd3::input_time_rng::legacy_move_text_cursor_next(insertion)
    );
    test.expect_equal(
        openswd3::input_time_rng::legacy_insert_text_bytes(
            insertion, kAscii.data()
        ),
        1U,
        "insert returns one"
    );
    constexpr std::array<u8, 9> kInserted{
        0xA9U,
        0x67U,
        0x58U,
        0xA5U,
        0x69U,
        0U,
        0U,
        0U,
        0U,
    };
    test.expect_equal(
        copy_bytes<9>(insertion),
        kInserted,
        "insert preserves prefix and complete suffix"
    );
    test.expect_equal(
        insertion.cursor_byte_offset(),
        3,
        "cursor advances by accepted inserted bytes"
    );

    constexpr std::array<u8, 3> kReplacement{0xC1U, 0xC9U, 0x00U};
    LegacyDbcsTextBuffer truncation{kTwoCharacters.data(), 4, 0, 0};
    static_cast<void>(
        openswd3::input_time_rng::legacy_move_text_cursor_next(truncation)
    );
    static_cast<void>(openswd3::input_time_rng::legacy_insert_text_bytes(
        truncation, kReplacement.data()
    ));
    constexpr std::array<u8, 5> kTruncated{
        0xA9U,
        0x67U,
        0xC1U,
        0xC9U,
        0U,
    };
    test.expect_equal(
        copy_bytes<5>(truncation),
        kTruncated,
        "full insertion truncates the old suffix at a character boundary"
    );
    test.expect_equal(
        truncation.cursor_byte_offset(),
        4,
        "full insertion moves the cursor to capacity"
    );
}

void test_language_and_ascii_character_paths(openswd3::test::Context& test) {
    constexpr std::array<u8, 1> kEmpty{0U};
    LegacyDbcsTextBuffer buffer{kEmpty.data(), 8, 0, 0};
    LegacyTextInputDriverState state;
    RecordingPorts ports;

    ports.ime_layout_result = true;
    test.expect_equal(
        dispatch(
            buffer,
            state,
            ports,
            openswd3::input_time_rng::kLegacyInputLanguageChangeMessage,
            0xAAAAAAAAU,
            0x12345678U
        ),
        1U,
        "language change returns one"
    );
    test.expect_equal(buffer.snapshot().ime_state, 1, "IME layout stores one");
    const std::vector<u32> kExpectedLayouts{0x12345678U};
    test.expect_equal(
        ports.queried_layouts,
        kExpectedLayouts,
        "language change queries only the second parameter"
    );
    static_cast<void>(dispatch(
        buffer,
        state,
        ports,
        openswd3::input_time_rng::kLegacyCharacterMessage,
        0x41U
    ));
    constexpr std::array<u8, 9> kAllZero{0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    test.expect_equal(
        copy_bytes<9>(buffer),
        kAllZero,
        "WM_CHAR is ignored while the IME flag is nonzero"
    );

    ports.ime_layout_result = false;
    static_cast<void>(dispatch(
        buffer,
        state,
        ports,
        openswd3::input_time_rng::kLegacyInputLanguageChangeMessage,
        0U,
        0x87654321U
    ));
    test.expect_equal(
        buffer.snapshot().ime_state, 0, "non-IME layout stores zero"
    );
    static_cast<void>(dispatch(
        buffer,
        state,
        ports,
        openswd3::input_time_rng::kLegacyCharacterMessage,
        0x1EU
    ));
    test.expect_equal(
        copy_bytes<9>(buffer), kAllZero, "bytes below 0x1F are ignored"
    );
    static_cast<void>(dispatch(
        buffer,
        state,
        ports,
        openswd3::input_time_rng::kLegacyCharacterMessage,
        0x25U
    ));
    const std::vector<u32> kExpectedSounds{
        openswd3::input_time_rng::kLegacyPercentInputSoundId,
    };
    test.expect_equal(
        ports.sound_ids,
        kExpectedSounds,
        "percent requests sound 0x8C instead of insertion"
    );
    test.expect_equal(
        copy_bytes<9>(buffer),
        kAllZero,
        "percent does not enter the text buffer"
    );

    static_cast<void>(dispatch(
        buffer,
        state,
        ports,
        openswd3::input_time_rng::kLegacyCharacterMessage,
        0x1FU
    ));
    static_cast<void>(dispatch(
        buffer,
        state,
        ports,
        openswd3::input_time_rng::kLegacyCharacterMessage,
        0x41U
    ));
    constexpr std::array<u8, 9> kAccepted{
        0x1FU,
        0x41U,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U,
    };
    test.expect_equal(
        copy_bytes<9>(buffer),
        kAccepted,
        "0x1F and ordinary ASCII enter in message order"
    );
}

void test_dbcs_latch_bugs(openswd3::test::Context& test) {
    constexpr std::array<u8, 1> kEmpty{0U};
    RecordingPorts ports;

    LegacyDbcsTextBuffer low_trail{kEmpty.data(), 8, 0, 0};
    LegacyTextInputDriverState low_trail_state;
    static_cast<void>(dispatch(
        low_trail,
        low_trail_state,
        ports,
        openswd3::input_time_rng::kLegacyCharacterMessage,
        0xA9U
    ));
    test.expect_equal(
        low_trail_state.dbcs_lead_byte_latch,
        1U,
        "negative byte sets the process-global DBCS latch"
    );
    static_cast<void>(dispatch(
        low_trail,
        low_trail_state,
        ports,
        openswd3::input_time_rng::kLegacyCharacterMessage,
        0x67U
    ));
    constexpr std::array<u8, 9> kNicoleFirst{
        0xA9U,
        0x67U,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U,
    };
    test.expect_equal(
        copy_bytes<9>(low_trail),
        kNicoleFirst,
        "negative lead plus positive trail is assembled by two inserts"
    );
    test.expect_equal(
        low_trail_state.dbcs_lead_byte_latch,
        0U,
        "positive trail clears the DBCS latch"
    );

    LegacyDbcsTextBuffer high_trail{kEmpty.data(), 8, 0, 0};
    LegacyTextInputDriverState high_trail_state;
    static_cast<void>(dispatch(
        high_trail,
        high_trail_state,
        ports,
        openswd3::input_time_rng::kLegacyCharacterMessage,
        0xA9U
    ));
    static_cast<void>(dispatch(
        high_trail,
        high_trail_state,
        ports,
        openswd3::input_time_rng::kLegacyCharacterMessage,
        0xE1U
    ));
    test.expect_equal(
        high_trail_state.dbcs_lead_byte_latch,
        1U,
        "negative trail is ignored and leaves the latch set"
    );
    static_cast<void>(dispatch(
        high_trail,
        high_trail_state,
        ports,
        openswd3::input_time_rng::kLegacyCharacterMessage,
        0x25U
    ));
    constexpr std::array<u8, 9> kLeadPercent{
        0xA9U,
        0x25U,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U,
    };
    test.expect_equal(
        copy_bytes<9>(high_trail),
        kLeadPercent,
        "pending lead makes percent a trail instead of a sound command"
    );
    test.expect_true(
        ports.sound_ids.empty(),
        "pending lead bypasses the percent sound branch"
    );
}

void test_ime_character_path(openswd3::test::Context& test) {
    constexpr std::array<u8, 1> kEmpty{0U};
    LegacyDbcsTextBuffer buffer{kEmpty.data(), 8, 0, 0};
    LegacyTextInputDriverState state{7U};
    RecordingPorts ports;
    ports.ime_layout_result = true;
    static_cast<void>(dispatch(
        buffer,
        state,
        ports,
        openswd3::input_time_rng::kLegacyInputLanguageChangeMessage,
        0U,
        1U
    ));

    test.expect_equal(
        dispatch(
            buffer,
            state,
            ports,
            openswd3::input_time_rng::kLegacyImeCharacterMessage,
            0xA967U
        ),
        0U,
        "WM_IME_CHAR is the sole zero-return path"
    );
    constexpr std::array<u8, 9> kExpected{
        0xA9U,
        0x67U,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U,
    };
    test.expect_equal(
        copy_bytes<9>(buffer),
        kExpected,
        "WM_IME_CHAR inserts high byte before low byte"
    );
    test.expect_equal(
        state.dbcs_lead_byte_latch,
        7U,
        "WM_IME_CHAR does not touch the process-global latch"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_cursor_helpers_and_enable_bug(test);
    test_key_dispatch(test);
    test_delete_and_backspace_bug(test);
    test_insertion_and_capacity(test);
    test_language_and_ascii_character_paths(test);
    test_dbcs_latch_bugs(test);
    test_ime_character_path(test);
    return test.exit_code();
}
