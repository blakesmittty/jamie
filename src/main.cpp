#include <iostream>

#include "NetworkController.h"
#include "AudioController.h"
#include "LLMController.h"
#include "Config.h"

#include "mcp_client.h"
#include "mcp_stdio_client.h"

int main() {
    struct Config config = generateConfig();
    if (config.useMic) {
        AudioController audio = AudioController();
        NetworkController network = NetworkController(&audio);
        audio.setNetwork(&network);
        LLMController llm = LLMController(&config, &audio);
        audio.setLLM(&llm);
        audio.start();
        std::thread t([&]{
            llm.waitForPrompts();
        });
        t.join();
        audio.clearPCM();
    } else {
        AudioController audio = AudioController();
        NetworkController network = NetworkController(&audio);
        audio.setNetwork(&network);
        struct Config config = generateConfig();
        LLMController llm = LLMController(&config, &audio);
        audio.setLLM(&llm);
        audio.start();
        // new
        std::thread t([&]{ 
            llm.waitForPrompts();
        });
        // end new
        while (true) {
            printf("\033[32m> \033[0m");
            std::string user;
            std::getline(std::cin, user);
            // llm.feed(user);
            llm.promptQueue.push(user);
        }
        t.join();
        audio.clearPCM();
    }
    printf("CLEANUP\n");

    return 0;
}