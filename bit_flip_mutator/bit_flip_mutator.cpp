#include "bit_flip_mutator.h"
#include "mutators.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <utils.h>
#include <jansson.h>
#include <jansson_helper.h>

/**
* This is the state struct for the mutator. It contains all necessary information
* for the mutator to pickup where it left off.
*/
struct bf_state
{
	char * input; ///< Original input given to the mutator
	size_t input_length; ///< Length of the original input
	size_t byte_index; ///< keep track of which byte we are working on
	size_t bit_index; ///< keep track of which bit we just flipped (0 - 7)
	size_t max_iteration; ///< The number of iterations needed to flip every bit
	size_t iteration; ///< The current iteration we are on
};
typedef struct bf_state bf_state_t;

/**
* This is the mutator struct that contains all the function pointers to a mutators
* various functions.
*/
mutator_t bf_mutator = {
	FUNCNAME(create),  ///< create() func. pointer
	FUNCNAME(cleanup), ///< cleanup() func. pointer
	FUNCNAME(mutate), ///< mutate() func. pointer
	FUNCNAME(get_state), ///< get_state() func. pointer
	FUNCNAME(free_state), ///< free_state() func. pointer
	FUNCNAME(set_state), ///< set_state() func. pointer
	FUNCNAME(get_current_iteration), ///< get_current_iteration() func. pointer
	FUNCNAME(get_total_iteration_count), ///< get_total_iteration_count() func. pointer
	FUNCNAME(set_input), ///< set_input() func. pointer
	FUNCNAME(help) ///< help() func. pointer
};

/**
* This function filled in m with all of the function pointers for this mutator.
* Note: This function only appears when compiled as a module. 
* When ALL_MUTATORS_IN_ONE is defined, this function will not exist, as there
* would be a name collision with all the other init() functions from other
* modules and there will not be any need for obtaining this struct as all the
* functions will just be called directly.  
* It’s just the code which uses modules which will want to use this struct.
*
* @param m - a pointer to a freshly malloc’ed mutator_t structure
* @return none 
*/
#ifndef ALL_MUTATORS_IN_ONE
BF_MUTATOR_API void init(mutator_t * m)
{
	memcpy(m, &bf_mutator, sizeof(mutator_t));
}
#endif

/**
* This function will allocate and initialize the mutator structure.  
* The lifetime of the structure which is allocated will be until 
* the cleanup() function is called.
*
* @param options - a json string that contains the mutator specific string of options.
* @param state - used to load a previously dumped state (with the get_state() function), 
* that defines the current iteration of the mutator.  This will be a mutator specific JSON string. 
* Alternatively, NULL can be provided to start a mutator without a previously dumped state.
*
* @param input - used to produce new mutated inputs later when the mutate function is called
* @param input_length - the size of the input buffer
* @return a mutator specific structure or NULL on failure.  The returned value should 
* not be used for anything other than passing to the various Mutator API functions.
*/
BF_MUTATOR_API void * FUNCNAME(create)(char * options, char * state, char * input, size_t input_length)
{
	//setup mem for the struct
	bf_state_t * bf_state;
	bf_state = (bf_state_t *)malloc(sizeof(bf_state_t));
	if (bf_state == NULL) //check for failed malloc
		return NULL;
	memset(bf_state, 0, sizeof(bf_state_t)); //zero out the mem

	bf_state->input = (char *)malloc(input_length);
	if (bf_state->input == NULL)
	{
		//malloc failed
		FUNCNAME(cleanup)(bf_state);
		return NULL;
	}
	
	memcpy(bf_state->input, input, input_length);//copy data to mutate into struct
	bf_state->input_length = input_length;
	bf_state->byte_index = 0;
	bf_state->bit_index = 0;
	bf_state->max_iteration = input_length * 8;
	bf_state->iteration = 0;

	if (state) //Check if a saved state was passed in
	{
		if (!FUNCNAME(set_state)(bf_state, state))
		{
			//IF setting the state with the a previously saved state fails, cleanup
			FUNCNAME(cleanup)(bf_state);
			return NULL;
		}
	}
	
	return bf_state;
}

/**
* This function will release any resources that the mutator has open 
* and free the mutator state structure.
* @param mutator_state - a mutator specific structure previously created by 
* the create function.  This structure will be freed and should not be referenced afterwards.
*/
BF_MUTATOR_API void FUNCNAME(cleanup)(void * mutator_state)
{
	bf_state_t * bf_state = (bf_state_t *)mutator_state;
	free(bf_state->input);
	free(bf_state);
}

/**
* This function will mutate the input given in the create function and return it in the output_buffer argument.  
* The size of the output_buffer will be mutator specific.  For example, some mutators may require this buffer
* to be larger than the original input (passed to the create() function) as it’s going to extend the original
* input in some way.  Other mutators will want it to be the same size.  Guidance on this will be specified by
* the mutator specific documentation.
*
* This mutator expects output_buffer to be the same size as the original input buffer passed to create()
*
* Mutation Scheme:
*	For every byte in the input, 8 bitmasks are XORed with the target input byte. One XOR operation is performed
*	per function call (meaning after 8 calls to mutate(), eight bits have been flipped). The bitmask is
*	calculated by left-shifting 0x1 by the bit_index, which can be 0-7. Once every bit has been flipped,
*	the byte_index is increased and the bit_index is reset to 0. This is done until all bits have been flipped!
*
* @param mutator_state - a mutator specific structure previously created by the create function.
* @param output_buffer - a buffer that the mutated input will be written to
* @param buffer_length - the size of the passed in buffer argument
* @return - the length of the mutated data, 0 when the mutator is out of mutations, or -1 on error
*/
BF_MUTATOR_API int FUNCNAME(mutate)(void * mutator_state, char * output_buffer, size_t buffer_length)
{
	bf_state_t * bf_state = (bf_state_t *)mutator_state;
	if (bf_state->iteration >= bf_state->max_iteration)
		return 0; //We flipped all the bits

	//The buffer must be at least as long as the input buffer
	if (buffer_length < bf_state->input_length)
		return -1;

	memcpy(output_buffer, bf_state->input, bf_state->input_length);
	//pull the target byte from memory
	char target_byte = (bf_state->input)[bf_state->byte_index];
	//mutate it
	char mutated_byte = target_byte ^ (0x1 << bf_state->bit_index);
	output_buffer[bf_state->byte_index] = mutated_byte;
	
	bf_state->bit_index++;
	bf_state->iteration++;

	if (bf_state->bit_index >= 8)
	{
		bf_state->bit_index = 0;
		bf_state->byte_index++;
	}

	return bf_state->input_length; // Length will never increase or decrease
}

