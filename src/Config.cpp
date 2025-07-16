#include <iostream>
#include <fstream>
#include <sstream>
#include <json.hpp>

#include "Config.h"

struct Config generateConfig() {
    struct Config config;
    std::stringstream dataBuffer;
    nlohmann::json jsonData;
    std::ifstream configFile("config.json");
    if (!configFile.is_open()) {
        std::cerr << "error opening config file\n";
    }
    dataBuffer << configFile.rdbuf();
    jsonData = nlohmann::json::parse(dataBuffer.str());

    config.modelPath = jsonData.at("config").at("model").at("path").get<std::string>();
    config.systemPrompt = jsonData.at("config").at("model").at("sysprompt-path").get<std::string>();
    config.nCtx = jsonData.at("config").at("model").at("context-length").get<int>();
    config.nGpuLayers = jsonData.at("config").at("model").at("gpu-layers").get<int>();
    config.printChat = jsonData.at("config").at("model").at("print-chat").get<bool>();
    config.useMic = jsonData.at("config").at("audio").at("use-mic").get<bool>();
    config.useTTS = jsonData.at("config").at("audio").at("use-tts").get<bool>();

    return config;


}
