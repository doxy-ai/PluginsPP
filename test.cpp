#include "wasm_export.h"
#include "bh_read_file.h"
#include "bh_getopt.h"
#include "plugin_base.hpp"
#include "../../plugin_manager.hpp"
#include <fstream>
#include <span>
#include <mutex>

std::vector<std::stop_token> wasm_thread_stop_tokens;

namespace pluginsplusplus {

extern "C" void ppp_cxa_throw(int32_t thrown_exception_, int32_t type_info_, int32_t dest_);

#define PPP_WASM_NO_CUSTOM_SYMBOLS

void register_custom_symbols(std::vector<NativeSymbol>& native_symbols)
#ifdef PPP_WASM_NO_CUSTOM_SYMBOLS
{}
#else
;
#endif

template<std::derived_from<plugin_base> PluginBase = plugin_base>
struct WASMPluginHandle: public PluginHandleBase<PluginBase> {
	std::vector<uint8_t> buffer; // TODO: Needs to survive for whole lifetime?
	wasm_module_t module = nullptr;
	wasm_module_inst_t module_inst = nullptr;
	wasm_exec_env_t exec_env = nullptr;
	uint32_t stack_size = 8092, heap_size = 8092;

	uint32_t go_host_id = -1;
	std::jthread go_thread;

	WASMPluginHandle(PluginBase** plugin = nullptr, uint32_t stack_size = 8092, uint32_t heap_size = 8092)
		: PluginHandleBase<PluginBase>(PluginHandleBase<PluginBase>::template GetTypeID<WASMPluginHandle>(), plugin), stack_size(stack_size), heap_size(heap_size) {}
	virtual ~WASMPluginHandle() {
		if (exec_env) wasm_runtime_destroy_exec_env(exec_env);
		if (module_inst) wasm_runtime_deinstantiate(module_inst);
		if (module) wasm_runtime_unload(module);
	}
	WASMPluginHandle(const WASMPluginHandle&) = delete;
	WASMPluginHandle(WASMPluginHandle&& o) { *this = std::move(o); }
	WASMPluginHandle& operator=(const WASMPluginHandle&) = delete;
	WASMPluginHandle& operator=(WASMPluginHandle&& o) {
		this->type = o.type;
		this->plugin = o.plugin;
		buffer = std::move(o.buffer);
		module = std::exchange(o.module, nullptr);
		module_inst = std::exchange(o.module_inst, nullptr);
		exec_env = std::exchange(o.exec_env, nullptr);
		stack_size = o.stack_size;
		heap_size = o.heap_size;
		go_host_id = o.go_host_id;
		go_thread = std::move(o.go_thread);
		func_get_plugin = o.func_get_plugin;
		func_is_plugin_threaded = o.func_is_plugin_threaded;
		started = o.started;
		func_step = o.func_step;
		return *this;
	}

	static WASMPluginHandle load(std::string_view path, uint32_t stack_size = 8092, uint32_t heap_size = 8092) {
		WASMPluginHandle out;
		if(!std::filesystem::exists(path))
			throw std::invalid_argument("Failed to open: " + std::string(path) + " (It doesn't seam to exist!)");

		{
			std::ifstream fin(std::string{path});
			fin.seekg(0, std::ios::end);
			out.buffer.resize(fin.tellg());
			fin.seekg(0, std::ios::beg);
			fin.read((char*)out.buffer.data(), out.buffer.size());
		}

		std::string error_buf; error_buf.resize(128); // TODO: Needs to survive for whole lifetime?
		out.module = wasm_runtime_load(out.buffer.data(), out.buffer.size(), error_buf.data(), error_buf.size());
		if (!out.module)
			throw std::runtime_error("Load wasm module failed. error: " + error_buf);

		out.module_inst = wasm_runtime_instantiate(out.module, stack_size, heap_size, error_buf.data(), error_buf.size());
		if (!out.module_inst)
			throw std::runtime_error("Instantiate wasm module failed. error: " + error_buf);
		
		out.stack_size = stack_size;
		out.heap_size = heap_size;

		out.exec_env = wasm_runtime_create_exec_env(out.module_inst, stack_size);
		if (!out.exec_env)
			throw std::runtime_error("Create wasm execution environment failed.\n");

		out._start(); // Invoke the plugin's main function (which calls load)
		out.plugin = out.get_plugin();
		return out;
	}
	static std::unique_ptr<PluginHandleBase<PluginBase>> load_unique(std::filesystem::path path) {
		return std::make_unique<WASMPluginHandle>(load(path.string()));
	}

