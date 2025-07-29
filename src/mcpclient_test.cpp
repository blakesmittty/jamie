#include "mcp_sse_client.h"
#include "mcp_stdio_client.h"
#include "json.hpp"

void server() {
    mcp::sse_client cli = mcp::sse_client("localhost", 8080, "/sse");
    if (cli.ping()) {
        printf("pinged http client");
    } else {
        printf("ping failed on http client");
    }
}

void stdioserver() {
    nlohmann::json caps = {
        {"test", "test"}
    };
    //mcp::stdio_client cli = mcp::stdio_client("npx mcp-server-filesystem ../../exfiles");
    mcp::stdio_client cli("npx mcp-server-filesystem ../../exfiles");
    if (!cli.initialize("jamie", "1.0.0")) {
        printf("client init failed\n");
    } else {
        printf("client init success\n");
    }
    if (cli.ping()) {
        printf("pinged stdio client\n");
    } else {
        printf("ping failed on stdio client\n");
    }

    nlohmann::json resources = cli.get_server_capabilities();
    printf("resources: %s\n", resources.dump().c_str());
    std::vector<mcp::tool> tools = cli.get_tools();
    for (int i = 0; i < tools.size(); i++) {
        printf("tool %d: %s\n", i, tools[i].name.c_str());
        printf("tool %d description: %s\n", i, tools[i].description.c_str());
        printf("tool %d params: %s\n", i, tools[i].parameters_schema.dump().c_str());
    }
    nlohmann::json params = {{"path", "../../exfiles/testDir"}};
    nlohmann::json res = cli.call_tool("create_directory", params);
    printf("result: %s\n", res.dump().c_str());
}

int main() {
    stdioserver();

    return 0;
}