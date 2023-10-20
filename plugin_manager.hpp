#pragma once

#include "plugin_handle.hpp"
#include <vector>
#include <map>
#include <unordered_map>


namespace pluginsplusplus {

	template<same_or_derived_from<plugin_base> PluginBase>
	struct PluginManager {
		using LoaderFunc = std::unique_ptr<PluginHandleBase<PluginBase>>(*)(std::filesystem::path path);
		static std::map<std::string, LoaderFunc> loaders;
		static void register_loader(std::string extension, LoaderFunc loader) { loaders[extension] = loader; }



		std::vector<std::unique_ptr<PluginHandleBase<PluginBase>>> plugins;
		std::unordered_map<std::string, std::unordered_map<std::string, void*>> functionRegistry; // Maps function names to their associated pointers
		// TODO: ^ How do we put some measure of performant security on void*s returned from dynamicly loaded functions?
		// TODO: ^ Do we need this to be inside something like a monitor? Race condition issues?

		PluginManager() {
			(void)PluginHandleBase<plugin_base>::GetTypeID(); // Make sure the base has type 0!
		}

		void register_plugin(std::unique_ptr<PluginHandleBase<PluginBase>>&& plugin) { 
			// If the plugin has opted into itself being able to register plugins, specify us as its manager!
			if((*plugin->plugin)->flags[plugin_base::CanRegister])
				((plugin_host_plugin_base<PluginBase>*)*plugin->plugin)->manager = this;

			plugins.emplace(plugins.cbegin(), std::move(plugin)); 
		}
		void register_plugin(PluginHandleBase<PluginBase>* plugin) { register_plugin(plugin); }


		template<typename F> // TODO: Replace with concept
		void register_function(const std::string& pluginName, const std::string& name, F function) {
			functionRegistry[pluginName][name] = (void*)function;
		}

		template<typename F = void>
		F* lookup_function(std::string name, bool throwIfNull = false) {
			auto pos = name.find(":");
			if(pos == std::string::npos) return nullptr; // There must be a :
			if(name[pos + 1] != ':') return nullptr; // There must be a ::
			auto plugin = name.substr(0, pos); // TODO: pos -1?
			name = name.substr(pos + 2);

			if(!functionRegistry.contains(plugin)) return nullptr;
			if(!functionRegistry[plugin].contains(name)) return nullptr;
			auto f = functionRegistry[plugin][name];
			[[unlikely]] if(throwIfNull && !f) throw std::invalid_argument("Function `" + name + "` was not found in the registry.");
			return f;
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
		}

		std::thread load_threaded(std::filesystem::path path) {
			return std::thread([this, path] { this->load(path); });
		}
	};

	// TODO: Will this cause name conflicts? Or is it fine since its templated?
	template<same_or_derived_from<plugin_base> PluginBase>
	std::map<std::string, typename PluginManager<PluginBase>::LoaderFunc> PluginManager<PluginBase>::loaders;

}