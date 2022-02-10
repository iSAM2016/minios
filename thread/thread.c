#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"

#define PG_SIZE 4096
/*****
 * 由 kernel_therad 去执行 function(func_arg)
 * 进入函数kernel_thread 时， 栈顶出是返回地址，因此栈顶+4的位置保存的是function
 * 栈顶+8保存的是func_arg
 */

static void kernel_thread(thread_func *function, void *func_arg)
{
    function(func_arg);
}
/**
 * @brief 初始化线程 thread_stack, 将待执行的函数和参数放到 thread_stack 中相应的位置
 *
 * @param pthread
 * @param function
 * @param func_arg
 */
void thread_create(struct task_struct *pthread, thread_func function, void *func_arg)
{ /**
   * 先预留中断使用栈的空间，可见thread.h定义的结构
   */
    pthread->self_kstack -= sizeof(struct intr_stack);

    /**
     * 在预留出线程空间，可见thread.h中定义
     */
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack *kthread_stack = (struct thread_stack *)pthread->self_kstack;
    kthread_stack->ebp = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

// 初始化线程基本信息
// 待初始化线程指针（PCB），线程名称，线程优先级
void init_thread(struct task_struct *pthread, char *name, int prio)
{
    memset(pthread, 0, sizeof(*pthread)); //清零
    strcpy(pthread->name, name);          //给线程的名字赋值

    pthread->status = TASK_RUNNING; //线程的状态
    pthread->status = TASK_READY;

    // self_kstack 是线程自己在内核态下使用的栈顶地址
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + PG_SIZE);
    pthread->stack_magic = 0x19870916; // 自定义魔数
}
// 线程所执行的函数是 function(func_arg)
struct task_struct *thread_start(char *name,           //线程名
                                 int prio,             //优先级
                                 thread_func function, //要执行的函数
                                 void *func_arg)       //函数的参数
{
    // PCB 都位于内核空间, 包括用户进程的 PCB 也是在内核空间
    // 由于 get_kernel_page 返回的是页的起始地址， 故 thread 指向的是 PCB 的最低地址。
    struct task_struct *thread = get_kernel_pages(1); //申请一页内核空间存放PCB

    init_thread(thread, name, prio);           //初始化线程
    thread_create(thread, function, func_arg); //创建线程

    asm volatile("movl %0, %%esp; \
                  pop %%ebp; \
                  pop %%ebx; \
                  pop %%edi; \
                  pop %%esi; \
                  ret;" ::"g"(thread->self_kstack)
                 : "memory");

    return thread;
}
