#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/moduleparam.h>
#include <linux/mm.h>
#include <linux/pid.h>
#include <asm/page.h>
#include <asm/pgtable.h>

MODULE_DESCRIPTION("LKM3: finds PA from VA for a given PID");
MODULE_AUTHOR("Prasoon");
MODULE_LICENSE("GPL");

static int pid = -1;
static unsigned long vaddr;
module_param(pid, int, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(pid, "Process ID");
module_param(vaddr, ulong, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(vaddr, "Virtual address in decimal");

// rwlock_t lock;
static int lkm3_init(void){
    // https://docs.kernel.org/mm/page_tables.html
    // https://stackoverflow.com/questions/58743052/getting-error-when-compiling-kernel-for-page-table-walk
    struct task_struct *p;
    struct mm_struct *mm;
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    struct page *page;
    unsigned long pa;
    if(pid < 0 || vaddr==0){
        pr_err("[LKM3] Invalid PID specified: %d or Invalid VA (%lu)\n", pid, vaddr);
        return -EINVAL; // Invalid input
    }
    rcu_read_lock();
    // p = find_task_by_vpid(pid);
    p = pid_task(find_vpid(pid), PIDTYPE_PID);
    if(!p){
        pr_err("[LKM3] No process found with PID: %d\n", pid);
        rcu_read_unlock();
        return -ESRCH; // No such process
    }
    mm = p->mm;
    if(!mm){
        pr_err("[LKM3] Process %d does not have a valid memory descriptor\n", pid);
        rcu_read_unlock();
        return -EINVAL;
    }
    pgd = pgd_offset(mm, vaddr);
    if(pgd_none(*pgd) || pgd_bad(*pgd)){
        pr_err("[LKM3] VA not mapped: Invalid PGD\n");
        rcu_read_unlock();
        return -EFAULT;
    }
    p4d = p4d_offset(pgd, vaddr);
    if(p4d_none(*p4d) || p4d_bad(*p4d)){
        pr_err("[LKM3] VA not mapped: Invalid P4D\n");
        rcu_read_unlock();
        return -EFAULT;
    }
    pud = pud_offset(p4d, vaddr);
    if(pud_none(*pud) || pud_bad(*pud)){
        pr_err("[LKM3] VA not mapped: Invalid PUD\n");
        rcu_read_unlock();
        return -EFAULT;
    }
    pmd = pmd_offset(pud, vaddr);
    if(pmd_none(*pmd) || pmd_bad(*pmd)){
        pr_err("[LKM3] VA not mapped: Invalid PMD\n");
        rcu_read_unlock();
        return -EFAULT;
    }
    pte = pte_offset_map(pmd, vaddr);
    if(!pte || pte_none(*pte)){
        pr_err("[LKM3] VA not mapped: No PTE\n");
        pte_unmap(pte);
        rcu_read_unlock();
        return -EFAULT;
    }
    if(!pte_present(*pte)){
        pr_err("[LKM3] VA not mapped: Page not present\n");
        pte_unmap(pte);
        rcu_read_unlock();
        return -EFAULT;
    }
    page = pte_page(*pte);
    if(!page){
        pr_err("[LKM3] VA not mapped: Could not find physical page\n");
        pte_unmap(pte);
        rcu_read_unlock();
        return -EFAULT;
    }
    // pa = page_to_phys(page) + (vaddr & ~PAGE_MASK);
    pa = (page_to_pfn(page) << PAGE_SHIFT) | (vaddr & ~PAGE_MASK);
    pr_info("[LKM3] Virtual address: 0x%lx / %lu\n", vaddr, vaddr);
    pr_info("[LKM3] Physical address: 0x%lx / %lu\n", pa, pa);
    pte_unmap(pte);
    rcu_read_unlock();
    return 0;
}

static void lkm3_exit(void){
    pr_info("[LKM3] Module LKM3 Unloaded\n");
    // pr_info("------------------\n");
}

module_init(lkm3_init);
module_exit(lkm3_exit);