	static void register_loaders() {
		pluginsplusplus::PluginManager<PluginBase>::register_loader("wasm", WASMPluginHandle<PluginBase>::load_unique);
	}




	wasm_function_inst_t func_get_plugin = nullptr;
private:
	PluginBase* real_plugin;
public:
	PluginBase** get_plugin() {
		uint32_t args[1] = {0};
		call_function("get_plugin", func_get_plugin, {args, 1});
		// if (!wasm_runtime_validate_app_addr(module_inst, *args, 1));
		// 	throw std::runtime_error("'get_plugin' returned an invalid pointer!");
		real_plugin = (PluginBase*) wasm_runtime_addr_app_to_native(module_inst, *args);
		return &real_plugin;
	}

	wasm_function_inst_t func_is_plugin_threaded = nullptr;
	bool is_plugin_threaded() {
		uint32_t args[1] = {0};
		call_function("is_plugin_threaded", func_is_plugin_threaded, {args, 1});
		return *args;
	}

	void _start() { 
		uint32_t args[1] = {0};
		call_function("_start", {args, 1});
	}


	std::mutex lock;
	void go() {
		if (go_host_id == (uint32_t)-1)
			go_host_id = wasm_thread_stop_tokens.size();

		go_thread = std::jthread([this](std::stop_token stop) {
			try {
				if (!wasm_runtime_init_thread_env())
					throw std::runtime_error("Failed to initialize thread environment.");

				// Wait until the main thread has finished adding the stop token to the list
				while(wasm_thread_stop_tokens.size() < go_host_id + 1) {}

				uint32_t args[1] = { go_host_id };
				std::scoped_lock _(lock);
				call_function("go", {args, 1});
			} catch(std::exception& e) {
				std::cerr << "Exception in Go thread: " << e.what() << std::endl;
			}
			wasm_runtime_destroy_thread_env();
		});

		if(wasm_thread_stop_tokens.size() <= go_host_id)
			wasm_thread_stop_tokens.resize(go_host_id + 1);
		wasm_thread_stop_tokens[go_host_id] = go_thread.get_stop_token();
	}

	bool started = false;
	wasm_function_inst_t func_step = nullptr;
	bool step() override {
		// if(!started) {
		// 	go();
		// 	started = true;
		// }

		uint32_t args[1] = {0};
		// std::scoped_lock _(lock);
		call_function("step", func_step, {args, 1});
		return *args == 0;
	}

	int stop() {
		uint32_t args[1] = {0};
		call_function("stop", {args, 1});
		return *args;
	}


protected:
	wasm_function_inst_t find_function(std::string name) {
		wasm_function_inst_t func = nullptr;
		if (!(func = wasm_runtime_lookup_function(module_inst, name.c_str(), nullptr)))
			throw std::runtime_error("The wasm function '" + std::string(name) + "' wasm function was not found/exported.");
		return func;
	}

	void call_function(std::string_view name, const wasm_function_inst_t& func, std::span<uint32_t> args = {(uint32_t*)nullptr, 0}) {
		if (!wasm_runtime_call_wasm(exec_env, func, args.size(), args.data()))
			throw std::runtime_error("Call to wasm function '" + std::string(name) + "' failed. error: " + std::string(wasm_runtime_get_exception(module_inst)));
	}

	void call_function(std::string_view name, wasm_function_inst_t& func, std::span<uint32_t> args = {(uint32_t*)nullptr, 0}) {
		if(!func) func = find_function(std::string(name));
		call_function(name, (const wasm_function_inst_t)func, args);
	}

	void call_function(std::string_view name, std::span<uint32_t> args = {(uint32_t*)nullptr, 0}) {
		const wasm_function_inst_t func = find_function(std::string(name));
		call_function(name, func, args);
	}
};

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


void print_usage(void) {
	fprintf(stdout, "Options:\r\n");
	fprintf(stdout, "  -f [path of wasm file] \n");
}

namespace ppp = pluginsplusplus;

int main(int argc, char *argv_main[]) {
	auto loader = ppp::WASMPluginLoader<ppp::plugin_base>::create();
	loader->start();

	int opt;
	char* wasm_path;
	while ((opt = getopt(argc, argv_main, "hf:")) != -1) {
		switch (opt) {
			case 'f':
				wasm_path = optarg;
				break;
			case 'h':
				print_usage();
				return 0;
			case '?':
				print_usage();
				return 0;
		}
	}
	if (optind == 1) {
		print_usage();
		return 0;
	}

	auto plugin = ppp::WASMPluginHandle<ppp::plugin_base>::load({wasm_path});
	while(plugin.step()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}