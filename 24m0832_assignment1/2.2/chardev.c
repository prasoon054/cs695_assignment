#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/rculist.h>
#include <linux/delay.h>
#include <asm/current.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Prasoon");
MODULE_DESCRIPTION("IOCTL driver for Dr. Bloom's operations");

#define DEVICE_NAME "chardev"
#define IOCTL_MAGIC 'B'
#define IOCTL_SET_PARENT _IOW(IOCTL_MAGIC, 1, int)
#define IOCTL_EMERGENCY _IOW(IOCTL_MAGIC, 2, int)

static long dr_bloom_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    pid_t pid;
    struct task_struct *tgt_tsk, *ch_tsk;
    struct list_head *lst;
    switch(cmd){
        case IOCTL_SET_PARENT:
            if(copy_from_user(&pid, (pid_t *)arg, sizeof(pid_t))){
                return -EFAULT;
            }
            rcu_read_lock();
            tgt_tsk = pid_task(find_vpid(pid), PIDTYPE_PID);
            if(!tgt_tsk){
                rcu_read_unlock();
                pr_err("Invalid PID: %d\n", pid);
                return -ESRCH;
            }
            raw_spin_lock(&tgt_tsk->pi_lock);
            raw_spin_lock(&current->pi_lock);
            list_del_rcu(&current->sibling);
            current->parent = tgt_tsk;
            pr_info("[IOCTL]: Parent: %d\n", current->parent->pid);
            current->real_parent = tgt_tsk;
            list_add_rcu(&current->sibling, &tgt_tsk->children);
            raw_spin_unlock(&current->pi_lock);
            raw_spin_unlock(&tgt_tsk->pi_lock);
            rcu_read_unlock();
            pr_info("[IOCTL]: Process %d is now a child of %d\n", current->pid, pid);
            break;
        case IOCTL_EMERGENCY:
            if(copy_from_user(&pid, (pid_t *)arg, sizeof(pid_t))){
                return -EFAULT;
            }
            rcu_read_lock();
            tgt_tsk = pid_task(find_vpid(pid), PIDTYPE_PID);
            if(!tgt_tsk){
                rcu_read_unlock();
                pr_err("Invalid PID: %d\n", pid);
                return -ESRCH;
            }
            raw_spin_lock(&tgt_tsk->pi_lock);
            list_for_each(lst, &tgt_tsk->children){
                ch_tsk = list_entry(lst, struct task_struct, sibling);
                send_sig(SIGTERM, ch_tsk, 1);
                pr_info("[IOCTL]: Terminated child process %d\n", ch_tsk->pid);
            }
            raw_spin_unlock(&tgt_tsk->pi_lock);
            rcu_read_unlock();
            msleep(1000);
            send_sig(SIGTERM, tgt_tsk, 1);
            pr_info("[IOCTL]: Terminated parent process %d\n", pid);
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static const struct file_operations dr_bloom_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = dr_bloom_ioctl,
};

static struct miscdevice dr_bloom_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &dr_bloom_fops,
};

static int __init dr_bloom_init(void) {
    int ret;

    ret = misc_register(&dr_bloom_device);
    if (ret) {
        pr_err("Failed to register device\n");
        return ret;
    }

    pr_info("Dr. Bloom device initialized\n");
    return 0;
}

static void __exit dr_bloom_exit(void) {
    misc_deregister(&dr_bloom_device);
    pr_info("Dr. Bloom device removed\n");
}

module_init(dr_bloom_init);
module_exit(dr_bloom_exit);
