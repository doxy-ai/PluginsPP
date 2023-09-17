#pragma once

#define CR_HOST // Required in the host only and before including cr.h
#include "plugin/plugin_base.hpp"

#include <filesystem>

namespace pluginsplusplus {

	extern uint16_t globalHandleTypeCounter;

	template<same_or_derived_from<plugin_base> PluginBase>
	struct PluginHandleBase {
		uint16_t type = GetTypeID<PluginHandleBase>();
		PluginBase** plugin = nullptr;

		PluginHandleBase(uint16_t type = GetTypeID<PluginHandleBase>(), PluginBase** plugin = nullptr) 
			: type(type), plugin(plugin) {}
		~PluginHandleBase() {}
		PluginHandleBase(const PluginHandleBase&) = default;
		PluginHandleBase(PluginHandleBase&&) = default;
		PluginHandleBase& operator=(const PluginHandleBase&) = default;
		PluginHandleBase& operator=(PluginHandleBase&&) = default;

		virtual bool step() { return true; }

		template<typename T = PluginHandleBase>
		static uint16_t GetTypeID() {
			static uint16_t id = globalHandleTypeCounter++;
			return id;
		}
	};

}