#pragma once
#include <inttypes.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(errno);				        \
		}							\
	} while (0)

typedef struct block_t block_t;
typedef struct miniblock_t miniblock_t;

struct miniblock_t {
	uint64_t start_address, write_address;
	size_t size;
	int8_t perm;
	void *rw_buffer;
	miniblock_t *next, *prev;
};

struct block_t {
	uint64_t start_address;
	size_t size;
	miniblock_t *miniblock_list; //practic head ul lui miniblock_t
	block_t *next, *prev;
};

typedef struct {
	uint64_t block_size;
	size_t size, mbs;
	block_t *head;
} list_t;

typedef struct {
	uint64_t arena_size;
	list_t *alloc_list;
} arena_t;

arena_t *alloc_arena(const uint64_t size);
void dealloc_arena(arena_t *arena);
void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size);
void free_block(arena_t *arena, const uint64_t address);
void read(arena_t *arena, uint64_t address, uint64_t size);
void write(arena_t *arena, const uint64_t address,
		   const uint64_t size, int8_t *data);
void pmap(const arena_t *arena);
void create_arena_list(arena_t *arena);
int verify_merge(arena_t *arena, uint64_t *v, size_t size);
void merge(arena_t *arena, const uint64_t address, const uint64_t size);
int should_merge(arena_t *arena, const uint64_t address, const uint64_t size);
