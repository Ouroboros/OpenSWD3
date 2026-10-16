#pragma once

#include <string>

#include "core/types.h"
#include "file/file.h"
#include "org/object_record.h"

namespace swd3
{

// mon.dat (objects and skill scripts) and level.dat (level tables), both kept open once read (org.cpp).
class Database
{
public:
    static constexpr u32 kScriptBytes = 0x400;
    static constexpr u16 kNameTag = 0x3E8;

    explicit Database(const std::string& directory);

    bool LoadSkill(SkillScript& skill, u16 id);
    bool LoadObject(ObjectRecord& record, u16 id);
    bool LoadLevelEntry(int group, int index, u32& experience);
    void CloseFiles();
    // The level.dat script of (group, index) (also read by the level-up of a party role).
    bool ReadLevelScript(int group, int index, std::vector<u8>& script);
    // Frees the description without uncounting it.
    static void FreeDescription(ObjectRecord& record);

    i32 descriptionBytes = 0;  // 0x0053CF48 bytes held by object descriptions
    ObjectRecord scratch;      // 0x0053CF50 an object looked up for its limits

private:
    File* OpenData(File& file, bool& open, const char* name);
    void ReadScript(File& file, u32 offset, std::vector<u8>& script);

    const std::string& directory;
    File monFile;            // 0x0053D020
    File levelFile;          // 0x0053D024
    bool monOpen = false;    // 0x0053CF40
    bool levelOpen = false;  // 0x0053CF44
};

}  // namespace swd3
