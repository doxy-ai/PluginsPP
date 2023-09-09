#include "plugin/plugin_base.hpp"

namespace ppp = pluginsplusplus;

struct DebugPlugin: public ppp::threaded_plugin_base {
	int value = 5;

	void load() override {
		std::cout << "load!" << std::endl;
	}
	void start() override {
		std::cout << "start!" << std::endl;
		ppp::threaded_plugin_base::start();
	}
	void go(std::stop_token) override {
		std::cout << "go!" << std::endl;
	}
	void stop() override {
		ppp::threaded_plugin_base::stop();
		std::cout << "stop!" << std::endl;
	}
	int main_thread_step() override {
		// std::cout << "step" << std::endl;
		value++;
		return 0;
	}

	static DebugPlugin* create() { return new DebugPlugin; }
};

PLUGIN_BOILERPLATE(DebugPlugin)