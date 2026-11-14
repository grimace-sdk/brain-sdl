#include "SDL_internal.h"

#include "../core/SDL_wince.h"
#include "audio/SDL_sysaudio.h"

#include <mmsystem.h>

#define WAVEOUT_BUFFERS 4
#define WAVEOUT_MIN_FRAMES 2048
#define WAVEOUT_LATE_LIMIT 8
#define WAVEOUT_RECOVERED 64

struct SDL_PrivateAudioData
{
    HWAVEOUT handle;
    HANDLE done;
    Uint8 *memory;
    WAVEHDR headers[WAVEOUT_BUFFERS];
    int next;
    int written;
    int late;
    int on_time;
    bool demoted;
    bool log_stats;
    Uint64 fill_started;
    Uint64 fill_total;
    Uint64 fill_max;
    Uint64 stats_started;
    int stats_buffers;
    int stats_late;
};

static bool WAVEOUT_SetError(const char *function, MMRESULT result)
{
    return SDL_SetError("%s failed: %u", function, (unsigned int)result);
}

static bool WAVEOUT_OpenDevice(SDL_AudioDevice *device)
{
    struct SDL_PrivateAudioData *hidden;
    WAVEFORMATEX format;
    MMRESULT result;
    int index;

    hidden = (struct SDL_PrivateAudioData *)SDL_calloc(1, sizeof(*hidden));
    if (!hidden) {
        return false;
    }
    device->hidden = hidden;

    device->spec.format = SDL_AUDIO_S16LE;
    if (device->spec.freq != 11025 && device->spec.freq != 22050) {
        device->spec.freq = 44100;
    }
    device->spec.channels = device->spec.channels >= 2 ? 2 : 1;
    if (device->sample_frames < WAVEOUT_MIN_FRAMES) {
        device->sample_frames = WAVEOUT_MIN_FRAMES;
    }
    SDL_UpdatedAudioDeviceFormat(device);

    SDL_zero(format);
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = (WORD)device->spec.channels;
    format.nSamplesPerSec = (DWORD)device->spec.freq;
    format.wBitsPerSample = 16;
    format.nBlockAlign = (WORD)(format.nChannels * 2);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

    hidden->log_stats = SDL_GetHintBoolean("SDL_BRAIN_LOG_FPS", false);

    hidden->done = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!hidden->done) {
        return WINCE_SetError("CreateEventW()");
    }

    result = waveOutOpen(&hidden->handle, WAVE_MAPPER, &format, (DWORD)hidden->done, 0, CALLBACK_EVENT);
    if (result != MMSYSERR_NOERROR) {
        hidden->handle = NULL;
        return WAVEOUT_SetError("waveOutOpen()", result);
    }

    hidden->memory = (Uint8 *)SDL_malloc((size_t)device->buffer_size * WAVEOUT_BUFFERS);
    if (!hidden->memory) {
        return false;
    }
    SDL_memset(hidden->memory, device->silence_value, (size_t)device->buffer_size * WAVEOUT_BUFFERS);

    for (index = 0; index < WAVEOUT_BUFFERS; index++) {
        WAVEHDR *header = &hidden->headers[index];
        header->lpData = (LPSTR)(hidden->memory + (size_t)index * device->buffer_size);
        header->dwBufferLength = (DWORD)device->buffer_size;
        result = waveOutPrepareHeader(hidden->handle, header, sizeof(*header));
        if (result != MMSYSERR_NOERROR) {
            return WAVEOUT_SetError("waveOutPrepareHeader()", result);
        }
        header->dwFlags |= WHDR_DONE;
    }
    return true;
}

static void TrackLateness(struct SDL_PrivateAudioData *hidden, bool late)
{
    if (hidden->written < WAVEOUT_BUFFERS) {
        return;
    }
    if (late) {
        hidden->on_time = 0;
        if (++hidden->late >= WAVEOUT_LATE_LIMIT && !hidden->demoted) {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
            hidden->demoted = true;
            SDL_Log("waveout: audio can't keep up, running at normal priority");
        }
    } else {
        hidden->late = 0;
        if (++hidden->on_time >= WAVEOUT_RECOVERED && hidden->demoted) {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
            hidden->demoted = false;
            SDL_Log("waveout: audio caught up, back to raised priority");
        }
    }
}

