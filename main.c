#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vma.h"
#include <errno.h>

/*
Functia transforma fiecare comanda intr-un numar pentru
a putea implementa un switch case in utilizarea comenzilor
Fiecare comanda data in cerinta va fi asignata cu numarul sau
corespunzator(ex: ALLOC_ARENA este prima din lista, deci
va avea asignat numarul 1).
*/
void tr(char *command)
{
	int i = 0;
	while (command[i] != '\0') {
		if (command[i] == '\n') {
			command[i] = '\0';
			return;
		}
		i++;
	}
}

int transform_int_command(char *command)
{
	tr(command);
	char *token = strtok(command, " ");
	if (!strcmp(token, command))
		if (!strcmp(command, "PMAP"))
			return 7;
	if (!strcmp(token, "ALLOC_ARENA"))
		return 1;
	if (!strcmp(token, "DEALLOC_ARENA"))
		return 2;
	if (!strcmp(token, "ALLOC_BLOCK"))
		return 3;
	if (!strcmp(token, "FREE_BLOCK"))
		return 4;
	if (!strcmp(token, "READ"))
		return 5;
	if (!strcmp(token, "WRITE"))
		return 6;
	return 0;
}

int exist_block(arena_t *arena, uint64_t *v)
{
	block_t *blc = arena->alloc_list->head;
	if (!arena->alloc_list->size)
		return 1;
	if (!blc)
		return 1;

	if (v[0] + v[1] <= blc->start_address)
		return 1;

	while (blc->next)
		blc = blc->next;
	if (blc->start_address + blc->size <= v[0])
		return 1;
	blc = arena->alloc_list->head;
	while (blc->start_address + blc->size <= v[0] && blc->next)
		blc = blc->next;
	if (blc->prev)
		blc = blc->prev;
	if (blc->start_address + blc->size <= v[0] &&
		v[0] + v[1] <= blc->next->start_address)
		return 1;
	return 0;
}

int verify_block(uint64_t *v, arena_t *arena)
{
	int ok1 = 1;
	if (v[0]  >= arena->arena_size) {
		printf("The allocated address is outside the size of arena\n");
		ok1 = 0;
		return ok1;
	}
	if (v[0] + v[1] > arena->arena_size && v[0] < arena->arena_size) {
		printf("The end address is past the size of the arena\n");
		ok1 = 0;
		return ok1;
	}
	if (!arena->alloc_list->head || !arena->alloc_list->size)
		return ok1;
	block_t *c = arena->alloc_list->head;
	while (c->next) {
		if (c->start_address <= v[0] && v[0] < c->start_address + c->size) {
			printf("This zone was already allocated.\n");
			ok1 = 0;
			return ok1;
		}
		if (c->start_address >= v[0] && c->start_address + c->size <= v[1]) {
			printf("This zone was already allocated.\n");
			ok1 = 0;
			return ok1;
		}
		if (v[0] < c->start_address && v[0] + v[1] > c->start_address) {
			printf("This zone was already allocated.\n");
			ok1 = 0;
			return ok1;
		}
		c = c->next;
	}
	if (c->start_address <= v[0]  && v[0] < c->start_address + c->size) {
		printf("This zone was already allocated.\n");
		ok1 = 0;
		return ok1;
	}
	if (c->start_address >= v[0] && c->start_address + c->size <= v[1]) {
		printf("This zone was already allocated.\n");
		ok1 = 0;
		return ok1;
	}
	if (v[0] < c->start_address && v[0] + v[1] > c->start_address) {
		printf("This zone was already allocated.\n");
		ok1 = 0;
		return ok1;
	}
	return ok1;
}

int verify_block_exist(arena_t *arena, uint64_t address)
{
	if (!arena->alloc_list->size)
		return 0;
	block_t *cur = arena->alloc_list->head;
	miniblock_t *m = cur->miniblock_list;
	uint64_t v;
	while (cur->next) {
		if (cur->start_address == address)
			return 1;
		v = cur->start_address + cur->size;
		if (cur->start_address <= address && v > address) {
			while (m->next) {
				if (m->start_address ==  address)
					return 1;
				m = m->next;
			}
			if (m->start_address ==  address)
				return 1;
		}
		cur = cur->next;
	}
	if (cur->start_address == address)
		return 1;
	v = cur->start_address + cur->size;
	if (cur->start_address <= address && v > address) {
		while (m->next) {
			if (m->start_address ==  address)
				return 1;
			m = m->next;
		}
		if (m->start_address ==  address)
			return 1;
	}
	return 0;
}

int8_t *get_write(char *command, uint64_t *values)
{
	int8_t *data;
	int k = 0, l;
	for (int i = 6; command[i] != ' '; i++)
		k++;
	l = 7 + k;
	for (int j = l; command[j] != ' '; j++)
		k++;
	k += 8;
	data = malloc(values[1] * sizeof(uint8_t));
	DIE(!data, "Cannot alloc memory for data");

	uint64_t size = values[1], i = 0;
	while (size) {
		if (command[k] == '\n') {
			command[k] = '\n';
			fgets(command, BUFSIZ, stdin);
		}
		data[i++] = command[k++];
		size--;
	}
	return data;
}

void get_read(char *command, uint64_t *valu)
{
	int k = 0, l;
	char s[100];
	for (int i = 5; command[i] != ' '; i++)
		s[k++] = command[i];
	s[k] = '\0';
	valu[0] = atoi(s);
	l = 6 + k;
	k = 0;
	for (int j = l; command[j] != ' '; j++)
		s[k++] = command[j];
	s[k] = '\0';
	valu[1] = atoi(s);
}

int main(void)
{
	int ok, ok1, okf;
	arena_t *arena;
	char command[BUFSIZ], copy[255], trash[25];
	uint64_t value, v[2], values[2], valu[2];
	int8_t *data;
	int com;
	do {
		fgets(command, BUFSIZ, stdin);
		strcpy(copy, command);
		com = transform_int_command(copy);
		switch (com) {
		case 1:
			sscanf(command, "%s%lu", trash, &value);
			arena = alloc_arena(value);
			create_arena_list(arena);
		break;
		case 2:
			dealloc_arena(arena);
		break;
		case 3:
			sscanf(command, "%s%lu%lu", trash, &v[0], &v[1]);
			ok1 = verify_block(v, arena);
			if (!ok1)
				break;
			ok = exist_block(arena, v);
			if (ok)
				alloc_block(arena, v[0], v[1]);
		break;
		case 4:
			sscanf(command, "%s%lu", trash, &value);
			okf = verify_block_exist(arena, value);
			if (okf == 0) {
				printf("Invalid address for free.\n");
				break;
			}
			free_block(arena, value);
		break;
		case 5:
			sscanf(command, "%s%lu%lu", trash, &valu[0], &valu[1]);
			read(arena, valu[0], valu[1]);
		break;
		case 6:
			sscanf(command, "%s%lu%lu", trash, &values[0], &values[1]);
			data = get_write(command, values);
			write(arena, values[0], values[1], data);
			free(data);
		break;
		case 7:
			pmap(arena);
		break;
		case 8:

		break;
		default:
			strcpy(copy, command);
			for (unsigned long i = 0; copy[i] != '\0'; i++) {
				if (copy[i] == ' ')
					printf("Invalid command. Please try again.\n");
			}
			printf("Invalid command. Please try again.\n");
		break;
		}
	} while (com != 2);
	return 0;
}
