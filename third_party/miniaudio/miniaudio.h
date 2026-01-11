/*
 * miniaudio - Single file audio playback and capture library
 *
 * This is a minimal stub providing the interface needed for SODA Player.
 * For production use, download the full library from:
 * https://github.com/mackron/miniaudio
 *
 * This stub uses ALSA directly on Linux.
 */

#ifndef MINIAUDIO_H
#define MINIAUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Result codes */
typedef enum {
    MA_SUCCESS = 0,
    MA_ERROR = -1,
    MA_INVALID_ARGS = -2,
    MA_INVALID_OPERATION = -3,
    MA_OUT_OF_MEMORY = -4,
    MA_OUT_OF_RANGE = -5,
    MA_ACCESS_DENIED = -6,
    MA_DOES_NOT_EXIST = -7,
    MA_ALREADY_EXISTS = -8,
    MA_TOO_MANY_OPEN_FILES = -9,
    MA_INVALID_FILE = -10,
    MA_TOO_BIG = -11,
    MA_PATH_TOO_LONG = -12,
    MA_NAME_TOO_LONG = -13,
    MA_NOT_DIRECTORY = -14,
    MA_IS_DIRECTORY = -15,
    MA_DIRECTORY_NOT_EMPTY = -16,
    MA_AT_END = -17,
    MA_NO_SPACE = -18,
    MA_BUSY = -19,
    MA_IO_ERROR = -20,
    MA_INTERRUPT = -21,
    MA_UNAVAILABLE = -22,
    MA_ALREADY_IN_USE = -23,
    MA_BAD_ADDRESS = -24,
    MA_BAD_SEEK = -25,
    MA_BAD_PIPE = -26,
    MA_DEADLOCK = -27,
    MA_TOO_MANY_LINKS = -28,
    MA_NOT_IMPLEMENTED = -29,
    MA_NO_MESSAGE = -30,
    MA_BAD_MESSAGE = -31,
    MA_NO_DATA_AVAILABLE = -32,
    MA_INVALID_DATA = -33,
    MA_TIMEOUT = -34,
    MA_NO_NETWORK = -35,
    MA_NOT_UNIQUE = -36,
    MA_NOT_SOCKET = -37,
    MA_NO_ADDRESS = -38,
    MA_BAD_PROTOCOL = -39,
    MA_PROTOCOL_UNAVAILABLE = -40,
    MA_PROTOCOL_NOT_SUPPORTED = -41,
    MA_PROTOCOL_FAMILY_NOT_SUPPORTED = -42,
    MA_ADDRESS_FAMILY_NOT_SUPPORTED = -43,
    MA_SOCKET_NOT_SUPPORTED = -44,
    MA_CONNECTION_RESET = -45,
    MA_ALREADY_CONNECTED = -46,
    MA_NOT_CONNECTED = -47,
    MA_CONNECTION_REFUSED = -48,
    MA_NO_HOST = -49,
    MA_IN_PROGRESS = -50,
    MA_CANCELLED = -51,
    MA_MEMORY_ALREADY_MAPPED = -52,
    MA_FORMAT_NOT_SUPPORTED = -100
} ma_result;

/* Format */
typedef enum {
    ma_format_unknown = 0,
    ma_format_u8 = 1,
    ma_format_s16 = 2,
    ma_format_s24 = 3,
    ma_format_s32 = 4,
    ma_format_f32 = 5
} ma_format;

/* Channel layout */
typedef enum {
    ma_channel_mix_mode_rectangular = 0,
    ma_channel_mix_mode_simple,
    ma_channel_mix_mode_custom_weights
} ma_channel_mix_mode;

/* Decoder config */
typedef struct {
    ma_format format;
    uint32_t channels;
    uint32_t sampleRate;
} ma_decoder_config;

/* Decoder */
typedef struct {
    ma_format outputFormat;
    uint32_t outputChannels;
    uint32_t outputSampleRate;
    void* pUserData;
    void* pBackend;
    uint64_t totalFrameCount;
    uint64_t currentFrame;
    bool atEnd;
} ma_decoder;

