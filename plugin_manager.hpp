#pragma once

#include "plugin_handle.hpp"
#include <vector>
// #include <type_traits>

// namespace detail {
//     // From: https://stackoverflow.com/questions/11251376/how-can-i-check-if-a-type-is-an-instantiation-of-a-given-class-template
//     template < template <typename...> class Template, typename T >
//     struct is_specialization_of : std::false_type {};

namespace pluginsplusplus {
//     struct is_specialization_of< Template, Template<Args...> > : std::true_type {};

//     template < template <typename...> class Template, typename T >
//     constexpr bool is_specialization_of_v = is_specialization_of<Template, T>::value;
// }

namespace ppp {

    template<std::derived_from<plugin_base> PluginBase>
    struct PluginManager {
        std::vector<std::unique_ptr<PluginHandleBase<PluginBase>>> plugins;

        void register_plugin(std::unique_ptr<PluginHandleBase<PluginBase>>&& plugin) { 
            // If the plugin has opted into registering plugins, specify us as its manager!
            if((*plugin->plugin)->can_register_plugin)
                ((plugin_host_plugin_base<PluginBase>*)*plugin->plugin)->manager = this;

            plugins.emplace_back(std::move(plugin)); 
        }
        void register_plugin(PluginHandleBase<PluginBase>* plugin) { register_plugin(plugin); }

        bool step() {
            bool success = true;
            for(auto& plugin: plugins)
                if(plugin) success &= plugin->step();
            return success;
        }
    };

}