#include "plugin/plugin_base.hpp"
#include "plugin_manager.hpp"

#include <iostream>

namespace ppp = pluginsplusplus;

void print_hello() {
    std::cout << "Hello World" << std::endl;
}

struct RegisterPlugin: public ppp::plugin_host_plugin_base<ppp::plugin_base> {
    std::string_view name() override { return "RegisterPlugin"; }

	void start() override {
        using Type = ppp::FunctionRegistry::Function::Type;
		register_function("print_hello", ppp::FunctionRegistry::Function{Type::Void, (void*)print_hello, {}});
	}

	static RegisterPlugin* create() { return new RegisterPlugin; }
};

PLUGIN_BOILERPLATE(RegisterPlugin)