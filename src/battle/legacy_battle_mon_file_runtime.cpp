#include "openswd3/battle/legacy_battle_mon_file_runtime.hpp"

#include <bit>
#include <limits>
#include <stdexcept>

namespace openswd3::battle {

resource_io::LegacyFile*
LegacyBattleMonFileRuntime::find_file(const compat::u32 handle) noexcept {
    if (handle == 0U || handle > files_.size()) {
        return nullptr;
    }

    return files_[handle - 1U].get();
}

LegacyBattleMonDatabaseCallReply LegacyBattleMonFileRuntime::invoke(
    const LegacyBattleMonDatabaseCallRequest& request,
    const std::span<compat::u8> destination,
    const std::filesystem::path& data_directory
) {
    LegacyBattleMonDatabaseCallReply reply{
        .ecx = request.ecx,
        .edx = request.edx,
    };
    using Call = LegacyBattleMonDatabaseCall;
    switch (request.call) {
    case Call::open_file: {
        // 0x00476AC6 and 0x00476E3D both use OPEN_ALWAYS, not OPEN_EXISTING.
        if (request.path == nullptr || request.desired_access != 0x80000000U ||
            request.share_mode != 1U ||
            request.security_attributes_token != 0U ||
            request.creation_disposition != 4U ||
            request.flags_and_attributes != 0x80U ||
            request.template_file_token != 0U) {
            throw std::invalid_argument("unsupported MON open request");
        }

        auto path = data_directory /
            (request.path->empty() ? std::filesystem::path{"mon.dat"}
                                   : *request.path);
        // Resolve the original lowercase literal against uppercase assets on
        // case-sensitive hosts. Do not retry an unrelated file after failure.
        std::error_code error;
        if (path.filename() == "mon.dat" &&
            !std::filesystem::exists(path, error)) {
            auto uppercase = path;
            uppercase.replace_filename("MON.DAT");
            if (std::filesystem::exists(uppercase, error)) {
                path = std::move(uppercase);
            }
        }

        reply.eax = 0xFFFFFFFFU;
        auto file = std::make_unique<resource_io::LegacyFile>();
        if (!file->open(
                path,
                resource_io::LegacyFileCreation::open_always,
                resource_io::LegacyFileAccess::read,
                resource_io::LegacyFileSharing::read,
                resource_io::LegacyFileOpenBehavior::direct_api
            )) {
            return reply;
        }

        if (files_.size() >= std::numeric_limits<compat::u32>::max() - 1U) {
            return reply;
        }

        files_.push_back(std::move(file));
        reply.eax = static_cast<compat::u32>(files_.size());
        return reply;
    }

    case Call::seek_file: {
        if (request.distance_high_token != 0U || request.move_method > 2U) {
            throw std::invalid_argument("unsupported MON seek request");
        }

        reply.eax = 0xFFFFFFFFU;
        auto* const file = find_file(request.handle);
        if (file == nullptr) {
            return reply;
        }

        const auto distance = std::bit_cast<compat::i32>(request.distance);
        switch (request.move_method) {
        case 0U:
            reply.eax = file->seek_begin_one_based(distance) - 1U;
            break;

        case 1U:
            reply.eax = file->seek_current_one_based(distance) - 1U;
            break;

        case 2U:
            reply.eax = file->seek_end_one_based(distance) - 1U;
            break;
        }

        return reply;
    }

    case Call::read_file: {
        if (request.requested_bytes > destination.size()) {
            throw std::invalid_argument("unsupported MON read request");
        }

        auto* const file = find_file(request.handle);
        if (file == nullptr) {
            return reply;
        }

        auto count = request.requested_bytes;
        const bool read = count == 0U ||
            file->read(
                destination,
                count,
                resource_io::LegacyFileReadBehavior::preserve_api_count
            );
        reply.eax = read ? 1U : 0U;
        reply.bytes_read = count;
        return reply;
    }

    default:
        throw std::invalid_argument(
            "non-file request passed to MON file runtime"
        );
    }
}

}  // namespace openswd3::battle
