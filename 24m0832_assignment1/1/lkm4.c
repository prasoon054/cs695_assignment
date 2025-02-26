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

MODULE_DESCRIPTION("LKM4: calculates VMA and mapped memory sizes");
MODULE_AUTHOR("Prasoon");
MODULE_LICENSE("GPL");

static int pid = -1;
module_param(pid, int, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(pid, "Process ID");

// rwlock_t lock;
static int lkm4_init(void){
    // https://docs.kernel.org/mm/page_tables.html
    // https://stackoverflow.com/questions/58743052/getting-error-when-compiling-kernel-for-page-table-walk
    // https://kernel.googlesource.com/pub/scm/linux/kernel/git/krzk/linux-w1/+/refs/heads/for-next/mm/vma.c
    // vma_iterator, vma_iter_init, for_each_vma
    struct task_struct *p;
    struct mm_struct *mm;
    struct vm_area_struct *vma;
    struct vma_iterator vmi;
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    // struct page *page;
    unsigned long pm = 0, vm = 0, start, end;
    if(pid < 0){
        pr_err("[LKM4] Invalid PID: %d\n", pid);
        return -EINVAL; // Invalid input
    }
    rcu_read_lock();
    // p = find_task_by_vpid(pid);
    p = pid_task(find_vpid(pid), PIDTYPE_PID);
    if(!p){
        pr_err("[LKM4] No process found with PID: %d\n", pid);
        rcu_read_unlock();
        return -ESRCH; // No such process
    }
    mm = p->mm;
    if(!mm){
        pr_err("[LKM4] Process %d does not have a valid memory descriptor\n", pid);
        rcu_read_unlock();
        return -EINVAL;
    }
    down_read(&mm->mmap_lock);
    vma_iter_init(&vmi, mm, 0);
    for_each_vma(vmi, vma){
        start = vma->vm_start;
        end = vma->vm_end;
        vm += end-start;
        while(start < end){
            pgd = pgd_offset(mm, start);
            if(pgd_none(*pgd) || pgd_bad(*pgd)){
                // pr_err("[LKM3] VA not mapped: Invalid PGD\n");
                // rcu_read_unlock();
                // return -EFAULT;
                start += PAGE_SIZE;
                continue;
            }
            p4d = p4d_offset(pgd, start);
            if(p4d_none(*p4d) || p4d_bad(*p4d)){
                // pr_err("[LKM3] VA not mapped: Invalid P4D\n");
                // rcu_read_unlock();
                // return -EFAULT;
                start += PAGE_SIZE;
                continue;
            }
            pud = pud_offset(p4d, start);
            if(pud_none(*pud) || pud_bad(*pud)){
                // pr_err("[LKM3] VA not mapped: Invalid PUD\n");
                // rcu_read_unlock();
                // return -EFAULT;
                start += PAGE_SIZE;
                continue;
            }
            pmd = pmd_offset(pud, start);
            if(pmd_none(*pmd) || pmd_bad(*pmd)){
                // pr_err("[LKM3] VA not mapped: Invalid PMD\n");
                // rcu_read_unlock();
                // return -EFAULT;
                start += PAGE_SIZE;
                continue;
            }
            pte = pte_offset_map(pmd, start);
            if(pte && pte_present(*pte)){
                // pr_err("[LKM3] VA not mapped: No PTE\n");
                // pte_unmap(pte);
                // rcu_read_unlock();
                // return -EFAULT;
                pm += PAGE_SIZE;
            }
            // if(!pte_present(*pte)){
            //     pr_err("[LKM3] VA not mapped: Page not present\n");
            //     pte_unmap(pte);
            //     rcu_read_unlock();
            //     return -EFAULT;
            // }
            // page = pte_page(*pte);
            // if(!page){
            //     pr_err("[LKM3] VA not mapped: Could not find physical page\n");
            //     pte_unmap(pte);
            //     rcu_read_unlock();
            //     return -EFAULT;
            // }
            pte_unmap(pte);
            start += PAGE_SIZE;
        }
    }
    up_read(&mm->mmap_lock);
    rcu_read_unlock();
    pr_info("[LKM4] Virtual Memory Size: %lu KiB\n", vm>>10);
    pr_info("[LKM4] Physical Memory Size: %lu KiB\n", pm>>10);
    return 0;
}

static void lkm4_exit(void){
    pr_info("[LKM4] Module LKM4 Unloaded\n");
    // pr_info("------------------\n");
}

module_init(lkm4_init);
module_exit(lkm4_exit);
