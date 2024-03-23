#include "vma.h"

arena_t *alloc_arena(const uint64_t size)
{
	arena_t *arena;
	arena = malloc(sizeof(arena_t));
	DIE(!arena, "Cannot alloc memory for the arena\n");
	arena->arena_size = size;
	arena->alloc_list = NULL;
	return arena;
}

void create_arena_list(arena_t *arena)
{
	arena->alloc_list = malloc(sizeof(list_t));
	DIE(!arena->alloc_list, "Cannot alloc list!");
	arena->alloc_list->size = 0;
	arena->alloc_list->mbs = 0;
	arena->alloc_list->block_size = 0;
	arena->alloc_list->head = NULL;
}

void dealloc_arena(arena_t *arena)
{
	if (!arena->alloc_list->size) {
		free(arena->alloc_list);
		free(arena);
		return;
	}
	block_t *cur = arena->alloc_list->head, *curn;
	miniblock_t *m = cur->miniblock_list, *mn;
	for (size_t i = 0; i < arena->alloc_list->size - 1; i++) {
		curn = cur->next;
		m = cur->miniblock_list;
		while (m->next) {
			mn = m->next;
			if (m->rw_buffer)
				free(m->rw_buffer);
			free(m);
			m = mn;
		}
		if (m->rw_buffer)
			free(m->rw_buffer);
		free(m);
		free(cur);
		cur = curn;
	}
	m = cur->miniblock_list;
	while (m->next) {
		mn = m->next;
		if (m->rw_buffer)
			free(m->rw_buffer);
		free(m);
		m = mn;
	}
	if (m->rw_buffer)
		free(m->rw_buffer);
	free(m);
	free(cur);
	free(arena->alloc_list);
	free(arena);
}

/*
1-inceput de lista la stanga
2-intre blocuri
3-la dreapta
4-stanga
5-final
*/

int verify_merge(arena_t *arena, uint64_t *v, size_t size)
{
	if (size == 0)
		return 0;
	block_t *cur = arena->alloc_list->head;
	if (!cur->next) {
		if (v[0] + v[1] == cur->start_address)
			return 1;
		if (cur->start_address + cur->size == v[0])
			return 3;
	}
	if (v[0] + v[1] == cur->start_address)
		return 1;//se lipesc doua blocuri la inceputul listei
	while (cur->start_address + cur->size != v[0] && cur->next)
		cur = cur->next;
	if (cur->start_address + cur->size == v[0])
		if (cur->next->start_address == v[0] + v[1])
			return 2;
	if (cur->start_address == v[0] + v[1])
		return 4; // se lipeste un bloc la stanga altuia
	while (cur->next)
		cur = cur->next;
	if (cur->size + cur->start_address == v[0])
		return 3; //se lipste la finalul listei un alt bloc;
	if (cur->start_address == v[0] + v[1])
		return 4;

	if (cur->prev)
		cur = cur->prev;
	if (cur->start_address + cur->size == v[0]) {
		if (cur->next->start_address == v[0] + v[1])
			return 2;//se baga un bloc intre alte doua blocuri
	} else {
		return 3;//se lipeste un bloc la dreapta altuia
	}
	return 0;
}

void merge1(arena_t *arena, const uint64_t address, const uint64_t size)
{
	block_t *curr = arena->alloc_list->head;
	miniblock_t *mb = curr->miniblock_list;

	mb = malloc(sizeof(miniblock_t));
	DIE(!mb, "Cannot alloc memory for the miniblock");
	curr->start_address = address;
	curr->size += size;
	mb->start_address = address;
	mb->size = size;
	mb->perm = 6;
	mb->rw_buffer = NULL;
	mb->next = curr->miniblock_list;
	curr->miniblock_list->prev = mb;
	mb->prev = NULL;
	curr->miniblock_list = mb;
	arena->alloc_list->block_size += size;
}

void merge2(arena_t *arena, const uint64_t address, const uint64_t size)
{
	block_t *curr = arena->alloc_list->head;
	block_t *cur2;
	miniblock_t *mb = curr->miniblock_list;

	mb = malloc(sizeof(miniblock_t));
	DIE(!mb, "Cannot alloc memory for the miniblock");
	mb->start_address = address;
	mb->size = size;
	mb->perm = 6;
	mb->rw_buffer = NULL;
	while (curr->size + curr->start_address != address)
		curr = curr->next;
	cur2 = curr->next;
	mb->next = cur2->miniblock_list;
	mb->prev = curr->miniblock_list;
	curr->miniblock_list->next = mb;
	cur2->miniblock_list->prev = mb;
	curr->size += size + cur2->size;
	if (cur2->next) {
		curr->next = cur2->next;
		cur2->next->prev = curr;
	} else {
		curr->next = NULL;
		curr->prev = NULL;
	}
	free(cur2);
	arena->alloc_list->size--;
	arena->alloc_list->block_size += size;
}

