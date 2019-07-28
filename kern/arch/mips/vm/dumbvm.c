/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <copyinout.h>
#include <vm.h>

/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground.
 */

/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12

/*
 * Wrap rma_stealmem in a spinlock.
 */
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

bool vm_booted = false;

static paddr_t cm_start; /* address of first coremap entry */
static paddr_t cm_end; /* one past end of last coremap entry */
static size_t cm_length; /* number of coremap entries */
static paddr_t mem_begin; /* address of first allocable memory - page aligned */

// Given a coremap index i, return the coremap entry at i
struct coremap_entry *get_entry(int i) {
	KASSERT(i >= 0);
	return (struct coremap_entry *) PADDR_TO_KVADDR(cm_start + i * sizeof(struct coremap_entry));
}

// Given a virtual address v_addr, return the corresponding coremap idx
size_t get_coremap_idx_from_va(vaddr_t v_addr) {
	KASSERT(v_addr);
	paddr_t p_addr = v_addr - MIPS_KSEG0;  // convert virtual address to physical address
	KASSERT(p_addr >= mem_begin);
	KASSERT((p_addr - mem_begin) % PAGE_SIZE == 0);
	return (size_t)((p_addr - mem_begin) / PAGE_SIZE);
}

void vm_bootstrap() {
	// Initialize cm_start and cm_end (start and end addresses of the coremap)
    paddr_t first_addr;
    paddr_t last_addr;
    ram_getsize(&first_addr, &last_addr);
	KASSERT(last_addr > first_addr);
    
    size_t pages_available_including_cm =  ((size_t)(last_addr - first_addr)) / PAGE_SIZE;
	paddr_t cm_end_temp = first_addr + pages_available_including_cm * sizeof(struct coremap_entry);
    size_t pages_available_excluding_cm =  ((size_t)(last_addr - cm_end_temp)) / PAGE_SIZE;
    size_t pages_in_cm = pages_available_including_cm - pages_available_excluding_cm;
    
	mem_begin = ROUNDUP(cm_end_temp, PAGE_SIZE);  // page-align
	cm_end = cm_end_temp - (pages_in_cm + 1) * sizeof(struct coremap_entry); // +1 to match the mem_begin ROUNDUP
	cm_end = ROUNDUP(cm_end, sizeof(struct coremap_entry));
	cm_start = first_addr;
	cm_length = (cm_end - cm_start) / sizeof(struct coremap_entry);
	KASSERT(cm_length < pages_available_excluding_cm);  // allow 1 unindexed page at the end

	// Set all entries to unused state
	for (size_t i = 0; i < cm_length; ++i) {
		struct coremap_entry *entry = get_entry(i);
		entry->used = false;
		entry->num_pages_allocated = 0;
	}

	vm_booted = true;
}

static
paddr_t
getppages(unsigned long npages)
{
	paddr_t addr;

	spinlock_acquire(&stealmem_lock);

	if (vm_booted) {
		addr = alloc_kpages_vm(npages);
	} else {
		addr = ram_stealmem(npages);
	}
	
	spinlock_release(&stealmem_lock);
	return addr;
}

/*
Returns true if there exists a block of n pages available starting
with start_idx, else false.
 */
bool exists_n_contiguous_pages(size_t n, size_t start_idx) {
	if (start_idx + n >= cm_length) {
		return false;  // the block goes out of bounds
	}

	for (size_t j = 0; j < n; ++j) {
		struct coremap_entry *entry = get_entry(start_idx + j);
		KASSERT(entry);
		if (entry->used) {
			return false;
		}
	}
	
	return true;
}

