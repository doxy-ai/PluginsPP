#include "bh_platform.h"
#include "wasm_export.h"
#include <cmath>
#include <utility>
#include <thread>
#include <vector>
#include <iostream>

std::vector<std::stop_token> wasm_thread_stop_tokens;

extern "C" bool ppp_stop_token_stop_requested(int32_t host_id) {
    return wasm_thread_stop_tokens.at(host_id).stop_requested();
}

extern "C" void __cxa_throw(void* thrown_exception, struct type_info *tinfo, void (*dest)(void*));
extern "C" void ppp_cxa_throw(wasm_exec_env_t exec_env, int32_t thrown_exception_, int32_t type_info_, int32_t dest_) {
    wasm_module_inst_t instance = get_module_inst(exec_env);
    void* thrown_exception = wasm_runtime_addr_app_to_native(instance, thrown_exception_);
    struct type_info * type_info = (struct type_info *)wasm_runtime_addr_app_to_native(instance, type_info_);
    void (*dest)(void*) = (void (*)(void*))wasm_runtime_addr_app_to_native(instance, dest_);
    std::cerr << "An exception was thrown! The plugin will now terminate!" << std::endl;
    __cxa_throw(thrown_exception, type_info, dest);
    // std::unreachable();
}