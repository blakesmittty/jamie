#pragma once

#include <string>
#include <vector>
#include <curl/curl.h>

class AudioController;

class NetworkController {
    private:
        AudioController *audio;
        CURL *curl;
        std::string elevenLabsKey;
        std::string voiceId;

        std::vector<uint8_t> fetchSpeech(std::string payload);
    public:
        NetworkController(AudioController *controller);
        ~NetworkController();

        std::string formatSpeechPayload(std::string text);
        void handleTextToSpeech(const std::string &text);

        std::vector<uint8_t> staticMp3Buffer;



};