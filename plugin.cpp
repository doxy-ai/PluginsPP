/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "plugin_base.hpp"

namespace ppp = pluginsplusplus;

struct DebugPlugin : public ppp::threaded_plugin_base<ppp::plugin_base> {
	int value = 5;

	void load() /*override*/ {
		std::cout << "load!" << std::endl;
	}
	void start() /*override*/ {
		std::cout << "start!" << std::endl;
	}
	void go(ppp::stop_token) /*override*/ {
		std::cout << "go!" << std::endl;
	}
	void stop() /*override*/ {
		std::cout << "stop!" << std::endl;
	}
	int step() /*override*/ {
		std::cout << "step" << std::endl;
		value++;
        return false;
	}

	static DebugPlugin* create() { return new DebugPlugin; }
};

PLUGIN_BOILERPLATE(DebugPlugin);