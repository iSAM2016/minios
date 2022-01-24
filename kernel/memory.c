#include "memory.h"
#include "stdint.h"
#include "print.h"

// 定义了一个页面的尺寸 4096 就是4kb
#define PG_SIZE 4096
/******************* 位图地址 *******************
 *  和线程有关系TODO:
 * 因为0xc009f000 是内核主线程栈顶， 0xc009e000 是内核主线程的pcd
 */
#define MEM_BITMAP_BASE 0xc009a000
/**
 * @brief  0xc0000000 是内核从虚拟地址3G 开始的地方 请参考手绘内存图
 * 0x100000意指跨过低端1MB 内存， 时虚拟地址在逻辑上的连续
 * 
 * 动态申请的内存都是在堆空间中完成的，我们为此也定义了内核所用的堆空间，堆也是内存，内存就得有地址，从哪个地址开始分配呢？这就是K_HEAP_START 的作用，堆的起始虚拟地址。
 * 在 loader 中我们已经通过设置页表把虚拟地址 0xc0000000～0xc00fffff 映射到了物理地址 0x00000000～ 0x000fffff（低端 1MB 的内存） ，故我们为了让虚拟地址连续，将堆的起始虚拟地址设为 0xc0100000。
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
 * @brief 初始化物理内存池的相关结构
 * 
 * @param all_mem 机器的总内存容量
 */
static void mem_pool_init(uint32_t all_mem)
{
    put_str("  mem_pool_init start\n");
    // 用来记录页目录表和页表占用的字节大小
    // (参考手绘图)页表大小 = 1 页的页目录表 + 第0 和第768个页目录项指向统一页表+ 第769-1022 个页目录项共指向254个页表，共256个页
    //最后一个页目录项（第1023 个pde） 指向页目录表
    uint32_t page_table_size = PG_SIZE * 256;
    // 用来记录当前已经使用的内存字节数 包括页表大小 和 低端 1MB 内存
    uint32_t used_mem = page_table_size + 0x100000;
    // 用它来存储目前可用的内存字节数
    uint32_t free_mem = all_mem - used_mem;
    //可用内存字节数 free_mem 转换成的物理页数,因为内存池中的内存单位是物理页。
    uint16_t all_free_pages = free_mem / PG_SIZE;
    // 1页为4kb 不管总内存是不是4K 的倍数
    // 对于以页为单位的内存分配策略不足1页的内存不用考虑了

    // 把可用的物页数分为两半。
    // 用来存储分配给内核的空闲物理页
    uint16_t kernel_free_pages = all_free_pages / 2;
    // 用户内存池的空闲物理页数量
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;

    // 记录位图的长度 一个比特代表一个页 一个字节8个页
    uint32_t kbm_length = kernel_free_pages / 8; // 占用字节数量
    // 用户内存池位图的长度
    uint32_t ubm_length = user_free_pages / 8; //占用字节数量

    // 记录内核物理内存池的其实地址
    uint32_t kp_start = used_mem;
    // 记录用户物理内存池的其实地址
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;

    /********************************************** 开始给内存池赋值 **********************/
    //本内存池所管理物理内存的起始地址
    kernel_pool.phy_addr_start = kp_start;
    user_pool.phy_addr_start = up_start;
    // 计算格子内存池中的容量字节数
    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE;

    // 位图字字节长度
    kernel_pool.pool_bitmap.btmp_tytes_len = kbm_length;
    user_pool.pool_bitmap.btmp_tytes_len = ubm_length;
    /************** 内核内存池 和用户内存池位图 ****************
 * 位图是全局的数据 长度不固定
 * 全局或静态的数据需要在编译是知道器其长度
 * 而我们需要根据总能内存大小算出需要多少字节
 * 所以改为指定一块内存来生成位图
 *************************************************************/
    // 内核使用的最高地址是0xc009f000 这是主线程的栈
    // 内核大小预计为70KB 左右
    // 32MB 内存占用的位图是2KB
    // 内核内存池的位图先定在MEM_BITMAP_BASE(0xc009a000) 处
    kernel_pool.pool_bitmap.bits = (void *)MEM_BITMAP_BASE;
    /**
     * 用户内存池的位图紧跟在内核内存池位图之后
     */
    user_pool.pool_bitmap.bits = (void *)(MEM_BITMAP_BASE + kbm_length);
    /********************输出内存池信息**********************/
    put_str("    kernel_pool_bitmap_start:");
    put_int((int)kernel_pool.pool_bitmap.bits);
    put_str(" kernel_pool_phy_addr_start:");
    put_int(kernel_pool.phy_addr_start);
    put_str("\n");
    put_str("    user_pool_bitmap_start:");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str(" user_pool_phy_addr_start:");
    put_int(user_pool.phy_addr_start);
    put_str("\n");

    /**
     * 将位图置0
     */
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);
    /**
     * 下面初始化内核虚拟地址的位图  按实际五路内存大小生成数组
     */
    kernel_vaddr.vaddr_bitmap.btmp_tytes_len = kbm_length;
    // 用于维护内核堆的虚拟地址，所以要和内核内存池大小一致
    /**
     * 位图的数组指向一块为使用的内存， 目前定位在内核内存池和用户内存池之外
     */
    kernel_vaddr.vaddr_bitmap.bits = (void *)(MEM_BITMAP_BASE + kbm_length + ubm_length);
    // 虚拟地址池的其实地址为 K_HEAP_START
    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("  mem_pool_init done\n");
}

/* 内存管理部分初始化入口 */
void mem_init()
{
    put_str("mem_init start\n");
    // 获取内存的总值
    uint32_t mem_bytes_total = (*(uint32_t *)(0xb00));
    mem_pool_init(mem_bytes_total); // 初始化内存池
    put_str("mem_init done\n");
}