#pragma once

#include <string>

struct Config {
    std::string modelPath; // using mistral 7b instruct q2_k
    std::string systemPrompt;
    int nGpuLayers;
    int nCtx;
    bool printChat;
    bool useMic;
    bool useTTS;
}; // getting from a config file
// need to get things from mcpbridge to generate system prompt

struct Config generateConfig();