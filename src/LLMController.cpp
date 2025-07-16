#include "LLMController.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>

#include "json.hpp"

#include "Functions.h"

#include "Config.h"


/**
 * TODO: need a way to shrink the size of messages and formatted messages, for formattedMessages, 
 * start and end pos could be useful.
 */


LLMController::LLMController(struct Config *config, AudioController *aud) {
    this->audio = aud;
    this->config = config;
    llama_backend_init();

    llama_model_params modelParams = llama_model_default_params();
    modelParams.n_gpu_layers = config->nGpuLayers;

    model = llama_model_load_from_file(config->modelPath.c_str(), modelParams);
    if (!model) {
        std::cerr << "error loading model\n";
        return;
    }
    vocab = llama_model_get_vocab(model);
    chatTemplate = llama_model_chat_template(model, nullptr);

    llama_context_params ctxParams = llama_context_default_params();
    ctxParams.n_ctx = config->nCtx;
    ctxParams.n_batch = config->nCtx;

    ctx = llama_init_from_model(model, ctxParams);
    formattedMessages.resize(llama_n_ctx(ctx));
    if (!ctx) {
        std::cerr << "error creating context from model\n";
        return;
    }
    sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(sampler, llama_sampler_init_min_p(0.05f, 1));
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.5f));
    llama_sampler_chain_add(sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED)); 
}

void LLMController::initLLM() {
    // process the system prompt
    // proceed to give context and examples 
    // utilize mcp functions
}

void LLMController::processSystemPrompt() {
    printf("[LOADING SYSTEM PROMPT]\n");
    std::string sysPrompt;
    std::ifstream sysPromptFile(this->config->systemPrompt);
    std::stringstream buffer;
    if (!sysPromptFile.is_open()) {
        fprintf(stderr, "system prompt file is not open");
    }
    buffer << sysPromptFile.rdbuf();
    sysPrompt = buffer.str();

    std::string formattedSysPrompt = format(sysPrompt);
    evaluate(formattedSysPrompt, true);
    this->messages.push_back({"assistant", strdup(currentResponse.c_str())});
    this->currentPromptStartPos = llama_chat_apply_template(chatTemplate, messages.data(), messages.size(), false, nullptr, 0);
    if (this->currentPromptStartPos < 0) {
        std::cerr << "error applying chat template (getting messages size in bytes with formatting)\n";
        return;
    }
    printf("[SYSTEM PROMPT PROCESSED]\n");
}

void LLMController::feed(std::string &prompt) {
    if (prompt.empty()) {
        std::cerr << "cant feed empty prompt\n";
        return;
    }
    printf("\t\t currentPromptStartPos: (%d)\n", currentPromptStartPos);
    printf("\t\t currentPromptEndPos: (%d)\n", currentPromptEndPos);
    currentPrompt = prompt;

    //currentPromptEndPos = format(prompt);

    std::string formattedPrompt = format(prompt);

    // std::string formattedPrompt(formattedMessages.begin() + currentPromptStartPos, formattedMessages.begin() + currentPromptEndPos);

    printf("\t\t formatted prompt = formattedMessages[currentPromptStartPos till currentPromptEndPos]\n");
    printf("\t\t formatted prompt: <%s>\n", formattedPrompt.c_str());

    evaluate(formattedPrompt, false);
    //currentPromptStartPos = currentPromptEndPos;
    messages.push_back({"assistant", strdup(currentResponse.c_str())});
    printf("\t\t pushed response to messages\n");
    currentPromptStartPos = llama_chat_apply_template(chatTemplate, messages.data(), messages.size(), false, nullptr, 0);
    if (currentPromptStartPos < 0) {
        std::cerr << "error applying chat template (getting messages size in bytes with formatting)\n";
        return;
    }

    printf("\t\t currentPromptStartPos: (%d)\n", currentPromptStartPos);
    printf("\t\t currentPromptEndPos: (%d)\n", currentPromptEndPos);
    printf("\t\t current prompt <%s>\n", currentPrompt.c_str());
    printf("\t\t current response: <%s>\n", currentResponse.c_str());
    printf("\t\t [TEXT TO PROCESS]\n");
    for (int i = 0; i < textChunksForProcessing.size(); i++) {
        printf("\t\t (%d): <%s>\n", i, textChunksForProcessing[i].c_str());
    }
    printf("\t\t [END TEXT TO PROCESS]\n");

    this->currentResponse.clear();
}

void LLMController::waitForPrompts() {
    processSystemPrompt();
    while (true) {
        if (!this->promptQueue.empty()) {
            std::string prompt = this->promptQueue.pop();
            feed(prompt);
        }
    }
}


// may remove return value since its being assigned to private member
/**
 * format takes a normal string and formats it with the models default chat template. 
 */
// refactor format to return the formatted string and assign currentPromptEndPos IN the method
std::string LLMController::format(std::string &prompt) {
  
    //std::cout << "prompt in format: " << prompt.c_str() << "\n";
    messages.push_back({"user", strdup(prompt.c_str())});
    if (!messages.data()) {
        std::cout << "messages data not here\n";
    }
    if (!formattedMessages.data()) {
        std::cout << "no formatted messages\n";
    }
    printf("\t\tpushed prompt to messages\n");

    // std::cout << "formattedMessages buffer size: " << formattedMessages.size() << "\n";
    //std::cout << "going to apply template\n";
    this->currentPromptEndPos = llama_chat_apply_template(chatTemplate, messages.data(), messages.size(), true, formattedMessages.data(), formattedMessages.size());
    //std::cout << "applied template\n";
    if (this->currentPromptEndPos > (int)formattedMessages.size()) {
        formattedMessages.resize(this->currentPromptEndPos);
        this->currentPromptEndPos = llama_chat_apply_template(chatTemplate, messages.data(), messages.size(), true, formattedMessages.data(), formattedMessages.size());
    }
    
    if (this->currentPromptEndPos < 0) {
        std::cerr << "error formatting prompt, code: " << this->currentPromptEndPos << "\n";
    }

    std::string formattedPrompt(formattedMessages.begin() + currentPromptStartPos, formattedMessages.begin() + currentPromptEndPos);

    return formattedPrompt; 
}

