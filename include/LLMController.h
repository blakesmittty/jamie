#pragma once

#include <string>
#include <vector>
#include <variant>

#include "Config.h"

#include "AudioController.h"
#include "ThreadSafeQueue.h"

#include "llama.h"


class LLMController {
private:
    struct Config *config;
    AudioController *audio;
    /**
     * struct llama_chat_message {
     *      const char *role;
     *      const char *message;
     * } 
     * free all messages
     */
    std::vector<llama_chat_message> messages;
    std::vector<char> formattedMessages;
    
    std::vector<std::string> textChunksForProcessing; //debug

    const char *chatTemplate;
    llama_model *model;
    const llama_vocab *vocab;
    llama_sampler *sampler;
    llama_context *ctx;

    // buffer that is filled with text pieces and searched for available text. once found, just clear it.
    std::string responseBuffer;
    // buffer we fill with what ought to be json we can eventually parse and make a function call.
    std::string functionCallBuffer;

    bool writingJson;

    // separators we look for inside of response generation
    const char *textSeparators[4] = {"! ", "? ", ", ", ". "};

    int currentPromptStartPos = 0;
    int currentPromptEndPos;
public:
    LLMController(struct Config *config, AudioController *aud);
    ~LLMController();

    std::string currentResponse;
    std::string currentPrompt;
    void initLLM();
    void processSystemPrompt();
    bool pieceHasSeparator(std::string &piece);
    void feed(std::string &prompt);
    void writeJson(std::string &piece);
    bool jsonIsValid();
    void parseFunctionCall();
    void callFunction(std::string &name, std::string params[]);
    void writeResponseToBuffers(llama_token token);
    void waitForPrompts(); 
    std::string format(std::string &prompt);
    void evaluate(std::string &formattedPrompt, bool isSystemPrompt);

    // queue of prompts for llm
    ThreadSafeQueue<std::string> promptQueue;


};