#pragma once

#include <mcp_stdio_client.h>
#include <mcp_sse_client.h>

#include "Config.h"

// will need a stdio client and http client implementation that we can go between since 
// some servers will be off device


/**
 * this is to be used from the outside
 * 
 * should probably print a lot of information to separate files.
 * 
 * we need to find a way to feed the llm data in a structured and controlled way.
 *      when we say controlled and structured, that means a few things
 *          1. keeping the length of the text short enough so we can feed the data in chunks, if that
 *             turns out to be necessary
 *          2. the model needs to know what is coming and what to do. i think we can provide a system prompt
 *             in which an example piece of context arrives and its told to respond with "ok" to save on inference time
 * 
 * we need a file that has a list of commands and urls for client initialization.
 */

class MCP {
private:
    std::vector<mcp::tool> tools;
    
public:
    MCP(struct Config *config);
    ~MCP();

    std::map<std::string, std::unique_ptr<mcp::client>> servers; 

    void initServers();

    // brainstorm
    void generateSystemPrompt();
    void initClients();
    void listTools();
    void listParameters();
    void listToolContext();



};