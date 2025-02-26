#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/moduleparam.h>

MODULE_DESCRIPTION("LKM2: lists children of a given process ID");
MODULE_AUTHOR("Prasoon");
MODULE_LICENSE("GPL");

static int pid = -1;
module_param(pid, int, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(pid, "PID of process whose children has to be listed in LKM2");

// rwlock_t lock;
static int lkm2_init(void){
    struct task_struct *p, *ch;
    struct list_head *lst;
    if(pid < 0){
        pr_err("Invalid PID specified: %d\n", pid);
        return -EINVAL; // Invalid input
    }
    rcu_read_lock();
    // p = find_task_by_vpid(pid);
    p = pid_task(find_vpid(pid), PIDTYPE_PID);
    if(!p){
        pr_err("[LKM2] No process found with PID: %d\n", pid);
        rcu_read_unlock();
        return -ESRCH; // No such process
    }
    list_for_each(lst, &p->children){
        ch = list_entry(lst, struct task_struct, sibling);
        pr_info("[LKM2] Child process PID: %d, State: %u\n", ch->pid, ch->__state);
    }
    rcu_read_unlock();
    return 0;
}

static void lkm2_exit(void){
    pr_info("[LKM2] Module LKM2 Unloaded\n");
    // pr_info("------------------\n");
}

module_init(lkm2_init);
module_exit(lkm2_exit);
