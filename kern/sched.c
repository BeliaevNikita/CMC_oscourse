#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/env.h>
#include <kern/monitor.h>

#ifndef trace_sched_my
#define trace_sched_my 0
#endif

struct Taskstate cpu_ts;
_Noreturn void sched_halt(void);

/* Choose a user environment to run and run it */
_Noreturn void
sched_yield(void) {
    /* Implement simple round-robin scheduling.
     *
     * Search through 'envs' for an ENV_RUNNABLE environment in
     * circular fashion starting just after the env was
     * last running.  Switch to the first such environment found.
     *
     * If no envs are runnable, but the environment previously
     * running is still ENV_RUNNING, it's okay to
     * choose that environment.
     *
     * If there are no runnable environments,
     * simply drop through to the code
     * below to halt the cpu */

    // LAB 3: Your code here:

    int64_t current_env_index;
    if (curenv != NULL) {
        current_env_index = curenv - envs;
    }
    else {
        current_env_index = -1; // `i` starts with 1. 1 + (-1) == 0
        if (trace_sched_my) {
            cprintf("sched_yield: curenv == NULL. Will run the env with index 0\n");
        }

    }

    bool found_env_to_switch_to = false;
    for (int64_t i = 1; i < NENV; ++i)
    {
        int64_t curretly_viewed_env_index = (i + current_env_index) % NENV;
        struct Env *curretly_viewed_env = &envs[curretly_viewed_env_index];
        if (curretly_viewed_env->env_status == ENV_RUNNABLE)
        {
            found_env_to_switch_to = true;
            if (trace_sched_my) {
                cprintf("Found a new env to run. Switching...\n");
            }
            env_run(curretly_viewed_env);
        }
    }

    if (!found_env_to_switch_to && (curenv != NULL) && (curenv->env_status == ENV_RUNNING))
    {
        if (trace_sched_my) {
            cprintf("No new env to run. Continue running the old one...\n");
        }
        env_run(curenv); // Not needed but who cares. "Almost a no-op"
    }

    if (trace_sched_my) {
        cprintf("Halt: curenv=%p status=%d\n", curenv, curenv ? curenv->env_status : -1);
    }
    cprintf("Halt\n");

    /* No runnable environments,
     * so just halt the cpu */
    sched_halt();
}

/* Halt this CPU when there is nothing to do. Wait until the
 * timer interrupt wakes it up. This function never returns */
_Noreturn void
sched_halt(void) {

    /* For debugging and testing purposes, if there are no runnable
     * environments in the system, then drop into the kernel monitor */
    int i;
    for (i = 0; i < NENV; i++)
        if (envs[i].env_status == ENV_RUNNABLE ||
            envs[i].env_status == ENV_RUNNING) break;
    if (i == NENV) {
        cprintf("No runnable environments in the system!\n");
        for (;;) monitor(NULL);
    }

    /* Mark that no environment is running on CPU */
    curenv = NULL;

    /* Reset stack pointer, enable interrupts and then halt */
    asm volatile(
            "movq $0, %%rbp\n"
            "movq %0, %%rsp\n"
            "pushq $0\n"
            "pushq $0\n"
            "sti\n"
            "hlt\n" ::"a"(cpu_ts.ts_rsp0));

    /* Unreachable */
    for (;;)
        ;
}
