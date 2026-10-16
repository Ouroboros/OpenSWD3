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
    SetCueFlag = 0x5C,
    ClearCueFlag = 0x5D,
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

constexpr int kOpcodeBytes = 2;        // the command word
constexpr std::size_t kWordBytes = 2;  // an operand

// The bytes of each command below 0xC2 (0: its length depends on its operands), as 0x00427920 advances past them.
constexpr std::array<u8, 0xC2> kCommandBytes = {
    0,  0,  0,  0,  0,  0,  0,  2,  4,  2,  6,  6,  8, 4, 4,  0,   // 0x00
    8,  8,  4,  2,  0,  8,  8,  0,  0,  4,  4,  14, 6, 6, 6,  6,   // 0x10
    10, 10, 4,  8,  8,  2,  4,  4,  8,  0,  2,  2,  6, 6, 4,  4,   // 0x20
    4,  4,  0,  2,  16, 2,  6,  4,  4,  4,  10, 4,  2, 2, 18, 0,   // 0x30
    2,  4,  16, 4,  4,  4,  0,  6,  4,  0,  2,  4,  6, 0, 0,  16,  // 0x40
    2,  10, 6,  16, 6,  0,  10, 0,  4,  0,  0,  4,  4, 4, 2,  2,   // 0x50
    0,  2,  4,  4,  6,  4,  6,  6,  6,  2,  4,  6,  6, 0, 6,  6,   // 0x60
    2,  6,  8,  4,  0,  6,  4,  4,  10, 2,  2,  10, 2, 0, 10, 10,  // 0x70
    6,  8,  8,  6,  8,  0,  10, 2,  6,  2,  14, 4,  6, 6, 8,  2,   // 0x80
    4,  6,  6,  2,  2,  2,  6,  10, 2,  10, 4,  2,  2, 8, 0,  0,   // 0x90
    2,  0,  4,  8,  8,  10, 10, 8,  8,  0,  2,  2,  2, 2, 6,  2,   // 0xA0
    2,  2,  4,  10, 2,  8,  8,  8,  12, 12, 10, 10, 6, 6, 6,  4,   // 0xB0
    2,  2,                                                         // 0xC0
};

constexpr int CommandBytes(EventCommand code)
{
    if (code == EventCommand::RunOn || code == EventCommand::RunOff || code == EventCommand::SkipQuick)
    {
        return kOpcodeBytes;
    }

    const auto index = static_cast<std::size_t>(code);
    return index < kCommandBytes.size() ? kCommandBytes[index] : 0;
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