/**
* This function will return the state of the mutator.  The returned value can be used to restart the 
* mutator at a later time, by passing it to the create or set_state function.  It is the caller’s 
* responsibility to free the memory allocated here.
*
* @param mutator_state - a mutator specific structure previously created by the create function.
* @ return a buffer that defines the current state of the mutator.  This will be a mutator specific JSON string. 
*/
BF_MUTATOR_API char * FUNCNAME(get_state)(void * mutator_state)
{
	bf_state_t * bf_state = (bf_state_t *)mutator_state;
	json_t *state_obj, *byte_index_obj, *bit_index_obj;
	char * ret;

	state_obj = json_object();
	byte_index_obj = json_integer(bf_state->byte_index);
	bit_index_obj = json_integer(bf_state->bit_index);
	if (!state_obj || !byte_index_obj || !bit_index_obj)
		return NULL;

	//create the final JSON blob
	json_object_set_new(state_obj, "byte_index", byte_index_obj);
	json_object_set_new(state_obj, "bit_index", bit_index_obj);
	ret = json_dumps(state_obj, 0);
	json_decref(state_obj);
	return ret;
}

/**
* This function will free a previously dumped state (via the get_state function) of the mutator.
* @param state - a previously dumped state buffer obtained by the get_state function.
*/
BF_MUTATOR_API void FUNCNAME(free_state)(char * state)
{
	free(state);
}

/**
* This function will set the current state of the mutator.  
* This can be used to restart a mutator once from a previous run.
* @param mutator_state - a mutator specific structure previously created by the
* create function.
* @param state - a previously dumped state buffer obtained by the get_state 
* function.  This will be a mutator specific JSON string. 
* @return 0 on success or non-zero on failure
*/
BF_MUTATOR_API int FUNCNAME(set_state)(void * mutator_state, char * state)
{
	int result, temp;

	if (!mutator_state)
		return -1;

	bf_state_t * bf_state = (bf_state_t *)mutator_state;

	if (state)
	{
		temp = get_int_options(state, "byte_index", &result);
		if (result > 0)
			bf_state->byte_index = temp;
		else
			return 1;

		temp = get_int_options(state, "bit_index", &result);
		if (result > 0)
			bf_state->bit_index = temp;
		else
			return 1;

		//Set the current iteration value
		bf_state->iteration = (bf_state->byte_index * 8) + bf_state->bit_index;
	}
	return 0;
}

/**
* This function will return the current iteration count of the mutator, i.e. 
* how many mutations have been generated with it.
* @param mutator_state - a mutator specific structure previously created by the
* create function.
* @return value - the number of previously generated mutations
*/
BF_MUTATOR_API int FUNCNAME(get_current_iteration)(void * mutator_state)
{
	bf_state_t * bf_state = (bf_state_t *)mutator_state;
	return bf_state->iteration;
}

/**
* This function will return the total possible number of mutations with this
* mutator.  For some mutators, this value won’t be possible to predict or the 
* mutator will be capable of an infinite number of mutations.
* @param mutator_state - a mutator specific structure previously created by the create function.
* @return the number of possible mutations with this mutator.  
*/
BF_MUTATOR_API int FUNCNAME(get_total_iteration_count)(void * mutator_state)
{
	bf_state_t * bf_state = (bf_state_t *)mutator_state;
	return bf_state->max_iteration;
}

/**
* This function will set the input(saved in the mutators state) to something new.
* This can be used to reinitialize a mutator with new data, without reallocating the entire state struct.
* @param mutator_state - a mutator specific structure previously created by the create function.
* @param new_input - The new input used to produce new mutated inputs later when the mutate function is called
* @param input_length - the size in bytes of the input buffer.
* @return 0 on success and -1 on failure
*/
BF_MUTATOR_API int FUNCNAME(set_input)(void * mutator_state, char * new_input, size_t input_length)
{
	bf_state_t * bf_state = (bf_state_t *)mutator_state;
	//Free the old input
	if(bf_state->input)
	{
		free(bf_state->input);
		bf_state->input = NULL;
	}
	
	bf_state->input = (char *)malloc(input_length);
	if (bf_state->input == NULL)
		return -1;

	memcpy(bf_state->input, new_input, input_length);
	return 0;
}

/**
* This function sets a help message for the mutator. This is useful 
* if the mutator takes a JSON options string in the create() function.
* @param help_str - A pointer that will be updated to point to the new help string.
* @return 0 on success and -1 on failure
*/
BF_MUTATOR_API int FUNCNAME(help)(char **help_str)
{
	*help_str = _strdup("The Bit Flip Mutator. It flips bits! No options\n");
	if (*help_str == NULL)
		return -1;
	return 0;
}
