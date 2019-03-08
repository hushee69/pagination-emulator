/*
 * Teyssierda@gmail.com
 * Authors: Harry Jandu
 *			Demba Cissé
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "queue.h"

#define MEM_FILE						"memory.dat"
#define PHYSICAL_MEMORY_SIZE			64
#define VIRTUAL_MEMORY_SIZE				64			// unused for now
#define PHYSICAL_MAX_PAGES				16			// to get random numbers between 0 and 16
#define PAGE_SIZE						4

/*
 * is_occupied tells if or not the frame
 * in physical memory is occupied or not
 * is_occupied will also give the process
 * number of the current process occupying
 * a page in the physical memory, therefore
 * if is_occupied is zero, there's no process
 * => no process can have process number 0!!
 * is_read - if the page has been read
 * is_modified - if the page has been modified
*/
typedef struct PAGE
{
	int number;
	int is_occupied;
	short is_read;
	short is_modified;
}Page;

/*
 * page table will have 3 important fields
 * page number - the page number in the virtual memory
 * frame number - the page number in the physical memory
 * offset - decalage
*/
typedef struct PAGE_TABLE
{
	Page page_number;
	Page frame_number;
	int total;
}Pagetable;

/*
 * virtual address
 * composed of page number and offset
*/
typedef struct VIRTUAL_ADDRESS
{
	int page_number;
	int offset;
}VirtualAddress;

/*
 * structure for a process
 * every process has its own page table
 * we will identify each process with
 * a unique integer that will differentiate
 * it from other integers
 * a queue will tell us the order
 * in which the entries are put
 * in the page table
*/
typedef struct PROCESS
{
	Queue entries_read;
	Queue entries_modified;
	Pagetable *pagetable;
	int process_number;
	int pagefaults;
}Process;

/*
 * this function creates a file with "size" lines
 * ------First idea
 * each line is a physical address in memory
 * ------Second idea
 * have addresses in the file separated by a colon
 * i.e. 0xf25a:instruction1
 * 0xf25b:instruction2
*/
Page *create_physical_memory()
{
	int i;
	Page *ret = malloc(sizeof(Page) * PHYSICAL_MEMORY_SIZE);
	FILE *fp = fopen(MEM_FILE, "wb");
	for( i = 0; i < PHYSICAL_MEMORY_SIZE; ++i )
	{
		ret[i].number = i;
		ret[i].is_occupied = 0;
		fwrite("\n", sizeof(char), 1, fp);
	}
	fclose(fp);
	printf("INFO: Physical memory created\n");
	
	return ret;
}

/*
 * start by creating virtual pages for the process
 * depending on the size given, we will divide the
 * process into pages of 4
 * e.g. if process size is 128, we have 128 / 4 = 32 virtual pages
 * process size is 200 => 200 / 4 = 50 virtual pages
*/
Pagetable *allocate_virtual_memory(int process_size)
{
	// if we are given a value like 197 => 197 / 4 = (int) 49.25 = 49
	// 49 + 1 = 50 pages in total to account for floating numbers
	int restant = process_size % 4;
	int nb_pages = process_size / 4;
	if( restant )
	{
		nb_pages += 1;
	}
	
	// debug line
	//printf("Nb pages = %d\n", nb_pages);
	// malloc nb_pages for page table
	// first instance of ret contains number of pages
	Pagetable *ret = malloc(sizeof(Pagetable) * nb_pages);
	ret->total = nb_pages;
	
	int i;
	Page p;
	for( i = 0; i < nb_pages; ++i )
	{
		p.number = i;
		ret[i].page_number = p;
		// for initializing all memory to -1, pointing at nothing
		p.number = -1;
		ret[i].frame_number = p;
	}
	
	return ret;
}

/*
 * free the virtual memory at the end of the program
*/
void free_virtual_memory(Pagetable **pt)
{
	int i;
	int total = (*pt)->total;
	for( i = 0; i < total; ++i )
	{
		free(*pt);
		*pt = NULL;
	}
	
	return;
}

void free_physical_memory(Page **frames)
{
	int i;
	for( i = 0; i < PHYSICAL_MEMORY_SIZE; ++i )
	{
		free(*frames);
		*frames = NULL;
	}
	
	return;
}

