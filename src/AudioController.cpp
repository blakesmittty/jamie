
#include <iostream>

#include <portaudio.h>

#include "json.hpp"

#include "NetworkController.h"
#include "AudioController.h"

#define INPUT_SAMPLE_RATE 16000
#define INPUT_FRAMES_PER_BUFFER 512//1024

#define OUTPUT_SAMPLE_RATE 44100
#define OUTPUT_FRAMES_PER_BUFFER 512

static int inputCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
    //std::cout << "Mic callback triggered\n";
    if (!inputBuffer) {
        std::cout << "No input buffer!" << std::endl; 
        return paContinue;
    }
   // std::cout << "buffers there\n" << std::endl;
    auto *ctx = static_cast<AudioContext *>(userData);
    if (ctx->playback->isPlaying) {
        return paContinue;  
    }
    
    const float *input = (const float *)inputBuffer;

    std::vector<short> chunk(framesPerBuffer);
    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        chunk[i] = (short)(input[i] * 32767.0f);
    }

    {
        std::lock_guard<std::mutex> lock(ctx->mic->micMutex);
        ctx->mic->micAudioQueue.push(std::move(chunk));
    }
    
    ctx->mic->cv.notify_one();

    return paContinue;
}



static int outputCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
    auto *ctx = static_cast<AudioContext *>(userData);
    float *out = (float *) outputBuffer;

    std::lock_guard<std::mutex> lock(ctx->playback->playbackMutex);


    // for (unsigned long i = 0; i < framesPerBuffer; ++i) {
    //     if (!ctx->playback->pcm.empty()) {
    //         float sample = ctx->playback->pcm.front();
    //         ctx->playback->pcm.pop();
    //         *out++ = sample;
    //         *out++ = sample;
    //     } 
    // }

    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        // Check if we have at least 2 samples (for stereo output)
        if (ctx->playback->pcm.size() >= 2) {
            *out++ = ctx->playback->pcm.front();
            ctx->playback->pcm.pop();
            *out++ = ctx->playback->pcm.front();
            ctx->playback->pcm.pop();
        } else if (ctx->playback->pcm.size() == 1) {
            // Only one sample left - duplicate it for stereo
            float sample = ctx->playback->pcm.front();
            ctx->playback->pcm.pop();
            *out++ = sample;
            *out++ = sample;
        } else {
            // No samples left - output silence
            *out++ = 0.0f;
            *out++ = 0.0f;
        }
    }

    if (ctx->playback->pcm.empty()) {
        ctx->playback->isPlaying = false;
    }

    return paContinue;
   
}

AudioController::AudioController() {
    std::cout << "=== audio controller constructor start ====\n" << std::endl;


    // allocate our shit
    ctx = new AudioContext;
    ctx->mic = new Mic;
    ctx->playback = new Playback;

        // Explicitly initialize atomic values
    ctx->mic->quit.store(false);
    ctx->playback->quit.store(false);
    ctx->playback->isPlaying.store(false);

    std::cout << "loading model..\n" << std::endl;
    voskModel = vosk_model_new("external/vosk/vosk-model-small-en-us-0.15");
    if (!voskModel) {
        std::cout << "failed to load model\n" << std::endl;
        return;
    }
    std::cout << "model loaded successfully\n" << std::endl;


    std::cout << "loading recognizer...\n" << std::endl;
    voskRecognizer = vosk_recognizer_new(voskModel, INPUT_SAMPLE_RATE);
    if (!voskRecognizer) {
        std::cout << "failed to load recognizer\n" << std::endl;
        return;
    }
    std::cout << "recognizer loaded successfully\n" << std::endl;

    ctx->mic->recognizer = voskRecognizer;

    // initialize decoder
    decoder = mpg123_new(NULL,NULL);
    mpg123_open_feed(decoder);
    mpg123_format_none(decoder);
    mpg123_format(decoder, OUTPUT_SAMPLE_RATE, 2, MPG123_ENC_FLOAT_32); // PCM output

    PaError err;

    err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "error initializing port audio: " << Pa_GetErrorText(err) << "\n" << std::endl;
        vosk_recognizer_free(voskRecognizer);
        vosk_model_free(voskModel);
    }

    PaStream *inputStream;
    PaStream *outputStream;

    std::cout << "opening input stream...\n" << std::endl;
    err = Pa_OpenDefaultStream(&inputStream, 1, 0, paFloat32, INPUT_SAMPLE_RATE, INPUT_FRAMES_PER_BUFFER, inputCallback, ctx);
    if (err != paNoError) {
        std::cout << "error opening input stream: " << Pa_GetErrorText(err) << "\n" << std::endl;
    }
    std::cout << "input stream opened successfully\n" << std::endl;
    for (int i = 0; i < Pa_GetDeviceCount(); i++) {
        std::cout << "input device: " << Pa_GetDeviceInfo(i) << std::endl;
    }
    std::cout << "mic being used: " << Pa_GetDeviceInfo(Pa_GetDefaultInputDevice()) << std::endl;


    std::cout << "opening output stream...\n" << std::endl;
    err = Pa_OpenDefaultStream(&outputStream, 0, 2, paFloat32, OUTPUT_SAMPLE_RATE, OUTPUT_FRAMES_PER_BUFFER, outputCallback, ctx);
    if (err != paNoError) {
        std::cout << "error opening output stream: " << Pa_GetErrorText(err) << "\n" << std::endl;
    }
    std::cout << "output stream opened successfully\n" << std::endl;

    // start streams
    err = Pa_StartStream(inputStream);
    if (err != paNoError) {
        std::cerr << "error starting input stream: " << Pa_GetErrorText(err) << "\n" << std::endl;
    }

    err = Pa_StartStream(outputStream);
    if (err != paNoError) {
        std::cerr << "error starting output stream: " << Pa_GetErrorText(err) << "\n" << std::endl;
    }

}

