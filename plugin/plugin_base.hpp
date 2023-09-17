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

	template<class Derived, class Base>
	concept same_or_derived_from = std::same_as<Derived, Base> || std::derived_from<Derived, Base>;

	struct plugin_base {
		enum Flags {
			Started = 0,
			CanRegisterPlugin,
			HasThread,
			Count,
		};

		std::bitset<Flags::Count> flags;

		virtual ~plugin_base() {}
		virtual void load() {}
		virtual void start() {}
		virtual void stop() {}
		virtual int step() { return 0; }

		static plugin_base* create() { return new plugin_base; }
	};

#ifndef PPP_NON_STANDARD_STOP_TOKEN
	using stop_token = std::stop_token;
#else
	// Implements the same interface as std::stop_token!
	struct stop_token {
		int32_t host_id = 0;
		[[nodiscard]] bool stop_requested() const noexcept;
		[[nodiscard]] bool stop_possible() const noexcept { return host_id; }
		[[nodiscard]] friend bool operator==( const stop_token& lhs, const stop_token& rhs ) noexcept {
			return lhs.host_id == rhs.host_id;
		}
	};
}

	template<>
	void std::swap<pluginsplusplus::stop_token>( pluginsplusplus::stop_token& lhs, pluginsplusplus::stop_token& rhs ) noexcept {
		std::swap(lhs.host_id, rhs.host_id);
	}

namespace pluginsplusplus {
#endif

	template<same_or_derived_from<plugin_base> PluginBase = plugin_base>
	struct threaded_plugin_base: public PluginBase {
#ifndef PPP_NON_STANDARD_THREADED_PLUGIN_BASE
		threaded_plugin_base() { this->flags.set(plugin_base::Flags::HasThread); }
		threaded_plugin_base(const threaded_plugin_base&) = default;
		threaded_plugin_base(threaded_plugin_base&&) = default;
		threaded_plugin_base& operator=(const threaded_plugin_base&) = default;
		threaded_plugin_base& operator=(threaded_plugin_base&&) = default;

		std::jthread thread;
		void start() override {
			thread = std::jthread([this](std::stop_token stop) { go(stop); });
		}
		void stop() override {
			thread.request_stop();
			thread.join();
		}
#else
		PPP_NON_STANDARD_THREADED_PLUGIN_BASE
#endif
		virtual void go(pluginsplusplus::stop_token) {}
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
			[[unlikely]] if(!plugin->flags[pluginsplusplus::plugin_base::Started]) {\
				plugin->start();\
				plugin->flags.set(pluginsplusplus::plugin_base::Started);\
			}\
			return plugin->step();\
		} catch (std::exception& e) {\
			std::cerr << e.what() << std::endl;\
			return -1;\
		}\
	}





	template<same_or_derived_from<plugin_base> PluginBase = plugin_base>
	struct PluginHandleBase;

	template<same_or_derived_from<plugin_base> PluginBase = plugin_base>
	struct PluginManager;

	template<same_or_derived_from<plugin_base> PluginBase = plugin_base>
	struct plugin_host_plugin_base: public PluginBase {
		plugin_host_plugin_base() { this->flags.set(plugin_base::Flags::CanRegisterPlugin); }
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