/* Device config */
typedef struct {
    int deviceType;
    uint32_t sampleRate;
    uint32_t channels;
    ma_format format;
    uint32_t periodSizeInFrames;
    uint32_t periods;
    void (*dataCallback)(void* pDevice, void* pOutput, const void* pInput, uint32_t frameCount);
    void* pUserData;
} ma_device_config;

/* Device */
typedef struct {
    void* pContext;
    int type;
    uint32_t sampleRate;
    uint32_t channels;
    ma_format format;
    void (*dataCallback)(void* pDevice, void* pOutput, const void* pInput, uint32_t frameCount);
    void* pUserData;
    void* pBackend;
    bool isStarted;
} ma_device;

/* Device type */
typedef enum {
    ma_device_type_playback = 1,
    ma_device_type_capture = 2,
    ma_device_type_duplex = 3,
    ma_device_type_loopback = 4
} ma_device_type;

/* Context */
typedef struct {
    void* pBackend;
} ma_context;

/* Function declarations */
ma_decoder_config ma_decoder_config_init(ma_format format, uint32_t channels, uint32_t sampleRate);
ma_result ma_decoder_init_file(const char* pFilePath, const ma_decoder_config* pConfig, ma_decoder* pDecoder);
ma_result ma_decoder_read_pcm_frames(ma_decoder* pDecoder, void* pFramesOut, uint64_t frameCount, uint64_t* pFramesRead);
ma_result ma_decoder_seek_to_pcm_frame(ma_decoder* pDecoder, uint64_t frameIndex);
ma_result ma_decoder_get_length_in_pcm_frames(ma_decoder* pDecoder, uint64_t* pLength);
ma_result ma_decoder_uninit(ma_decoder* pDecoder);

ma_device_config ma_device_config_init(ma_device_type deviceType);
ma_result ma_device_init(ma_context* pContext, const ma_device_config* pConfig, ma_device* pDevice);
ma_result ma_device_start(ma_device* pDevice);
ma_result ma_device_stop(ma_device* pDevice);
void ma_device_uninit(ma_device* pDevice);

#ifdef __cplusplus
}
#endif

/* Implementation */
#ifdef MINIAUDIO_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <dlfcn.h>

/* Internal decoder backend (uses libsndfile if available, otherwise raw PCM) */
typedef struct {
    FILE* file;
    char* filepath;
    uint64_t dataOffset;
    uint64_t dataSize;
    uint32_t bytesPerFrame;
    void* sndfile;
    void* sndfileLib;
} ma_decoder_backend;

typedef struct {
    snd_pcm_t* pcm;
    pthread_t thread;
    bool running;
    ma_device* device;
} ma_device_backend;

ma_decoder_config ma_decoder_config_init(ma_format format, uint32_t channels, uint32_t sampleRate) {
    ma_decoder_config config;
    config.format = format;
    config.channels = channels;
    config.sampleRate = sampleRate;
    return config;
}

/* Simple WAV header parser */
typedef struct {
    char riff[4];
    uint32_t fileSize;
    char wave[4];
} wav_header;

typedef struct {
    char id[4];
    uint32_t size;
} wav_chunk;

typedef struct {
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
} wav_fmt;

