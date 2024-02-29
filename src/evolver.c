#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "types.h"
#include "getter.h"
#include "instset.h"
#include "memory.h"
#include "evolver.h"

static boolean g_is_init;
static uint32 g_last_changed_address;
static uint32 g_calls_on_last_cycle;
static uint32 g_state[4];

void _sal_evo_init(void)
{
	/* Start up the evolver module. We simply set the 128 bits into a random
	state by calling 'rand()'.
	*/
	assert(!g_is_init);
	srand((uint32)time(NULL));
	g_state[0] = rand();
	g_state[1] = rand();
	g_state[2] = rand();
	g_state[3] = rand();
	g_is_init = TRUE;
}

void _sal_evo_quit(void)
{
	/* Quit the evolver module. Reset everything back to zero.
	*/
	assert(g_is_init);
	g_is_init = FALSE;
	g_last_changed_address = 0;
	g_calls_on_last_cycle = 0;
	memset(g_state, 0, sizeof(uint32) * 4);
}

void _sal_evo_load_from(FILE *file)
{
	/* Load evolver state from a binary file.
	*/
	assert(!g_is_init);
	assert(file);
	fread(&g_is_init, sizeof(boolean), 1, file);
	fread(&g_last_changed_address, sizeof(uint32), 1, file);
	fread(&g_calls_on_last_cycle, sizeof(uint32), 1, file);
	fread(&g_state, sizeof(uint32), 4, file);
}

void _sal_evo_save_into(FILE *file)
{
	/* Save evolver state into a binary file.
	*/
	assert(g_is_init);
	assert(file);
	fwrite(&g_is_init, sizeof(boolean), 1, file);
	fwrite(&g_last_changed_address, sizeof(uint32), 1, file);
	fwrite(&g_calls_on_last_cycle, sizeof(uint32), 1, file);
	fwrite(&g_state, sizeof(uint32), 4, file);
}

/* Getter methods for the evolver module.
*/
UINT32_GETTER(evo, last_changed_address)
UINT32_GETTER(evo, calls_on_last_cycle)

uint32 sal_evo_get_state(uint8 state_index)
{
	/* Get part of the evolver's internal state (32 bits of 128 total bits) as
	an unsigned int.
	*/
	assert(g_is_init);
	assert(state_index < 4);
	return g_state[state_index];
}

static uint32 generate_random_number(void)
{
	/* Generate a single 32 bit random number. This module makes use of the
	XOR-Shift pseudo-rng. We use XOR-Shift because it's extremely lightweight
	and fast, while providing quite good results. Find more info about it here:
	>>> https://en.wikipedia.org/wiki/Xorshift
	*/
	uint32 tmp1;
	uint32 tmp2;
	assert(g_is_init);
	tmp2 = g_state[3];
	tmp2 ^= tmp2 << 11;
	tmp2 ^= tmp2 >> 8;
	g_state[3] = g_state[2];
	g_state[2] = g_state[1];
	g_state[1] = tmp1 = g_state[0];
	tmp2 ^= tmp1;
	tmp2 ^= tmp1 >> 19;
	g_state[0] = tmp2;
	g_calls_on_last_cycle++;
	return tmp2;
}

void _sal_evo_randomize_at(uint32 address)
{
	/* Place a random instruction into a given address.
	*/
	uint8 inst;
	assert(g_is_init);
	assert(sal_mem_is_address_valid(address));
	inst = generate_random_number() % INST_COUNT;
	g_last_changed_address = address;
	sal_mem_set_inst(address, inst);
}

void _sal_evo_cycle(void)
{
	/* During each simulation cycle, a random 32 bit integer is generated. If
	this integer represents a 'valid' address in memory
	(i.e. new_rand < memory_size), this address becomes hit by a cosmic ray
	(randomized). This simple mutation scheme is enough to drive evolution in
	Salis.
	*/
	uint32 address;
	assert(g_is_init);
	g_calls_on_last_cycle = 0;
	address = generate_random_number();

	if (sal_mem_is_address_valid(address)) {
		_sal_evo_randomize_at(address);
	}
}
