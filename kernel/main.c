#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"

void k_thread_a(void *);

void main(void)
{
   put_str("I am kernel\n");
   init_all();
   // ASSERT(1 == 2);
   // asm volatile("sti"); // 为演示中断处理,在此临时开中断
   // 从内核物理内存池中申请内存，成功返回虚拟地址，失败返回NULL
   // void *addr = get_kernel_pages(5);
   // put_str("\n get_kernel_page start vaddr is:");
   // put_int((uint32_t)addr);

   thread_start("k_thread_a", 31, k_thread_a, "hello world\n");

   put_str("\n");
   while (1)
      ;
   return 0;
}

void k_thread_a(void *arg)
{
   char *para = arg;
   while (1)
   {
      put_str(para);
   }
}