void LLMController::evaluate(std::string &prompt, bool isSystemPrompt) {
    printf("llm evaluating prompt...\n");
    int nTokens = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), NULL, 0, true, true);
    std::vector<llama_token> tokens(nTokens);
    if (llama_tokenize(vocab, prompt.c_str(), prompt.size(), tokens.data(), tokens.size(), true, true) < 0) {
        std::cerr << "error tokenizing system prompt\n" << std::endl; 
    }
    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
    llama_token newTokenId;

    printf("[RESPONSE] ");
    while (true) {
        int ctxLen = llama_n_ctx(ctx);
        int usedCtxLen = llama_memory_seq_pos_max(llama_get_memory(ctx), 0);
        if (usedCtxLen + batch.n_tokens > ctxLen) {
            printf("\033[0m\n");
            fprintf(stderr, "context size exceeded\n");
            exit(0);
        }
        if (llama_decode(ctx, batch)) { // why the fuck is 0 true
            printf("error decoding\n");
        }

        newTokenId = llama_sampler_sample(sampler, ctx, -1);

        if (llama_vocab_is_eog(vocab, newTokenId)) {
            break;
        }
        
        if (!isSystemPrompt) writeResponseToBuffers(newTokenId);

        batch = llama_batch_get_one(&newTokenId, 1);
        
    }
    printf(" [END RESPONSE]\n");
    // send full reponse for speech translation
    //this->audio->responseQueue.push(currentResponse);

    

}

// check if there is a space after the characters we want to determine sendworthiness

void LLMController::writeResponseToBuffers(llama_token token) {
    char pieceBuffer[256];
    int n = llama_token_to_piece(this->vocab, token, pieceBuffer, sizeof(pieceBuffer), 0, true);
    if (n < 0) {
        std::cerr << "failed to convert token to piece\n";
    }
    std::string piece(pieceBuffer, n);
    if (this->config->printChat) printf("%s", piece.c_str());
    fflush(stdout);

    if (piece.find('{') != std::string::npos && !this->writingJson) this->writingJson = true;
    if (this->writingJson) {
        writeJson(piece);
    } else {
        this->responseBuffer += piece;
        this->currentResponse += piece;
        if (pieceHasSeparator(piece)) {
            if (this->config->useTTS) this->audio->responseQueue.push(this->responseBuffer);
            this->textChunksForProcessing.push_back(this->responseBuffer);
            this->responseBuffer.clear();
        }
    }
}

void LLMController::writeJson(std::string &piece) {
    static uint8_t endBraceCount = 0;
    this->functionCallBuffer += piece;

    if (piece.find('}') != std::string::npos) {
        endBraceCount++;
    }

    if (endBraceCount == 3 && jsonIsValid()) {
        printf("JSON IS VALID\n");
        endBraceCount = 0;
        parseFunctionCall();
        this->writingJson = false;
    }
}

bool LLMController::jsonIsValid() {
    return nlohmann::json::accept(this->functionCallBuffer);
}
// need a better way to jump to functions as of now.
void LLMController::parseFunctionCall() {
    printf("===== MADE FUNCTION CALL =====\n");
    // printf("functionCallBuffer -> <%s>", this->functionCallBuffer.c_str());

    nlohmann::json data = nlohmann::json::parse(this->functionCallBuffer);
    //nlohmann::json function = data.at("function_call");
    nlohmann::json function = data["function_call"];

    printf("function params\n");

    std::string functionName = function.begin().key().c_str();

    printf("function name actual: %s\n", functionName.c_str());

   // std::string paramBuffer[5/*make a #define MAX_PARAMS*/];

    std::string paramBuffer[5/*make a constant*/];

    auto params = function.begin().value();
    uint8_t i = 0;
    for (auto it = params.begin(); it != params.end(); it++, i++) {
        std::string param = it->get<std::string>();
        paramBuffer[i] = param;
        printf("param %s\n", paramBuffer[i].c_str());
    }
    callFunction(functionName, paramBuffer);




    // make jamie have an always on hotline you can hit up and tell him to do shit like yo jamie preheat the oven to 375
    this->functionCallBuffer.clear();
}

void LLMController::callFunction(std::string &name, std::string params[]) { 
    if (name == "get_weather") {
        getWeather(params[0]);
    } else if (name == "web_search") {
        webSearch(params[0], std::stoi(params[1]));
    } else if (name == "get_time") {
        getTime();
    }
}

//build out deep response checks

bool LLMController::pieceHasSeparator(std::string &piece) {
    bool foundSep = false;
    for (uint8_t i = 0; i < 4; i++) {
        int pos = this->responseBuffer.find(*(this->textSeparators[i]), 0);
        if (pos != std::string::npos) {
            foundSep = true;
            break;
        }
    }
    return foundSep;
}


LLMController::~LLMController() {
    for (auto & msg : messages) {
        free(const_cast<char *>(msg.content));
    }
    llama_sampler_free(sampler);
    llama_free(ctx);
    llama_model_free(model);
    llama_backend_free();
}

