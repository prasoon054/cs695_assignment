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

MODULE_DESCRIPTION("LKM5: calculates THP usage for a given process");
MODULE_AUTHOR("Prasoon");
MODULE_LICENSE("GPL");

static int pid = -1;
module_param(pid, int, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(pid, "Process ID");

// rwlock_t lock;
static int lkm5_init(void){
    struct task_struct *p;
    struct mm_struct *mm;
    struct vm_area_struct *vma;
    struct vma_iterator vmi;
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    // pte_t *pte;
    // struct page *page;
    unsigned long thp_size = 0, thp_count = 0, start, end;
    if(pid < 0){
        pr_err("[LKM5] Invalid PID: %d\n", pid);
        return -EINVAL; // Invalid input
    }
    rcu_read_lock();
    // p = find_task_by_vpid(pid);
    p = pid_task(find_vpid(pid), PIDTYPE_PID);
    if(!p){
        pr_err("[LKM5] No process found with PID: %d\n", pid);
        rcu_read_unlock();
        return -ESRCH; // No such process
    }
    mm = p->mm;
    if(!mm){
        pr_err("[LKM5] Process %d does not have a valid memory descriptor\n", pid);
        rcu_read_unlock();
        return -EINVAL;
    }
    down_read(&mm->mmap_lock);
    vma_iter_init(&vmi, mm, 0);
    for_each_vma(vmi, vma){
        start = vma->vm_start;
        end = vma->vm_end;
        while(start < end){
            pgd = pgd_offset(mm, start);
            if(pgd_none(*pgd) || pgd_bad(*pgd)){
                // pr_err("[LKM3] VA not mapped: Invalid PGD\n");
                // rcu_read_unlock();
                // return -EFAULT;
                // start += PAGE_SIZE;
                continue;
            }
            p4d = p4d_offset(pgd, start);
            if(p4d_none(*p4d) || p4d_bad(*p4d)){
                // pr_err("[LKM3] VA not mapped: Invalid P4D\n");
                // rcu_read_unlock();
                // return -EFAULT;
                // start += PAGE_SIZE;
                continue;
            }
            pud = pud_offset(p4d, start);
            if(pud_none(*pud) || pud_bad(*pud)){
                // pr_err("[LKM3] VA not mapped: Invalid PUD\n");
                // rcu_read_unlock();
                // return -EFAULT;
                // start += PUD_SIZE;
                continue;
            }
            pmd = pmd_offset(pud, start);
            if(pmd && pmd_trans_huge(*pmd)){
                thp_size += PMD_SIZE;
                thp_count++;
                // pr_info("[LKM5] Reaches here\n");
                start += PMD_SIZE;
            }
            else{
                start += PAGE_SIZE;
            }
        }
    }
    up_read(&mm->mmap_lock);
    rcu_read_unlock();
    pr_info("[LKM5] THP Size: %lu KiB, THP count: %lu\n", thp_size>>10, thp_count);
    return 0;
}

static void lkm5_exit(void){
    pr_info("[LKM5] Module LKM5 Unloaded\n");
}

module_init(lkm5_init);
module_exit(lkm5_exit);
