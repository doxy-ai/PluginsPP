#pragma once

#include "handle.hpp"

namespace pluginsplusplus {

	template<std::derived_from<pluginsplusplus::plugin_base> PluginBase = pluginsplusplus::plugin_base>
	struct WASMPluginLoader: public pluginsplusplus::plugin_host_plugin_base<PluginBase> {
		// void register_plugin(std::unique_ptr<PluginHandleBase<PluginBase>>&& plugin) {
		// 	manager->register_plugin(std::move(plugin));
		// }

		std::vector<NativeSymbol> native_symbols = {
			{ "__cxa_throw", (void*)ppp_cxa_throw, "(iii)", nullptr },
		};

		// static char global_heap_buf[512 * 1024];

		void load() override {}
		void start() override {
			register_custom_symbols(native_symbols);

			RuntimeInitArgs init_args = {
				.mem_alloc_type = Alloc_With_System_Allocator,
				// .mem_alloc_type = Alloc_With_Pool;
				// .mem_alloc_option.pool.heap_buf = global_heap_buf;
				// .mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
				.native_module_name = "env",
				.native_symbols = native_symbols.data(),
				.n_native_symbols = (uint32_t)native_symbols.size(),
			};

	#ifdef PPP_WASM_CUSTOM_INIT_ARGS
			register_custom_init_args(init_args);
	#endif

			if (!wasm_runtime_full_init(&init_args)) 
				throw std::runtime_error("WASM runtime environment initialization failed.");

			WASMPluginHandle<PluginBase>::register_loaders();
		}
		void stop() override {
			wasm_runtime_destroy();
		}
		// int step() override { return 0; }

		static WASMPluginLoader* create() { return new WASMPluginLoader; }
	};

}