#pragma once

#include <global_types.h>

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the NOP_MUTATOR_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// NOP_MUTATOR_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef NOP_MUTATOR_EXPORTS
#define NOP_MUTATOR_API extern "C" __declspec(dllexport)
#else
#define NOP_MUTATOR_API __declspec(dllimport)
#endif

#include "mutators.h"

#define MUTATOR_NAME "nop"

NOP_MUTATOR_API void * FUNCNAME(create)(char * options, char * state, char * input, size_t input_length);
NOP_MUTATOR_API void FUNCNAME(cleanup)(void * mutator_state);
NOP_MUTATOR_API int FUNCNAME(mutate)(void * mutator_state, char * buffer, size_t buffer_length);
NOP_MUTATOR_API char * FUNCNAME(get_state)(void * mutator_state);
NOP_MUTATOR_API void FUNCNAME(free_state)(char * state);
NOP_MUTATOR_API int FUNCNAME(set_state)(void * mutator_state, char * state);
NOP_MUTATOR_API int FUNCNAME(get_current_iteration)(void * mutator_state);
NOP_MUTATOR_API int FUNCNAME(get_total_iteration_count)(void * mutator_state);
NOP_MUTATOR_API int FUNCNAME(set_input)(void * mutator_state, char * new_input, size_t input_length);
NOP_MUTATOR_API int FUNCNAME(help)(char ** help_str);

#ifndef ALL_MUTATORS_IN_ONE
NOP_MUTATOR_API void init(mutator_t * m);
#endif