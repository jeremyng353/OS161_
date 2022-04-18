#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>

static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
static struct spinlock coremap_lock = SPINLOCK_INITIALIZER;

struct page_table {
    uint32_t vpage_num[128];
    uint32_t ppage_num[128];
};

struct coremap_entry {
    uint32_t use;
    uint32_t vaddr;
};

uint32_t total_pages;
paddr_t last_addr;
paddr_t first_addr;
struct coremap_entry *coremap;
int bootstrap_done = 0;

void vm_bootstrap(void) {
    last_addr = ram_getsize();
    first_addr = ram_getfirstfree();
	total_pages = ((last_addr - first_addr) / PAGE_SIZE);
	bootstrap_done = 1;
	coremap = (struct coremap_entry *)PADDR_TO_KVADDR(first_addr);
	// first entry in coremap is being used to store the coremap
	coremap[0].use = 1;
}

static paddr_t getppages(unsigned long npages) {
	if (!bootstrap_done) {
		spinlock_acquire(&stealmem_lock);

		paddr_t addr = ram_stealmem(npages);

		spinlock_release(&stealmem_lock);
		return addr;
	} else {
		spinlock_acquire(&coremap_lock);
	    for (uint32_t i = 0; i < total_pages; i++) {
	        if (coremap[i].use == 0) {
	            coremap[i].use = 1;
	            coremap[i].vaddr = PADDR_TO_KVADDR(first_addr + (i << 12));
	            spinlock_release(&coremap_lock);
	            return first_addr + (i << 12);
	        }
	    }
		spinlock_release(&coremap_lock);
	    panic("No physical memory left!");
	    return 0;
	}
}

// taken from dumbvm.c
vaddr_t alloc_kpages(unsigned npages) {
	paddr_t pa;
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void free_kpages(vaddr_t vaddr) {
    spinlock_acquire(&coremap_lock);
    for (uint32_t i = 0; i < total_pages; i++) {
        if (coremap[i].vaddr == vaddr) {
            KASSERT(coremap[i].use == 1);
            coremap[i].use = 0;
            coremap[i].vaddr = 0;
            break;
        }
    }
    spinlock_release(&coremap_lock);
}

void
vm_tlbshootdown_all(void) {
	panic("tlbshootdown_all");
}

void
vm_tlbshootdown(const struct tlbshootdown *ts) {
	(void)ts;
	panic("tlbshootdown");
}

int vm_fault(int faulttype, vaddr_t faultaddress) {
	switch(faulttype) {
		case VM_FAULT_READONLY:
			panic("dumbvm: got VM_FAULT_READONLY\n");
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
	        // check page table for the mapping
			for (uint32_t i = 0; i < 128; i++) {
				if (curproc->pt->vpage_num[i] == faultaddress >> 12) {
					// evict a tlb entry and replace it with the page table entry
					uint32_t entryhi = faultaddress;		
					uint32_t entrylo = curproc->pt->ppage_num[i] << 12 | TLBLO_DIRTY | TLBLO_VALID;
					int spl = splhigh();
					tlb_random(entryhi, entrylo);
					splx(spl);
					return 0;
				}
			}

			// paging from disk
			for (uint32_t i = 0; i < total_pages; i++) {
			    if (coremap[i].vaddr == faultaddress) {
			        // update page table
			        curproc->pt->vpage_num[i % 128] = faultaddress >> 12;
			        curproc->pt->ppage_num[i % 128] = i;
			        
			        // update tlb
			        uint32_t entryhi = faultaddress;
			        uint32_t entrylo = i << 12 | TLBLO_DIRTY | TLBLO_VALID;
			        int spl = splhigh();
			        tlb_random(entryhi, entrylo);
			        splx(spl);
			    }
			}
			return 0;
	    default:
			return EINVAL;
			
	}
}

void get_range(vaddr_t *first, vaddr_t *last) {
	*first = PADDR_TO_KVADDR(first_addr);
	*last = PADDR_TO_KVADDR(last_addr);
}

void set_last(vaddr_t last) {
	last_addr = KVADDR_TO_PADDR(last);
}
