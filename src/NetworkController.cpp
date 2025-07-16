#include <iostream>

#include "dotenv.h"
#include "json.hpp"

#include "NetworkController.h"
#include "AudioController.h"

static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t totalSize = size * nmemb;
    auto *buffer = static_cast<std::vector<uint8_t> *>(userdata);

    buffer->insert(buffer->end(), ptr, ptr + totalSize);
    return totalSize;
}

NetworkController::NetworkController(AudioController *audioController) {
    audio = audioController;

    curl = curl_easy_init();
    if (!curl) {
        std::cerr << "couldn't init curl\n" << std::endl;
        return;
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);

    dotenv::init();
    elevenLabsKey = dotenv::getenv("ELEVEN_LABS_KEY");

    voiceId = "6OzrBCQf8cjERkYgzSg8";

}

std::string NetworkController::formatSpeechPayload(std::string text) {
    nlohmann::json payload = {
        {"text", text},
        {"model_id", "eleven_multilingual_v2"},
        {"voice_settings", {
            {"stability", 0.5},
            {"similarity_boost", 0.75}
        }}
    };

    //std::cout << "payload: " << payload.dump() << std::endl;
    printf("sending speech payload to labs...\n");

    return payload.dump();
    
}

void NetworkController::handleTextToSpeech(const std::string &text) {
    std::string payload = formatSpeechPayload(text);
    std::vector<uint8_t> mp3 = fetchSpeech(payload);
    std::cout << "mp3 size in handletexttospeech: " << mp3.size() << "\n" << std::endl;
    if (!mp3.empty()) {
        printf("pushing new mp3 buffer to mp3Queue\n");
        audio->pushMp3(std::move(mp3));
    } else {
        std::cerr << "failed to fetch mp3 data for speech\n" << std::endl; 
    }
}


std::vector<uint8_t> NetworkController::fetchSpeech(std::string payload) {
    std::string url = "https://api.elevenlabs.io/v1/text-to-speech/" + voiceId + "/stream?output_format=mp3_44100_128";
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string keyHeaderStr = "xi-api-key: " + elevenLabsKey;
    headers = curl_slist_append(headers, keyHeaderStr.c_str());

    // allocating our buffer on the stack, brand new, each call is destroying output.
    printf("made new mp3 buffer\n");
    std::vector<uint8_t> mp3Buffer;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mp3Buffer);

    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        std::cout << "Response received" << "\n" << std::endl;
        // std::cout << res << "\n" << std::endl;
        // std::cout << mp3Buffer.data() << std::endl; 
    } else {
        std::cerr << "CURL error: " << curl_easy_strerror(res) << "\n";
    }

    curl_slist_free_all(headers); // always clean headers even on error

    return mp3Buffer;

}

NetworkController::~NetworkController() {
    if (curl) {
        curl_easy_cleanup(curl);
    }
}