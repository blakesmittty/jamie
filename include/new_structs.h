#pragma once

#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "vosk_api.h"
#include "mpg123.h"

struct Mic {
    std::queue<std::vector<short>> micAudioQueue;
    std::mutex micMutex;
    std::condition_variable cv;
    std::atomic<bool> quit{false};
    VoskRecognizer *recognizer = nullptr;
};

struct Playback {
    mpg123_handle *decoder;
    std::queue<float> pcm;
    std::mutex playbackMutex;
    std::atomic<bool> quit{false};
    std::atomic<bool> isPlaying{false};
};

struct AudioContext {
    Mic *mic;
    Playback *playback;
};

struct MP3Buffer {
    std::vector<unsigned char> data;
};

