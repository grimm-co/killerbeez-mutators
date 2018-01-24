#include "mutator_tester.h"
#include "mutators.h"
#include "global_types.h"
#include "utils.h"

#include <jansson.h>
#include <jansson_helper.h>

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


//The list of test functions
static test_info_t test_info[NUM_TESTS] =
{
	{ test_all,    "Run all tests!" }, //test_all MUST be the first entry in the test_info array
	{ test_mutate, "Test the mutate() function, this will print each iteration of mutation" },
	{ test_state,  "Test the get_state() and set_state() functions." },
};

/** This function sets up the mutator for testing. This test program is designed
* To aid in the debugging of a mutator DLL. The dll is loaded like it would be in the 
* full blown fuzzer and a series of tests are run against it to find common errors.
* @return - if the process fails, it will return 1, if a test fails, the error code 
* returned will be its test number + 100. For example if test 1 fails main will return 101
* 0 is returned on success!
*/
int main(int argc, char *argv[])
{
	//Args
	char *test_type_to_convert = NULL, *mutator_path = NULL, *mutator_options = NULL, *seed_file = NULL; //args
	char *seed_buffer = NULL;
	char *test_string = NULL;
	int ret;
	unsigned long test_num;
	size_t seed_length;
	mutator_t * mutator;
	void * mutator_state;

	if (argc < 3)
	{
		print_usage(argv[0]);
		return 0;
	}

	test_type_to_convert = argv[1];
	mutator_path = argv[2];
	if(argc > 3)
		mutator_options = argv[3];

	//Convert the test type to int
	test_num = strtoul(test_type_to_convert, &test_string, 10);
	if (test_string == test_type_to_convert || test_num >= NUM_TESTS || *test_string != '\0') { //Check for empty str, and overflow
		printf("Invalid test number!");
		return 1;
	}

	if (argc < 5) {
		seed_length = 32;
		seed_buffer = (char *)malloc(seed_length);
		memset(seed_buffer, 0xaa, seed_length);
		printf("Seed file not specified: Using default data\n");
	}
	else
	{
		//Load the seed buffer from a file
		seed_file = argv[4];
		seed_length = read_file(seed_file, &seed_buffer);
		if (seed_length <= 0)
		{
			printf("Could not read seed file or empty seed file: %s\n", seed_file);
			return 1;
		}
	}

	printf("Loaded %zi bytes as seed data\n", seed_length);

	//Load the DLL
	mutator = load_mutator(mutator_path);
	if (mutator == NULL) {
		printf("Load mutator returned a NULL pointer\n");
		return 1;
	}
	//Setup the mutator
	mutator_state = setup_mutator(mutator, mutator_options, seed_buffer, seed_length);
	if (!mutator_state) {
		printf("setup_mutator() failed\n");
		return 1;
	}

	//Everything is setup, now do the tests.
	ret = test_info[test_num].func(mutator, mutator_state, mutator_options, seed_buffer, seed_length);
	if (ret)
		ret = 100 + test_num;
	
	mutator->cleanup(mutator_state);
	return ret;
}

/**
* This function loads the target DLL (the mutator) from disk and sets it up
* for testing. A struct containing function pointers for the mutator is returned.
*
* @param mutator_path - a pointer to a string that is the path to the mutator
* @return mutator - a struct containing function pointers to each mutator function
*/
mutator_t * load_mutator(char * mutator_path) {
	printf("Target Mutator: %s\n", mutator_path);
	
	//Load the library
	wchar_t * dll_filename;
	mutator_t * mutator{};
	HINSTANCE handle;

	void(*init_ptr)(mutator_t *);

	dll_filename = convert_char_array_to_wchar(mutator_path, NULL);
	handle = LoadLibrary(dll_filename);
	free(dll_filename);
	if (!handle)
	{
		printf("Loading target mutator: [FAILED]\n");
		return NULL; //Failed to load DLL
	}
	else
		printf("Loading target mutator: [SUCCESS]\n");
	
	//Init the mutator
	init_ptr = (void(*)(mutator_t *))GetProcAddress(handle, "init");
	if (!init_ptr) { //The library didn't have the init function
		FreeLibrary(handle);
		printf("ERROR: Malformed DLL, Missing init()\n");
		return NULL;
	}
	//Call the mutator's init function to initailize the mutators struct
	mutator = (mutator_t *)malloc(sizeof(mutator_t));
	init_ptr(mutator);
	return mutator;
}

/**
* This function initinalizes the mutator. It calls its create function to setup 
* the mutators state struct, and returns it. This struct is required for all other
* mutator spicific function calls.
* 
* @param mutator - the mutator struct returned by load_mutator which contains the 
* function pointers for the mutator functions specified in the API.
* @param mutator_options - a JSON string that contains the mutator options
* @param seed_buffer - The data buffer used to seed the mutator
* @param seed_length - The length of the seed_buffer in bytes
* @return mutator_state - the state struct for a spicific mutator
*/
void * setup_mutator(mutator_t * mutator, char * mutator_options, char * seed_buffer, size_t seed_length)
{
	void * mutator_state;

	mutator_state = mutator->create(mutator_options, NULL, seed_buffer, seed_length);
	if (!mutator_state)
	{
		printf("Bad mutator options or saved state");
		return NULL;
	}
	return mutator_state;
}

