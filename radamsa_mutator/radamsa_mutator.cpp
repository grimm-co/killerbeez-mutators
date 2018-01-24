//Windows API
#include <windows.h> 
#include <process.h>
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "radamsa_mutator.h"

#include <utils.h>
#include <jansson.h>
#include <jansson_helper.h>

typedef struct radamsa_state
{
	char * input;

	size_t input_length;

	//The iteration number
	int iteration;

	//The seed for radamsa
	int seed;

	//The path to radamsa.exe
	char * path;

	//The port to bind radamsa.exe to
	int port;

	//The handle to the radamsa instance
	HANDLE process;

} radamsa_state_t;

void cleanup_process(radamsa_state_t * radamsa_state);
int start_process(radamsa_state_t * radamsa_state);


mutator_t radamsa_mutator = {
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
RADAMSA_MUTATOR_API void init(mutator_t * m)
{
	memcpy(m, &radamsa_mutator, sizeof(mutator_t));
}
#endif

RADAMSA_MUTATOR_API radamsa_state_t * setup_options(char * options)
{
	radamsa_state_t * state;
	state = (radamsa_state_t *)malloc(sizeof(radamsa_state_t));
	if (!state)
		return NULL;
	memset(state, 0, sizeof(radamsa_state_t));

	//Setup defaults
	srand((unsigned int)time(NULL));
	state->path = _strdup("C:\\killerbeez\\radamsa\\radamsa\\bin\\radamsa.exe"); //strdup'd so we can uniformly free it later
	state->port = 10000 + (rand() % 50000);
	state->seed = rand();

	if (!options || !strlen(options))
		return state;

	PARSE_OPTION_STRING(state, options, path, "path", FUNCNAME(cleanup));
	PARSE_OPTION_INT(state, options, port, "port", FUNCNAME(cleanup));
	return state;
}

RADAMSA_MUTATOR_API void * FUNCNAME(create)(char * options, char * state, char * input, size_t input_length)
{
	radamsa_state_t * radamsa_state;
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
		return NULL;

	radamsa_state = setup_options(options);
	if (!radamsa_state)
		return NULL;

	radamsa_state->input = (char *)malloc(input_length);
	if (!radamsa_state->input)
	{
		FUNCNAME(cleanup)(radamsa_state);
		return NULL;
	}
	memcpy(radamsa_state->input, input, input_length);
	radamsa_state->input_length = input_length;

	if (FUNCNAME(set_state)(radamsa_state, state))
	{
		FUNCNAME(cleanup)(radamsa_state);
		return NULL;
	}
	return radamsa_state;
}

RADAMSA_MUTATOR_API void FUNCNAME(cleanup)(void * mutator_state)
{
	radamsa_state_t * radamsa_state = (radamsa_state_t *)mutator_state;
	cleanup_process(radamsa_state);
	free(radamsa_state->input);
	free(radamsa_state->path);
	free(radamsa_state);
}

RADAMSA_MUTATOR_API int FUNCNAME(mutate)(void * mutator_state, char * buffer, size_t buffer_length)
{
	radamsa_state_t * radamsa_state = (radamsa_state_t *)mutator_state;
	SOCKET sock = INVALID_SOCKET;
	sockaddr_in clientService;
	int total_read = 0;
	int result;

	//Increment the iteration number
	radamsa_state->iteration++;

	//Create a socket for us to connect to the radamsa daemon
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
		return -1;

	//connect to the radamsa daemon
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr("127.0.0.1");
	clientService.sin_port = htons(radamsa_state->port);
	result = connect(sock, (SOCKADDR *)&clientService, sizeof(clientService));
	if (result == SOCKET_ERROR)
		return -1;

	//Read radamsa's response
	result = 1;
	while (total_read < (int)buffer_length && result > 0)
	{
		result = recv(sock, buffer + total_read, buffer_length - total_read, 0);
		if (result > 0)
			total_read += result;
		else if (result < 0) //Error, then break
			total_read = -1;
	}

	closesocket(sock);

	if (total_read == 0) //In some non-error cases, radamsa just returns 0 bytes
	{ //Since we don't want to do this, just call the mutator again
		total_read = FUNCNAME(mutate)(mutator_state, buffer, buffer_length);
	}
	return total_read;
}

RADAMSA_MUTATOR_API char * FUNCNAME(get_state)(void * mutator_state)
{
	radamsa_state_t * radamsa_state = (radamsa_state_t *)mutator_state;
	json_t *state_obj, *iteration_obj, *seed_obj;
	char * ret;

	state_obj = json_object();
	iteration_obj = json_integer(radamsa_state->iteration);
	seed_obj = json_integer(radamsa_state->seed);
	if (!state_obj || !iteration_obj || !seed_obj)
		return NULL;

	json_object_set_new(state_obj, "iteration", iteration_obj);
	json_object_set_new(state_obj, "seed", seed_obj);
	ret = json_dumps(state_obj, 0);
	json_decref(state_obj);
	return ret;
}

RADAMSA_MUTATOR_API void FUNCNAME(free_state)(char * state)
{
	free(state);
}

RADAMSA_MUTATOR_API int FUNCNAME(set_state)(void * mutator_state, char * state)
{
	int result, temp;
	int ret = 1;
	radamsa_state_t * radamsa_state = (radamsa_state_t *)mutator_state;

	if (state)
	{
		temp = get_int_options(state, "iteration", &result);
		if (result > 0)
			radamsa_state->iteration = temp;
		else
			return 1;

		temp = get_int_options(state, "seed", &result);
		if (result > 0)
			radamsa_state->seed = temp;
		else
			return 1;
	}

	cleanup_process(radamsa_state);
	return start_process(radamsa_state);
}

RADAMSA_MUTATOR_API int FUNCNAME(get_current_iteration)(void * mutator_state)
{
	radamsa_state_t * radamsa_state = (radamsa_state_t *)mutator_state;
	return radamsa_state->iteration;
}

RADAMSA_MUTATOR_API int FUNCNAME(get_total_iteration_count)(void * mutator_state)
{
	return -1; //Infinite
}

/**
* This function will set the input(saved in the mutators state) to something new.
* This can be used to reinitialize a mutator with new data, without reallocating the entire state struct.
* @param mutator_state - a mutator specific structure previously created by the create function.
* @param new_input - The new input used to produce new mutated inputs later when the mutate function is called
* @param input_length - the size in bytes of the input buffer.
* @return 0 on success and -1 on failure
*/
RADAMSA_MUTATOR_API int FUNCNAME(set_input)(void * mutator_state, char * new_input, size_t input_length)
{
	radamsa_state_t * radamsa_state = (radamsa_state_t *)mutator_state;
	if (radamsa_state->input) {
		free(radamsa_state->input);
		radamsa_state->input = NULL;
	}
	radamsa_state->input = (char *)malloc(input_length);
	if (!radamsa_state->input) { //malloc failed
		return -1;
	}
	radamsa_state->input_length = input_length;
	memcpy(radamsa_state->input, new_input, input_length);
	FUNCNAME(set_state)(mutator_state, NULL); //give the new input to radamsa.exe
	return 0;
}

/**
* This function sets a help message for the mutator. This is useful
* if the mutator takes a JSON options string in the create() function.
* @param help_str - A pointer that will be updated to point to the new help string.
* @return 0 on success and -1 on failure
*/
RADAMSA_MUTATOR_API int FUNCNAME(help)(char** help_str)
{
	*help_str = strdup(
		"radamsa - Radamsa mutator (Starts and calls radamsa to mutate input)\n"
		"Options:\n"
		"\tpath                  The path to radamsa.exe\n"
		"\tport                  The port to tell radamsa to bind to when starting up\n"
		"\n"
	);
	if (*help_str == NULL)
		return -1;
	return 0;
}

void cleanup_process(radamsa_state_t * radamsa_state)
{
	if (radamsa_state->process)
	{
		TerminateProcess(radamsa_state->process, 9);
		CloseHandle(radamsa_state->process);
		radamsa_state->process = NULL;
	}
}

int start_process(radamsa_state_t * radamsa_state)
{
	char cmd_line[256];
	snprintf(cmd_line, sizeof(cmd_line) - 1, "%s -o :%d -n inf -s %d ", radamsa_state->path, radamsa_state->port, radamsa_state->seed);
	if (radamsa_state->iteration != 0)
		snprintf(cmd_line, sizeof(cmd_line) - 1, "%s -S %d ", cmd_line, radamsa_state->iteration);
	return start_process_and_write_to_stdin(cmd_line, radamsa_state->input, radamsa_state->input_length, &radamsa_state->process);
}