void AudioController::setLLM(LLMController *llm) {
    this->llm = llm;
}

void AudioController::start() {
    running = true;
    sttThread = std::thread(&AudioController::recognitionLoop, this);
    ttsThread = std::thread(&AudioController::ttsNetworkLoop, this);
    decodeThread = std::thread(&AudioController::decodeLoop, this);
    std::cout << "audio started\n" << std::endl;
}

std::string AudioController::textFromJson(std::string textJson) {
    nlohmann::json json = nlohmann::json::parse(textJson);
    return json["text"];
}

void AudioController::pushMp3(std::vector<uint8_t> mp3) {
    std::cout << "pushed mp3 to thread safe queue\n" << std::endl;
    mp3Queue.push(std::move(mp3));
}

void AudioController::setNetwork(NetworkController *network) {
    net = network;
}

void AudioController::clearPCM() {
    std::queue<float> empty;
    this->ctx->playback->pcm.swap(empty);
}

void AudioController::recognitionLoop() {
    std::cout << "in recognition loop\n" << std::endl;
    while (running) {
        std::vector<short> chunk;
        {
            std::unique_lock<std::mutex> lock(ctx->mic->micMutex);
            ctx->mic->cv.wait(lock, [&]{return !ctx->mic->micAudioQueue.empty() || ctx->mic->quit;});
            if (ctx->mic->quit) return;
            chunk = std::move(ctx->mic->micAudioQueue.front());
            ctx->mic->micAudioQueue.pop();
        }

        //std::cout << "Processing chunk of size: " << chunk.size() << std::endl; 
        if (vosk_recognizer_accept_waveform(voskRecognizer, (const char *)chunk.data(), chunk.size() * sizeof(short))) {
            std::string jsonResult = (std::string)vosk_recognizer_final_result(voskRecognizer);
            std::string text = textFromJson(jsonResult);
            std::string target = "jamie";
            //std::string prompt = this->promptQueue.pop();
            int pos = text.find(target, 0);
            if (pos != std::string::npos) {
                std::string newPrompt = text.substr(pos + 5, text.size() - 1);
                printf("\t\tprompt without jamie: <%s>\n", newPrompt.c_str());
                std::cout << "Sending prompt -> " << newPrompt << "\n" << std::endl;
                //textQueue.push(text);
                this->llm->promptQueue.push(newPrompt);
            }
        } 
        // else {
            
        //     // Also check partial results
        //     std::string partialResult = vosk_recognizer_partial_result(voskRecognizer);
        //     std::cout << "Partial: " << partialResult << std::endl;
        
        // }
    }
}

void AudioController::ttsNetworkLoop() {
    std::cout << "in tts loop\n" << std::endl;
    while (running) {
        std::string text = this->responseQueue.pop();
        if (!text.empty()) {
            std::cout << "sending text to labs\n" << std::endl;
            // while (this->ctx->playback->isPlaying) {
            //     std::this_thread::sleep_for(std::chrono::milliseconds(5));
            // }
            net->handleTextToSpeech(text);
        }
    }
}

void AudioController::decodeLoop() {
    std::cout << "in decode loop\n" << std::endl;
    while (running) {
        std::vector<uint8_t> mp3 = mp3Queue.pop();
        resetDecoder();
        // {
        //     std::lock_guard<std::mutex> lock(ctx->playback->playbackMutex);
        //     std::queue<float> empty;
        //     std::swap(ctx->playback->pcm, empty);  // clear old PCM
        // }

        mpg123_feed(decoder, mp3.data(), mp3.size());

        long rate;
        int channels, encoding;
        if (mpg123_getformat(decoder, &rate, &channels, &encoding) == MPG123_OK) {
            std::cout << "Decoded format - Rate: " << rate << "Hz, Channels: " << channels << ", Encoding: " << encoding << std::endl;
            std::cout << "Expected format - Rate: " << OUTPUT_SAMPLE_RATE << "Hz" << std::endl;
        }

        unsigned char buffer[8192];
        size_t done = 0;

        while (true) {
            int err = mpg123_read(decoder, buffer, sizeof(buffer), &done);

            if (err == MPG123_ERR || err == MPG123_NEED_MORE) break;

            float *pcmSamples = reinterpret_cast<float *>(buffer);
            size_t sampleCount = done / sizeof(float);

            std::lock_guard<std::mutex> lock(ctx->playback->playbackMutex);
            for (size_t i = 0; i < sampleCount; ++i) {
                ctx->playback->pcm.push(pcmSamples[i]);
            }
        }
        ctx->playback->isPlaying = true;
    }
}


void AudioController::resetDecoder() {
    if (decoder) {
        mpg123_close(decoder);
        mpg123_delete(decoder);
    }

    decoder = mpg123_new(NULL, NULL);
    mpg123_open_feed(decoder);
    mpg123_format_none(decoder);
    mpg123_format(decoder, OUTPUT_SAMPLE_RATE, 2, MPG123_ENC_FLOAT_32);
}

AudioController::~AudioController() {
    running = false;
    ctx->mic->quit = true;
    ctx->playback->quit = true;

    if (sttThread.joinable()) sttThread.join();
    if (ttsThread.joinable()) ttsThread.join();
    if (decodeThread.joinable()) decodeThread.join();
    if (ctx) {
        delete ctx->mic;
        delete ctx->playback;
        delete ctx;
    }
    if (voskModel) vosk_model_free(voskModel);
    if (voskRecognizer) vosk_recognizer_free(voskRecognizer);
    if (decoder) {
        mpg123_close(decoder);
        mpg123_delete(decoder);
    }

    Pa_Terminate();
}

