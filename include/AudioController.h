#pragma once


#include "ThreadSafeQueue.h"
#include "vosk_api.h"
#include "mpg123.h"
#include "LLMController.h"

#include "new_structs.h"

class NetworkController;
class LLMController;
/**
 * AudioController is a class that handles both microphone input and speaker output. In terms of output, 
 * this object also handles decoding the mp3 stream data and turning it into pcm data that can be played.
 */
class AudioController {
    private:
        VoskRecognizer *voskRecognizer;
        VoskModel *voskModel;
        mpg123_handle *decoder;
        AudioContext *ctx;
        NetworkController *net;
        LLMController *llm;

        std::atomic<bool> running = false;
        std::thread sttThread;
        std::thread ttsThread;
        std::thread decodeThread;


        //ThreadSafeQueue<std::string> textQueue;
        ThreadSafeQueue<std::vector<uint8_t>> mp3Queue;
        
    public:
        AudioController();

        ThreadSafeQueue<std::string> responseQueue;

        // === Init ===
        void start();
        void setNetwork(NetworkController *net);
        void setLLM(LLMController *llm);

        // === Threads ===
        void recognitionLoop();
        void ttsNetworkLoop();
        void decodeLoop();

        // === Utility ===
        std::string textFromJson(std::string textJson);
        void pushMp3(std::vector<uint8_t> mp3);
        void clearPCM();
        void resetDecoder();

        ~AudioController();
};