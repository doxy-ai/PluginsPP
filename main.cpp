
#define CR_DEBUG // Adds debug messages!
#include "plugin_manager.hpp"

namespace ppp = pluginsplusplus;

int main() {
    ppp::PluginManager<ppp::plugin_base> plugins;

    // for(auto file: std::filesystem::directory_iterator("."))
    //     for(auto& [extension, _]: ppp::PluginManager<ppp::plugin_base>::loaders)
    //         if(file.is_regular_file() and file.path().extension() == extension) {
    //             plugins.load(file.path());
    //             break;
    //         }

    plugins.load("plugin.so");

    while(plugins.step())
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
}