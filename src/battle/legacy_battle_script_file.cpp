#include "openswd3/battle/legacy_battle_script_file.hpp"

#include "openswd3/battle/legacy_battle_assets.hpp"

#include <algorithm>
#include <bit>
#include <stdexcept>

namespace openswd3::battle {

compat::u32 LegacyBattleScriptFileRuntime::open_script_file(
    const std::filesystem::path& path
) {
    auto file = std::make_unique<resource_io::LegacyFile>();
    if (!file->open(
            path,
            resource_io::LegacyFileCreation::open_always,
            resource_io::LegacyFileAccess::read,
            resource_io::LegacyFileSharing::read,
            resource_io::LegacyFileOpenBehavior::direct_api
        )) {
        return 0xFFFFFFFFU;
    }

    compat::u32 handle = 1U;
    while (files_.contains(handle)) {
        ++handle;
    }

    if (handle == 0xFFFFFFFFU) {
        return handle;
    }

    files_.emplace(handle, std::move(file));
    return handle;
}

compat::u32 LegacyBattleScriptFileRuntime::seek_script_file(
    const compat::u32 handle,
    const compat::i32 distance,
    const LegacyBattleScriptSeekOrigin origin
) {
    const auto found = files_.find(handle);
    // The original loader continues with INVALID_HANDLE_VALUE after open failure.
    if (found == files_.end()) {
        return 0xFFFFFFFFU;
    }

    return (origin == LegacyBattleScriptSeekOrigin::begin
                ? found->second->seek_begin_one_based(distance)
                : found->second->seek_current_one_based(distance)) -
        1U;
}

LegacyBattleScriptFileReadReply LegacyBattleScriptFileRuntime::read_script_file(
    const compat::u32 handle, const std::span<compat::u8> destination
) {
    const auto found = files_.find(handle);
    if (found == files_.end()) {
        return {};
    }

    auto count = static_cast<compat::u32>(destination.size());
    const bool read = found->second->read(
        destination,
        count,
        resource_io::LegacyFileReadBehavior::preserve_api_count
    );
    return {.eax = read ? 1U : 0U, .bytes_written = count};
}

compat::u32
LegacyBattleScriptFileRuntime::close_script_file(const compat::u32 handle) {
    const auto found = files_.find(handle);
    if (found == files_.end()) {
        return 0U;
    }

    const bool closed = found->second->close();
    files_.erase(found);
    return closed ? 1U : 0U;
}

LegacyBattleScriptWindowResult load_legacy_battle_script_window_file(
    LegacyBattleAssets& assets,
    compat::u32& cursor,
    LegacyBattleScriptFilePort& files,
    const LegacyBattleScriptWindowRequest& request
) {
    LegacyBattleScriptWindowResult result;
    if (request.enabled == 0U) {
        result.return_eax = 1U;
        return result;
    }

    if (assets.script_file_opened == 0U) {
        assets.figtalk_path =
            resolve_legacy_battle_script_path(request.data_root);
        const auto handle = files.open_script_file(assets.figtalk_path);
        assets.script_file_handle = handle;
        if (handle == 0U) {
            return result;
        }

        assets.script_file_opened = 1U;
    }

    static_cast<void>(files.seek_script_file(
        assets.script_file_handle, 0x204, LegacyBattleScriptSeekOrigin::begin
    ));
    static_cast<void>(files.seek_script_file(
        assets.script_file_handle,
        std::bit_cast<compat::i32>(request.battle_id * 4U - 4U),
        LegacyBattleScriptSeekOrigin::current
    ));
    // Zero is host storage only. An unread suffix cannot be consumed without
    // an explicit original stack snapshot.
    auto offset_bytes =
        request.offset_stack_bytes.value_or(std::array<compat::u8, 4>{});
    const auto offset_read =
        files.read_script_file(assets.script_file_handle, offset_bytes);
    if (offset_read.bytes_written > offset_bytes.size()) {
        throw std::logic_error("script file port wrote beyond offset buffer");
    }

    result.offset_bytes_written = offset_read.bytes_written;
    if (offset_read.bytes_written != offset_bytes.size() &&
        !request.offset_stack_bytes.has_value()) {
        result.status =
            LegacyBattleScriptWindowStatus::offset_stack_unavailable;
        return result;
    }

    compat::u32 data_offset = 0U;
    for (std::size_t index = 0U; index < offset_bytes.size(); ++index) {
        data_offset |= static_cast<compat::u32>(offset_bytes[index])
            << (index * 8U);
    }

    assets.figtalk_data_offset = data_offset;
    static_cast<void>(files.seek_script_file(
        assets.script_file_handle,
        std::bit_cast<compat::i32>(data_offset + 0x200U),
        LegacyBattleScriptSeekOrigin::begin
    ));
    // Existing host adaptation: fixed owned storage replaces the allocation.
    // Publication precedes clearing and the file read, as at 46E1A1..46E1C7.
    cursor = 0U;
    assets.script_capacity = kLegacyBattleScriptWindowSize;
    std::ranges::fill(assets.script, compat::u8{});
    result.window_read =
        files.read_script_file(assets.script_file_handle, assets.script);
    assets.figtalk_actual_size = result.window_read.bytes_written;
    result.return_eax = 1U;
    return result;
}

LegacyBattleScriptFileReadReply load_legacy_battle_script_page_file(
    LegacyBattleAssets& assets,
    compat::u32& cursor,
    LegacyBattleScriptFilePort& files,
    const compat::u32 data_offset
) {
    // Release/allocation remain represented by the existing fixed storage.
    assets.script_capacity = kLegacyBattleScriptPageSize;
    cursor = 0U;
    auto page =
        std::span<compat::u8>{assets.script}.first(kLegacyBattleScriptPageSize);
    std::ranges::fill(page, compat::u8{});
    static_cast<void>(files.seek_script_file(
        assets.script_file_handle,
        std::bit_cast<compat::i32>(data_offset + 0x200U),
        LegacyBattleScriptSeekOrigin::begin
    ));
    // 46E233 reloads the current pointer after SetFilePointer returns.
    const auto capacity =
        std::min<std::size_t>(assets.script_capacity, assets.script.size());
    if (cursor > capacity || kLegacyBattleScriptPageSize > capacity - cursor) {
        return {.destination_unavailable = true};
    }

    const auto destination = std::span<compat::u8>{assets.script}.subspan(
        cursor, kLegacyBattleScriptPageSize
    );
    const auto reply =
        files.read_script_file(assets.script_file_handle, destination);
    assets.figtalk_actual_size = reply.bytes_written;
    assets.figtalk_page_offset = data_offset;
    return reply;
}

void close_legacy_battle_script_file(
    LegacyBattleAssets& assets, LegacyBattleScriptFilePort& files
) {
    if (assets.script_file_handle != 0xFFFFFFFFU) {
        static_cast<void>(files.close_script_file(assets.script_file_handle));
        assets.script_file_handle = 0xFFFFFFFFU;
    }

    assets.script_file_opened = 0U;
}

}  // namespace openswd3::battle
