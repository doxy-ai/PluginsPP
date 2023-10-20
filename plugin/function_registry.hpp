#pragma once

#include <unordered_map>
#include <vector>
#include <stdexcept>

namespace pluginsplusplus {

	struct FunctionRegistry {
		struct Function {
			enum Type {
				INVALID = 0,
				Void,
				I64,
				// F64,
				// String,
				// TODO: List support somehow?
			};

			Type returnType;
			void* f;
			std::vector<Type> parameterTypes;
		};

		std::unordered_map<std::string, std::unordered_map<std::string, Function>> registry; // Maps function names to their associated pointers
			// TODO: ^ How do we put some measure of performant security on void*s returned from dynamically loaded functions?
			// TODO: ^ Do we need this to be inside something like a monitor? Race condition issues?
		
		void register_function(std::string_view pluginName, std::string_view name, Function function) {
			registry[std::string(pluginName)][std::string(name)] = function;
		}

		template<typename F = void>
		F* lookup_function(std::string_view name, bool throwIfNull = false) {
			auto pos = name.find(":");
			if(pos == std::string::npos) return nullptr; // There must be a :
			if(name[pos + 1] != ':') return nullptr; // There must be a ::
			auto plugin = std::string(name.substr(0, pos)); // TODO: pos -1?
			auto function = std::string(name.substr(pos + 2));

			if(!registry.contains(plugin)) return nullptr;
			if(!registry[plugin].contains(function)) return nullptr;
			auto f = registry[plugin][function].f;
			[[unlikely]] if(throwIfNull && !f) throw std::invalid_argument("Function `" + std::string(name) + "` was not found in the registry.");
			return (F*)f;
		}

		template<typename Return = void, typename... Args>
		Return call_function(std::string name, Args... args) {
			// TODO: Maybe add some verification here that the types align with what was proposed in the registration!

			auto f = lookup_function<Return(Args...)>(name, true);
			if constexpr(not std::is_same_v<Return, void>)
				return f(std::forward<Args>(args)...);
			else f(std::forward<Args>(args)...);
		}
	};

}