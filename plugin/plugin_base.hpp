#pragma once
#include "cr.h"
#include <cassert>
#include <thread>
#include <exception>
#include <iostream>
#include <utility>
#include <concepts>
#include <bitset>

namespace pluginsplusplus {

	struct plugin_base {
		enum Flags {
			Initialized = 0,
			CanRegisterPlugin,
			HasThread,
			Count,
		};
		static constexpr const char* CanRegisterPluginString = "10";
		static constexpr const char* CanRegisterPluginAndHasThreadString = "11";

		std::bitset<Flags::Count> flags;

		virtual ~plugin_base() {}
		virtual void load() {}
		virtual void start() {}
		virtual void stop() {}
		virtual int main_thread_step() { return 0; }

		static plugin_base* create() { return new plugin_base; }
	};

	struct threaded_plugin_base: public plugin_base {
		std::jthread thread;
		void start() override {
			thread = std::jthread([this](std::stop_token stop) { go(stop); });
		}
		void stop() override {
			thread.request_stop();
			thread.join();
		}
		virtual void go(std::stop_token) {}
	};

	#define PLUGIN_BOILERPLATE(Type)\
	pluginsplusplus::plugin_base* plugin = nullptr;\
	\
	CR_EXPORT int cr_main(struct cr_plugin *ctx, enum cr_op operation) {\
		static bool loadStep = true; /*We step once in load to make sure things are actually loaded!*/\
		assert(ctx);\
		try {\
			switch (operation) {\
				case CR_LOAD:\
					if(!plugin) plugin = Type::create();\
					plugin->load();\
					return 0;\
				case CR_UNLOAD:\
				case CR_CLOSE:\
					plugin->stop();\
					if(operation == CR_CLOSE) delete std::exchange(plugin, nullptr);\
					return 0;\
			}\
			[[unlikely]] if (loadStep) {/* Skip the step associated with a load*/\
				loadStep = false;\
				return 0;\
			}\
			[[unlikely]] if(!plugin->flags[pluginsplusplus::plugin_base::Initialized]) {\
				plugin->start();\
				plugin->flags.set(pluginsplusplus::plugin_base::Initialized);\
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
		plugin_host_plugin_base() requires(!std::derived_from<threaded_plugin_base, PluginBase>) : plugin_base(CanRegisterPluginString) {}
		plugin_host_plugin_base() requires(std::derived_from<threaded_plugin_base, PluginBase>) : plugin_base(CanRegisterPluginAndHasThreadString) {}
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