static bool parse_wav_header(FILE* f, wav_fmt* fmt, uint64_t* dataOffset, uint64_t* dataSize) {
    wav_header header;
    if (fread(&header, sizeof(header), 1, f) != 1) return false;
    if (memcmp(header.riff, "RIFF", 4) != 0 || memcmp(header.wave, "WAVE", 4) != 0) return false;

    wav_chunk chunk;
    bool foundFmt = false, foundData = false;

    while (fread(&chunk, sizeof(chunk), 1, f) == 1) {
        if (memcmp(chunk.id, "fmt ", 4) == 0) {
            if (chunk.size < sizeof(wav_fmt)) return false;
            if (fread(fmt, sizeof(wav_fmt), 1, f) != 1) return false;
            if (chunk.size > sizeof(wav_fmt)) {
                fseek(f, chunk.size - sizeof(wav_fmt), SEEK_CUR);
            }
            foundFmt = true;
        } else if (memcmp(chunk.id, "data", 4) == 0) {
            *dataOffset = ftell(f);
            *dataSize = chunk.size;
            foundData = true;
            break;
        } else {
            fseek(f, chunk.size, SEEK_CUR);
        }
    }

    return foundFmt && foundData;
}

ma_result ma_decoder_init_file(const char* pFilePath, const ma_decoder_config* pConfig, ma_decoder* pDecoder) {
    if (!pFilePath || !pDecoder) return MA_INVALID_ARGS;

    memset(pDecoder, 0, sizeof(*pDecoder));

    ma_decoder_backend* backend = (ma_decoder_backend*)calloc(1, sizeof(ma_decoder_backend));
    if (!backend) return MA_OUT_OF_MEMORY;

    backend->file = fopen(pFilePath, "rb");
    if (!backend->file) {
        free(backend);
        return MA_DOES_NOT_EXIST;
    }

    backend->filepath = strdup(pFilePath);

    /* Try to parse as WAV */
    wav_fmt fmt;
    if (parse_wav_header(backend->file, &fmt, &backend->dataOffset, &backend->dataSize)) {
        pDecoder->outputChannels = fmt.numChannels;
        pDecoder->outputSampleRate = fmt.sampleRate;
        pDecoder->outputFormat = (fmt.bitsPerSample == 16) ? ma_format_s16 :
                                 (fmt.bitsPerSample == 32) ? ma_format_f32 : ma_format_s16;
        backend->bytesPerFrame = fmt.numChannels * (fmt.bitsPerSample / 8);
        pDecoder->totalFrameCount = backend->dataSize / backend->bytesPerFrame;
        fseek(backend->file, backend->dataOffset, SEEK_SET);
    } else {
        /* Fallback: try sndfile for other formats */
        rewind(backend->file);
        fclose(backend->file);
        backend->file = NULL;

        /* Try loading libsndfile dynamically */
        backend->sndfileLib = dlopen("libsndfile.so.1", RTLD_LAZY);
        if (backend->sndfileLib) {
            typedef void* (*sf_open_t)(const char*, int, void*);
            typedef int64_t (*sf_readf_float_t)(void*, float*, int64_t);
            typedef int64_t (*sf_seek_t)(void*, int64_t, int);
            typedef int (*sf_close_t)(void*);

            sf_open_t sf_open = (sf_open_t)dlsym(backend->sndfileLib, "sf_open");
            if (sf_open) {
                struct {
                    int64_t frames;
                    int samplerate;
                    int channels;
                    int format;
                    int sections;
                    int seekable;
                } sfinfo = {0};

                backend->sndfile = sf_open(pFilePath, 0x10, &sfinfo); /* SFM_READ = 0x10 */
                if (backend->sndfile) {
                    pDecoder->outputChannels = sfinfo.channels;
                    pDecoder->outputSampleRate = sfinfo.samplerate;
                    pDecoder->outputFormat = ma_format_f32;
                    pDecoder->totalFrameCount = sfinfo.frames;
                    backend->bytesPerFrame = sfinfo.channels * sizeof(float);
                }
            }
        }

        if (!backend->sndfile) {
            free(backend->filepath);
            if (backend->sndfileLib) dlclose(backend->sndfileLib);
            free(backend);
            return MA_FORMAT_NOT_SUPPORTED;
        }
    }

    pDecoder->pBackend = backend;
    pDecoder->currentFrame = 0;
    pDecoder->atEnd = false;

    return MA_SUCCESS;
}

