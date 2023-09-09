#include "plugin_manager.hpp"

int main() {
    (void)ppp::PluginHandleBase<ppp::plugin_base>::GetTypeID(); // Make sure the base has type 0!
    ppp::PluginManager<ppp::plugin_base> plugins;

    auto p = std::make_unique<ppp::SharedPluginHandle<ppp::plugin_base>>(ppp::SharedPluginHandle<ppp::plugin_base>::load("/home/shared/Dev/Plugin++/plugin.so"));
    plugins.register_plugin(std::move(p));

    while(plugins.step())
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
}