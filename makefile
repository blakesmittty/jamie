CXX = g++

INCLUDES = -Iinclude \
 	-Iexternal/cpp-mcp/include \
  	-Iexternal/llama.cpp/include \
   	-Iexternal/llama.cpp/ggml/include \
   	-Iexternal/vosk/include \
	-Iexternal/include

CXXFLAGS = -std=c++17 -Wall $(INCLUDES)

# LIBRARIES
LLAMA_LIB = -Lexternal/llama.cpp -lllama
LLAMA_RPATH = -Wl,-rpath=external/llama.cpp

VOSK_LIB = -Lexternal/vosk -lvosk
VOSK_RPATH = -Wl,-rpath=external/vosk 

# static lib, no runtime path
MCP_LIB = -Lexternal/cpp-mcp -lmcp
# END LIBRARIES

SOURCES = src/AudioController.cpp src/LLMController.cpp src/NetworkController.cpp src/Config.cpp src/Functions.cpp src/main.cpp

LDFLAGS = $(LLAMA_LIB) $(LLAMA_RPATH) $(VOSK_LIB) $(VOSK_RPATH) $(MCP_LIB) -lportaudio -lpthread -ldl -lssl -lcrypto -lmpg123 -lcurl

TARGET = jamie 

OBJECTS = $(SOURCES:.cpp=.o)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(TARGET)