ma_result ma_decoder_read_pcm_frames(ma_decoder* pDecoder, void* pFramesOut, uint64_t frameCount, uint64_t* pFramesRead) {
    if (!pDecoder || !pDecoder->pBackend) return MA_INVALID_ARGS;

    ma_decoder_backend* backend = (ma_decoder_backend*)pDecoder->pBackend;
    uint64_t framesRead = 0;

    if (backend->file) {
        /* WAV reading */
        size_t bytesToRead = frameCount * backend->bytesPerFrame;
        size_t bytesRead = fread(pFramesOut, 1, bytesToRead, backend->file);
        framesRead = bytesRead / backend->bytesPerFrame;
    } else if (backend->sndfile) {
        /* sndfile reading */
        typedef int64_t (*sf_readf_float_t)(void*, float*, int64_t);
        sf_readf_float_t sf_readf_float = (sf_readf_float_t)dlsym(backend->sndfileLib, "sf_readf_float");
        if (sf_readf_float) {
            framesRead = sf_readf_float(backend->sndfile, (float*)pFramesOut, frameCount);
        }
    }

    pDecoder->currentFrame += framesRead;
    if (framesRead < frameCount) {
        pDecoder->atEnd = true;
    }

    if (pFramesRead) *pFramesRead = framesRead;
    return (framesRead > 0) ? MA_SUCCESS : MA_AT_END;
}

ma_result ma_decoder_seek_to_pcm_frame(ma_decoder* pDecoder, uint64_t frameIndex) {
    if (!pDecoder || !pDecoder->pBackend) return MA_INVALID_ARGS;

    ma_decoder_backend* backend = (ma_decoder_backend*)pDecoder->pBackend;

    if (backend->file) {
        long offset = backend->dataOffset + (frameIndex * backend->bytesPerFrame);
        if (fseek(backend->file, offset, SEEK_SET) != 0) return MA_ERROR;
    } else if (backend->sndfile) {
        typedef int64_t (*sf_seek_t)(void*, int64_t, int);
        sf_seek_t sf_seek = (sf_seek_t)dlsym(backend->sndfileLib, "sf_seek");
        if (sf_seek) {
            if (sf_seek(backend->sndfile, frameIndex, SEEK_SET) < 0) return MA_ERROR;
        }
    }

    pDecoder->currentFrame = frameIndex;
    pDecoder->atEnd = false;
    return MA_SUCCESS;
}

ma_result ma_decoder_get_length_in_pcm_frames(ma_decoder* pDecoder, uint64_t* pLength) {
    if (!pDecoder || !pLength) return MA_INVALID_ARGS;
    *pLength = pDecoder->totalFrameCount;
    return MA_SUCCESS;
}

ma_result ma_decoder_uninit(ma_decoder* pDecoder) {
    if (!pDecoder || !pDecoder->pBackend) return MA_INVALID_ARGS;

    ma_decoder_backend* backend = (ma_decoder_backend*)pDecoder->pBackend;

    if (backend->file) fclose(backend->file);
    if (backend->sndfile) {
        typedef int (*sf_close_t)(void*);
        sf_close_t sf_close = (sf_close_t)dlsym(backend->sndfileLib, "sf_close");
        if (sf_close) sf_close(backend->sndfile);
    }
    if (backend->sndfileLib) dlclose(backend->sndfileLib);
    free(backend->filepath);
    free(backend);

    memset(pDecoder, 0, sizeof(*pDecoder));
    return MA_SUCCESS;
}

ma_device_config ma_device_config_init(ma_device_type deviceType) {
    ma_device_config config;
    memset(&config, 0, sizeof(config));
    config.deviceType = deviceType;
    config.sampleRate = 44100;
    config.channels = 2;
    config.format = ma_format_f32;
    config.periodSizeInFrames = 1024;
    config.periods = 3;
    return config;
}