void merge3(arena_t *arena, const uint64_t address, const uint64_t size)
{
	block_t *curr = arena->alloc_list->head;
	miniblock_t *mb = curr->miniblock_list, *au;

	mb = malloc(sizeof(miniblock_t));
	DIE(!mb, "Cannot alloc memory for the miniblock!");
	while (curr->start_address + curr->size != address)
		curr = curr->next;
	curr->size += size;
	mb->size = size;
	mb->start_address = address;
	mb->perm = 6;
	mb->rw_buffer = NULL;
	au = curr->miniblock_list;
	while (au->start_address + au->size != mb->start_address)
		au = au->next;
	mb->prev = au;
	mb->next = NULL;
	au->next = mb;
	arena->alloc_list->block_size += size;
}

void merge4(arena_t *arena, const uint64_t address, const uint64_t size)
{
	block_t *curr = arena->alloc_list->head;
	miniblock_t *mb = curr->miniblock_list;

	mb = malloc(sizeof(miniblock_t));
	DIE(!mb, "Cannot alloc memory for miniblock!");
	while (curr->start_address != address + size)
		curr = curr->next;
	curr->size += size;
	curr->start_address = address;
	mb->size = size;
	mb->start_address = address;
	mb->perm = 6;
	mb->rw_buffer = NULL;
	mb->next = curr->miniblock_list;
	mb->prev = NULL;
	curr->miniblock_list->prev = mb;
	curr->miniblock_list = mb;
	arena->alloc_list->block_size += size;
}

void merge(arena_t *arena, const uint64_t address, const uint64_t size)
{
	uint64_t v[2];
	v[0] = address;
	v[1] = size;
	int ok = verify_merge(arena, v, arena->alloc_list->size);
	switch (ok) {
	case 1:
		merge1(arena, address, size);
	break;
	case 2:
		merge2(arena, address, size);
	break;
	case 3:
		merge3(arena, address, size);
	break;
	case 4:
		merge4(arena, address, size);
	break;
	}
}

int should_merge(arena_t *arena, const uint64_t address, const uint64_t size)
{
	block_t *cur = arena->alloc_list->head;
	if (!cur->next && arena->alloc_list->head) {
		if (address + size == cur->start_address)
			return 1;
		if (address == cur->start_address + cur->size)
			return 1;
		return 0;
	}
	if (cur->next) {
		if (address + size == cur->start_address)
			return 1;
		if (address == cur->start_address + cur->size)
			return 1;
	}

	cur = cur->next;
	while (cur->next) {
		if (address + size == cur->start_address)
			return 1;
		if (address == cur->start_address + cur->size)
			return 1;
		cur = cur->next;
	}
	if (address + size == cur->start_address)
		return 1;
	if (address == cur->start_address + cur->size)
		return 1;
	return 0;
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	if (arena->alloc_list->size) {
		int ok = should_merge(arena, address, size);
		if (ok) {
			arena->alloc_list->mbs++;
			merge(arena, address, size);
			return;
		}
	}

	block_t *block = malloc(sizeof(block_t));
	DIE(!block, "Cannot alloc memory for block list!");
	block->start_address = address;
	block->size = size;

	block->miniblock_list = malloc(sizeof(miniblock_t));
	DIE(!block->miniblock_list, "Cannot alloc memory for miniblock list!");

	block->miniblock_list->start_address = address;
	block->miniblock_list->size = size;
	block->miniblock_list->perm = 6;
	block->miniblock_list->rw_buffer = NULL;
	block->miniblock_list->next = NULL;
	block->miniblock_list->prev = NULL;
	arena->alloc_list->mbs++;

/*
Daca lista e goala, si trebuie adaugat un block
*/
	if (arena->alloc_list->size == 0) {
		arena->alloc_list->head = block;
		block->next = NULL;
		block->prev = NULL;
		arena->alloc_list->size++;
		arena->alloc_list->block_size += size;
		return;
	}
	arena->alloc_list->size++;
	arena->alloc_list->block_size += size;
/*
Cand se adauga un element la inceputul listei
*/
	block_t *cur = arena->alloc_list->head;
	if (cur->start_address > address + size) {
		arena->alloc_list->head = block;
		block->next = cur;
		block->prev = NULL;
		cur->prev = block;
		return;
	}

/*
Adauga un bloc intre alte doua blocuri deja existente
*/
	cur = arena->alloc_list->head;
	while (cur->size + cur->start_address < address && cur->next)
		cur = cur->next;
	if (cur->next && address < cur->start_address)
		cur = cur->prev;
	if (cur->next) {
		block->next = cur->next;
		block->prev = cur;
		cur->next->prev = block;
		cur->next = block;
		return;
	}
/*
Se adauga un element la final;
*/
	cur = arena->alloc_list->head;
	while (cur->next)
		cur = cur->next;
	if (!cur->next) {
		block->prev = cur;
		block->next = NULL;
		cur->next = block;
		return;
	}
}

