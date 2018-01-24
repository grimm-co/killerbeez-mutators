#include "nop_mutator.h"
#include "mutators.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct nop_state
{
	char * input;
	size_t input_length;
	int iteration;
};
typedef struct nop_state nop_state_t;

mutator_t nop_mutator = {
	FUNCNAME(create),
	FUNCNAME(cleanup),
	FUNCNAME(mutate),
	FUNCNAME(get_state),
	FUNCNAME(free_state),
	FUNCNAME(set_state),
	FUNCNAME(get_current_iteration),
	FUNCNAME(get_total_iteration_count),
	FUNCNAME(set_input),
	FUNCNAME(help)
};

#ifndef ALL_MUTATORS_IN_ONE
NOP_MUTATOR_API void init(mutator_t * m)
{
	memcpy(m, &nop_mutator, sizeof(mutator_t));
}
#endif

NOP_MUTATOR_API void * FUNCNAME(create)(char * options, char * state, char * input, size_t input_length)
{
	nop_state_t * nop_state;
	nop_state = (nop_state_t *)malloc(sizeof(nop_state_t));
	if (!nop_state)
		return NULL;
	memset(nop_state, 0, sizeof(nop_state_t));

	nop_state->input = (char *)malloc(input_length);
	if (!nop_state->input)
	{
		FUNCNAME(cleanup)(nop_state);
		return NULL;
	}
	memcpy(nop_state->input, input, input_length);
	nop_state->input_length = input_length;
	return nop_state;
}

NOP_MUTATOR_API void FUNCNAME(cleanup)(void * mutator_state)
{
	nop_state_t * nop_state = (nop_state_t *)mutator_state;
	free(nop_state->input);
	free(nop_state);
}

NOP_MUTATOR_API int FUNCNAME(mutate)(void * mutator_state, char * buffer, size_t buffer_length)
{
	nop_state_t * nop_state = (nop_state_t *)mutator_state;
	nop_state->iteration++;
	memcpy(buffer, nop_state->input, nop_state->input_length > buffer_length ? buffer_length : nop_state->input_length);
	return nop_state->input_length;
}

NOP_MUTATOR_API char * FUNCNAME(get_state)(void * mutator_state)
{
	return "";
}

NOP_MUTATOR_API void FUNCNAME(free_state)(char * state)
{
}

NOP_MUTATOR_API int FUNCNAME(set_state)(void * mutator_state, char * state)
{
	return 0;
}

NOP_MUTATOR_API int FUNCNAME(get_current_iteration)(void * mutator_state)
{
	nop_state_t * nop_state = (nop_state_t *)mutator_state;
	return nop_state->iteration;
}

NOP_MUTATOR_API int FUNCNAME(get_total_iteration_count)(void * mutator_state)
{
	return -1;//Infinite
}

/**
* This function will set the input(saved in the mutators state) to something new.
* This can be used to reinitialize a mutator with new data, without reallocating the entire state struct.
* @param mutator_state - a mutator specific structure previously created by the create function.
* @param new_input - The new input used to produce new mutated inputs later when the mutate function is called
* @param input_length - the size in bytes of the input buffer.
* @return 0 on success and -1 on failure
*/
NOP_MUTATOR_API int FUNCNAME(set_input)(void * mutator_state, char * new_input, size_t input_length)
{
	nop_state_t * nop_state = (nop_state_t *)mutator_state;
	if (nop_state->input) {
		free(nop_state->input);
		nop_state->input = NULL;
	}
	nop_state->input = (char *)malloc(input_length);
	if (nop_state->input == NULL) { //malloc failed
		return -1;
	}
	nop_state->input_length = input_length;
	memcpy(nop_state->input, new_input, input_length);
	return 0;
}

/**
* This function sets a help message for the mutator. This is useful
* if the mutator takes a JSON options string in the create() function.
* @param help_str - A pointer that will be updated to point to the new help string.
* @return 0 on success and -1 on failure
*/
NOP_MUTATOR_API int FUNCNAME(help)(char** help_str)
{
	*help_str = _strdup(
		"nop - NOP mutator (doesn't mutate the input, mostly for debugging)\n"
		"\n"
	);
	if (*help_str == NULL)
		return -1;
	return 0;
}