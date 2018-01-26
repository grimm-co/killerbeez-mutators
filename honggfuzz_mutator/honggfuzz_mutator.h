#pragma once

#ifdef HONGGFUZZ_MUTATOR_EXPORTS
#define HONGGFUZZ_MUTATOR_API extern "C" __declspec(dllexport)
#else
#define HONGGFUZZ_MUTATOR_API __declspec(dllimport)
#endif

#include <global_types.h>
#include "mutators.h"

#define MUTATOR_NAME "honggfuzz"

HONGGFUZZ_MUTATOR_API void * FUNCNAME(create)(char * options, char * state, char * input, size_t input_length);
HONGGFUZZ_MUTATOR_API void FUNCNAME(cleanup)(void * mutator_state);
HONGGFUZZ_MUTATOR_API int FUNCNAME(mutate)(void * mutator_state, char * buffer, size_t buffer_length);
HONGGFUZZ_MUTATOR_API char * FUNCNAME(get_state)(void * mutator_state);
HONGGFUZZ_MUTATOR_API void FUNCNAME(free_state)(char * state);
HONGGFUZZ_MUTATOR_API int FUNCNAME(set_state)(void * mutator_state, char * state);
HONGGFUZZ_MUTATOR_API int FUNCNAME(get_current_iteration)(void * mutator_state);
HONGGFUZZ_MUTATOR_API int FUNCNAME(get_total_iteration_count)(void * mutator_state);
HONGGFUZZ_MUTATOR_API int FUNCNAME(set_input)(void * mutator_state, char * new_input, size_t input_length);
HONGGFUZZ_MUTATOR_API int FUNCNAME(help)(char ** help_str);

#ifndef ALL_MUTATORS_IN_ONE
HONGGFUZZ_MUTATOR_API void init(mutator_t * m);
#endif