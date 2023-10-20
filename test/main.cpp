
#define CR_DEBUG // Adds debug messages!
#include "plugin_manager.hpp"
#include "dynamic_plugin_handle.hpp"

namespace ppp = pluginsplusplus;

int main() {
	ppp::DynamicPluginHandle<ppp::plugin_base>::register_loaders();
	ppp::PluginManager<ppp::plugin_base> plugins;

	// for(auto file: std::filesystem::directory_iterator("."))
	// 	for(auto& [extension, _]: ppp::PluginManager<ppp::plugin_base>::loaders)
	// 		if(file.is_regular_file() and file.path().extension() == extension) {
	// 			plugins.load(file.path());
	// 			break;
	// 		}

	// plugins.load("libDebugPlugin.so");
	plugins.load("libCallPlugin.so"); // TODO: This has to be loaded before register or the program crashes!
	plugins.load("libRegisterPlugin.so");

	while(plugins.step())
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
}