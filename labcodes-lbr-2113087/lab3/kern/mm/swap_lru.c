#include <defs.h>
#include <riscv.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <swap_lru.h>
#include <list.h>

/* [wikipedia]The simplest Page Replacement Algorithm(PRA) is a FIFO algorithm. The first-in, first-out
 * page replacement algorithm is a low-overhead algorithm that requires little book-keeping on
 * the part of the operating system. The idea is obvious from the name - the operating system
 * keeps track of all the pages in memory in a queue, with the most recent arrival at the back,
 * and the earliest arrival in front. When a page needs to be replaced, the page at the front
 * of the queue (the oldest page) is selected. While FIFO is cheap and intuitive, it performs
 * poorly in practical application. Thus, it is rarely used in its unmodified form. This
 * algorithm experiences Belady's anomaly.
 *
 * Details of FIFO PRA
 * (1) Prepare: In order to implement FIFO PRA, we should manage all swappable pages, so we can
 *              link these pages into pra_list_head according the time order. At first you should
 *              be familiar to the struct list in list.h. struct list is a simple doubly linked list
 *              implementation. You should know howto USE: list_init, list_add(list_add_after),
 *              list_add_before, list_del, list_next, list_prev. Another tricky method is to transform
 *              a general list struct to a special struct (such as struct page). You can find some MACRO:
 *              le2page (in memlayout.h), (in future labs: le2vma (in vmm.h), le2proc (in proc.h),etc.
 */

list_entry_t pra_list_head;

/*
 * (2) _lru_init_mm: init pra_list_head and let  mm->sm_priv point to the addr of pra_list_head.
 *              Now, From the memory control struct mm_struct, we can access FIFO PRA
 */
static int
_lru_init_mm(struct mm_struct *mm)
{     
     list_init(&pra_list_head);
     mm->sm_priv = &pra_list_head;
     cprintf(" mm->sm_priv %x in lru_init_mm\n",mm->sm_priv);
     return 0;
}
/*
 * (3)_lru_map_swappable: According LRU PRA, we should link the most recent arrival page at the back of pra_list_head queue
 */
static int
_lru_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in)
{
    list_entry_t *head=(list_entry_t*) mm->sm_priv;
    list_entry_t *entry=&(page->pra_page_link);
 
    assert(entry != NULL && head != NULL);
    //record the page access situlation

    list_add(head, entry);
    return 0;
}
/*
 *  (4)_lru_swap_out_victim: According FIFO PRA, we should unlink the  earliest arrival page in front of pra_list_head qeueue,
 *                            then set the addr of addr of this page to ptr_page.
 */
static int
_lru_swap_out_victim(struct mm_struct *mm, struct Page ** ptr_page, int in_tick)
{
    list_entry_t *head=(list_entry_t*) mm->sm_priv;
    assert(head != NULL);
    assert(in_tick==0);
     /* Select the victim */
     //(1)  unlink the  earliest arrival page in front of pra_list_head qeueue
     //(2)  set the addr of addr of this page to ptr_page
    list_entry_t *le = head->next;
    // 遍历check_mm_struct的链表，记录visited最大的page对应的list_entry
    uint_t max=0;

    struct list_entry_t* maxle=NULL;
    while (le!=head) {
        struct Page* page = le2page(le,pra_page_link);
        if(page->visited>max) {
            max = page->visited;
            maxle = le;
        }
        le = le->next;
    }
    
    if (maxle != head) {
        list_del(maxle);
        *ptr_page = le2page(maxle, pra_page_link);
    } else {
        *ptr_page = NULL;
    }
    return 0; 
}


static int
_lru_check_swap(void) {
    cprintf("write Virt Page c in lru_check_swap\n");
    lru_access(0x3000, 0x0c);
    assert(pgfault_num==4);
    cprintf("write Virt Page a in lru_check_swap\n");
    lru_access(0x1000, 0x0a);
    assert(pgfault_num==4);
    cprintf("write Virt Page d in lru_check_swap\n");
    lru_access(0x4000, 0x0d);
    assert(pgfault_num==4);
    cprintf("write Virt Page b in lru_check_swap\n");
    lru_access(0x2000, 0x0b);
    assert(pgfault_num==4);
    cprintf("write Virt Page e in lru_check_swap\n");
    lru_access(0x5000, 0x0e);
    assert(pgfault_num==5);
    cprintf("write Virt Page b in lru_check_swap\n");
    lru_access(0x3000, 0x0c);
    assert(pgfault_num==6);
    cprintf("write Virt Page a in lru_check_swap\n");
    lru_access(0x1000, 0x0a);
    assert(pgfault_num==7);
    cprintf("write Virt Page b in lru_check_swap\n");
    lru_access(0x2000, 0x0b);
    assert(pgfault_num==7);
    cprintf("write Virt Page c in lru_check_swap\n");
    lru_access(0x3000, 0x0c);
    assert(pgfault_num==7);
    cprintf("write Virt Page d in lru_check_swap\n");
    lru_access(0x4000, 0x0d);
    assert(pgfault_num==8);
    cprintf("write Virt Page e in lru_check_swap\n");
    lru_access(0x5000, 0x0e);
    assert(pgfault_num==9);
    cprintf("write Virt Page a in lru_check_swap\n");
    assert(*(unsigned char *)0x1000 == 0x0a);
    lru_access(0x1000, 0x0a);
    assert(pgfault_num==10);
    return 0;
}


static int
_lru_init(void)
{
    return 0;
}

static int
_lru_set_unswappable(struct mm_struct *mm, uintptr_t addr)
{
    return 0;
}

static int
_lru_tick_event(struct mm_struct *mm)
{ return 0; }




struct swap_manager swap_manager_lru =
{
     .name            = "lru swap manager",
     .init            = &_lru_init,
     .init_mm         = &_lru_init_mm,
     .tick_event      = &_lru_tick_event,
     .map_swappable   = &_lru_map_swappable,
     .set_unswappable = &_lru_set_unswappable,
     .swap_out_victim = &_lru_swap_out_victim,
     .check_swap      = &_lru_check_swap,
};
