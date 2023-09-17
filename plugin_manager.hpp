#pragma once

#include "plugin_handle.hpp"
#include <vector>
#include <map>


namespace pluginsplusplus {

	template<same_or_derived_from<plugin_base> PluginBase>
	struct PluginManager {
		using LoaderFunc = std::unique_ptr<PluginHandleBase<PluginBase>>(*)(std::filesystem::path path);
		static std::map<std::string, LoaderFunc> loaders;
		static void register_loader(std::string extension, LoaderFunc loader) { loaders[extension] = loader; }



		std::vector<std::unique_ptr<PluginHandleBase<PluginBase>>> plugins;

		PluginManager() {
			(void)PluginHandleBase<plugin_base>::GetTypeID(); // Make sure the base has type 0!
		}

		void register_plugin(std::unique_ptr<PluginHandleBase<PluginBase>>&& plugin) { 
			// If the plugin has opted into itself being able to register plugins, specify us as its manager!
			if((*plugin->plugin)->flags[plugin_base::CanRegisterPlugin])
				((plugin_host_plugin_base<PluginBase>*)*plugin->plugin)->manager = this;

			plugins.emplace(plugins.cbegin(), std::move(plugin)); 
		}
		void register_plugin(PluginHandleBase<PluginBase>* plugin) { register_plugin(plugin); }

		bool step() {
			bool success = true;
			for(auto& plugin: plugins)
				if(plugin) success &= plugin->step();
			return success;
		}

		void load(std::filesystem::path path) {
			if(!loaders.contains(path.extension().string()))
				throw std::invalid_argument("Failed to load: " + path.string() + " (I'm not sure how to load " + path.extension().string() + " files!)");

			register_plugin(loaders[path.extension().string()](std::filesystem::absolute(path)));
		}
	};

	// TODO: Will this cause name conflicts? Or is it fine since its templated?
	template<same_or_derived_from<plugin_base> PluginBase>
	std::map<std::string, typename PluginManager<PluginBase>::LoaderFunc> PluginManager<PluginBase>::loaders;

}