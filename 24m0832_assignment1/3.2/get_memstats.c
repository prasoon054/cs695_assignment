#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/moduleparam.h>
#include <linux/pid.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#define SYSFS_DIR_NAME "mem_stats"
#define DEFAULT_UNIT "B"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Prasoon");
MODULE_DESCRIPTION("Kernel module to expose process memory stats using sysfs");

static int pid = -1;
static char unit[2] = DEFAULT_UNIT;
static struct kobject *mem_stats_kobj;
unsigned long vm = -1;
unsigned long pm = -1;

static unsigned long convert_memory_size(unsigned long size_in_bytes){
    if(strcmp(unit, "K")==0)
        return size_in_bytes >> 10;
    else if(strcmp(unit, "M")==0)
        return size_in_bytes >> 20;
    return size_in_bytes;
}

static int get_mem_stats(unsigned long *vm, unsigned long *pm){
    struct task_struct *p;
    struct mm_struct *mm;
    struct vm_area_struct *vma;
    struct vma_iterator vmi;
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    unsigned long start, end;
    *vm = 0;
    *pm = 0;
    if(pid < 0){
        pr_err("Invalid PID: %d\n", pid);
        return -EINVAL;
    }
    rcu_read_lock();
    p = pid_task(find_vpid(pid), PIDTYPE_PID);
    if(!p){
        pr_err("No process found with PID: %d\n", pid);
        rcu_read_unlock();
        return -ESRCH;
    }
    mm = p->mm;
    if(!mm){
        pr_err("Process %d does not have a valid memory descriptor\n", pid);
        rcu_read_unlock();
        return -EINVAL;
    }
    down_read(&mm->mmap_lock);
    vma_iter_init(&vmi, mm, 0);
    for_each_vma(vmi, vma){
        start = vma->vm_start;
        end = vma->vm_end;
        *vm += end-start;
        while(start < end){
            pgd = pgd_offset(mm, start);
            if(pgd_none(*pgd) || pgd_bad(*pgd)){
                start += PAGE_SIZE;
                continue;
            }
            p4d = p4d_offset(pgd, start);
            if(p4d_none(*p4d) || p4d_bad(*p4d)){
                start += PAGE_SIZE;
                continue;
            }
            pud = pud_offset(p4d, start);
            if(pud_none(*pud) || pud_bad(*pud)){
                start += PAGE_SIZE;
                continue;
            }
            pmd = pmd_offset(pud, start);
            if(pmd_none(*pmd) || pmd_bad(*pmd)){
                start += PAGE_SIZE;
                continue;
            }
            pte = pte_offset_map(pmd, start);
            if(pte && pte_present(*pte)){
                *pm += PAGE_SIZE;
            }
            pte_unmap(pte);
            start += PAGE_SIZE;
        }
    }
    up_read(&mm->mmap_lock);
    rcu_read_unlock();
    return 0;
}

static ssize_t pid_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf){
    return sprintf(buf, "%d\n", pid);
}

static ssize_t pid_store(struct kobject *kobj, struct kobj_attribute *kattr, const char *buf, size_t count){
    sscanf(buf, "%d", &pid);
    return count;
}

static ssize_t unit_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf){
    return sprintf(buf, "%s\n", unit);
}

static ssize_t unit_store(struct kobject *kobj, struct kobj_attribute *kattr, const char *buf, size_t count){
    if(strncmp(buf, "B", 1)==0 || strncmp(buf, "K", 1)==0 || strncmp(buf, "M", 1)==0){
        strncpy(unit, buf, 1);
        unit[1] = '\0';
    }
    else{
        pr_err("Invalid unit. Use 'B', 'K' or 'M'");
    }
    return count;
}

static ssize_t vm_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf){
    if(pid>=0 && get_mem_stats(&vm, &pm)==0){
        return sprintf(buf, "%lu\n", convert_memory_size(vm));
    }
    return sprintf(buf, "-1\n");
}

static ssize_t pm_show(struct kobject *kobj, struct kobj_attribute *kattr, char *buf){
    if(pid>=0 && get_mem_stats(&vm, &pm)==0){
        return sprintf(buf, "%lu\n", convert_memory_size(pm));
    }
    return sprintf(buf, "-1\n");
}

static struct kobj_attribute pid_attr = __ATTR(pid, 0664, pid_show, pid_store);
static struct kobj_attribute unit_attr = __ATTR(unit, 0664, unit_show, unit_store);
static struct kobj_attribute vm_attr = __ATTR(vm, 0444, vm_show, NULL);
static struct kobj_attribute pm_attr = __ATTR(pm, 0444, pm_show, NULL);

static struct attribute *attrs[] = {
    &pid_attr.attr,
    &unit_attr.attr,
    &vm_attr.attr,
    &pm_attr.attr,
    NULL,
};

static struct attribute_group attr_grp = {
    .attrs = attrs,
};

static int __init mem_stats_init(void){
    int retval;
    mem_stats_kobj = kobject_create_and_add(SYSFS_DIR_NAME, kernel_kobj);
    if(!mem_stats_kobj){
        pr_err("Failed to create /sys/kernel/%s\n", SYSFS_DIR_NAME);
        return -ENOMEM;
    }
    retval = sysfs_create_group(mem_stats_kobj, &attr_grp);
    if(retval){
        kobject_put(mem_stats_kobj);
        pr_err("Failed to create sysfs group\n");
        return retval;
    }
    pr_info("mem_stats: Module loaded\n");
    return 0;
}

static void __exit mem_stats_exit(void){
    sysfs_remove_group(mem_stats_kobj, &attr_grp);
    kobject_put(mem_stats_kobj);
    pr_info("mem_stats: Module unloaded\n");
}

module_init(mem_stats_init);
module_exit(mem_stats_exit);
