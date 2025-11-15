#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/binfmts.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/bitmap.h>

static char *programPath = "";
static char *policyPath = "";

module_param(programPath, charp, 0444);
MODULE_PARM_DESC(programPath, "Path to the target program.");
module_param(policyPath, charp, 0444);
MODULE_PARM_DESC(policyPath, "Path to the policy.");

extern void (*hook_ptr)(int);

struct nfaTranstion {
    int fromState;
    int toState;
    int symbol;
};

struct nfa {
    int numStates;
    int startState;
    int numFinalStates;
    int *finalStates;
    int numTransitions;
    struct nfaTranstion *transitions;
};

struct policyHeader {
    int numStates;
    int startState;
    int numFinalStates;
    int numTransitions;
};

static struct nfa *activePolicy = NULL;
static int numStates = 0;
static unsigned long *frontier = NULL;
static pid_t monitoredPid = 0;

void libcMonitorHook(int libcallno);
static int handler_pre(struct kprobe *p, struct pt_regs *regs);
static int handler_exit(struct kprobe *p, struct pt_regs *regs);
static struct nfa* loadNFA(const char* path);
static void freeNFA(void);

static struct kprobe kp = {
    .symbol_name = "bprm_execve",
    .pre_handler = handler_pre,
};

static struct kprobe kp_exit = {
    .symbol_name = "do_exit",
    .pre_handler = handler_exit,
};

void libcMonitorHook(int libcallno) {

    pid_t pid = current->pid;
    unsigned long *newFrontier = NULL;
    int i;

    if (activePolicy == NULL) 
        return;

    if (monitoredPid == 0)
        monitoredPid = pid;

    if (monitoredPid != pid) {
        send_sig(SIGKILL, current, 0);
        return;
    }

    newFrontier = bitmap_zalloc(numStates, GFP_KERNEL);
    if(!newFrontier) {
        send_sig(SIGKILL, current, 0);
        return;
    }

    for(i = 0; i < activePolicy->numTransitions; i++) {
        struct nfaTranstion *t = &activePolicy->transitions[i];

        if(test_bit(t->fromState, frontier) && t->symbol == libcallno) {
            set_bit(t->toState, newFrontier);
        }
    }

    if(bitmap_empty(newFrontier, numStates)) {
        printk(KERN_ALERT "Monitor: POLICY VIOLATION! PID %d: No valid transition for libcall %d.\n",
               pid, libcallno);
        bitmap_free(newFrontier);
        send_sig(SIGKILL, current, 0);
        return;
    }

    bitmap_free(frontier);
    frontier = newFrontier;
}

static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    struct linux_binprm *bprm = (struct linux_binprm *)regs->di;

    if (strcmp(bprm->filename, programPath) != 0 || activePolicy != NULL) {
        return 0;
    }

    printk(KERN_INFO "Monitor: Monitoring program %s\n", programPath);
    printk(KERN_INFO "Monitor: Loading NFA policy from %s\n", policyPath);

    activePolicy = loadNFA(policyPath);
    if (activePolicy == NULL) {
        printk(KERN_ERR "Monitor: Failed to load NFA policy.\n");
        return 0;
    }

    numStates = activePolicy->numStates;
    frontier = bitmap_zalloc(numStates, GFP_KERNEL);
    if (!frontier) {
        freeNFA();
        return 0;
    }

    set_bit(activePolicy->startState, frontier);
    monitoredPid = 0;

    return 0;
}

static int handler_exit(struct kprobe *p, struct pt_regs *regs) {
    pid_t pid = current->pid;

    if (activePolicy != NULL && monitoredPid == pid) {
        printk(KERN_INFO "Monitor: Monitored PID %d is exiting.\n", pid);
        freeNFA();
    }

    return 0;
}

static int __init monitor_start(void) {
    int ret;

    hook_ptr = libcMonitorHook;

    ret = register_kprobe(&kp);
    if (ret < 0) {
        hook_ptr = NULL;
        return ret;
    }

    ret = register_kprobe(&kp_exit);
    if (ret < 0) {
        unregister_kprobe(&kp);
        hook_ptr = NULL;
        return ret;
    }

    printk(KERN_INFO "Monitor: Module Loaded.\n");
    return 0;
}

static void __exit monitor_end(void) {
    unregister_kprobe(&kp);
    unregister_kprobe(&kp_exit);
    hook_ptr = NULL;
    freeNFA();
    printk(KERN_INFO "Monitor: Module Unloaded.\n");
}

module_init(monitor_start);
module_exit(monitor_end);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Abhishek Pandey");
MODULE_DESCRIPTION("A Linux Kernel Module for Monitoring Library Calls.");

static struct nfa* loadNFA(const char* path) {
    struct file *fp;
    struct policyHeader header;
    struct nfa *policy = NULL;
    int ret;
    loff_t pos = 0;

    fp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(fp)) {
        printk(KERN_ERR "Monitor: Failed to open policy file %s (ERROR_CODE %ld)\n",
               path, PTR_ERR(fp));
        return NULL;
    }

    ret = kernel_read(fp, &header, sizeof(struct policyHeader), &pos);
    if (ret != sizeof(header)) {
        printk(KERN_ERR "Monitor: Failed to read policy header.\n");
        filp_close(fp, NULL);
        return NULL;
    }

    policy = kmalloc(sizeof(struct nfa), GFP_KERNEL);
    if (!policy) {
        filp_close(fp, NULL);
        return NULL;
    }

    policy->numStates = header.numStates;
    policy->startState = header.startState;
    policy->numFinalStates = header.numFinalStates;
    policy->numTransitions = header.numTransitions;

    if(policy->numFinalStates > 0) {
        size_t finalSize = policy->numFinalStates * sizeof(int);
        policy->finalStates = kmalloc(finalSize, GFP_KERNEL);
        if (!policy->finalStates) {
            kfree(policy);
            filp_close(fp, NULL);
            return NULL;
        }
        kernel_read(fp, policy->finalStates, finalSize, &pos);
    } else {
        policy->finalStates = NULL;
    }

    if(policy->numTransitions > 0) {
        size_t transSize = policy->numTransitions * sizeof(struct nfaTranstion);
        policy->transitions = kmalloc(transSize, GFP_KERNEL);
        if (!policy->transitions) {
            kfree(policy->finalStates);
            kfree(policy);
            filp_close(fp, NULL);
            return NULL;
        }
        kernel_read(fp, policy->transitions, transSize, &pos);
    } else {
        policy->transitions = NULL;
    }

    printk(KERN_INFO "Monitor: Policy Loaded Successfully.\n");
    filp_close(fp, NULL);

    return policy;
}

static void freeNFA(void)
{
    if (activePolicy) {
        kfree(activePolicy->transitions);
        kfree(activePolicy->finalStates);
        kfree(activePolicy);
        activePolicy = NULL;
    }
    if(frontier) {
        bitmap_free(frontier);
        frontier = NULL;
    }
    numStates = 0;
    monitoredPid = 0;
}
