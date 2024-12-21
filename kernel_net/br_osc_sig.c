#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/signal.h>
#include <linux/sched/signal.h>

#include "br_private.h"
#include "br_private_stp.h"

//#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
//#define HAVE_PROC_OPS
//#endif

#define PROC_FILE_NAME "osc_process_pid"

static struct proc_dir_entry *proc_file;
static pid_t osc_process_pid = -1; // 用户进程 PID

// 注册用户进程 PID
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos) {
    char pid_str[16] = {0};
    long pid = -1;

    if (count >= sizeof(pid_str))
        return -EINVAL;

    if (copy_from_user(pid_str, buffer, count))
        return -EFAULT;
    pid_str[count] = '\0';

    if (kstrtol(pid_str, 10, &pid) < 0)
        return -EINVAL;

    osc_process_pid = (pid_t)pid;
    printk(KERN_DEBUG"Kernel process registered with PID: %d\n", osc_process_pid);
    return count;
}

#ifdef HAVE_PROC_OPS 
static const struct proc_ops proc_file_ops = {
    .proc_write = proc_write,
};
#else
static const struct file_operations proc_file_ops = {
    .write = proc_write, 
};
#endif

#if 0
// 内核触发函数：发送信号给用户态
void trigger_osc_function(void) {
    struct task_struct *task;

    printk(KERN_DEBUG"----kernel trigger_user_function test111-----\r\n");

    if (osc_process_pid < 0) {
        printk(KERN_DEBUG"No user process registered.\n");
        return;
    }

    // 获取目标进程的 task_struct
    task = pid_task(find_get_pid(osc_process_pid), PIDTYPE_PID);
    if (!task) {
        printk(KERN_DEBUG"Failed to find user process with PID: %d\n", osc_process_pid);
        return;
    }

    // 向目标进程发送信号 SIGUSR1
    if (send_sig(SIGUSR1, task, 0) < 0)
        printk(KERN_DEBUG"Failed to send signal to process PID: %d\n", osc_process_pid);
    else
        printk(KERN_DEBUG"Signal sent to process PID: %d\n", osc_process_pid);

    printk(KERN_DEBUG"----kernel trigger_user_function test222-----\r\n");
}
#endif

void trigger_osc_function(u16 port_id, u8 state) {
    struct pid *pid_struct;
    struct task_struct *task;
    struct siginfo info;
    //u16 port_id = 0;
    //u8 state = 0;

    if (osc_process_pid < 0) {
        printk(KERN_DEBUG"No user process registered.\n");
        return;
    }

    // 获取 PID 的结构体
    pid_struct = find_get_pid(osc_process_pid);
    if (!pid_struct) {
        printk(KERN_DEBUG"Invalid PID: %d\n", osc_process_pid);
        return;
    }

    // 获取进程的 task_struct
    task = pid_task(pid_struct, PIDTYPE_PID);
    if (!task) {
        printk(KERN_DEBUG"Failed to find task for PID: %d\n", osc_process_pid);
        return;
    }

    // 填充信号信息
    memset(&info, 0, sizeof(struct siginfo));
    info.si_signo = SIGUSR1;     // 信号类型
    info.si_code = SI_QUEUE;    // 用户自定义信号
    info.si_int = (port_id << 8) | state;// 数据

    // 向用户态进程发送信号
    if (send_sig_info(SIGUSR1, (struct kernel_siginfo *)&info, task) < 0) {
        printk(KERN_DEBUG"Failed to send signal to user process\n");
    } else {
        printk(KERN_DEBUG"Signal sent to user process (PID: %d) port info 0x%x\n", osc_process_pid, info.si_int);
    }
}

// 模块加载时创建 /proc 文件
static int __init signal_module_init(void) {
    proc_file = proc_create(PROC_FILE_NAME, 0666, NULL, &proc_file_ops);
    if (!proc_file)
        return -ENOMEM;

    pr_info("/proc/%s created\n", PROC_FILE_NAME);
    return 0;
}

// 模块卸载时删除 /proc 文件
static void __exit signal_module_exit(void) {
    proc_remove(proc_file);
    pr_info("/proc/%s removed\n", PROC_FILE_NAME);
}

module_init(signal_module_init);
module_exit(signal_module_exit);

MODULE_AUTHOR("liujia");
MODULE_LICENSE("GPL");

