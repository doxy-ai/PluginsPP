#pragma once
#include "cr.h"
#include <cassert>
#include <thread>
#include <exception>
#include <iostream>
#include <utility>
#include <concepts>

namespace pluginsplusplus {

    struct plugin_base {
        bool can_register_plugin = false;

        virtual ~plugin_base() {}
        virtual void go(std::stop_token) {}
        virtual void stop() {}
        virtual int main_thread_step() { return 0; }

        static plugin_base* create() { return new plugin_base; }
    };


    #define PLUGIN_BOILERPLATE(Type)\
    pluginsplusplus::plugin_base* plugin = nullptr;\
    std::jthread thread;\
    \
    CR_EXPORT int cr_main(struct cr_plugin *ctx, enum cr_op operation) {\
        assert(ctx);\
        try {\
            switch (operation) {\
                case CR_LOAD:\
                    if(!plugin) plugin = Type::create();\
                    thread = std::jthread([](std::stop_token stop) { plugin->go(stop); });\
                    return 0;\
                case CR_UNLOAD:\
                case CR_CLOSE:\
                    plugin->stop();\
                    thread.request_stop();\
                    thread.join();\
                    if(operation == CR_CLOSE) delete std::exchange(plugin, nullptr);\
                    return 0;\
            }\
            return plugin->main_thread_step();\
        } catch (std::exception& e) {\
            std::cerr << e.what() << std::endl;\
            return -1;\
        }\
    }





    template<std::derived_from<plugin_base> PluginBase = plugin_base>
    struct PluginHandleBase;

    template<std::derived_from<plugin_base> PluginBase = plugin_base>
    struct PluginManager;

    template<std::derived_from<plugin_base> PluginBase = plugin_base>
    struct plugin_host_plugin_base: public plugin_base {
        plugin_host_plugin_base() : plugin_base(true) {}
        plugin_host_plugin_base(const plugin_host_plugin_base&) = default;
        plugin_host_plugin_base(plugin_host_plugin_base&&) = default;
        plugin_host_plugin_base& operator=(const plugin_host_plugin_base&) = default;
        plugin_host_plugin_base& operator=(plugin_host_plugin_base&&) = default;

        PluginManager<PluginBase>* manager;
        void register_plugin(std::unique_ptr<PluginHandleBase<PluginBase>>&& plugin) {
            manager->register_plugin(std::move(plugin));
        }
    };

}
