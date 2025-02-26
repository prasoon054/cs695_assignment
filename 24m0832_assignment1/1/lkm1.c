#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

MODULE_DESCRIPTION("LKM1: lists processes in running or runnable state");
MODULE_AUTHOR("Prasoon");
MODULE_LICENSE("GPL");

// rwlock_t lock;
// https://elixir.bootlin.com/linux/v6.1/source/include/linux/sched.h
static int lkm1_init(void){
    struct task_struct *p;
    pr_info("[LKM1] Runnable Processes\nPID\tPROC\n");
    pr_info("[LKM1] ------------------\n");
    // read_lock(&lock);
    rcu_read_lock();
    for_each_process(p){
        // pr_info("PID: %d | Process Name: %s | State: %u\n", p->pid, p->comm, p->__state);
        if(p->__state == TASK_RUNNING){
            pr_info("[LKM1] %d\t%s\n", p->pid, p->comm);
        }
    }
    // read_unlock(&lock);
    rcu_read_unlock();
    return 0;
}

static void lkm1_exit(void){
    pr_info("[LKM1] Module LKM1 Unloaded\n");
    // pr_info("[LKM1] ------------------\n");
}

module_init(lkm1_init);
module_exit(lkm1_exit);
