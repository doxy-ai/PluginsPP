#include "bh_read_file.h"
#include "bh_getopt.h"

#define PPP_WASM_NO_CUSTOM_SYMBOLS
#include "../plugin.hpp"


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