void split_block(arena_t *arena, const uint64_t address, miniblock_t *m)
{
	block_t *cur = arena->alloc_list->head, *b1, *b2;
	miniblock_t *k, *g;
	b1 = malloc(sizeof(block_t));
	DIE(!b1, "Cannot alloc memory for the block");
	b2 = malloc(sizeof(block_t));
	DIE(!b2, "Cannot alloc memory for the block");
	arena->alloc_list->head = b1;
	b1->start_address = cur->start_address;
	b1->miniblock_list = cur->miniblock_list;
	b1->size = m->start_address - cur->start_address;
	k = b1->miniblock_list;
	while (k->start_address + k->size < address && k->next)
		k = k->next;
	k->next = NULL;
	b2->start_address = m->start_address + m->size;
	b2->size = cur->size - b1->size - m->size;
	b2->miniblock_list = m->next;
	g = b2->miniblock_list;
	g->prev = NULL;
	b1->next = b2;
	b1->prev = NULL;
	b2->next = NULL;
	b2->prev = b1;
	arena->alloc_list->size++;
	arena->alloc_list->mbs--;
	arena->alloc_list->block_size -= m->size;
	if (m->rw_buffer)
		free(m->rw_buffer);
	free(m);
	free(cur->miniblock_list);
	free(cur);
}

void free1m(arena_t *arena, miniblock_t *m)
{
	block_t *cur = arena->alloc_list->head, *b1, *b2;
	if (cur == arena->alloc_list->head) {
		cur->start_address = m->start_address;
		arena->alloc_list->head = cur->next;
		arena->alloc_list->size--;
		arena->alloc_list->mbs--;
		arena->alloc_list->block_size -= m->size;
		if (m->rw_buffer)
			free(m->rw_buffer);
		free(m);
		cur->next->prev  = NULL;
		free(cur);
		return;
	}
	if (cur->next && cur->prev) {
		arena->alloc_list->size--;
		arena->alloc_list->mbs--;
		arena->alloc_list->block_size -= m->size;
		if (m->rw_buffer)
			free(m->rw_buffer);
		free(m);
		b1 = cur->prev;
		b2 = cur->next;
		b1->next = b2;
		b2->prev = b1;
		free(cur->miniblock_list);
		free(cur);
		return;
	}
	if (!cur->next) {
		arena->alloc_list->size--;
		arena->alloc_list->mbs--;
		arena->alloc_list->block_size -= m->size;
		if (m->rw_buffer)
			free(m->rw_buffer);
		free(m);
		cur->prev->next = NULL;
		free(cur->miniblock_list);
		free(cur);
		return;
	}
}

void free2(arena_t *arena)
{
	block_t *cur = arena->alloc_list->head;
	miniblock_t *m, *au, *par;
	m = cur->miniblock_list;
	while (m->next) {
		if (cur->start_address == m->start_address) {
			cur->start_address = m->next->start_address;
			au = cur->miniblock_list;
			au->next = m->next;
			m->next->prev = au;
			arena->alloc_list->mbs--;
			arena->alloc_list->block_size -= m->size;
			if (m->rw_buffer)
				free(m->rw_buffer);
			free(m);
			return;
		}
		if (m->next && m->prev) {
			arena->alloc_list->mbs--;
			arena->alloc_list->block_size -= m->size;
			au = m->next;
			par = m->prev;
			au->next = par;
			par->prev = au;
			if (m->rw_buffer)
				free(m->rw_buffer);
			free(m);
			return;
		}
		if (!m->next) {
			m->prev->next = NULL;
			arena->alloc_list->mbs--;
			arena->alloc_list->block_size -= m->size;
			if (m->rw_buffer)
				free(m->rw_buffer);
			free(m);
			return;
		}
		m = m->next;
	}
	if (!m->next) {
		m->prev->next = NULL;
		arena->alloc_list->mbs--;
		arena->alloc_list->block_size -= m->size;
		if (m->rw_buffer)
			free(m->rw_buffer);
		free(m);
		return;
	}
}

