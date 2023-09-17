#pragma once

#define PPP_NON_STANDARD_STOP_TOKEN
#define PPP_NON_STANDARD_THREADED_PLUGIN_BASE
#include "../plugin_base.hpp"

#ifndef PPP_WASM_IMPL

#define WASI_EXPORT(name) __attribute__((export_name(#name))) extern "C"

#undef PLUGIN_BOILERPLATE
#define PLUGIN_BOILERPLATE(Type)\
	Type* plugin = nullptr;\
	int main() {\
	/*try {*/\
		plugin = Type::create();\
		plugin->load();\
		return 0;\
	/*} catch (std::exception& e) {\
		std::cerr << e.what() << std::endl;\
		return -1;\
	}*/\
	}\
\
	WASI_EXPORT(step) int32_t step() {\
	/*try{*/\
		[[unlikely]] if(!plugin->flags[pluginsplusplus::plugin_base::Started]) {\
			plugin->start();\
			plugin->flags.set(pluginsplusplus::plugin_base::Started);\
		}\
		return plugin->step();\
	/*} catch (std::exception& e) {\
		std::cerr << e.what() << std::endl;\
		return -1;\
	}*/\
	}\
\
	WASI_EXPORT(stop) int32_t stop() {\
	/*try {*/\
		plugin->stop();\
		return 0;\
	/*} catch (std::exception& e) {\
		std::cerr << e.what() << std::endl;\
		return -1;\
	}*/\
	} \
\
	WASI_EXPORT(get_plugin) void* get_plugin() { return plugin; }\
	WASI_EXPORT(is_plugin_threaded) bool is_plugin_threaded() { return plugin->flags[pluginsplusplus::plugin_base::Flags::HasThread]; }\
\
	WASI_EXPORT(go) void go(int32_t host_id) {\
		if constexpr(requires{ plugin->go({host_id}); })\
			plugin->go({host_id});\
		else std::cout << "Plugin doesn't support threaded mode!" << std::endl;\
	}\
\
	extern "C" bool ppp_stop_token_stop_requested(int32_t host_id);\
	[[nodiscard]] bool pluginsplusplus::stop_token::stop_requested() const noexcept {\
		return ppp_stop_token_stop_requested(host_id);\
	}\
\
	static constexpr size_t EXCEPTION_BUFF_SIZE = 1024;\
	char exception_buff[EXCEPTION_BUFF_SIZE];\
\
	extern "C" void* __cxa_allocate_exception(size_t thrown_size) {\
		if (thrown_size > EXCEPTION_BUFF_SIZE) printf("Exception too big");\
		return exception_buff;\
	}

#endif