static void LogStats(SDL_AudioDevice *device, bool late)
{
    struct SDL_PrivateAudioData *hidden = device->hidden;
    Uint64 now = SDL_GetTicksNS();

    if (hidden->stats_started == 0) {
        hidden->stats_started = now;
    }
    hidden->stats_late += late ? 1 : 0;
    if (now - hidden->stats_started < SDL_NS_PER_SECOND * 5) {
        return;
    }
    SDL_Log("waveout: %d buffers of %d frames, %d late, fill avg %u us max %u us, %d Hz %d ch",
            hidden->stats_buffers, device->sample_frames, hidden->stats_late,
            (unsigned int)(hidden->stats_buffers ? hidden->fill_total / hidden->stats_buffers / 1000 : 0),
            (unsigned int)(hidden->fill_max / 1000), device->spec.freq, device->spec.channels);
    hidden->stats_started = now;
    hidden->stats_buffers = 0;
    hidden->stats_late = 0;
    hidden->fill_total = 0;
    hidden->fill_max = 0;
}

static bool WAVEOUT_WaitDevice(SDL_AudioDevice *device)
{
    struct SDL_PrivateAudioData *hidden = device->hidden;
    WAVEHDR *header = &hidden->headers[hidden->next];

    while (!(header->dwFlags & WHDR_DONE)) {
        if (SDL_GetAtomicInt(&device->shutdown)) {
            return true;
        }
        WaitForSingleObject(hidden->done, 100);
    }
    hidden->fill_started = SDL_GetTicksNS();
    return true;
}

static Uint8 *WAVEOUT_GetDeviceBuf(SDL_AudioDevice *device, int *buffer_size)
{
    struct SDL_PrivateAudioData *hidden = device->hidden;
    return (Uint8 *)hidden->headers[hidden->next].lpData;
}

static bool WAVEOUT_PlayDevice(SDL_AudioDevice *device, const Uint8 *buffer, int buflen)
{
    struct SDL_PrivateAudioData *hidden = device->hidden;
    WAVEHDR *header = &hidden->headers[hidden->next];
    bool late = (hidden->headers[(hidden->next + WAVEOUT_BUFFERS - 1) % WAVEOUT_BUFFERS].dwFlags & WHDR_DONE) != 0;
    MMRESULT result;

    TrackLateness(hidden, late);
    if (hidden->log_stats) {
        LogStats(device, late && hidden->written >= WAVEOUT_BUFFERS);
    }
    header->dwFlags &= ~WHDR_DONE;
    result = waveOutWrite(hidden->handle, header, sizeof(*header));
    if (result != MMSYSERR_NOERROR) {
        header->dwFlags |= WHDR_DONE;
        return WAVEOUT_SetError("waveOutWrite()", result);
    }
    hidden->next = (hidden->next + 1) % WAVEOUT_BUFFERS;
    hidden->written++;
    if (hidden->log_stats && hidden->fill_started) {
        Uint64 fill = SDL_GetTicksNS() - hidden->fill_started;
        hidden->fill_total += fill;
        if (fill > hidden->fill_max) {
            hidden->fill_max = fill;
        }
        hidden->stats_buffers++;
    }
    return true;
}

static void WAVEOUT_CloseDevice(SDL_AudioDevice *device)
{
    struct SDL_PrivateAudioData *hidden = device->hidden;
    int index;

    if (!hidden) {
        return;
    }
    if (hidden->handle) {
        waveOutReset(hidden->handle);
        for (index = 0; index < WAVEOUT_BUFFERS; index++) {
            if (hidden->headers[index].dwFlags & WHDR_PREPARED) {
                waveOutUnprepareHeader(hidden->handle, &hidden->headers[index], sizeof(WAVEHDR));
            }
        }
        waveOutClose(hidden->handle);
    }
    if (hidden->done) {
        CloseHandle(hidden->done);
    }
    SDL_free(hidden->memory);
    SDL_free(hidden);
    device->hidden = NULL;
}

static bool WAVEOUT_Init(SDL_AudioDriverImpl *impl)
{
    if (waveOutGetNumDevs() == 0) {
        return SDL_SetError("No waveOut devices");
    }
    impl->OpenDevice = WAVEOUT_OpenDevice;
    impl->WaitDevice = WAVEOUT_WaitDevice;
    impl->GetDeviceBuf = WAVEOUT_GetDeviceBuf;
    impl->PlayDevice = WAVEOUT_PlayDevice;
    impl->CloseDevice = WAVEOUT_CloseDevice;
    impl->OnlyHasDefaultPlaybackDevice = true;
    return true;
}

AudioBootStrap PRIVATEAUDIO_bootstrap = {
    "waveout", "Windows CE waveOut", WAVEOUT_Init, false, false
};
