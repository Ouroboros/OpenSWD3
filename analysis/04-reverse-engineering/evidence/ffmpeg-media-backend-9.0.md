# FFmpeg 9.0 stream and video backend

## Scope

Status: `platform_adapted`, `unit_tested`, `real_asset_tested`, `linux_runtime_integrated`, `windows_runtime_integrated`.

This backend completes the deferred media implementation behind the existing platform-neutral contracts:

- BGM/MP3 playback through `LegacyStreamBackend` and `LegacyStreamManager`.
- BIK/OP playback through `LegacyVideoBackend` and `LegacyVideoPlayer`.
- SDL3 remains the final audio device and presentation owner.
- No FFmpeg type or function crosses into the audio/video compatibility core, Story VM, or application orchestration.

The original Miles/Bink behavior contracts remain defined by their existing LST evidence and fake-backend tests. FFmpeg is a platform replacement for the proprietary decoders, not a new compatibility contract.

## Dependency lock and licensing

The backend consumes the BtbN n9.0 `lgpl-shared` Windows x64 and Linux x64 packages recorded in `dependencies/ffmpeg/9.0/SOURCE.md`.

CMake:

- selects `windows-x64` or `linux-x64` without network access;
- accepts an explicit `OPENSWD3_FFMPEG_ROOT` override;
- verifies local headers, import libraries, and runtime libraries;
- imports `avformat`, `avcodec`, `avutil`, `swresample`, and `swscale`;
- builds the project-owned `openswd3_ffmpeg` shared library;
- builds one shared SDL3 fallback so the application and media library use the same SDL device/runtime state;
- copies the project library, SDL3, the five FFmpeg runtime libraries, and upstream `LICENSE.txt` beside the application and real-media test executable.

Linux uses `$ORIGIN` first in the shared library RUNPATH. Windows tests prove the copied DLL set loads without relying on an FFmpeg installation.

## Stream backend

`make_legacy_stream_backend()` creates a backend bound to the configured game data directory.

For each opened stream it:

1. normalizes legacy backslash paths without changing the compatibility-layer filename contract;
2. opens the media through libavformat and selects the best audio stream;
3. decodes through libavcodec;
4. converts to interleaved stereo float at 48 kHz through libswresample;
5. opens an SDL3 playback stream and applies Miles-compatible user-data, volume, loop, start, status, and millisecond-position operations;
6. reports retained status `4`, terminal status `2`, and pre-start status `8` to the unchanged `LegacyStreamManager`.

A zero loop count requeues indefinitely, positive counts preserve finite replay behavior, and manager-owned fade/status ordering remains unchanged.

The real-media test opens `Music/Map_Ca12.mp3`, whose locked properties are mono MP3, 44.1 kHz, approximately 4.023 seconds. It verifies open, user data, volume, infinite-loop retained status, and a 4,000-4,050 ms decoded duration through the real FFmpeg and SDL dummy-device path.

## Video backend

`make_legacy_video_backend()` creates a single-process handle backend for Bink containers.

For each opened movie it:

1. selects and opens the Bink video decoder;
2. derives the frame cadence and frame count from the real stream time base;
3. optionally opens the embedded Bink audio decoder, resamples it to stereo float 48 kHz, and queues it to SDL3;
4. implements the existing wait/decode/copy/frame-count/frame-number/advance/service/close ABI;
5. converts the current frame to RGB555 or RGB565 through libswscale directly into the centered legacy destination span;
6. uses monotonic SDL nanoseconds for the existing frame-wait contract;
7. keeps volume and audio-device ownership inside the backend.

The real-media test opens `Video/firegod.bik`, verifies the 640x480 summary and 176-frame count, decodes/copies the first Bink frame, then decodes a second frame so interleaved embedded-audio packets pass through the SDL queue. It separately opens the actual OP asset `Video/opening.bik`, locks its 640x480/7,369-frame stream, and performs the same two-frame path without running the 245-second movie. The same backend covers the remaining real Bink assets selected by the existing Story VM filename adaptation.

## Application integration

The SDL main runtime no longer instantiates the unavailable stream backend or immediate-complete video backend. It constructs both FFmpeg factories after the configured data root is resolved, then passes only their base interfaces to the existing managers and Story VM ports.

No original or OpenSWD3 game executable was launched during verification.

## Verification

- Linked runtime version begins with `n9.0`.
- Linux real SDL media tests: MP3 and Bink paths passed.
- Windows LLVM real SDL media tests: MP3 and Bink paths passed.
- Linux core no-SDL/no-FFmpeg configuration remained independent and passed `186/186`.
- Linux app full gate passed `192/192`.
- Windows LLVM app full gate passed `192/192`.
- Linux ELF dependencies resolve all five FFmpeg libraries from copied application output files; no FFmpeg dependency is missing.
- Linux and Windows application/test output directories contain `openswd3_ffmpeg`, the shared SDL3 runtime, all five FFmpeg runtime libraries, and `LICENSE.txt`.
- ELF and PE dependency inspection proves both the application and `openswd3_ffmpeg` resolve the same shared SDL3 runtime instance.
