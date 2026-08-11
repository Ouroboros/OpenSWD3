#include "openswd3/story_scene/legacy_dialog_text.hpp"

#include "openswd3/compat/legacy_decimal.hpp"

#include <array>
#include <bit>
#include <string_view>

namespace openswd3::story_scene {
namespace {

using compat::u8;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr std::size_t kLegacyDialogSegmentCapacity = 256U;
constexpr std::size_t kLegacyDialogSoundPayloadCapacity = 16U;

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
  return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
  return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 wrapping_add(const i32 left,
                                         const i32 right) noexcept {
  return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] constexpr i32 wrapping_multiply(const i32 left,
                                              const i32 right) noexcept {
  return from_bits(to_bits(left) * to_bits(right));
}

[[nodiscard]] constexpr u16 low_word(const i32 value) noexcept {
  return static_cast<u16>(to_bits(value) & 0xFFFFU);
}

[[nodiscard]] constexpr u8 parameter_digit(const u8 parameter) noexcept {
  return static_cast<u8>(parameter - static_cast<u8>('0'));
}

[[nodiscard]] constexpr i32 sign_extend_byte(const u8 value) noexcept {
  return value < 0x80U ? static_cast<i32>(value)
                       : static_cast<i32>(value) - 0x100;
}

[[nodiscard]] constexpr LegacyDialogTextFrameStatus malformed_status(
    const LegacyDialogTextTokenStatus status) noexcept {
  return status == LegacyDialogTextTokenStatus::end_of_buffer ||
                 status == LegacyDialogTextTokenStatus::dangling_double_byte
             ? LegacyDialogTextFrameStatus::source_out_of_bounds
             : LegacyDialogTextFrameStatus::malformed_control;
}

[[nodiscard]] constexpr bool has_bytes(const std::span<const u8> text,
                                       const std::size_t index,
                                       const std::size_t count) noexcept {
  return index <= text.size() && count <= text.size() - index;
}

[[nodiscard]] constexpr bool marker(const std::span<const u8> text,
                                    const std::size_t index,
                                    const u8 second) noexcept {
  return has_bytes(text, index, 2U) && text[index] == static_cast<u8>('%') &&
         text[index + 1U] == second;
}

[[nodiscard]] LegacyDialogTextToken fixed_token(
    const std::span<const u8> text, const std::size_t index,
    const LegacyDialogTextTokenKind kind, const std::size_t size) noexcept {
  return LegacyDialogTextToken{
      .status = LegacyDialogTextTokenStatus::completed,
      .kind = kind,
      .source_index = index,
      .next_index = index + size,
      .bytes = text.subspan(index, size),
  };
}

[[nodiscard]] LegacyDialogTextToken parameter_token(
    const std::span<const u8> text, const std::size_t index,
    const LegacyDialogTextTokenKind kind) noexcept {
  if (!has_bytes(text, index, 3U)) {
    return LegacyDialogTextToken{
        .status = LegacyDialogTextTokenStatus::truncated_control,
        .kind = kind,
        .source_index = index,
        .next_index = text.size(),
    };
  }
  LegacyDialogTextToken token = fixed_token(text, index, kind, 3U);
  token.parameter = text[index + 2U];
  return token;
}

[[nodiscard]] LegacyDialogTextToken period_payload_token(
    const std::span<const u8> text, const std::size_t index,
    const LegacyDialogTextTokenKind kind) noexcept {
  const std::size_t payload_start = index + 2U;
  for (std::size_t cursor = payload_start; cursor < text.size(); ++cursor) {
    if (text[cursor] != static_cast<u8>('.')) {
      continue;
    }
    LegacyDialogTextToken token =
        fixed_token(text, index, kind, cursor - index + 1U);
    token.payload = text.subspan(payload_start, cursor - payload_start);
    token.uppercase_variant = text[index + 1U] >= static_cast<u8>('A') &&
                              text[index + 1U] <= static_cast<u8>('Z');
    return token;
  }
  return LegacyDialogTextToken{
      .status = LegacyDialogTextTokenStatus::missing_period,
      .kind = kind,
      .source_index = index,
      .next_index = text.size(),
  };
}

} // namespace

LegacyDialogTextToken next_legacy_dialog_text_token(
    const std::span<const u8> text, const std::size_t source_index) noexcept {
  if (source_index >= text.size()) {
    return LegacyDialogTextToken{
        .status = LegacyDialogTextTokenStatus::end_of_buffer,
        .source_index = source_index,
        .next_index = source_index,
    };
  }

  if (marker(text, source_index, static_cast<u8>('Q'))) {
    return fixed_token(text, source_index,
                       LegacyDialogTextTokenKind::terminator, 2U);
  }
  if (marker(text, source_index, static_cast<u8>('N'))) {
    return fixed_token(text, source_index,
                       LegacyDialogTextTokenKind::line_break, 2U);
  }
  if (marker(text, source_index, static_cast<u8>('L'))) {
    return fixed_token(text, source_index,
                       LegacyDialogTextTokenKind::delayed_page_break, 2U);
  }
  if (marker(text, source_index, static_cast<u8>('P'))) {
    return fixed_token(text, source_index,
                       LegacyDialogTextTokenKind::page_break, 2U);
  }
  if (marker(text, source_index, static_cast<u8>('S'))) {
    return parameter_token(text, source_index,
                           LegacyDialogTextTokenKind::character_delay);
  }
  if (marker(text, source_index, static_cast<u8>('C'))) {
    return parameter_token(text, source_index,
                           LegacyDialogTextTokenKind::both_colors);
  }
  if (has_bytes(text, source_index, 2U) &&
      text[source_index] == static_cast<u8>('D') &&
      text[source_index + 1U] == static_cast<u8>('%')) {
    return parameter_token(text, source_index,
                           LegacyDialogTextTokenKind::secondary_color);
  }
  if (marker(text, source_index, static_cast<u8>('G'))) {
    return parameter_token(text, source_index,
                           LegacyDialogTextTokenKind::text_style);
  }
  if (marker(text, source_index, static_cast<u8>('B')) ||
      marker(text, source_index, static_cast<u8>('b'))) {
    return period_payload_token(text, source_index,
                                LegacyDialogTextTokenKind::choice);
  }
  if (marker(text, source_index, static_cast<u8>('A')) ||
      marker(text, source_index, static_cast<u8>('a'))) {
    return period_payload_token(text, source_index,
                                LegacyDialogTextTokenKind::sound);
  }
  if (marker(text, source_index, static_cast<u8>('K')) ||
      marker(text, source_index, static_cast<u8>('k'))) {
    LegacyDialogTextToken token = fixed_token(
        text, source_index, LegacyDialogTextTokenKind::close, 2U);
    token.uppercase_variant = text[source_index + 1U] == static_cast<u8>('K');
    return token;
  }

  const std::size_t byte_count =
      (text[source_index] & 0x80U) != 0U ? 2U : 1U;
  if (!has_bytes(text, source_index, byte_count)) {
    return LegacyDialogTextToken{
        .status = LegacyDialogTextTokenStatus::dangling_double_byte,
        .kind = LegacyDialogTextTokenKind::text,
        .source_index = source_index,
        .next_index = text.size(),
    };
  }
  return fixed_token(text, source_index, LegacyDialogTextTokenKind::text,
                     byte_count);
}

LegacyDialogTextFrameResult update_legacy_dialog_text(
    LegacyDialogMessage &message, const LegacyDialogTextFrameInput &input,
    LegacyDialogTextPorts &ports) noexcept {
  LegacyDialogTextFrameResult result;
  if (message.text_cursor_index > message.text.size() ||
      message.page_stop_index > message.text.size()) {
    result.status = LegacyDialogTextFrameStatus::source_out_of_bounds;
    return result;
  }

  auto &record = message.record;
  std::array<u8, kLegacyDialogSegmentCapacity> segment{};
  std::size_t segment_size{};
  std::size_t consumed{};
  std::size_t first_line_break_offset{};
  i32 line_position = 1;
  i32 line_index{};
  i32 choice_index = -1;
  u16 foreground = record.foreground_index;
  u16 secondary = record.secondary_index;
  u8 style = record.text_style;
  bool previous_token_was_text{};

  const auto emit = [&](const std::span<const u8> bytes_to_draw,
                        const i32 position, const bool selected) noexcept {
    const bool draw_ok = ports.draw_segment(LegacyDialogSegmentDrawRequest{
        .destination_x = wrapping_multiply(position, input.scale),
        .destination_y = wrapping_multiply(
            wrapping_multiply(line_index, 2), input.scale),
        .nul_terminated_text = bytes_to_draw,
        .foreground_index = foreground,
        .secondary_index = secondary,
        .style = selected ? static_cast<u8>(0x84U) : style,
        .selected_choice = selected,
    });
    ++result.segment_draw_count;
    if (!draw_ok) {
      ++result.segment_draw_failure_count;
    }
  };

  const auto flush_segment = [&]() noexcept {
    segment[segment_size] = 0U;
    emit(std::span<const u8>{segment.data(), segment_size + 1U},
         line_position, false);
  };

  const auto reset_after_flush = [&]() noexcept {
    line_position = wrapping_add(
        line_position, static_cast<i32>(segment_size));
    segment.fill(0U);
    segment_size = 0U;
  };

  const auto save_page_style = [&]() noexcept {
    record.saved_foreground_index = foreground;
    record.saved_secondary_index = secondary;
    record.saved_text_style =
        static_cast<u8>(record.saved_text_style | style);
  };

  const auto finish = [&](const LegacyDialogTextFrameStatus status) noexcept {
    result.status = status;
    result.consumed_byte_count = consumed;
    result.next_visible_index = message.page_stop_index;
    return result;
  };

  for (;;) {
    if (consumed > message.text.size() - message.text_cursor_index) {
      return finish(LegacyDialogTextFrameStatus::source_out_of_bounds);
    }
    const std::size_t source_index = message.text_cursor_index + consumed;
    if (source_index > message.text.size()) {
      return finish(LegacyDialogTextFrameStatus::source_out_of_bounds);
    }

    if ((record.character_countdown & 0x8000U) != 0U) {
      if (source_index > message.page_stop_index &&
          previous_token_was_text) {
        record.character_countdown = record.character_delay;
        message.page_stop_index = source_index;
        flush_segment();
        return finish(LegacyDialogTextFrameStatus::page_limit_reached);
      }
      previous_token_was_text = false;
    } else if (source_index == message.page_stop_index) {
      flush_segment();
      return finish(LegacyDialogTextFrameStatus::page_limit_reached);
    }

    const LegacyDialogTextToken token =
        next_legacy_dialog_text_token(message.text, source_index);
    if (token.status != LegacyDialogTextTokenStatus::completed) {
      result.token_status = token.status;
      return finish(malformed_status(token.status));
    }

    if (token.kind == LegacyDialogTextTokenKind::terminator) {
      flush_segment();
      if ((record.flags & kLegacyDialogFlagAlternateDirectTransition) == 0U) {
        if (record.lifetime_started_at == 0U) {
          record.lifetime_started_at = input.current_tick;
        }
        if ((record.flags & kLegacyDialogFlagDirectRectangle) != 0U) {
          record.display_counter = 0x10U;
        }
      }
      record.flags |= kLegacyDialogFlagTerminated;
      return finish(LegacyDialogTextFrameStatus::terminator_reached);
    }

    const i32 occupied_bytes = wrapping_add(
        line_position, static_cast<i32>(segment_size));
    const bool width_overflow =
        wrapping_multiply(occupied_bytes, input.scale) >
        static_cast<i32>(record.width);
    if (width_overflow ||
        token.kind == LegacyDialogTextTokenKind::line_break) {
      flush_segment();
      const i32 next_line_height = wrapping_multiply(
          wrapping_multiply(wrapping_add(line_index, 1), 2), input.scale);
      if (next_line_height >= static_cast<i32>(record.height)) {
        record.flags |= kLegacyDialogFlagPageBoundary;
        if (token.kind == LegacyDialogTextTokenKind::line_break) {
          consumed += token.bytes.size();
          message.page_stop_index = message.text_cursor_index + consumed;
        }
        if (record.lifetime_started_at == 0U) {
          record.lifetime_started_at = input.current_tick;
        }
        if (input.force_complete ||
            (record.flags & kLegacyDialogFlagFastPage) != 0U) {
          message.page_stop_index = message.text_cursor_index + consumed;
        }
        save_page_style();
        return finish(LegacyDialogTextFrameStatus::page_boundary_reached);
      }

      segment.fill(0U);
      segment_size = 0U;
      line_position = 1;
      ++line_index;
      if (token.kind == LegacyDialogTextTokenKind::line_break) {
        consumed += token.bytes.size();
      }
      if (first_line_break_offset == 0U) {
        first_line_break_offset = consumed;
      }
      continue;
    }

    switch (token.kind) {
    case LegacyDialogTextTokenKind::delayed_page_break:
      if (source_index >= message.page_stop_index) {
        flush_segment();
        message.text_cursor_index += first_line_break_offset;
        if (message.page_stop_index > message.text.size() ||
            message.text.size() - message.page_stop_index < 2U) {
          result.token_status = LegacyDialogTextTokenStatus::end_of_buffer;
          return finish(LegacyDialogTextFrameStatus::source_out_of_bounds);
        }
        message.page_stop_index += 2U;
        return finish(LegacyDialogTextFrameStatus::page_limit_reached);
      }
      consumed += token.bytes.size();
      break;

    case LegacyDialogTextTokenKind::page_break:
      flush_segment();
      consumed += token.bytes.size();
      message.page_stop_index = message.text_cursor_index + consumed;
      if (record.lifetime_started_at == 0U) {
        record.lifetime_started_at = input.current_tick;
      }
      if (input.force_complete ||
          (record.flags & kLegacyDialogFlagFastPage) != 0U) {
        message.page_stop_index = message.text_cursor_index + consumed;
      }
      save_page_style();
      record.flags |= kLegacyDialogFlagPageBoundary;
      return finish(LegacyDialogTextFrameStatus::page_boundary_reached);

    case LegacyDialogTextTokenKind::character_delay: {
      consumed += token.bytes.size();
      const u16 delay = static_cast<u16>(
          input.base_character_delay + parameter_digit(token.parameter));
      record.character_delay = static_cast<u16>(delay * 2U);
      if ((record.flags & kLegacyDialogFlagClosing) != 0U) {
        record.character_delay = 0U;
      }
      break;
    }

    case LegacyDialogTextTokenKind::both_colors:
      consumed += token.bytes.size();
      flush_segment();
      reset_after_flush();
      foreground = low_word(sign_extend_byte(token.parameter) -
                            static_cast<i32>('0'));
      secondary = foreground;
      break;

    case LegacyDialogTextTokenKind::secondary_color:
      consumed += token.bytes.size();
      flush_segment();
      reset_after_flush();
      secondary = low_word(sign_extend_byte(token.parameter) -
                           static_cast<i32>('0'));
      break;

    case LegacyDialogTextTokenKind::text_style: {
      consumed += token.bytes.size();
      flush_segment();
      reset_after_flush();
      const i32 raw_style = sign_extend_byte(token.parameter) -
                            static_cast<i32>('0');
      style = raw_style == 0
                  ? static_cast<u8>(4U)
                  : raw_style == 1 ? static_cast<u8>(0x10U)
                                   : static_cast<u8>(raw_style);
      break;
    }

    case LegacyDialogTextTokenKind::choice: {
      record.flags |= kLegacyDialogFlagHasChoices;
      ++choice_index;
      flush_segment();
      reset_after_flush();
      consumed += token.bytes.size();

      if (token.payload.size() >= kLegacyDialogSegmentCapacity) {
        return finish(
            LegacyDialogTextFrameStatus::segment_buffer_overflow);
      }
      std::array<u8, kLegacyDialogSegmentCapacity> choice_text{};
      for (std::size_t index = 0U; index < token.payload.size(); ++index) {
        choice_text[index] = token.payload[index];
      }
      const bool selected = input.selected_choice_index == choice_index;
      emit(std::span<const u8>{choice_text.data(), token.payload.size() + 1U},
           line_position, selected);
      ++result.choice_draw_count;
      if (selected) {
        ports.draw_selected_choice_background(
            LegacyDialogChoiceBackgroundRequest{
                .destination_x =
                    wrapping_multiply(line_position, input.scale),
                .destination_y = wrapping_multiply(
                    wrapping_multiply(line_index, 2), input.scale),
                .width = wrapping_multiply(
                    static_cast<i32>(token.payload.size()), input.scale),
                .height = wrapping_multiply(2, input.scale),
                .surface_width = wrapping_multiply(40, input.scale),
            });
      }

      if (token.uppercase_variant) {
        const i32 relative_left =
            wrapping_multiply(line_position, input.scale);
        const i32 relative_top = wrapping_multiply(
            wrapping_multiply(line_index, 2), input.scale);
        const i32 absolute_left = wrapping_add(
            static_cast<i32>(record.left), relative_left);
        const i32 absolute_top =
            wrapping_add(static_cast<i32>(record.top), relative_top);
        const i32 absolute_right = wrapping_add(
            absolute_left,
            wrapping_multiply(static_cast<i32>(token.payload.size()),
                              input.scale));
        const i32 absolute_bottom =
            wrapping_add(absolute_top, wrapping_multiply(2, input.scale));
        try {
          message.choices.push_back(LegacyDialogChoiceHotspot{
              .kind = 1U,
              .left = low_word(absolute_left),
              .top = low_word(absolute_top),
              .right = low_word(absolute_right),
              .bottom = low_word(absolute_bottom),
          });
        } catch (...) {
          return finish(
              LegacyDialogTextFrameStatus::choice_allocation_failed);
        }
        ++result.choice_hotspot_count;
        message.text[source_index + 1U] = static_cast<u8>('b');
      }
      line_position = wrapping_add(
          line_position, static_cast<i32>(token.payload.size()));
      break;
    }

    case LegacyDialogTextTokenKind::sound: {
      consumed += token.bytes.size();
      if (token.payload.size() >= kLegacyDialogSoundPayloadCapacity) {
        return finish(
            LegacyDialogTextFrameStatus::segment_buffer_overflow);
      }
      i32 ignored = static_cast<i32>(token.payload.size());
      const auto payload = std::string_view{
          reinterpret_cast<const char *>(token.payload.data()),
          token.payload.size()};
      static_cast<void>(
          compat::parse_legacy_decimal_or_terminate(payload, ignored));
      if (token.uppercase_variant) {
        ports.play_choice_sound();
        ++result.sound_count;
        message.text[source_index + 1U] = static_cast<u8>('a');
      }
      break;
    }

    case LegacyDialogTextTokenKind::close:
      consumed += token.bytes.size();
      record.character_delay = 0U;
      record.character_countdown = 0U;
      record.flags |= kLegacyDialogFlagClosing;
      result.closing_requested = true;
      break;

    case LegacyDialogTextTokenKind::text:
      if (segment_size > segment.size() - 1U - token.bytes.size()) {
        return finish(
            LegacyDialogTextFrameStatus::segment_buffer_overflow);
      }
      for (const u8 byte : token.bytes) {
        segment[segment_size++] = byte;
      }
      consumed += token.bytes.size();
      previous_token_was_text = true;
      break;

    case LegacyDialogTextTokenKind::terminator:
    case LegacyDialogTextTokenKind::line_break:
      break;
    }
  }
}

} // namespace openswd3::story_scene