/*
 * helper function to show a page number
 * and which frame number it's associated with
*/
void show_page(Page *p, Page *f)
{
	printf("page %d => frame %d\n", p->number, f->number);
	
	return;
}

/*
 * helper function to show process' virtual memory addresses
*/
void show_virtual_memory(Process *process)
{
	int i;
	Pagetable *pt = process->pagetable;
	int total = pt->total;
	
	printf("Page table for process %d\n", process->process_number);
	for( i = 0; i < total; ++i )
	{
		if( pt[i].frame_number.number > -1 )
		{
			show_page(&pt[i].page_number, &pt[i].frame_number);
		}/*
		else
		{
			show_page(&pt[i].page_number, &pt[i].frame_number);
		}*/
	}
	
	return;
}

/*
 * helper function to show OS physical memory addresses
*/
void show_physical_memory(Page *frames)
{
	int i;
	for( i = 0; i < PHYSICAL_MEMORY_SIZE; ++i )
	{
		if( frames[i].is_occupied > 0 )
		{
			printf("Frame number %d is occupied by process number %d\n", frames[i].number, frames[i].is_occupied);
		}
	}
	
	return;
}

/*
 * generate random number [0, bound[
*/
int generate_random(int bound)
{
	return rand() % bound;
}

/*
 * generate random number [lower, upper[
 * used in giving random sizes to process
*/
int generate_random_with_bounds(int lower, int upper)
{
	return (rand() % (lower - upper)) + lower;
}


/*
 * to map virtual memory to physical memory
 * it maps a full page(4kb) at a time
 * needs a page table to update the page information
 * and also the frame information
 * generate a random frame number
 * and associate it to a page number
 * the frames pointer will check if the frame index
 * is already occupied by a different process
 * if it is, then it will not allocate
*/
/*
void map_virtual_mem_to_physical(Process *process, int page_number, Page *frames)
{
	// physical page
	Page f;
	// page table
	Pagetable *pt = process->pagetable;
	
	// generate random number between [0, 15]
	// 15 is the max number of pages in physical memory
	// map 4 pages at a time
	int i;
	for( i = 0; i < PAGE_SIZE; ++i )
	{
		f.number = generate_random(PHYSICAL_MAX_PAGES);
		// while the random page offset generated is occupied
		// keep looking for another page
		while( frames[f.number].is_occupied )
		{
			// debug line
			//printf("generating new frame number\n");
			f.number = generate_random(PHYSICAL_MAX_PAGES);
		}
		// update process read entry queue
		q_push(&process->entries_read, (page_number + i));
		pt[page_number + i].frame_number = f;
		// update main memory frames here
		frames[f.number].number = f.number;
		frames[f.number].is_occupied = process->process_number;
	}
	
	return;
}
*/

/*
 * page replacement
 * FIFO
 * process contains pagetable
 * virtual page number: the virtual page to be replaced
 * frames: update the physical memory global variable
*/
void pagetable_replace_entry_fifo(Process *process, int virtual_page_number, Page *frames)
{
	// physical page
	Page f;
	// page table
	Pagetable *pt = process->pagetable;
	
	// checkig what is in the page table - debug
	/*
	for( i = 0; i < pt->total; ++i )
	{
		show_page(&pt[i].page_number, &pt[i].frame_number);
	}
	*/
	
	/*
	 * virtual_page_number is an index into the page table array
	 * it gives direct access to the page and we can check if it
	 * is mapped to physical memory, -1 if not
	 * if this function is called
	 * then we are sure that there has been a page fault
	 * so we update the page entries in the process pagetable
	 * and the frames global variable
	*/
	
	// if this function is called, there has been a pagefault
	process->pagefaults++;
	
	/*
	 * verify that all four frames have been
	 * used up and that there is no more space
	 * pop head of queue and push at end of queue
	 * the new page to be added
	*/
	if( process->entries_read.length >= PAGE_SIZE )
	{
		int prev = q_pop(&process->entries_read);
		q_push(&process->entries_read, virtual_page_number);
		// update current virtual page entry with the corresponding frame
		pt[virtual_page_number].frame_number = pt[prev].frame_number;
		// update previous virtual page entry to -1 (page fault)
		pt[prev].frame_number.number = -1;
	}
	else
	{
		// generate random number between [0, 15]
		// 15 is the max number of pages in physical memory
		f.number = generate_random(PHYSICAL_MAX_PAGES);
		// while the random page offset generated is occupied
		// keep looking for another page
		while( frames[f.number].is_occupied )
		{
			// debug line
			//printf("generating new frame number\n");
			f.number = generate_random(PHYSICAL_MAX_PAGES);
		}
		// update process read entry queue
		q_push(&process->entries_read, virtual_page_number);
		pt[virtual_page_number].frame_number = f;
		// update main memory frames here
		frames[f.number].number = f.number;
		frames[f.number].is_occupied = process->process_number;
	}
	
	// debug line - show the entries in process that have been read
	q_show(&process->entries_read);
}