/**
* This function prints the usage statment for the program.
*
* @param argv - The array of command line arguments
* @return none
*/
void print_usage(char *executable_name)
{
	int i;
	printf("\nusage: %s test_type \"/path/to/mutator.dll\" [\"JSON Mutator Options String\" [path/to/input/data]]\n", executable_name);
	printf("Valid Test Types:\n\n");
	for (i = 0; i < NUM_TESTS; i++)
		printf("\t %d - %s\n", i, test_info[i].usage_info);
}

/**
* This function runs all other tests in the test_info struct.
*
* @param mutator - the mutator struct representing the mutator to be tested, returned by load_mutator
* @param mutator_state - the state struct for the mutator being tested.  Currently unused for this test.
* @param mutator_options - a JSON string that contains the mutator options
* @param seed_buffer - The data buffer used to seed the mutator
* @param seed_length - The length of the seed_buffer in bytes
* @return int - the results of the tests. 0 for success and nonzero for fail
*/
int test_all(mutator_t * mutator, void * mutator_state, char * mutator_options, char * seed_buffer, size_t seed_length) {
	int test_num, ret = 0;
	void * single_test_mutator_state;

	for (test_num = 1; test_num < NUM_TESTS && !ret; test_num++)
	{
		single_test_mutator_state = setup_mutator(mutator, mutator_options, seed_buffer, seed_length);
		if (!single_test_mutator_state) {
			printf("setup_mutator() failed\n");
			return 1;
		}
		ret = test_info[test_num].func(mutator, single_test_mutator_state, mutator_options, seed_buffer, seed_length);
		mutator->cleanup(single_test_mutator_state);
	}
	return ret;
}

/**
* This function tests several testcases around the mutators mutate() function.
* This allows the user to see if the data is being mutated in the expected manner.
* It also ensures that each iteration of mutation is being tracked appropriately.
* 
* @param mutator - the mutator struct representing the mutator to be tested, returned by load_mutator
* @param mutator_state - the state struct for the mutator being tested, This state should
* be at the starting state for the mutator (iteration 0)
* @param mutator_options - a JSON string that contains the mutator options
* @param seed_buffer - The data buffer used to seed the mutator
* @param seed_length - The length of the seed_buffer in bytes
* @return int - the results of the tests. 0 for success and 1 for fail
*/
int test_mutate(mutator_t * mutator, void * mutator_state, char * mutator_options, char * seed_buffer, size_t seed_length) {

	int total_iterations, mut_iter, i, limit;
	char * mutate_buffer = (char *)malloc(seed_length);
	int ret;

	printf("+--------+\n");
	printf("| TEST 1 |\n");
	printf("+--------+\n\n");

	total_iterations = mutator->get_total_iteration_count(mutator_state);
	printf("The mutator reported %d required iterations.\n\n", total_iterations);
	printf("=== Original Data ===\n");
	print_hex(seed_buffer, seed_length);
	printf("\n\n\n");

	limit = total_iterations;
	if (total_iterations == -1)
		limit = 64;

	for (i = 0; i <= limit; i++) {
		printf("=== Iteration %d ===\n", i);
		mut_iter = mutator->get_current_iteration(mutator_state);
		if (i != mut_iter) {
			printf("ERROR: The mutator reports that it is on iteration %d but the real iteration is %d\n", mut_iter, i);
			return 1;
		}
		ret = mutator->mutate(mutator_state, mutate_buffer, seed_length);
		printf("mutated buffer, %3d bytes:\n", ret);
		if (ret != -1 && ret != 0) {
			print_hex(mutate_buffer, ret);
		}
		printf("\n\n");

		if (ret == 0 && i == total_iterations) {
			printf("The mutator reported that everything has been mutated on iteration %d of %d\n", i, total_iterations);
			break;
		} else if (ret == 0) {
			printf("ERROR: The mutator reported that everything has been mutated on iteration %d, but previously "
				"reported it had %d total iterations\n", i, total_iterations);
			return 1;
		} else if (ret == -1) {
			printf("ERROR: the mutator reported an error!\n");
			return 1;
		} else if (i == limit && total_iterations != -1) {
			printf("ERROR: The expected number of mutations were performed (%d), but the mutator did not return 0\n", total_iterations);
		}
	}

	if (total_iterations != -1 && ret != 0)
	{
		for (i = 1; i < 100 && ret != 0; i++)
			ret = mutator->mutate(mutator_state, mutate_buffer, seed_length);

		if (ret == 0 && total_iterations != -1)
			printf("ERROR: it took %d extra iterations for the mutator to return 0", i-1);
		else
			printf("ERROR: the mutator did not return 0 even after %d extra iterations", i-1);
		return 1;
	}

	return 0;
}

