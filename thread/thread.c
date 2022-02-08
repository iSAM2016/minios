#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"

#define PG_SIZE 4096
/* 由 kernel_therad 去执行 function(func_arg)*/
static void kernel_thread(thread_func *function, void *func_arg)
{
    function(func_arg);
}

void thread_create(struct task_struct *pthread, thread_func function, void *func_arg)
{
    pthread->self_kstack
}