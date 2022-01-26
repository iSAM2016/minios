#include "print.h"
#include "init.h"
#include "debug.h"

void main(void)
{
   put_str("I am kernel\n");
   init_all();
   ASSERT(1 == 2);
   // asm volatile("sti"); // 为演示中断处理,在此临时开中断

   void *addr = get_kernel_pages(5);
   put_str("\n get_kernel_page start vaddr is:");
   put_int((uint32_t)addr);
   put_str("\n");
   while (1)
      ;
   return 0;
}
