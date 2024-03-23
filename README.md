# Description

The program simulates a Virtual Memory Allocator. Each line is read from stdin and evaluated, meaning that each valid command corresponds to a number. (The numbers correspond to the order of the commands given in the requirement, for example, ALLOC_ARENA has the number 1, etc.). In the case of an invalid command, the number of spaces in that line is counted, and nr_spaces + 1 messages of "Invalid command. Please try again.\n" will be displayed.

For most functions, in order to process numerical data such as addresses, sizes, etc., sscanf is used on the command (the variable that saves the read line).

The allocation of the arena is done conceptually, meaning that only the blocks are checked to be within the interval 0 -- arena->arena_size. Subsequently, blocks are allocated (each block initially having one mini-block), and they are merged when they are adjacent (meaning a larger block with multiple mini-blocks is created). The current state of the arena can be visualized using the PMAP command, which shows the occupied memory areas and information about the occupied memory. Finally, in the DEALLOC_ARENA function, all allocated resources are released, and the program is closed.

For each function, when allocating, deallocating, writing, or reading, generally the same cases are checked: the arena has not been allocated, the arena is allocated but has no elements in it, there is a block with one mini-block or with multiple mini-blocks, there are multiple blocks, etc. The functions try to handle all possible cases.

For the function allocating blocks, we check beforehand if the block to be allocated could be attached to another block or two. If this operation can be performed, only an extension of the initial block is made, and the new block is inserted as a mini-block into the existing one.

In the case of deallocating an address, we check if it is in the middle of another block, in which case there is no contiguous memory area, and it is split into two linked blocks. (Note: a more efficient implementation would have been to reduce the initial block to where the address to be eliminated started, and only allocate and link the next block - with links to the lists of mini-blocks made carefully).

Certain variables, such as x, k, g, etc., are auxiliary variables that help with either easier code readability (attempting to avoid lines longer than 80 characters) or for cases where we need to store the next mini-blocks or blocks.

In the pmap function, the check k > arena->alloc_list->mbs is meant to avoid entering an infinite loop in case something went wrong with the allocation/deallocation of blocks.
