#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/kernel_stat.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/slab.h>
#include <asm/io.h>

#define PROCFS_NAME "get_pgfaults"
#define MESSAGE "Hello World!\n"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Prasoon");
MODULE_DESCRIPTION("A /proc file system module that returns number of pgfaults");

static unsigned long total_pgfaults;
unsigned long ev[NR_VM_EVENT_ITEMS];

static ssize_t get_pgfaults_read(struct file *file, char __user *buffer, size_t count, loff_t *offset){
    // get total pgfaults in total_pgfaults
    // https://elixir.bootlin.com/linux/v4.7/source/arch/s390/appldata/appldata_mem.c#L88
    all_vm_events(ev);
    total_pgfaults = ev[PGFAULT];
    char buf[64];
    int len = snprintf(buf, 64, "Page faults: %lu\n", total_pgfaults);
    if (*offset >= len)
        return 0;
    if (count > len - *offset)
        count = len - *offset;
    if (copy_to_user(buffer, buf+*offset, count))
        return -EFAULT;
    *offset += count;
    return count;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = get_pgfaults_read,
};

static int __init get_pgfaults_init(void){
    struct proc_dir_entry *entry;
    entry = proc_create(PROCFS_NAME, 0, NULL, &proc_file_ops);
    if (!entry) {
        pr_err("Failed to create /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }
    pr_info("get_pgfaults: Module loaded successfully\n");
    return 0;
}

static void __exit get_pgfaults_exit(void){
    remove_proc_entry(PROCFS_NAME, NULL);
    pr_info("get_pgfaults: Module unloaded\n");
}

module_init(get_pgfaults_init);
module_exit(get_pgfaults_exit);
