#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/seek.h>
#include <kern/stat.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <copyinout.h>
#include <vfs.h>
#include <vnode.h>
#include <openfile.h>
#include <filetable.h>
#include <syscall.h>
#include <vm.h>

int sys_sbrk(intptr_t amount, vaddr_t *retval) {
    // retval returns the previous value of the break or when error, ((void *)-1)
    // break is process's end addr of heap region, and sbrk adjusts the break by amount given

    // if amount results in page-aligned break addresses, good
    // otherwise reject with EINVAL

    // heap grows up
    // cannot set end of heap lower than beginning of heap
    
    (void)amount;
    (void)retval;
    return 0;
    
    /*
    if (amount % PAGE_SIZE != 0) {
        *retval = -1;
        return EINVAL;
    }

    vaddr_t first, last;
    get_range(&first, &last);

    if (last + amount < first) {
        *retval = -1;
        return EINVAL;
    }

    *retval = last;
    set_last(last + amount);
    return 0;
    */
}
