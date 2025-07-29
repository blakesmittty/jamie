#pragma once

#include <string>

struct MCPInfo {
    std::vector<std::string> &stdinServerCommands;
    std::vector<std::string> &httpServerURLs;
    std::vector<std::string> &serverNames;
};

struct Config {
    std::string modelPath; // using mistral 7b instruct q2_k
    std::string systemPrompt;
    int nGpuLayers;
    int nCtx;
    bool printChat;
    bool useMic;
    bool useTTS;
    struct MCPInfo *mcpInfo;
}; // getting from a config file
// need to get things from mcpbridge to generate system prompt

struct MCPInfo parseMCPInfo();
struct Config generateConfig();