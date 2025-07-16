#include <string>
#include <optional>
#include "json.hpp"

namespace mcp {
    struct JsonRpcRequest {
        std::string jsonrpc = "2.0";
        std::string method;
        nlohmann::json params;
        std::optional<std::string> id;
    };

    struct JsonRpcResponse {
        std::string jsonrpc = "2.0";
        std::optional<nlohmann::json> result;
        std::optional<nlohmann::json> error;
        std::optional<std::string> id;
    };

    struct JsonRpcErr {
        int code;
        std::string message;
        std::optional<nlohmann::json> data;
    };

    struct ToolParameter {
        std::string name;
        std::string type;
        std::string description;
        bool required = false;
        nlohmann::json default_value;
    };

    struct Tool {
        std::string name;
        std::string description;
        std::vector<ToolParameter> parameters;
        std::string server_name;
    };

    struct ToolCall {
        std::string tool_name;
        std::string server_name;
        nlohmann::json arguments;
        std::string call_id;
    };

    struct ToolResult {
        std::string call_id;
        bool success;
        nlohmann::json content;
        std::string error_message;
        std::chrono::milliseconds execution_time;
    };

    // MCP Server configuration
    // may or may not need
    struct ServerConfig {
        // std::string name;
        // std::string command;
        // std::vector<std::string> args;
        // std::string working_directory;
        // std::map<std::string, std::string> environment;
        // std::chrono::seconds timeout{30};
        // int max_retries = 3;
        // bool auto_restart = true;
    };
}

class MCPBridge {
private:
    std::vector<mcp::Tool> tools;



public:
    MCPBridge();
    ~MCPBridge();

};

