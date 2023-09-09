#include "plugin/plugin_base.hpp"

struct DebugPlugin: public pluginsplusplus::plugin_base {
    int value = 5;

    void go(std::stop_token) override {
        std::cout << "go!" << std::endl;
    }
    void stop() override {
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