// Sets the n contiguous entries starting at start_idx to used and sets
// the num_pages_allocated to n
void allocate_n_contiguous_pages(int n, size_t start_idx) {
	KASSERT(n > 0);
	KASSERT(start_idx + n <= cm_length); // assert in bounds

	// Set num_pages_allocated of the first entry
	struct coremap_entry *start_addr = get_entry(start_idx);
	KASSERT(start_addr);
	start_addr->num_pages_allocated = n;

	// Set the used flag of each entry in the block to true
	for (size_t i = 0; i < (size_t)n; ++i) {
		struct coremap_entry *entry = get_entry(start_idx + i);
		KASSERT(entry);
		entry->used = true;
	}
}

paddr_t alloc_kpages_vm(int npages) {
	paddr_t pa = 0;
	size_t start_idx;
	for (start_idx = 0; start_idx < cm_length; ++start_idx) {
		if (!exists_n_contiguous_pages(npages, start_idx)) {
			continue;
		}
		allocate_n_contiguous_pages(npages, start_idx);
		pa = mem_begin + start_idx * PAGE_SIZE;
		break;
	}
	//kprintf("\nalloc_kpages_vm: Allocated %d pages starting at physical address 0x%x with coremap index %d\n", npages, pa, (int)start_idx);
	return pa;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t alloc_kpages(int npages) {
	KASSERT(npages >= 0);
	if (npages == 0) {
		return 0;
	}
	paddr_t pa;
	pa = getppages(npages);

	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void free_kpages_pa(paddr_t p_addr) {
	free_kpages(PADDR_TO_KVADDR(p_addr));
}

void free_kpages(vaddr_t addr) {
	if (!addr) {
		return;
	}
	spinlock_acquire(&stealmem_lock);

	// Get the coremap entry of the first page
	size_t begin_idx = get_coremap_idx_from_va(addr);
	struct coremap_entry *begin_entry = get_entry(begin_idx);
	KASSERT(begin_entry);
	
	int num_pages_allocated = begin_entry->num_pages_allocated;
	if (num_pages_allocated == 0) {
		return;
	}
	//kprintf("\nfree_kpages: freeing %d pages starting at physical address 0x%x with coremap idx %d\n", num_pages_allocated, addr - MIPS_KSEG0, (int)begin_idx);

	KASSERT(begin_idx + num_pages_allocated <= cm_length);

	for (int i = 0; i < num_pages_allocated; ++i) {
		struct coremap_entry *entry = get_entry(begin_idx + i);
		KASSERT(entry);
		KASSERT(entry->used);
		entry->used = false;
		entry->num_pages_allocated = 0;
	}

	spinlock_release(&stealmem_lock);
}

void
vm_tlbshootdown_all(void)
{
	panic("dumbvm tried to do tlb shootdown?!\n");
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
	uint32_t ehi, elo;
	struct addrspace *as;
	int spl;
	bool is_read_only = false;

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
			return EROFS;
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = curproc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	KASSERT(as->as_vbase1 != 0);
	KASSERT(as->as_pbase1 != 0);
	KASSERT(as->as_npages1 != 0);
	KASSERT(as->as_vbase2 != 0);
	KASSERT(as->as_pbase2 != 0);
	KASSERT(as->as_npages2 != 0);
	KASSERT(as->as_stackpbase != 0);
	KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	KASSERT((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	KASSERT((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	KASSERT((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		paddr = (faultaddress - vbase1) + as->as_pbase1;
		is_read_only = true;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		paddr = (faultaddress - vbase2) + as->as_pbase2;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		paddr = (faultaddress - stackbase) + as->as_stackpbase;
	}
	else {
		return EFAULT;
	}

	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		if (is_read_only && as->load_elf_completed) {
			elo&=~TLBLO_DIRTY;
		}
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		tlb_write(ehi, elo, i);
		splx(spl);
		return 0;
	}

	ehi = faultaddress;
	elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	if (is_read_only && as->load_elf_completed) {
		elo&=~TLBLO_DIRTY;
	}
	tlb_random(ehi, elo);
	splx(spl);
	return 0;
}

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

	as->as_vbase1 = 0;
	as->as_pbase1 = 0;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	as->as_pbase2 = 0;
	as->as_npages2 = 0;
	as->as_stackpbase = 0;
	as->load_elf_completed = false;

	return as;
}

void
as_destroy(struct addrspace *as)
{
	free_kpages_pa(as->as_stackpbase);
	free_kpages_pa(as->as_pbase2);
	free_kpages_pa(as->as_pbase1);
	kfree(as);
}

void
as_activate(void)
{
	int i, spl;
	struct addrspace *as;

	as = curproc_getas();
#ifdef UW
        /* Kernel threads don't have an address spaces to activate */
#endif
	if (as == NULL) {
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

void
as_deactivate(void)
{
	/* nothing */
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages; 

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	/* We don't use these - all pages are read-write */
	(void)readable;
	(void)writeable;
	(void)executable;

	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
		return 0;
	}

	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("dumbvm: Warning: too many regions\n");
	return EUNIMP;
}

static
void
as_zero_region(paddr_t paddr, unsigned npages)
{
	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
}

int
as_prepare_load(struct addrspace *as)
{
	KASSERT(as->as_pbase1 == 0);
	KASSERT(as->as_pbase2 == 0);
	KASSERT(as->as_stackpbase == 0);

	as->as_pbase1 = getppages(as->as_npages1);
	if (as->as_pbase1 == 0) {
		return ENOMEM;
	}

	as->as_pbase2 = getppages(as->as_npages2);
	if (as->as_pbase2 == 0) {
		return ENOMEM;
	}

	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
	if (as->as_stackpbase == 0) {
		return ENOMEM;
	}
	
	as_zero_region(as->as_pbase1, as->as_npages1);
	as_zero_region(as->as_pbase2, as->as_npages2);
	as_zero_region(as->as_stackpbase, DUMBVM_STACKPAGES);

	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	(void)as;
	return 0;
}

/*
args: array of arguments without the final NULL value. This function will populate the 
last NULL value.
num_args: number of arguments.
 */
int
as_define_stack(struct addrspace *as, vaddr_t *stackptr, char **args, size_t num_args)
{
	KASSERT(as->as_stackpbase != 0);
	*stackptr = USERSTACK;
	size_t got;
	int result;
	
	// If arguments are being passed
	if (args && num_args > 0) {
		// Push args onto the user stack and save their addresses
		// in temp_arg_locations so we can push the addresses onto 
		// the user stack later
		vaddr_t **temp_arg_locations = kmalloc(sizeof(vaddr_t *) * (num_args + 1));
		for (size_t i = 0; i < num_args; ++i) {
			int argsize = strlen(args[i]) + 1;
			int offset = ROUNDUP(argsize * sizeof(char), 8);
			*stackptr -= offset;
			result = copyoutstr(args[i], (userptr_t)*stackptr, argsize, &got);
			if (result) {
				kfree(temp_arg_locations);
				return result;
			}
			temp_arg_locations[i] = (vaddr_t*)*stackptr;
		}
		temp_arg_locations[num_args] = NULL; // Last arg is NULL

		// Push the addresses of the args, which will become the argv value in 
		// the new program's main function
		for (int i = num_args; i >= 0; --i) {
			int offset = ROUNDUP(sizeof(vaddr_t *), 4);
			*stackptr -= offset;
			result = copyout(&temp_arg_locations[i], (userptr_t)*stackptr, sizeof(vaddr_t *));
			if (result) {
				kfree(temp_arg_locations);
				return result;
			}
		}

		kfree(temp_arg_locations);
	}

	return 0;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;

	/* (Mis)use as_prepare_load to allocate some physical memory. */
	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}

	KASSERT(new->as_pbase1 != 0);
	KASSERT(new->as_pbase2 != 0);
	KASSERT(new->as_stackpbase != 0);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
		old->as_npages1*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
		old->as_npages2*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
		DUMBVM_STACKPAGES*PAGE_SIZE);
	
	*ret = new;
	return 0;
}
