# jamie
Jamie is a smart home assistant. Currently with capability for reasoning and function calling.

To build, run "make jamie" at the root of the project.

config.json has multiple options; 
    if you run with use-tts enabled, we reach out to eleven labs for speech generation, else text is printed.
    if you run with use-mic enabled, we use microphone input as opposed to stdin.

    Jamie works best with Phi-4 as of now (Phi-4-mini-instruct-Q4_K_M.gguf).