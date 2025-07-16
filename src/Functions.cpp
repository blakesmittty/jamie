#include "Functions.h"

#include <chrono>
#include <iostream>
#include <iomanip>

void getTime() {
    auto now = std::chrono::system_clock::now();

    // Convert to time_t
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    // Convert to tm struct
    std::tm *tm = std::localtime(&now_c);

    // Print formatted time
    std::cout << "FUNCTION CALL:\n\t Current time: " << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << "\n" <<  std::endl;
}

void getWeather(std::string &location) {
    printf("FUNCTION CALL:\n\t The weather is fucking awful in <%s>\n", location.c_str());
}

void webSearch(std::string &query, int maxResults) {
    printf("FUNCTION CALL:\n\t");
    printf("Searching the web for <%s>, max results = %d\n", query.c_str(), maxResults);
}