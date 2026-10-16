#include "audio/audio.h"

#include <cstring>

#include "crt/crt.h"
#include "data/globals.h"

namespace swd3 {

using namespace mss;

namespace {

constexpr std::uint32_t kFading = 0x80000000u;
constexpr int kSoundEntries = 3000;
constexpr std::uint32_t kSoundTableBytes = kSoundEntries * sizeof(SoundEntry);
constexpr std::uint32_t kSoundIndexBytes = kSoundEntries * 0x2C;
constexpr std::int32_t kRiff = 0x46464952;
constexpr int kMinRate = 0x2B11;

SequenceNode* sentinelOf(SequencePlayer* player) { return reinterpret_cast<SequenceNode*>(player->m_sentinel); }

SampleNode* sentinelOf(SoundEffects* effects) { return reinterpret_cast<SampleNode*>(&effects->m_sentinel); }

void releaseSound(SoundEffects* effects, HSAMPLE sample) {
    const std::int32_t index = AIL_sample_user_data(sample, 0) - 1;
    SoundEntry& entry = effects->m_table[index];
    --entry.users;
    if (entry.users == 0) {
        AIL_mem_free_lock(entry.data);
        entry.data = nullptr;
    }
}

}  // namespace

// swd32.exe 0x00485340
AudioSystem::AudioSystem() { m_started = 0; }

// swd32.exe 0x00485350
AudioSystem::~AudioSystem() { Shutdown(); }

// swd32.exe 0x00485360: also loads MSS32.DLL to read string 1 (unused)
void AudioSystem::Startup(const char* redistDirectory) {
    AIL_set_redist_directory(redistDirectory);
    AIL_startup();
    w32::HANDLE library = w32::LoadLibraryA("MSS32.DLL");
    if (reinterpret_cast<std::uintptr_t>(library) > 0x20) {
        char text[0x400];
        w32::LoadStringA(library, 1, text, sizeof(text));
        w32::FreeLibrary(library);
    }
    m_started = 1;
}

// swd32.exe 0x004853C0
void AudioSystem::Shutdown() {
    AIL_shutdown();
    m_started = 0;
}

// swd32.exe 0x00486580
SequencePlayer::SequencePlayer() {
    m_driver = nullptr;
    m_opened = 0;
    m_enabled = 0;
    m_unknown4 = 0x1E;
    m_active = nullptr;
    m_free = nullptr;
}

// swd32.exe 0x004866A0
bool SequencePlayer::IsOpen() { return m_opened == 1; }

// swd32.exe 0x004866B0
bool SequencePlayer::IsEnabled() { return m_enabled == 1; }

// swd32.exe 0x00486A30
SequenceNode* SequencePlayer::AllocNode() {
    SequenceNode* node = m_free;
    m_free = nullptr;
    if (node != nullptr) {
        m_free = node->next;
    }
    return node;
}

// swd32.exe 0x00486A50
SequenceNode* SequencePlayer::PushActive(SequenceNode* node) {
    if (node == nullptr) {
        return nullptr;
    }
    node->next = m_active;
    m_active = node;
    return node;
}

// swd32.exe 0x004852C0
bool SequencePlayer::FreeNode(SequenceNode* node) {
    if (node == nullptr) {
        return false;
    }
    node->next = m_free;
    m_free = node;
    return true;
}

// swd32.exe 0x00484DC0
MidiPlayer::~MidiPlayer() { Close(); }

// swd32.exe 0x00484DD0: tries the MIDI mapper, then device 0; one sequence handle
bool MidiPlayer::Open(int) {
    auto* driver = reinterpret_cast<HMDIDRIVER*>(&m_driver);
    if (AIL_midiOutOpen(driver, nullptr, -1) != 0) {
        std::strcpy(m_error, AIL_last_error());
        if (AIL_midiOutOpen(driver, nullptr, 0) != 0) {
            std::strcat(m_error, " //");
            std::strcat(m_error, AIL_last_error());
            m_driver = nullptr;
            m_enabled = 0;
            return false;
        }
    }
    m_opened = 1;
    m_enabled = 1;
    auto* node = static_cast<SequenceNode*>(AIL_mem_alloc_lock(sizeof(SequenceNode)));
    node->handle = AIL_allocate_sequence_handle(static_cast<HMDIDRIVER>(m_driver));
    node->data = nullptr;
    node->fadeStep = 0;
    node->volume = 0;
    node->flags = 0;
    node->next = m_free;
    m_free = node;
    return true;
}

// swd32.exe 0x00484ED0
int MidiPlayer::Close() {
    if (!IsOpen()) {
        return 0;
    }
    while (SequenceNode* node = m_active) {
        m_active = node->next;
        AIL_end_sequence(static_cast<HSEQUENCE>(node->handle));
        AIL_release_sequence_handle(static_cast<HSEQUENCE>(node->handle));
        AIL_mem_free_lock(node->data);
        AIL_mem_free_lock(node);
    }
    while (SequenceNode* node = m_free) {
        m_free = node->next;
        AIL_release_sequence_handle(static_cast<HSEQUENCE>(node->handle));
        AIL_mem_free_lock(node);
    }
    AIL_midiOutClose(static_cast<HMDIDRIVER>(m_driver));
    m_opened = 0;
    m_enabled = 0;
    return 1;
}

// swd32.exe 0x00484F60: loads a MIDI file and starts it as sequence `id`. A file that cannot be opened leaks the
// node; an init_sequence result of -1 is reported but played anyway.
int MidiPlayer::Play(const char* fileName, int id, int volume, int loops) {
    if (!IsOpen() || !IsEnabled() || id == 0) {
        return 0;
    }
    SequenceNode* node = AllocNode();
    if (node == nullptr) {
        return 0;
    }
    node->data = nullptr;
    node->fadeStep = 0;
    auto* file = static_cast<File*>(crt::operator_new(sizeof(File)));
    if (file != nullptr) {
        file = new (file) File();
    }
    if (!file->Open(fileName, w32::OPEN_ALWAYS, w32::GENERIC_READ)) {
        std::strcpy(m_error, AIL_last_error());
        if (file != nullptr) {
            file->~File();
            crt::operator_delete(file);
        }
        return 0;
    }
    w32::DWORD size = file->GetSize();
    node->data = static_cast<char*>(AIL_mem_alloc_lock(size));
    file->Read(node->data, &size);
    file->Close();
    if (file != nullptr) {
        file->~File();
        crt::operator_delete(file);
    }
    const S32 result = AIL_init_sequence(static_cast<HSEQUENCE>(node->handle), node->data, 0);
    if (result == -1) {
        std::strcpy(m_error, AIL_last_error());
    } else if (result == 0) {
        std::strcpy(m_error, AIL_last_error());
        AIL_mem_free_lock(node->data);
        node->data = nullptr;
        FreeNode(node);
        return 0;
    }
    node->volume = 0x10 * ClampVolume(volume);
    AIL_set_sequence_user_data(static_cast<HSEQUENCE>(node->handle), 0, id);
    AIL_set_sequence_volume(static_cast<HSEQUENCE>(node->handle), node->volume / 0x10, 0);
    AIL_set_sequence_loop_count(static_cast<HSEQUENCE>(node->handle), loops);
    AIL_start_sequence(static_cast<HSEQUENCE>(node->handle));
    PushActive(node);
    return id;
}

// swd32.exe 0x00485180: fades and retires sequences. A sequence in an unexpected state is retired when the
// previous one was (the first one never is).
bool MidiPlayer::Update() {
    if (!IsOpen() || !IsEnabled()) {
        return false;
    }
    SequenceNode* previous = sentinelOf(this);
    SequenceNode* node = m_active;
    bool retiredLast = false;
    while (node != nullptr) {
        bool retire = false;
        if (!(node->flags & kFading)) {
            switch (AIL_sequence_status(static_cast<HSEQUENCE>(node->handle))) {
            case SMP_FREE:
            case SMP_DONE:
                retire = true;
                break;
            case SMP_PLAYING:
            case SMP_STOPPED:
            case SMP_PLAYINGBUTRELEASED:
                retiredLast = false;
                break;
            default:
                retire = retiredLast;
                break;
            }
        } else {
            node->volume -= node->fadeStep;
            const int volume = ClampVolume(node->volume / 0x10);
            if (volume != 0) {
                AIL_set_sequence_volume(static_cast<HSEQUENCE>(node->handle), volume, 0);
                retiredLast = false;
            } else {
                retire = true;
            }
        }
        if (!retire) {
            previous = node;
            node = node->next;
            continue;
        }
        retiredLast = true;
        AIL_end_sequence(static_cast<HSEQUENCE>(node->handle));
        AIL_mem_free_lock(node->data);
        SequenceNode* next = node->next;
        node->data = nullptr;
        node->fadeStep = 0;
        node->flags = 0;
        previous->next = next;
        node->next = m_free;
        m_free = node;
        node = previous->next;
    }
    return false;
}

// swd32.exe 0x00485290
bool MidiPlayer::IsStopped(int id) {
    if (id != 0) {
        return Find(id) == nullptr;
    }
    return m_active == nullptr;
}

// swd32.exe 0x004852F0
SequenceNode* MidiPlayer::Find(int id) {
    for (SequenceNode* node = m_active; node != nullptr; node = node->next) {
        AIL_sequence_user_data(static_cast<HSEQUENCE>(node->handle), 0);
        if (id == AIL_sequence_user_data(static_cast<HSEQUENCE>(node->handle), 0)) {
            return node;
        }
    }
    return nullptr;
}

// swd32.exe 0x004865A0
StreamPlayer::~StreamPlayer() { Close(); }

// swd32.exe 0x004865B0: two stream slots
bool StreamPlayer::Open(HDIGDRIVER driver) {
    m_driver = driver;
    m_opened = 1;
    m_enabled = 1;
    for (int i = 0; i < 2; ++i) {
        auto* node = static_cast<SequenceNode*>(AIL_mem_alloc_lock(sizeof(SequenceNode)));
        node->handle = nullptr;
        node->data = nullptr;
        node->fadeStep = 0;
        node->volume = 0;
        node->flags = 0;
        node->next = m_free;
        m_free = node;
    }
    return true;
}

// swd32.exe 0x00486610
int StreamPlayer::Close() {
    if (!IsOpen()) {
        return 0;
    }
    while (SequenceNode* node = m_active) {
        m_active = node->next;
        if (node->data != nullptr) {
            AIL_mem_free_lock(node->data);
        }
        AIL_close_stream(static_cast<HSTREAM>(node->handle));
        AIL_mem_free_lock(node);
    }
    while (SequenceNode* node = m_free) {
        m_free = node->next;
        if (node->data != nullptr) {
            AIL_mem_free_lock(node->data);
        }
        AIL_close_stream(static_cast<HSTREAM>(node->handle));
        AIL_mem_free_lock(node);
    }
    m_opened = 0;
    m_enabled = 0;
    return 1;
}

// swd32.exe 0x004866C0
int StreamPlayer::SetVolume(int id, int volume) {
    if (!IsOpen()) {
        return 0;
    }
    SequenceNode* node = Find(id);
    if (node == nullptr) {
        return -1;
    }
    node->volume = 0x10 * ClampVolume(volume);
    AIL_set_stream_volume(static_cast<HSTREAM>(node->handle), ClampVolume(volume));
    return AIL_stream_volume(static_cast<HSTREAM>(node->handle));
}

// swd32.exe 0x00486730: no open / enabled check
int StreamPlayer::Play(const char* fileName, int id, int volume, int loops) {
    if (id == 0 || Find(id) != nullptr) {
        return 0;
    }
    SequenceNode* node = AllocNode();
    if (node == nullptr) {
        return 0;
    }
    node->data = nullptr;
    node->fadeStep = 0;
    node->handle = AIL_open_stream(static_cast<HDIGDRIVER>(m_driver), fileName, 0);
    if (node->handle == nullptr) {
        std::strcpy(m_error, AIL_last_error());
        FreeNode(node);
        return 0;
    }
    node->data = static_cast<char*>(AIL_mem_alloc_lock(static_cast<U32>(std::strlen(fileName) + 1)));
    std::strcpy(node->data, fileName);
    node->volume = 0x10 * ClampVolume(volume);
    AIL_set_stream_user_data(static_cast<HSTREAM>(node->handle), 0, id);
    AIL_set_stream_volume(static_cast<HSTREAM>(node->handle), ClampVolume(volume));
    AIL_set_stream_loop_count(static_cast<HSTREAM>(node->handle), loops);
    AIL_start_stream(static_cast<HSTREAM>(node->handle));
    PushActive(node);
    return id;
}

// swd32.exe 0x00486860
int StreamPlayer::FadeOut(int id, int steps) {
    if (!IsOpen() || !IsEnabled() || id == 0) {
        return 0;
    }
    SequenceNode* node = Find(id);
    if (node == nullptr) {
        return 0;
    }
    S32 total;
    S32 current;
    AIL_stream_ms_position(static_cast<HSTREAM>(node->handle), &total, &current);
    node->flags |= kFading;
    SetFadeStep(node, steps);
    return id;
}

// swd32.exe 0x004868E0
int SetFadeStep(SequenceNode* node, int steps) {
    const int step = node->volume / steps;
    node->fadeStep = step;
    if (step == 0) {
        node->fadeStep = 1;
    }
    return step;
}

// swd32.exe 0x00486900: like MidiPlayer::Update for streams; a finished stream is muted before closing. The open
// and enabled flags are not checked.
bool StreamPlayer::Update() {
    SequenceNode* previous = sentinelOf(this);
    SequenceNode* node = m_active;
    if (node == nullptr) {
        return false;
    }
    bool retiredLast = false;
    while (node != nullptr) {
        bool retire = false;
        if (!(node->flags & kFading)) {
            switch (AIL_stream_status(static_cast<HSTREAM>(node->handle))) {
            case SMP_DONE:
                AIL_set_stream_volume(static_cast<HSTREAM>(node->handle), 0);
                retire = true;
                break;
            case SMP_PLAYING:
            case SMP_STOPPED:
            case SMP_PLAYINGBUTRELEASED:
                retiredLast = false;
                break;
            default:
                retire = retiredLast;
                break;
            }
        } else {
            node->volume -= node->fadeStep;
            const int volume = ClampVolume(node->volume / 0x10);
            if (volume != 0) {
                AIL_set_stream_volume(static_cast<HSTREAM>(node->handle), volume);
                retiredLast = false;
            } else {
                retire = true;
            }
        }
        if (!retire) {
            previous = node;
            node = node->next;
            continue;
        }
        retiredLast = true;
        AIL_set_stream_volume(static_cast<HSTREAM>(node->handle), 0);
        AIL_close_stream(static_cast<HSTREAM>(node->handle));
        AIL_mem_free_lock(node->data);
        SequenceNode* next = node->next;
        node->handle = nullptr;
        node->data = nullptr;
        node->fadeStep = 0;
        node->flags = 0;
        previous->next = next;
        node->next = m_free;
        m_free = node;
        node = previous->next;
    }
    return false;
}

// swd32.exe 0x00486A10
bool StreamPlayer::IsStopped(int id) { return Find(id) == nullptr; }

// swd32.exe 0x00486A70
SequenceNode* StreamPlayer::Find(int id) {
    for (SequenceNode* node = m_active; node != nullptr; node = node->next) {
        if (id == AIL_stream_user_data(static_cast<HSTREAM>(node->handle), 0)) {
            return node;
        }
    }
    return nullptr;
}

// swd32.exe 0x00486260
int ClampVolume(int volume) {
    if (volume > 0x7F) {
        return 0x7F;
    }
    if (volume < 0) {
        return 0;
    }
    return volume;
}

// swd32.exe 0x00486280: -63..64 to the Miles pan range 0..127
int ClampPan(int pan) {
    const int value = pan + 0x3F;
    if (value > 0x7F) {
        return 0x7F;
    }
    if (value < 0) {
        return 0;
    }
    return value;
}

// swd32.exe 0x00485930
SoundEffects::SoundEffects() {
    m_opened = 0;
    m_enabled = 0;
    std::memset(m_config, 0, sizeof(m_config));
    std::memset(m_error, 0, sizeof(m_error));
}

// swd32.exe 0x00485960
SoundEffects::~SoundEffects() { Close(); }

// swd32.exe 0x004859B0: opens all.snd and a 44.1 kHz 16-bit stereo output, halving the rate (then 8-bit) while
// waveOut refuses and avoiding emulated DirectSound; allocates up to 16 samples
bool SoundEffects::Open(const char* directory) {
    if (IsOpen()) {
        return true;
    }
    if (!LoadIndex(directory)) {
        w32::lstrcpyA(m_error, "SFX file connot open.");
        return false;
    }
    m_channels = 2;
    m_rate = 0xAC44;
    m_bits = 0x10;
    AIL_set_preference(DIG_USE_WAVEOUT, 0);
    for (;;) {
        if (m_rate < kMinRate) {
            m_driver = nullptr;
            return false;
        }
        PCMWAVEFORMAT& format = G.waveFormat;
        format.wFormatTag = 1;
        format.nChannels = static_cast<std::uint16_t>(m_channels);
        format.nSamplesPerSec = static_cast<std::uint32_t>(m_rate);
        format.nAvgBytesPerSec = static_cast<std::uint32_t>(m_rate * m_channels * (m_bits / 8));
        format.nBlockAlign = static_cast<std::uint16_t>(m_channels * (m_bits / 8));
        format.wBitsPerSample = static_cast<std::uint16_t>(m_bits);
        if (AIL_waveOutOpen(&m_driver, nullptr, -1, &format) != 0) {
            std::strcpy(m_error, AIL_last_error());
            if (AIL_get_preference(DIG_USE_WAVEOUT) != 0) {
                m_rate /= 2;
                if (m_rate < kMinRate && m_bits == 0x10) {
                    m_rate = 0xAC44;
                    m_bits = 8;
                }
            } else {
                AIL_set_preference(DIG_USE_WAVEOUT, 1);
            }
            continue;
        }
        AIL_digital_configuration(m_driver, nullptr, nullptr, m_config);
        if (AIL_get_preference(DIG_USE_WAVEOUT) != 0 || std::strstr(m_config, "Emulated") == nullptr) {
            break;
        }
        AIL_waveOutClose(m_driver);
        AIL_set_preference(DIG_USE_WAVEOUT, 1);
        AIL_set_preference(DIG_USE_WAVEOUT, 1);
    }
    m_opened = 1;
    m_enabled = 1;
    m_voices = AIL_get_preference(DIG_MIXER_CHANNELS) - 8;
    if (m_voices > 0x10) {
        m_voices = 0x10;
    }
    m_active = nullptr;
    for (int count = 0; count < m_voices;) {
        auto* node = static_cast<SampleNode*>(AIL_mem_alloc_lock(sizeof(SampleNode)));
        node->sample = AIL_allocate_sample_handle(m_driver);
        if (node->sample == nullptr) {
            AIL_mem_free_lock(node);
            std::strcpy(m_error, AIL_last_error());
            return true;
        }
        AIL_init_sample(node->sample);
        ++count;
        node->next = m_free;
        m_free = node;
    }
    return true;
}

// swd32.exe 0x00485C20: leaves m_opened set
bool SoundEffects::Close() {
    if (!IsOpen()) {
        return true;
    }
    while (SampleNode* node = m_active) {
        m_active = node->next;
        AIL_end_sample(node->sample);
        AIL_release_sample_handle(node->sample);
        AIL_mem_free_lock(node);
    }
    while (SampleNode* node = m_free) {
        m_free = node->next;
        AIL_end_sample(node->sample);
        AIL_release_sample_handle(node->sample);
        AIL_mem_free_lock(node);
    }
    FreeIndex();
    AIL_waveOutClose(m_driver);
    return true;
}

// swd32.exe 0x00485CC0
bool SoundEffects::IsOpen() { return m_opened == 1; }

// swd32.exe 0x00485CD0
bool SoundEffects::IsEnabled() { return m_enabled == 1; }

// swd32.exe 0x00485CE0: plays `data` (RIFF image, else MP3 of `size` bytes) or sound `id` of all.snd. A failed
// sample file frees the image even when the caller passed it. Always returns 0.
int SoundEffects::Play(std::uint8_t* data, int id, int volume, int pan, int loops, int size) {
    if (!IsOpen() || !IsEnabled() || id == 0) {
        return 0;
    }
    std::uint8_t* image = data;
    if (data == nullptr) {
        image = LoadSound(id);
        if (image == nullptr) {
            return 0;
        }
    }
    SampleNode* node = AllocNode();
    if (node == nullptr) {
        return 0;
    }
    AIL_init_sample(node->sample);
    std::int32_t magic;
    std::memcpy(&magic, image, 4);
    const S32 loaded = magic == kRiff ? AIL_set_sample_file(node->sample, image, 0)
                                      : AIL_set_named_sample_file(node->sample, ".mp3", image, size, 0);
    if (!loaded) {
        std::strcpy(m_error, AIL_last_error());
        AIL_mem_free_lock(image);
        FreeNode(node);
        return 0;
    }
    ++m_table[id - 1].users;
    m_table[id - 1].data = image;
    AIL_set_sample_user_data(node->sample, 0, id);
    AIL_set_sample_volume(node->sample, ClampVolume(volume));
    AIL_set_sample_pan(node->sample, ClampPan(pan));
    AIL_set_sample_loop_count(node->sample, loops);
    AIL_start_sample(node->sample);
    PushActive(node);
    return 0;
}

// swd32.exe 0x00485E90
int SoundEffects::Stop(int id) {
    if (!IsOpen() || !IsEnabled() || id == 0) {
        return 0;
    }
    SampleNode* node = Unlink(id);
    if (node == nullptr) {
        return 0;
    }
    AIL_end_sample(node->sample);
    releaseSound(this, node->sample);
    FreeNode(node);
    return 0;
}

// swd32.exe 0x00485F30
bool SoundEffects::StopAll() {
    if (!IsOpen() || !IsEnabled()) {
        return false;
    }
    for (SampleNode* node = m_active; node != nullptr;) {
        AIL_end_sample(node->sample);
        releaseSound(this, node->sample);
        SampleNode* next = node->next;
        node->next = m_free;
        m_free = node;
        node = next;
    }
    m_active = nullptr;
    return true;
}

// swd32.exe 0x00485FE0
int SoundEffects::SetVolume(int id, int volume) {
    if (!IsOpen() || !IsEnabled()) {
        return 0;
    }
    SampleNode* node = Find(id);
    const int value = ClampVolume(volume);
    if (node != nullptr) {
        AIL_set_sample_volume(node->sample, value);
    }
    return 0;
}

// swd32.exe 0x00486030: returns the clamped pan
int SoundEffects::SetPan(int id, int pan) {
    if (!IsOpen() || !IsEnabled()) {
        return 0;
    }
    SampleNode* node = Find(id);
    const int value = ClampPan(pan);
    if (node != nullptr) {
        AIL_set_sample_pan(node->sample, value);
    }
    return value;
}

// swd32.exe 0x00486080: retires finished samples
bool SoundEffects::Update() {
    if (!IsOpen() || !IsEnabled()) {
        return false;
    }
    SampleNode* previous = sentinelOf(this);
    SampleNode* node = m_active;
    while (node != nullptr) {
        const U32 status = AIL_sample_status(node->sample);
        if (status != 0 && status <= SMP_DONE) {
            previous->next = node->next;
            node->next = m_free;
            m_free = node;
            AIL_set_sample_volume(node->sample, 0);
            AIL_end_sample(node->sample);
            releaseSound(this, node->sample);
            node = previous->next;
        } else {
            previous = node;
            node = node->next;
        }
    }
    return true;
}

// swd32.exe 0x00486160
bool SoundEffects::FreeNode(SampleNode* node) {
    if (node == nullptr) {
        return false;
    }
    node->next = m_free;
    m_free = node;
    return true;
}

// swd32.exe 0x00486190
SampleNode* SoundEffects::AllocNode() {
    SampleNode* node = m_free;
    m_free = nullptr;
    if (node != nullptr) {
        m_free = node->next;
    }
    return node;
}

// swd32.exe 0x004861B0
SampleNode* SoundEffects::PushActive(SampleNode* node) {
    if (node == nullptr) {
        return nullptr;
    }
    node->next = m_active;
    m_active = node;
    return node;
}

// swd32.exe 0x004861D0
SampleNode* SoundEffects::Find(int id) {
    for (SampleNode* node = m_active; node != nullptr; node = node->next) {
        if (id == AIL_sample_user_data(node->sample, 0)) {
            return node;
        }
    }
    return nullptr;
}

// swd32.exe 0x00486210
SampleNode* SoundEffects::Unlink(int id) {
    SampleNode* previous = sentinelOf(this);
    for (SampleNode* node = m_active; node != nullptr; node = node->next, previous = previous->next) {
        if (id == AIL_sample_user_data(node->sample, 0)) {
            previous->next = node->next;
            return node;
        }
    }
    return nullptr;
}

// swd32.exe 0x004862A0
HDIGDRIVER SoundEffects::GetDriver() { return m_driver; }

// swd32.exe 0x004862B0: reads the 3000 entry index of <directory>all.snd; the path buffer leaks when the file
// cannot be opened
bool SoundEffects::LoadIndex(const char* directory) {
    auto* path = static_cast<char*>(AIL_mem_alloc_lock(0x400));
    std::strcpy(path, directory);
    std::strcat(path, "all.snd");
    if (!File::Open(path, w32::OPEN_ALWAYS, w32::GENERIC_READ)) {
        return false;
    }
    AIL_mem_free_lock(path);
    m_table = static_cast<SoundEntry*>(AIL_mem_alloc_lock(kSoundTableBytes));
    std::memset(m_table, 0, kSoundTableBytes);
    SeekBegin(0x1C);
    w32::DWORD bytes = kSoundIndexBytes;
    auto* index = static_cast<std::uint32_t*>(AIL_mem_alloc_lock(kSoundIndexBytes));
    Read(index, &bytes);
    for (int i = 0; i < kSoundEntries; ++i) {
        const std::uint32_t* entry = index + 0xB * i;
        m_table[i].sizeFlags = entry[5] & 0x3FFFFFF;
        m_table[i].offset = entry[6];
        m_table[i].users = 0;
        m_table[i].data = nullptr;
        m_table[i].sizeFlags |= (entry[8] & 3) << 0x1A;
    }
    AIL_mem_free_lock(index);
    return true;
}

// swd32.exe 0x00486430
bool SoundEffects::FreeIndex() {
    if (m_table == nullptr) {
        return true;
    }
    for (int i = 0; i < kSoundEntries; ++i) {
        if (m_table[i].data != nullptr) {
            AIL_mem_free_lock(m_table[i].data);
        }
    }
    AIL_mem_free_lock(m_table);
    m_table = nullptr;
    File::Close();
    return true;
}

// swd32.exe 0x00486490: sound `id` as a RIFF image. Kind 0 entries are a fmt record without the RIFF header, which
// is rebuilt in front of it; kinds other than 1 get the header fields patched (kinds 2 and 3 overwrite their own
// data, and short sounds overflow the buffer with the template string).
std::uint8_t* SoundEffects::LoadSound(int id) {
    const SoundEntry& entry = m_table[id - 1];
    if (entry.offset == 0) {
        return nullptr;
    }
    SeekBegin(static_cast<w32::LONG>(entry.offset));
    std::uint32_t pad = 0;
    const std::uint32_t sizeFlags = entry.sizeFlags;
    w32::DWORD size = sizeFlags & 0x3FFFFFF;
    const std::uint32_t kind = sizeFlags >> 0x1A;
    if (kind == 0) {
        pad = 0x18;
    }
    auto* image = static_cast<std::uint8_t*>(AIL_mem_alloc_lock(size + pad));
    std::strcpy(reinterpret_cast<char*>(image), "RIFF....WAVEfmt ....0123456789012345data..");
    Read(image + pad, &size);
    if (kind != 1) {
        std::memmove(image + 0x14, image + 0x18, 0x12);
        const std::uint32_t fmtSize = 0x10;
        std::memcpy(image + 0x10, &fmtSize, 4);
        std::memcpy(image + 4, &size, 4);
        const std::uint16_t ta = 0x6174;
        std::memcpy(image + 0x26, &ta, 2);
        const std::uint32_t dataSize = size - 0x18;
        std::memcpy(image + 0x28, &dataSize, 4);
    }
    return image;
}

// swd32.exe 0x004853E0
MusicQueue::MusicQueue() {
    m_unknown4 = 0;
    m_unknown8 = 0;
    m_mode = 3;
    m_pending = 3;
    m_unknown0 = 0x1E;
    m_volume = 0x7F;
    std::memset(&m_current, 0, sizeof(m_current));
    std::memset(m_list2, 0, sizeof(m_list2));
    std::memset(m_list1, 0, sizeof(m_list1));
}

// swd32.exe 0x00485440
MusicQueue::~MusicQueue() {
    ClearSlots(2);
    ClearSlots(1);
}

// swd32.exe 0x00485460
int MusicQueue::ClearSlots(int kind) {
    MusicSlot* slots;
    if (kind == 1) {
        slots = m_list1;
    } else if (kind == 2) {
        slots = m_list2;
    } else {
        return 0;
    }
    for (int i = 0; i < 2; ++i) {
        if (slots[i].name != nullptr) {
            AIL_mem_free_lock(slots[i].name);
        }
    }
    std::memset(slots, 0, 2 * sizeof(MusicSlot));
    return 0;
}

// swd32.exe 0x004854B0: once the current piece has stopped, applies a pending list switch and starts the next
// entry of the current list (a stream start is preceded by MessageBeep)
int MusicQueue::Update() {
    if (m_current.type == 1) {
        if (!G.streamPlayer.IsStopped(m_current.id)) {
            return 0;
        }
    } else if (m_current.type == 2) {
        if (!G.midiPlayer.IsStopped(m_current.id)) {
            return 0;
        }
    }
    std::memset(&m_current, 0, sizeof(m_current));
    const std::int32_t pending = m_pending;
    if (pending > 0) {
        if (pending <= 2) {
            m_mode = pending;
            m_pending = 0;
            if (pending == 1) {
                m_index1 = 0;
            } else {
                m_index2 = 0;
            }
        } else if (pending == 3) {
            m_mode = 0;
            m_pending = 0;
        }
    }
    std::int32_t* index;
    std::int32_t* loop;
    MusicSlot* slot;
    if (m_mode == 1) {
        index = &m_index1;
        if (*index >= 2) {
            return 0;
        }
        slot = &m_list1[*index];
        loop = &m_loop1;
    } else if (m_mode == 2) {
        index = &m_index2;
        if (*index >= 2) {
            return 0;
        }
        slot = &m_list2[*index];
        loop = &m_loop2;
    } else {
        return 0;
    }
    m_current = *slot;
    if (m_current.name != nullptr) {
        if (m_current.type == 1) {
            w32::MessageBeep(0);
            G.streamPlayer.Play(m_current.name, m_current.id, m_volume, 1);
        } else if (m_current.type == 2) {
            G.midiPlayer.Play(m_current.name, m_current.id, m_volume, 1);
        }
    }
    ++*index;
    if (*loop == 1 && *index >= 2) {
        *index = 0;
    }
    return 0;
}

// swd32.exe 0x00485610: volume 0..11 of the options scaled to 0..128
int PlaySound(std::uint16_t id, int volume) {
    return G.soundEffects.Play(nullptr, id, (volume << 7) / 11, 0, 1, 0);
}

// swd32.exe 0x00485650
int SetSoundPan(std::uint16_t id, int pan) { return G.soundEffects.SetPan(id, pan); }

// swd32.exe 0x00485670
int PlaySoundAt(std::uint16_t id, std::uint16_t volume) {
    G.soundEffects.Play(nullptr, id, (volume << 7) / 11, 0, 1, 0);
    return 1;
}

// swd32.exe 0x00485710
int StopMusic() { return G.streamPlayer.FadeOut(100, 1); }

// swd32.exe 0x00485720
int StopSound(std::uint16_t id) {
    G.soundEffects.Stop(id);
    return 1;
}

// swd32.exe 0x00485740
int StopAllSounds() {
    G.soundEffects.StopAll();
    return 1;
}

// swd32.exe 0x00485830
bool IsMusicStopped() { return G.streamPlayer.IsStopped(100); }

// swd32.exe 0x00485850
int SetMusicVolume(int volume) { return G.streamPlayer.SetVolume(100, (volume << 7) / 11); }

// swd32.exe 0x00485880
int UpdateMusicFade() {
    if (G.musicFadeMode == 1) {
        G.streamPlayer.FadeOut(100, 1);
        G.musicFadeMode = 0;
        G.musicFading = 0;
        return 0;
    }
    if (G.musicFadeMode == 2) {
        const int result = G.streamPlayer.FadeOut(100, G.musicFadeLength);
        G.musicFading = G.musicFadeLength;
        return result;
    }
    return G.musicFadeMode - 2;
}

// swd32.exe 0x004858D0
int PollMusicFade() {
    if (G.musicFadeMode == 0) {
        return 0;
    }
    if (G.musicFadeMode != 2) {
        return G.musicFadeMode - 2;
    }
    const bool stopped = G.streamPlayer.IsStopped(100);
    if (stopped) {
        G.musicFadeMode = 0;
        G.musicFading = 0;
    }
    return stopped;
}

// swd32.exe 0x00485910
int SetMusicFade(int mode, int length) {
    G.musicFadeMode = mode;
    if (mode == 2) {
        G.musicFadeLength = length;
        return length;
    }
    return mode;
}

}  // namespace swd3
