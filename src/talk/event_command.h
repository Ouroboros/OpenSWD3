#pragma once

#include <array>
#include <cstddef>

#include "core/types.h"

namespace swd3
{

// The commands of the event scripts (Talk<n>.dat, interpreter 0x00427920; see reverse/analysis/talk-script.md). Odd
// dialog commands wait for their window.
enum class EventCommand : u16
{
    DialogWait = 1,
    Dialog = 2,
    SizedDialogWait = 3,
    SizedDialog = 4,
    PlainDialogWait = 5,
    PlainDialog = 6,
    DialogNoSkip = 7,
    DialogTimeout = 8,
    DialogNoAdvance = 9,
    SetAction = 0xA,
    SetDirection = 0xB,
    PutRoleOnTile = 0xC,
    StepRole = 0xD,
    WaitDialogClosed = 0xE,
    Jump = 0xF,
    JumpIfMoveIdle = 0x10,
    JumpIfMoveRunning = 0x11,
    ResumeMove = 0x12,
    ResumeAllMoves = 0x13,
    MoveRoles = 0x14,
    JumpIfFlag = 0x15,
    JumpIfNotFlag = 0x16,
    JumpIfAllFlags = 0x17,
    JumpIfAnyFlag = 0x18,
    SetFlag = 0x19,
    ClearFlag = 0x1A,
    ChangeMap = 0x1B,
    SetRoleTalk = 0x1C,
    SetVariable = 0x1D,
    AddVariable = 0x1E,
    SubtractVariable = 0x1F,
    JumpIfVariableAtLeast = 0x20,
    JumpIfVariableAtMost = 0x21,
    SetClock = 0x22,
    JumpIfClockReached = 0x23,
    JumpIfClockPassed = 0x24,
    MarkClock = 0x25,
    RemoveRole = 0x26,
    HideRole = 0x27,
    PutRole = 0x28,
    JumpByChoice = 0x29,
    LockPlayer = 0x2A,
    UnlockPlayer = 0x2B,
    SetSpeed = 0x2C,
    SetAct = 0x2D,
    RestoreAct = 0x2E,
    RestoreAction = 0x2F,
    RestoreDirection = 0x30,
    ClearSpeed = 0x31,
    PanCamera = 0x32,
    WaitPan = 0x33,
    FadeColors = 0x34,
    WaitFade = 0x35,
    SeekAct = 0x36,
    LayerFront = 0x37,
    LayerMiddle = 0x38,
    LayerBack = 0x39,
    ScreenAnimation = 0x3A,
    Sound = 0x3B,
    ShowScreen = 0x3C,
    BlackScreen = 0x3D,
    PlaceRoleOnMap = 0x3E,
    Quake = 0x3F,
    StopQuake = 0x40,
    JoinFollowers = 0x41,
    LeaveFollowers = 0x42,
    Wait = 0x43,
    ClearRoleHidden = 0x44,
    SetRoleHidden = 0x45,
    PanCameraTo = 0x46,
    AttachAct = 0x47,
    DetachAct = 0x48,
    PanCameraToRole = 0x49,
    StopFade = 0x4A,
    HoldRole = 0x4B,
    TurnToFace = 0x4C,
    FixSpeed = 0x4D,
    FreeSpeed = 0x4E,
    FlyingSprite = 0x4F,
    DialogOverlay = 0x50,
    HeadIn = 0x51,
    HeadOut = 0x52,
    LineEffect = 0x53,
    LineEffectMode = 0x54,
    PlayVideo = 0x55,
    ChangeHead = 0x56,
    RandomJump = 0x57,
    StartBattle = 0x58,
    RecordDialogWait = 0x59,
    RecordDialog = 0x5A,
    RoleNamePlate = 0x5B,
    SetInParty = 0x5C,
    ClearInParty = 0x5D,
    CameraHold = 0x5E,
    CameraFollow = 0x5F,
    MapMovie = 0x60,
    WaitMapMovie = 0x61,
    Skip = 0x62,
    WaitMovieFrame = 0x63,
    SetEvent = 0x64,
    SetRoleFastMove = 0x65,
    RoleExamine = 0x66,
    RoleNoTalkPointer = 0x67,
    DialogOffset = 0x68,
    DialogSingleLine = 0x69,
    WaitAnimationA = 0x6A,
    WaitActRounds = 0x6B,
    DialogOrigin = 0x6C,
    StepRoles = 0x6D,
    JumpIfNoneMoving = 0x6E,
    JumpIfAnyMoving = 0x6F,
    WaitEffects = 0x70,
    SoundLong = 0x71,
    ChangeMusic = 0x72,
    MusicVolume = 0x73,
    PutRoles = 0x74,
    RoleUnk10 = 0x75,
    CloseDialog = 0x76,
    WaitDialogRead = 0x77,
    SetActData = 0x78,
    DialogBubble = 0x79,
    StopSkipping = 0x7A,
    SceneMusic = 0x7B,
    DialogLogged = 0x7C,
    AddLog = 0x7D,
    JumpIfActionNot = 0x7E,
    JumpIfAction = 0x7F,
    AddItem = 0x80,
    JumpIfItem = 0x81,
    JumpIfNoItem = 0x82,
    LearnSkill = 0x83,
    Equip = 0x84,
    ItemMenu = 0x85,
    ChangeVitals = 0x86,
    SaveMenu = 0x87,
    RoleStepsAct = 0x88,
    StopMusic = 0x89,
    JumpIfFar = 0x8A,
    WaitDialogTyped = 0x8B,
    RoleFacesTalker = 0x8C,
    MusicFade = 0x8D,
    StartTimer = 0x8E,
    StopTimer = 0x8F,
    EndGame = 0x90,
    RoleTouchEvent = 0x91,
    RoleCellEffects = 0x92,
    HideLoadingScreen = 0x93,
    SpotlightOn = 0x94,
    SpotlightOff = 0x95,
    SpotlightAt = 0x96,
    SpotlightMove = 0x97,
    WaitSpotlight = 0x98,
    ScreenAnimationB = 0x99,
    WaitAnimationB = 0x9A,
    EnterDemonPot = 0x9B,
    LeaveDemonPot = 0x9C,
    SetReturnMap = 0x9D,
    CopyMedia = 0x9E,
    DeleteMedia = 0x9F,
    DialogCentred = 0xA0,
    OpenScript = 0xA1,
    VariableNamePlate = 0xA2,
    JumpIfNotMap = 0xA3,
    JumpIfMap = 0xA4,
    JumpIfItemsAtLeast = 0xA5,
    JumpIfItemsAtMost = 0xA6,
    JumpIfWorn = 0xA7,
    JumpIfNotWorn = 0xA8,
    MoveRolesActing = 0xA9,
    ClearFileNameA = 0xAA,
    SetFileNameA = 0xAB,
    ClearFileNameB = 0xAC,
    SetFileNameB = 0xAD,
    RoleSolid = 0xAE,
    PauseMapMovie = 0xAF,
    ResumeMapMovie = 0xB0,
    GatherParty = 0xB1,
    PassWalls = 0xB2,
    Ripple = 0xB3,
    PauseGame = 0xB4,
    SetVariableWide = 0xB5,
    AddVariableWide = 0xB6,
    SubtractVariableWide = 0xB7,
    JumpIfVariableAtLeastWide = 0xB8,
    JumpIfVariableAtMostWide = 0xB9,
    JumpIfStatAtLeast = 0xBA,
    JumpIfStatAtMost = 0xBB,
    SetStat = 0xBC,
    AddStat = 0xBD,
    SubtractStat = 0xBE,
    WaitCameraY = 0xBF,
    WaitMusicFade = 0xC0,
    WaitVideo = 0xC1,
    RunOn = 0x400,
    RunOff = 0x401,
    SkipQuick = 0x402,
    End = 0x3FFF,
    None = 0xFFFF,  // left in the last command by PlaceRoleOnMap
};

// The command number in the first word of a command (bits 14 and 15 mark a command in progress).
constexpr u16 kCommandMask = 0x3FFF;
constexpr u32 kTargetsEnd = 0xFF00FF00;  // ends the jump targets of JumpByChoice and RandomJump

constexpr int kOpcodeBytes = 2;        // the command word
constexpr std::size_t kWordBytes = 2;  // an operand
constexpr std::size_t kCodeWord = 0;   // the words of a command: its command word, then the operands
constexpr std::size_t kFirstOperand = 1;

// The bytes of a command below 0xC2 as 0x00427920 advances past it (0: its length depends on its operands).
struct CommandLength
{
    EventCommand code;
    u8 bytes;
};

// One entry per command, in command order (checked below).
constexpr std::array<CommandLength, 0xC2> kCommandBytes = {
    CommandLength{EventCommand{0}, 0},                           // 0x00 (no command)
    CommandLength{EventCommand::DialogWait, 0},                  // 0x01
    CommandLength{EventCommand::Dialog, 0},                      // 0x02
    CommandLength{EventCommand::SizedDialogWait, 0},             // 0x03
    CommandLength{EventCommand::SizedDialog, 0},                 // 0x04
    CommandLength{EventCommand::PlainDialogWait, 0},             // 0x05
    CommandLength{EventCommand::PlainDialog, 0},                 // 0x06
    CommandLength{EventCommand::DialogNoSkip, 2},                // 0x07
    CommandLength{EventCommand::DialogTimeout, 4},               // 0x08
    CommandLength{EventCommand::DialogNoAdvance, 2},             // 0x09
    CommandLength{EventCommand::SetAction, 6},                   // 0x0A
    CommandLength{EventCommand::SetDirection, 6},                // 0x0B
    CommandLength{EventCommand::PutRoleOnTile, 8},               // 0x0C
    CommandLength{EventCommand::StepRole, 4},                    // 0x0D
    CommandLength{EventCommand::WaitDialogClosed, 4},            // 0x0E
    CommandLength{EventCommand::Jump, 0},                        // 0x0F
    CommandLength{EventCommand::JumpIfMoveIdle, 8},              // 0x10
    CommandLength{EventCommand::JumpIfMoveRunning, 8},           // 0x11
    CommandLength{EventCommand::ResumeMove, 4},                  // 0x12
    CommandLength{EventCommand::ResumeAllMoves, 2},              // 0x13
    CommandLength{EventCommand::MoveRoles, 0},                   // 0x14
    CommandLength{EventCommand::JumpIfFlag, 8},                  // 0x15
    CommandLength{EventCommand::JumpIfNotFlag, 8},               // 0x16
    CommandLength{EventCommand::JumpIfAllFlags, 0},              // 0x17
    CommandLength{EventCommand::JumpIfAnyFlag, 0},               // 0x18
    CommandLength{EventCommand::SetFlag, 4},                     // 0x19
    CommandLength{EventCommand::ClearFlag, 4},                   // 0x1A
    CommandLength{EventCommand::ChangeMap, 14},                  // 0x1B
    CommandLength{EventCommand::SetRoleTalk, 6},                 // 0x1C
    CommandLength{EventCommand::SetVariable, 6},                 // 0x1D
    CommandLength{EventCommand::AddVariable, 6},                 // 0x1E
    CommandLength{EventCommand::SubtractVariable, 6},            // 0x1F
    CommandLength{EventCommand::JumpIfVariableAtLeast, 10},      // 0x20
    CommandLength{EventCommand::JumpIfVariableAtMost, 10},       // 0x21
    CommandLength{EventCommand::SetClock, 4},                    // 0x22
    CommandLength{EventCommand::JumpIfClockReached, 8},          // 0x23
    CommandLength{EventCommand::JumpIfClockPassed, 8},           // 0x24
    CommandLength{EventCommand::MarkClock, 2},                   // 0x25
    CommandLength{EventCommand::RemoveRole, 4},                  // 0x26
    CommandLength{EventCommand::HideRole, 4},                    // 0x27
    CommandLength{EventCommand::PutRole, 8},                     // 0x28
    CommandLength{EventCommand::JumpByChoice, 0},                // 0x29
    CommandLength{EventCommand::LockPlayer, 2},                  // 0x2A
    CommandLength{EventCommand::UnlockPlayer, 2},                // 0x2B
    CommandLength{EventCommand::SetSpeed, 6},                    // 0x2C
    CommandLength{EventCommand::SetAct, 6},                      // 0x2D
    CommandLength{EventCommand::RestoreAct, 4},                  // 0x2E
    CommandLength{EventCommand::RestoreAction, 4},               // 0x2F
    CommandLength{EventCommand::RestoreDirection, 4},            // 0x30
    CommandLength{EventCommand::ClearSpeed, 4},                  // 0x31
    CommandLength{EventCommand::PanCamera, 10},                  // 0x32
    CommandLength{EventCommand::WaitPan, 2},                     // 0x33
    CommandLength{EventCommand::FadeColors, 16},                 // 0x34
    CommandLength{EventCommand::WaitFade, 2},                    // 0x35
    CommandLength{EventCommand::SeekAct, 6},                     // 0x36
    CommandLength{EventCommand::LayerFront, 4},                  // 0x37
    CommandLength{EventCommand::LayerMiddle, 4},                 // 0x38
    CommandLength{EventCommand::LayerBack, 4},                   // 0x39
    CommandLength{EventCommand::ScreenAnimation, 10},            // 0x3A
    CommandLength{EventCommand::Sound, 4},                       // 0x3B
    CommandLength{EventCommand::ShowScreen, 2},                  // 0x3C
    CommandLength{EventCommand::BlackScreen, 2},                 // 0x3D
    CommandLength{EventCommand::PlaceRoleOnMap, 18},             // 0x3E
    CommandLength{EventCommand::Quake, 0},                       // 0x3F
    CommandLength{EventCommand::StopQuake, 2},                   // 0x40
    CommandLength{EventCommand::JoinFollowers, 4},               // 0x41
    CommandLength{EventCommand::LeaveFollowers, 16},             // 0x42
    CommandLength{EventCommand::Wait, 4},                        // 0x43
    CommandLength{EventCommand::ClearRoleHidden, 4},             // 0x44
    CommandLength{EventCommand::SetRoleHidden, 4},               // 0x45
    CommandLength{EventCommand::PanCameraTo, 10},                // 0x46
    CommandLength{EventCommand::AttachAct, 6},                   // 0x47
    CommandLength{EventCommand::DetachAct, 4},                   // 0x48
    CommandLength{EventCommand::PanCameraToRole, 8},             // 0x49
    CommandLength{EventCommand::StopFade, 2},                    // 0x4A
    CommandLength{EventCommand::HoldRole, 4},                    // 0x4B
    CommandLength{EventCommand::TurnToFace, 6},                  // 0x4C
    CommandLength{EventCommand::FixSpeed, 6},                    // 0x4D
    CommandLength{EventCommand::FreeSpeed, 4},                   // 0x4E
    CommandLength{EventCommand::FlyingSprite, 16},               // 0x4F
    CommandLength{EventCommand::DialogOverlay, 2},               // 0x50
    CommandLength{EventCommand::HeadIn, 10},                     // 0x51
    CommandLength{EventCommand::HeadOut, 6},                     // 0x52
    CommandLength{EventCommand::LineEffect, 16},                 // 0x53
    CommandLength{EventCommand::LineEffectMode, 6},              // 0x54
    CommandLength{EventCommand::PlayVideo, 0},                   // 0x55
    CommandLength{EventCommand::ChangeHead, 10},                 // 0x56
    CommandLength{EventCommand::RandomJump, 0},                  // 0x57
    CommandLength{EventCommand::StartBattle, 4},                 // 0x58
    CommandLength{EventCommand::RecordDialogWait, 0},            // 0x59
    CommandLength{EventCommand::RecordDialog, 0},                // 0x5A
    CommandLength{EventCommand::RoleNamePlate, 4},               // 0x5B
    CommandLength{EventCommand::SetInParty, 4},                  // 0x5C
    CommandLength{EventCommand::ClearInParty, 4},                // 0x5D
    CommandLength{EventCommand::CameraHold, 2},                  // 0x5E
    CommandLength{EventCommand::CameraFollow, 2},                // 0x5F
    CommandLength{EventCommand::MapMovie, 0},                    // 0x60
    CommandLength{EventCommand::WaitMapMovie, 2},                // 0x61
    CommandLength{EventCommand::Skip, 4},                        // 0x62
    CommandLength{EventCommand::WaitMovieFrame, 4},              // 0x63
    CommandLength{EventCommand::SetEvent, 6},                    // 0x64
    CommandLength{EventCommand::SetRoleFastMove, 4},             // 0x65
    CommandLength{EventCommand::RoleExamine, 6},                 // 0x66
    CommandLength{EventCommand::RoleNoTalkPointer, 6},           // 0x67
    CommandLength{EventCommand::DialogOffset, 6},                // 0x68
    CommandLength{EventCommand::DialogSingleLine, 2},            // 0x69
    CommandLength{EventCommand::WaitAnimationA, 4},              // 0x6A
    CommandLength{EventCommand::WaitActRounds, 6},               // 0x6B
    CommandLength{EventCommand::DialogOrigin, 6},                // 0x6C
    CommandLength{EventCommand::StepRoles, 0},                   // 0x6D
    CommandLength{EventCommand::JumpIfNoneMoving, 6},            // 0x6E
    CommandLength{EventCommand::JumpIfAnyMoving, 6},             // 0x6F
    CommandLength{EventCommand::WaitEffects, 2},                 // 0x70
    CommandLength{EventCommand::SoundLong, 6},                   // 0x71
    CommandLength{EventCommand::ChangeMusic, 8},                 // 0x72
    CommandLength{EventCommand::MusicVolume, 4},                 // 0x73
    CommandLength{EventCommand::PutRoles, 0},                    // 0x74
    CommandLength{EventCommand::RoleUnk10, 6},                   // 0x75
    CommandLength{EventCommand::CloseDialog, 4},                 // 0x76
    CommandLength{EventCommand::WaitDialogRead, 4},              // 0x77
    CommandLength{EventCommand::SetActData, 10},                 // 0x78
    CommandLength{EventCommand::DialogBubble, 2},                // 0x79
    CommandLength{EventCommand::StopSkipping, 2},                // 0x7A
    CommandLength{EventCommand::SceneMusic, 10},                 // 0x7B
    CommandLength{EventCommand::DialogLogged, 2},                // 0x7C
    CommandLength{EventCommand::AddLog, 0},                      // 0x7D
    CommandLength{EventCommand::JumpIfActionNot, 10},            // 0x7E
    CommandLength{EventCommand::JumpIfAction, 10},               // 0x7F
    CommandLength{EventCommand::AddItem, 6},                     // 0x80
    CommandLength{EventCommand::JumpIfItem, 8},                  // 0x81
    CommandLength{EventCommand::JumpIfNoItem, 8},                // 0x82
    CommandLength{EventCommand::LearnSkill, 6},                  // 0x83
    CommandLength{EventCommand::Equip, 8},                       // 0x84
    CommandLength{EventCommand::ItemMenu, 0},                    // 0x85
    CommandLength{EventCommand::ChangeVitals, 10},               // 0x86
    CommandLength{EventCommand::SaveMenu, 2},                    // 0x87
    CommandLength{EventCommand::RoleStepsAct, 6},                // 0x88
    CommandLength{EventCommand::StopMusic, 2},                   // 0x89
    CommandLength{EventCommand::JumpIfFar, 14},                  // 0x8A
    CommandLength{EventCommand::WaitDialogTyped, 4},             // 0x8B
    CommandLength{EventCommand::RoleFacesTalker, 6},             // 0x8C
    CommandLength{EventCommand::MusicFade, 6},                   // 0x8D
    CommandLength{EventCommand::StartTimer, 8},                  // 0x8E
    CommandLength{EventCommand::StopTimer, 2},                   // 0x8F
    CommandLength{EventCommand::EndGame, 4},                     // 0x90
    CommandLength{EventCommand::RoleTouchEvent, 6},              // 0x91
    CommandLength{EventCommand::RoleCellEffects, 6},             // 0x92
    CommandLength{EventCommand::HideLoadingScreen, 2},           // 0x93
    CommandLength{EventCommand::SpotlightOn, 2},                 // 0x94
    CommandLength{EventCommand::SpotlightOff, 2},                // 0x95
    CommandLength{EventCommand::SpotlightAt, 6},                 // 0x96
    CommandLength{EventCommand::SpotlightMove, 10},              // 0x97
    CommandLength{EventCommand::WaitSpotlight, 2},               // 0x98
    CommandLength{EventCommand::ScreenAnimationB, 10},           // 0x99
    CommandLength{EventCommand::WaitAnimationB, 4},              // 0x9A
    CommandLength{EventCommand::EnterDemonPot, 2},               // 0x9B
    CommandLength{EventCommand::LeaveDemonPot, 2},               // 0x9C
    CommandLength{EventCommand::SetReturnMap, 8},                // 0x9D
    CommandLength{EventCommand::CopyMedia, 0},                   // 0x9E
    CommandLength{EventCommand::DeleteMedia, 0},                 // 0x9F
    CommandLength{EventCommand::DialogCentred, 2},               // 0xA0
    CommandLength{EventCommand::OpenScript, 0},                  // 0xA1
    CommandLength{EventCommand::VariableNamePlate, 4},           // 0xA2
    CommandLength{EventCommand::JumpIfNotMap, 8},                // 0xA3
    CommandLength{EventCommand::JumpIfMap, 8},                   // 0xA4
    CommandLength{EventCommand::JumpIfItemsAtLeast, 10},         // 0xA5
    CommandLength{EventCommand::JumpIfItemsAtMost, 10},          // 0xA6
    CommandLength{EventCommand::JumpIfWorn, 8},                  // 0xA7
    CommandLength{EventCommand::JumpIfNotWorn, 8},               // 0xA8
    CommandLength{EventCommand::MoveRolesActing, 0},             // 0xA9
    CommandLength{EventCommand::ClearFileNameA, 2},              // 0xAA
    CommandLength{EventCommand::SetFileNameA, 2},                // 0xAB
    CommandLength{EventCommand::ClearFileNameB, 2},              // 0xAC
    CommandLength{EventCommand::SetFileNameB, 2},                // 0xAD
    CommandLength{EventCommand::RoleSolid, 6},                   // 0xAE
    CommandLength{EventCommand::PauseMapMovie, 2},               // 0xAF
    CommandLength{EventCommand::ResumeMapMovie, 2},              // 0xB0
    CommandLength{EventCommand::GatherParty, 2},                 // 0xB1
    CommandLength{EventCommand::PassWalls, 4},                   // 0xB2
    CommandLength{EventCommand::Ripple, 10},                     // 0xB3
    CommandLength{EventCommand::PauseGame, 2},                   // 0xB4
    CommandLength{EventCommand::SetVariableWide, 8},             // 0xB5
    CommandLength{EventCommand::AddVariableWide, 8},             // 0xB6
    CommandLength{EventCommand::SubtractVariableWide, 8},        // 0xB7
    CommandLength{EventCommand::JumpIfVariableAtLeastWide, 12},  // 0xB8
    CommandLength{EventCommand::JumpIfVariableAtMostWide, 12},   // 0xB9
    CommandLength{EventCommand::JumpIfStatAtLeast, 10},          // 0xBA
    CommandLength{EventCommand::JumpIfStatAtMost, 10},           // 0xBB
    CommandLength{EventCommand::SetStat, 6},                     // 0xBC
    CommandLength{EventCommand::AddStat, 6},                     // 0xBD
    CommandLength{EventCommand::SubtractStat, 6},                // 0xBE
    CommandLength{EventCommand::WaitCameraY, 4},                 // 0xBF
    CommandLength{EventCommand::WaitMusicFade, 2},               // 0xC0
    CommandLength{EventCommand::WaitVideo, 2},                   // 0xC1
};

constexpr bool InCommandOrder()
{
    for (std::size_t index = 0; index < kCommandBytes.size(); ++index)
    {
        if (static_cast<std::size_t>(kCommandBytes[index].code) != index)
        {
            return false;
        }
    }

    return true;
}

static_assert(InCommandOrder(), "kCommandBytes must list the commands in order");

constexpr int CommandBytes(EventCommand code)
{
    if (code == EventCommand::RunOn || code == EventCommand::RunOff || code == EventCommand::SkipQuick)
    {
        return kOpcodeBytes;
    }

    const auto index = static_cast<std::size_t>(code);
    return index < kCommandBytes.size() ? kCommandBytes[index].bytes : 0;
}

// The odd dialog commands wait for their window.
constexpr bool WaitsForWindow(EventCommand command)
{
    return (static_cast<u16>(command) & 1) != 0;
}

// Of ClearFileNameA..SetFileNameB, the odd ones set the name.
constexpr bool SetsFileName(EventCommand command)
{
    return (static_cast<u16>(command) & 1) != 0;
}

}  // namespace swd3