static void* audio_thread(void* arg) {
    ma_device* device = (ma_device*)arg;
    ma_device_backend* backend = (ma_device_backend*)device->pBackend;

    size_t bufferSize = 1024 * device->channels * sizeof(float);
    void* buffer = malloc(bufferSize);

    while (backend->running) {
        if (device->dataCallback) {
            device->dataCallback(device, buffer, NULL, 1024);

            snd_pcm_sframes_t frames = snd_pcm_writei(backend->pcm, buffer, 1024);
            if (frames < 0) {
                snd_pcm_recover(backend->pcm, frames, 0);
            }
        }
    }

    free(buffer);
    return NULL;
}

ma_result ma_device_init(ma_context* pContext, const ma_device_config* pConfig, ma_device* pDevice) {
    if (!pConfig || !pDevice) return MA_INVALID_ARGS;

    memset(pDevice, 0, sizeof(*pDevice));

    ma_device_backend* backend = (ma_device_backend*)calloc(1, sizeof(ma_device_backend));
    if (!backend) return MA_OUT_OF_MEMORY;

    int err = snd_pcm_open(&backend->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        free(backend);
        return MA_UNAVAILABLE;
    }

    snd_pcm_hw_params_t* hwparams;
    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_hw_params_any(backend->pcm, hwparams);

    snd_pcm_hw_params_set_access(backend->pcm, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(backend->pcm, hwparams, SND_PCM_FORMAT_FLOAT_LE);

    unsigned int rate = pConfig->sampleRate ? pConfig->sampleRate : 44100;
    snd_pcm_hw_params_set_rate_near(backend->pcm, hwparams, &rate, 0);

    unsigned int channels = pConfig->channels ? pConfig->channels : 2;
    snd_pcm_hw_params_set_channels(backend->pcm, hwparams, channels);

    snd_pcm_uframes_t buffer_size = pConfig->periodSizeInFrames * pConfig->periods;
    snd_pcm_hw_params_set_buffer_size_near(backend->pcm, hwparams, &buffer_size);

    err = snd_pcm_hw_params(backend->pcm, hwparams);
    if (err < 0) {
        snd_pcm_close(backend->pcm);
        free(backend);
        return MA_ERROR;
    }

    pDevice->sampleRate = rate;
    pDevice->channels = channels;
    pDevice->format = ma_format_f32;
    pDevice->dataCallback = pConfig->dataCallback;
    pDevice->pUserData = pConfig->pUserData;
    pDevice->pBackend = backend;
    pDevice->isStarted = false;
    backend->device = pDevice;

    return MA_SUCCESS;
}

ma_result ma_device_start(ma_device* pDevice) {
    if (!pDevice || !pDevice->pBackend) return MA_INVALID_ARGS;
    if (pDevice->isStarted) return MA_SUCCESS;

    ma_device_backend* backend = (ma_device_backend*)pDevice->pBackend;
    backend->running = true;

    if (pthread_create(&backend->thread, NULL, audio_thread, pDevice) != 0) {
        return MA_ERROR;
    }

    pDevice->isStarted = true;
    return MA_SUCCESS;
}

ma_result ma_device_stop(ma_device* pDevice) {
    if (!pDevice || !pDevice->pBackend) return MA_INVALID_ARGS;
    if (!pDevice->isStarted) return MA_SUCCESS;

    ma_device_backend* backend = (ma_device_backend*)pDevice->pBackend;
    backend->running = false;

    pthread_join(backend->thread, NULL);
    snd_pcm_drain(backend->pcm);

    pDevice->isStarted = false;
    return MA_SUCCESS;
}

void ma_device_uninit(ma_device* pDevice) {
    if (!pDevice || !pDevice->pBackend) return;

    if (pDevice->isStarted) {
        ma_device_stop(pDevice);
    }

    ma_device_backend* backend = (ma_device_backend*)pDevice->pBackend;
    snd_pcm_close(backend->pcm);
    free(backend);

    memset(pDevice, 0, sizeof(*pDevice));
}

#endif /* MINIAUDIO_IMPLEMENTATION */

#endif /* MINIAUDIO_H */