/*
 * OR MAYBE
 * map one page at a time
 * each page contains 4 instructions (implemented later)
*/
void map_virtual_mem_to_physical(Process *process, int page_number, Page *frames)
{
	// page table
	Pagetable *pt = process->pagetable;
	
	// check if page is already mapped
	if( pt[page_number].frame_number.number > -1 )
	{
		// debug line
		//printf("already mapped, no page fault\n");
		return;
	}
	else
	{
		pagetable_replace_entry_fifo(process, page_number, frames);
	}
	
	return;
}

/*
 * calculate physical address from virtual address
*/
int virtual_to_physical_address_translation(Process *process, VirtualAddress *va)
{
	Pagetable *pt = process->pagetable;
	int virtual_page = va->page_number;
	int offset = va->offset;
	int physical_page = pt[virtual_page].frame_number.number;
	int addr = physical_page + offset;
	
	return addr;
}

/*
 * function to create processes
 * this function only creates the process
 * it is not loaded into memory so
 * there is no memory mapping done
*/
Process *create_simple_process(int process_number, int process_size)
{
	Process *ret = malloc(sizeof(Process));
	
	ret->process_number = process_number;
	ret->pagetable = allocate_virtual_memory(process_size);
	
	return ret;
}

/*
 * show process info
*/
void show_process_info(Process *process)
{
	printf("process id: %d\n", process->process_number);
	
	return;
}

/*
 * free process
*/
void free_process(Process **process)
{
	Process *temp = *process;
	free_virtual_memory(&temp->pagetable);
	q_delete(&temp->entries_read);
	q_delete(&temp->entries_modified);
	free(temp);
	temp = NULL;
	
	return;
}

/*
 * generate random page numbers
 * sleep for a bit
 * send them to be processed
*/
void cpu_emulator(Process *process, Page *frames)
{
	int index = 0;
	int requested_addresses[] = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 1, 2, 0};
	
	while( index < (sizeof(requested_addresses) / sizeof(int)) )
	{
		map_virtual_mem_to_physical(process, requested_addresses[index++], frames);
		show_virtual_memory(process);
		usleep(500000);
	}
}

/*
 * every OS needs a MMU to convert virtual address
 * to physical addresses, this function takes in a 
 * page table and does the conversion
 * returns a physical address
*/

int main()
{
	// to initialize a random seed
	srand(time(NULL));
	
	// create physical memory
	Page *frames = create_physical_memory();
	
	// show physical memory - debug line
	show_physical_memory(frames);
	
	Process *p1 = create_simple_process(1, 200);
	cpu_emulator(p1, frames);
	show_virtual_memory(p1);
	printf("total page faults: %d\n", p1->pagefaults);
	
	/*
	Process *p2 = create_simple_process(2, 200);
	load_process_into_memory(p2, frames);
	show_virtual_memory(p2);
	pagetable_replace_entry_fifo(p2, 0, frames);
	*/
	
	Process *p2 = create_simple_process(2, 250);
	cpu_emulator(p2, frames);
	show_virtual_memory(p2);
	printf("total page faults: %d\n", p2->pagefaults);
	
	show_physical_memory(frames);
	
	free_process(&p1);
	/*
	free_process(&p2);
	free_process(&p3);
	*/
	free_physical_memory(&frames);
	
	return 0;
}