void free_block(arena_t *arena, const uint64_t address)
{
	block_t *cur = arena->alloc_list->head;
	miniblock_t *m;
	if (arena->alloc_list->size == 1) {
		m = cur->miniblock_list;
		if (!m->next) {
			arena->alloc_list->size--;
			arena->alloc_list->mbs--;
			arena->alloc_list->block_size -= m->size;
			if (m->rw_buffer)
				free(m->rw_buffer);
			free(m);
			free(cur);
			arena->alloc_list->head = NULL;
			return;
		}
		for (size_t i = 0; i < arena->alloc_list->mbs; i++) {
			if (m->start_address == address) {
				if (m->start_address == cur->miniblock_list->start_address) {
					cur->miniblock_list = m->next;
					cur->start_address = m->next->start_address;
					arena->alloc_list->mbs--;
					arena->alloc_list->block_size -= m->size;
					if (m->rw_buffer)
						free(m->rw_buffer);
					free(m);
					return;
				}
				if (m->prev && m->next) {
					split_block(arena, address, m);
					return;
				}
				if (!m->next) {
					m->prev->next = NULL;
					arena->alloc_list->mbs--;
					arena->alloc_list->block_size -= m->size;
					if (m->rw_buffer)
						free(m->rw_buffer);
					free(m);
					return;
				}
			}
			m = m->next;
		}
	}
	cur = arena->alloc_list->head;
	for (size_t i = 0; i < arena->alloc_list->size; i++) {
		m = cur->miniblock_list;
		uint64_t x = cur->start_address + cur->size;
		if (x == m->size + m->start_address) {
			free1m(arena, m);
			return;
		}  //mai multe miniblock uri
		free2(arena);
		cur = cur->next;
	}
}

void read(arena_t *arena, uint64_t address, uint64_t size)
{
	if (!arena->alloc_list->head) {
		printf("Invalid address for read.\n");
		return;
	}
	if (!arena->alloc_list->size) {
		printf("Invalid address for read.\n");
		return;
	}
	int8_t *p;
	block_t *cur = arena->alloc_list->head;
	while (cur->next && cur->start_address < address)
		cur = cur->next;
	miniblock_t *m;
	if (cur->miniblock_list) {
		m = cur->miniblock_list;
	} else {
		printf("Invalid address for read.\n");
		return;
	}
	if (m->start_address <= address) {
		if (size >= m->size) {
			if (size > m->size) {
				printf("Warning: size was bigger than the block size. ");
				printf("Reading %lu characters.\n", m->size);
			}
			p = (int8_t *)m->rw_buffer;
			for (size_t i = address - m->start_address; i < m->size; i++)
				printf("%c", (int8_t)p[i]);
			printf("\n");
			return;
		}
		p = (int8_t *)m->rw_buffer;
		if (address == m->start_address) {
			for (size_t i = address - m->start_address; i < size; i++)
				printf("%c", (int8_t)p[i]);
			printf("\n");
			return;
		}
		for (size_t i = address - m->start_address; i <= size; i++)
			printf("%c", (int8_t)p[i]);
		printf("\n");
		return;
	}
}

void write_one(arena_t *arena, const uint64_t address,
			   const uint64_t size, int8_t *data)
{
	block_t *cur = arena->alloc_list->head;
	miniblock_t *m = cur->miniblock_list;
	if (m->start_address == address && !m->next) {
		if (size > m->size) {
			printf("Warning: size was bigger than the block size. ");
			printf("Writing %lu characters.\n", m->size);
			m->rw_buffer = malloc(size);
			DIE(!m->rw_buffer, "Cannot alloc memory");
			memcpy(m->rw_buffer, data, m->size);
			return;
		}
		if (size <= m->size) {
			m->rw_buffer = malloc(size + 1);
			DIE(!m->rw_buffer, "Cannot alloc memory");
			memcpy(m->rw_buffer, data, size);
			return;
		}
	} else {
		if (size > cur->size) {
			printf("Warning: size was bigger than the block size. Writing ");
			printf("%lu characters.\n", cur->size);
			int8_t i = 0;
			while (m->next) {
				m->rw_buffer = malloc(size);
				DIE(!m->rw_buffer, "Cannot alloc memory");
				memcpy(m->rw_buffer, data + i, m->size);
				i++;
				m = m->next;
			}
			m->rw_buffer = malloc(size);
			DIE(!m->rw_buffer, "Cannot alloc memory");
			memcpy(m->rw_buffer, data + i, m->size);
			return;
		}
	}
}

