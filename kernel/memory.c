#include "memory.h"
#include "stdint.h"
#include "print.h"

// 定义了一个页面的尺寸 4096 就是4kb
#define PG_SIZE 4096
/******************* 位图地址 *******************
 *  因为oxc009f000 是内核主线程栈顶， 0xc009e000 是内核主线程的pcd
 * 一个页框大小的位图可表示128MB 内存。位图安置安排在地址 0xc0009a000 
 * 这样本系统最大支持 4个页框的位图  就是512MB
 */
#define MEM_BITMAP_BASE 0xc009a000
/**
 * @brief  0xc0000000 是内核从虚拟地址3G 开始的地方 
 * 0x100000意指跨过低端1MB 内存， 时虚拟地址在逻辑上的连续
 */
#define K_HEAP_START 0xc0100000

//  内存池结构 ，主要生成两个实例，用于管理内核内存池和用户内存池
struct pool
{
    struct bitmap pool_bitmap; // 本内存池用到的位图结构，用于管理物理内存
    uint32_t phy_addr_start;   // 本内存池所管理物理内存的起始地址
    uint32_t pool_size;        // 本内存池字节容量
};

struct pool kernel_pool, user_pool; // 生成内核内存池和用户内存池
struct virtual_addr kernel_vaddr;   //此机构用来给内核分配虚拟地址
/**
 * @brief 初始化内存池
 * 
 * @param all_mem 
 */
static void mem_pool_init(uint32_t all_mem)
{
    put_str("mem_pool_init start\n");
    uint32_t page_table_size = PG_SIZE * 256;
    // 页表大小 = 1 页的页目录表 + 第0 和第768个页目录项指向统一页表+ 第769-1022 个页目录项共指向254个页表，共256个页

    uint32_t used_mem = page_table_size + 0x100000;
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;
    //  1页为4kb 不管总内存是不是4K 的倍数
    // 对于以页为单位的内存分配策略不足1页的内存不用考虑了
}
