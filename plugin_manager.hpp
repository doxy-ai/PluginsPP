#pragma once

#include "plugin_handle.hpp"
#include <map>

namespace pluginsplusplus {

	template<same_or_derived_from<plugin_base> PluginBase>
	struct PluginManager {
		using LoaderFunc = std::unique_ptr<PluginHandleBase<PluginBase>>(*)(std::filesystem::path path);
		static std::map<std::string, LoaderFunc> loaders; // TODO: Should this be a hashtable instead?
		static void register_loader(std::string extension, LoaderFunc loader) { loaders[extension] = loader; }


		PluginManager() {
			(void)PluginHandleBase<plugin_base>::GetTypeID(); // Make sure the base has type 0!
		}


		std::vector<std::unique_ptr<PluginHandleBase<PluginBase>>> plugins;

		void register_plugin(std::unique_ptr<PluginHandleBase<PluginBase>>&& plugin) {
			// Make sure the plugin handle has a pointer to us!
			plugin->manager = this;

			// If the plugin has opted into itself being able to register plugins, specify us as its manager!
			if((*plugin->plugin)->flags[plugin_base::CanRegister])
				((plugin_host_plugin_base<PluginBase>*)*plugin->plugin)->manager = this;

			plugins.emplace(plugins.cbegin(), std::move(plugin));
		}
		void register_plugin(PluginHandleBase<PluginBase>* plugin) { register_plugin(plugin); }



		FunctionRegistry functionRegistry;

		void register_function(std::string_view pluginName, std::string_view functionName, FunctionRegistry::Function function) {
			functionRegistry.register_function(pluginName, functionName, function);
		}

		template<typename F = void>
		F* lookup_function(std::string_view name, bool throwIfNull = true) {
			return functionRegistry.lookup_function<F>(name, throwIfNull);
		}

		template<typename Return = void, typename... Args>
		Return call_function(std::string_view name, Args... args) {
			return functionRegistry.call_function(name, std::forward<Args>(args)...);
		}



		bool stopped = false;
		bool step() {
			if(stopped) return false;

			bool success = true;
			for(auto& plugin: plugins)
				if(plugin) success &= plugin->step();
			return success;
		}

		void request_stop() { // TODO: Do we want to provide the ability to request that an individual plugin be stopped?
			for(auto& plugin: plugins)
				if(plugin) plugin->stop();
			stopped = true;
		}

		void load(std::filesystem::path path) {
			if(!loaders.contains(path.extension().string()))
				throw std::invalid_argument("Failed to load: " + path.string() + " (I'm not sure how to load " + path.extension().string() + " files!)");

			register_plugin(loaders[path.extension().string()](std::filesystem::absolute(path)));
			// TODO: If the loader is responsible for calling load()... then load gets called before the plugin is registered and we can't do any registration stuff...
			// 		So load() needs to get called by register_plugin, not the loader
		}

		std::thread load_threaded(std::filesystem::path path) {
			return std::thread([this, path] { this->load(path); });
		}
	};

	// TODO: Will this cause name conflicts? Or is it fine since its templated?
	template<same_or_derived_from<plugin_base> PluginBase>
	std::map<std::string, typename PluginManager<PluginBase>::LoaderFunc> PluginManager<PluginBase>::loaders;

}