void write(arena_t *arena, const uint64_t address,
		   const uint64_t size, int8_t *data)
{
	if (!arena->alloc_list->head) {
		printf("Invalid address for write.\n");
		return;
	}
	if (!arena->alloc_list->size) {
		printf("Invalid address for write.\n");
		return;
	}
	void *p;
	uint64_t cont = 0;
	block_t *cur = arena->alloc_list->head;
	miniblock_t *m = cur->miniblock_list;
	while (cur->next && cur->start_address < address)
		cur = cur->next;
	if (!cur->next && cur->start_address + cur->size < address) {
		printf("Invalid address for write.\n");
		return;
	}
	if (!cur->prev && cur->start_address > address) {
		printf("Invalid address for write.\n");
		return;
	}
	if (cur->prev && cur->prev->start_address + cur->prev->size < address &&
		cur->start_address > address) {
		printf("Invalid address for write.\n");
		return;
	}
	if (arena->alloc_list->size == 1) {
		write_one(arena, address, size, data);
		return;
	}
	m = cur->miniblock_list;
	if (address + size - cur->start_address > cur->size) {
		int k = address - cur->start_address;
		printf("Warning: size was bigger than the block size. ");
		printf("Writing %lu characters.\n", cur->size - k);
		if (address == m->start_address) {
			cont += m->size;
			p = data;
			m->rw_buffer = malloc(cur->size + 1);
			DIE(!m->rw_buffer, "Cannot alloc memory");
			memcpy(m->rw_buffer, data, cur->size);
			m = m->next;
			p += *data + m->size;
			while (cont < size && m->next) {
				if (size - cont < m->size) {
					m->rw_buffer = malloc(m->size + 1);
					DIE(!m->rw_buffer, "Cannot alloc memory");
					memcpy(m->rw_buffer, data, size - cont);
					cont = size;
					return;
				}
				m->rw_buffer = malloc(m->size);
				DIE(!m->rw_buffer, "Cannot alloc memory");
				memcpy(m->rw_buffer, p, m->size);
				p += *data + m->size;
				m = m->next;
				cont += m->size;
			}
			return;
		}
	}

	if (size <= m->size && address != m->start_address) {
		m->write_address = address;
		m->rw_buffer = malloc(m->size + 1);
		DIE(!m->rw_buffer, "Cannot alloc memory");
		p = m->rw_buffer + (address - m->start_address);
		memcpy(m->rw_buffer, p, size + 1);
		return;
	}
}

void pmap(const arena_t *arena)
{
	if (!arena->alloc_list->size) {
		printf("Total memory: 0x%lX bytes\n", arena->arena_size);
		printf("Free memory: 0x%lX bytes\n",
			   arena->arena_size - arena->alloc_list->block_size);
		printf("Number of allocated blocks: %lu\n", arena->alloc_list->size);
		printf("Number of allocated miniblocks: %lu\n", arena->alloc_list->mbs);
		return;
	}
	printf("Total memory: 0x%lX bytes\n", arena->arena_size);
	printf("Free memory: 0x%lX bytes\n",
		   arena->arena_size - arena->alloc_list->block_size);
	printf("Number of allocated blocks: %lu\n", arena->alloc_list->size);
	printf("Number of allocated miniblocks: %lu\n", arena->alloc_list->mbs);
	block_t *cur = arena->alloc_list->head;
	miniblock_t *m = cur->miniblock_list;
	for (unsigned long i = 0; i < arena->alloc_list->size; i++) {
		printf("\nBlock %ld begin\n", i + 1);
		printf("Zone: 0x%lX - 0x%lX\n",
			   cur->start_address, cur->start_address + cur->size);
		m = cur->miniblock_list;
		int k = 0;
		if (!m->next) {
			printf("Miniblock %d:\t\t0x%lX\t\t-\t\t0x%lX\t\t| RW-\n",
				   k + 1, m->start_address, m->start_address + m->size);
			printf("Block %ld end\n", i + 1);
			cur = cur->next;
		} else {
			while (m->next) {
				printf("Miniblock %d:\t\t0x%lX\t\t-\t\t0x%lX\t\t| RW-\n",
					   k + 1, m->start_address, m->start_address + m->size);
				k++;
				m = m->next;
				if ((size_t)k > arena->alloc_list->mbs)
					break;
			}
			printf("Miniblock %d:\t\t0x%lX\t\t-\t\t0x%lX\t\t| RW-\n",
				   k + 1, m->start_address, m->start_address + m->size);
			printf("Block %ld end\n", i + 1);
			cur = cur->next;
		}
	}
}
