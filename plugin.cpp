// Compile with: ${WASICC} plugin.cpp -o plugin.wasm -std=c++20 -O3 -g -target wasm32-wasi-threads -Wl,--allow-undefined -fno-exceptions 

#include "plugin_base.hpp"

namespace ppp = pluginsplusplus;

struct DebugPlugin : public ppp::plugin_base {
	int value = 5;

	void load() override {
		std::cout << "load!" << std::endl;
	}
	void start() override {
		std::cout << "start!" << std::endl;
		// throw std::runtime_error("This is supposed to fail!");
	}
	// void go(ppp::stop_token) override { 
	// 	std::cout << "go!" << std::endl;
	// }
	void stop() override {
		std::cout << "stop!" << std::endl;
	}
	int step() override {
		value++;
		std::cout << "step " << value << std::endl;
        return false;
	}

	static DebugPlugin* create() { return new DebugPlugin; }
	void go(ppp::stop_token) {} // Required until clang gets better C++20 support!
};

PLUGIN_BOILERPLATE(DebugPlugin)