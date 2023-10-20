#pragma once

#include "plugin_manager.hpp"

namespace pluginsplusplus {

	template<same_or_derived_from<plugin_base> PluginBase = plugin_base>
	struct DynamicPluginHandle: public PluginHandleBase<PluginBase> {
		cr_plugin ctx;

		DynamicPluginHandle(PluginBase** plugin = nullptr, cr_plugin ctx = {}) 
			: PluginHandleBase<PluginBase>(PluginHandleBase<PluginBase>::template GetTypeID<DynamicPluginHandle>(), plugin), ctx(ctx) {}
		virtual ~DynamicPluginHandle() { if(ctx.p) cr_plugin_close(ctx); }
		DynamicPluginHandle(const DynamicPluginHandle&) = delete;
		DynamicPluginHandle(DynamicPluginHandle&& o) { *this = std::move(o); }
		DynamicPluginHandle& operator=(const DynamicPluginHandle&) = delete;
		DynamicPluginHandle& operator=(DynamicPluginHandle&& o) {
			this->type = o.type;
			this->plugin = o.plugin;
			ctx = o.ctx;
			o.ctx.p = nullptr;
			return *this;
		}

		static DynamicPluginHandle load(std::string_view path) { 
			DynamicPluginHandle out;
			if(!std::filesystem::exists(path))
				throw std::invalid_argument("Failed to open: " + std::string(path) + " (It doesn't seam to exist!)");
			cr_plugin_open(out.ctx, path.data());
			out.step(); // This is nessicary for references to be valid! There is a check in the plugin's main to skip this!
			return out;
		}
		static std::unique_ptr<PluginHandleBase<PluginBase>> load_unique(std::filesystem::path path) {
			return std::make_unique<DynamicPluginHandle>(load(path.string()));
		}

		static void register_loaders() {
			PluginManager<plugin_base>::register_loader(std::filesystem::path(CR_PLUGIN("a")).extension().string(), &DynamicPluginHandle<plugin_base>::load_unique);
		}

		bool step() override {
			bool changed = cr_plugin_changed(ctx);
			if(cr_plugin_update(ctx))
				return false;

			// Update pointers if the .so file changes 
			if(changed) {
				so_handle handle = ((cr_internal*)ctx.p)->handle;
				this->plugin = (PluginBase**)cr_so_symbol(handle, "plugin");
				
				std::cout << this->plugin << std::endl;

				return this->plugin != nullptr;
			}
			return true;
		}

		void stop() override { // TODO: Nessicary?
			cr_plugin_unload(ctx, false, true);
		}
	};

}