#include "plugin/plugin_base.hpp"
#include "plugin_manager.hpp"

#include <iostream>

namespace ppp = pluginsplusplus;

struct CallPlugin: public ppp::plugin_host_plugin_base<ppp::plugin_base> {

	void start() override {
        auto print_hello = lookup_function<void()>("RegisterPlugin::print_hello");

		print_hello();
	}

	static CallPlugin* create() { return new CallPlugin; }
};

PLUGIN_BOILERPLATE(CallPlugin)