/**
* This function tests several testcases around the mutators get_state() and set_state functions.
* This allows the user to check if the state of a mutator is being correctly saved and restored.
*
* @param mutator - the mutator struct returned by load_mutator
* @param mutator_state - the state struct for a spicific mutator
* @param mutator_options - a JSON string that contains the mutator options
* @param seed_buffer - The data buffer used to seed the mutator
* @param seed_length - The length of the seed_buffer in bytes
* @return int - the results of the tests. 0 for success and 1 for fail
*/
int test_state(mutator_t * mutator, void * mutator_state, char * mutator_options, char * seed_buffer, size_t seed_length) {
	
	size_t total_iterations, i;
	char * mutate_buffer = (char *)malloc(seed_length);
	char * new_mutate_buffer = (char *)malloc(seed_length);
	char * old_saved_state_buffer;
	char * new_saved_state_buffer;
	json_t * old_JSON_state;
	json_t * new_JSON_state;
	int ret, old_iter, new_iter, old_mutate_length, new_mutate_length;
	void * new_mutator_state;

	printf("+--------+\n");
	printf("| TEST 2 |\n");
	printf("+--------+\n\n");

	if (!mutate_buffer || !new_mutate_buffer)
	{
		printf("Malloc failed\n");
		free(mutate_buffer);
		free(new_mutate_buffer);
		return 1;
	}

	total_iterations = mutator->get_total_iteration_count(mutator_state);
	if (total_iterations == -1) {
		total_iterations = 64;
	}
	printf("Mutating the data %zi times\n", total_iterations / 2 );
	for (i = 0; i <= total_iterations / 2; i++) {
		ret = mutator->mutate(mutator_state, mutate_buffer, seed_length);
		if (ret == 0 || ret == -1)
			printf("ERROR: The mutate() function returned an error or finished pre-maturely. Run test 1 for more info\n");
	}
	printf("Mutation stopped on iteration %zi\n", i);
	printf("Saving the mutators state...\n");
	old_saved_state_buffer = (char *)mutator->get_state(mutator_state);
	printf("Here is the OLD JSON string:\n%s\n", old_saved_state_buffer);

	//Setup a new mutator to restore the state into
	new_mutator_state = setup_mutator(mutator, mutator_options, seed_buffer, seed_length);
	if (!new_mutator_state) {
		printf("setup_mutator() failed\n");
		free(mutate_buffer);
		free(new_mutate_buffer);
		mutator->free_state(old_saved_state_buffer);
		return 1;
	}

	//set the state from the old -> new
	printf("Restoring the mutators state...\n");
	ret = mutator->set_state(new_mutator_state, old_saved_state_buffer);
	if (ret) {
		printf("set_state() returned error code %i\n", ret);
	}
	new_saved_state_buffer = (char *)mutator->get_state(new_mutator_state);
	printf("Here is the NEW JSON string:\n%s\n", new_saved_state_buffer);

	//Compare JSON states to see if they were saved and restored correctly
	old_JSON_state = json_string(old_saved_state_buffer);
	mutator->free_state(old_saved_state_buffer);
	if (old_JSON_state == NULL) {
		printf("Failed to convert old JSON string to JSON object\n");
		free(mutate_buffer);
		free(new_mutate_buffer);
		mutator->free_state(new_saved_state_buffer);
		mutator->cleanup(new_mutator_state);
		return 1;
	}
	new_JSON_state = json_string(new_saved_state_buffer);
	mutator->free_state(new_saved_state_buffer);
	if (new_JSON_state == NULL) {
		printf("Failed to convert new JSON string to JSON object\n");
		free(old_JSON_state);
		free(mutate_buffer);
		free(new_mutate_buffer);
		mutator->cleanup(new_mutator_state);
		return 1;
	}
	if (!json_equal(old_JSON_state, new_JSON_state)) {
		printf("The mutator failed to restore state properly\n");
		printf("old json: %s\n", old_saved_state_buffer);
		printf("new json: %s\n", new_saved_state_buffer);
		free(mutate_buffer);
		free(new_mutate_buffer);
		mutator->cleanup(new_mutator_state);
		return 1;
	}
	free(old_JSON_state);
	free(new_JSON_state);
	printf("The saved states are equal, this is expected\n");

	//Get the iteration count and call mutate once, just to make sure that they work
	old_iter = mutator->get_current_iteration(mutator_state);
	new_iter = mutator->get_current_iteration(new_mutator_state);

	old_mutate_length = mutator->mutate(mutator_state, mutate_buffer, seed_length);
	new_mutate_length = mutator->mutate(new_mutator_state, new_mutate_buffer, seed_length);

	if (old_iter == new_iter
		&& old_mutate_length == new_mutate_length
		&& !memcmp(mutate_buffer, new_mutate_buffer, old_mutate_length)) {
		printf("Success! The mutator has restored its state\n");
		ret = 0;
	} else {
		printf("The mutator failed to mutate properly after restoring the state\n"
			"Original mutator iteration count %d New mutator iteration count %d\n"
			"Original mutator output length %d new mutator output length %d\n", old_iter, new_iter, old_mutate_length, new_mutate_length);
		ret = 1;
	}

	free(mutate_buffer);
	free(new_mutate_buffer);
	mutator->cleanup(new_mutator_state);
	return ret;
}