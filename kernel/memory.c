#include "memory.h"
#include "bitmap.h"
#include "stdint.h"
#include "global.h"
#include "debug.h"
#include "print.h"
#include "string.h"

// 定义了一个页面的尺寸 4096 就是4kb
#define PG_SIZE 4096
/******************* 位图地址 *******************
 * 内存位图的基址
 * 虚拟地址
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

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22) // 页目录 返回虚拟地址的高十位
#define PTX_IDX(addr) ((addr & 0x003ff000) >> 12) // 页表项 返回虚拟地址的中间10 位

//  内存池结构 ，主要生成两个实例，用于管理内核内存池和用户内存池
struct pool
{
    struct bitmap pool_bitmap; // 本内存池用到的位图结构，用于管理物理内存
    uint32_t phy_addr_start;   // 本内存池所管理物理内存的起始地址
    uint32_t pool_size;        // 本内存池字节容量
};

struct pool kernel_pool, user_pool; // 生成内核内存池和用户内存池
struct virtual_addr kernel_vaddr;   //此结构用来给内核分配虚拟地址
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
     * ********************************* 下面初始化内核虚拟地址的位图  按实际五路内存大小生成数组***************************8
     */
    // 用于维护内核堆的虚拟地址，所以要和内核内存池大小一致
    kernel_vaddr.vaddr_bitmap.btmp_tytes_len = kbm_length;
    /**
     * 位图的数组指向一块为使用的内存， 目前定位在内核内存池和用户内存池之外
     */
    kernel_vaddr.vaddr_bitmap.bits = (void *)(MEM_BITMAP_BASE + kbm_length + ubm_length);
    // 内核虚拟地址池的起始地址为 K_HEAP_START
    kernel_vaddr.vaddr_start = K_HEAP_START; //0xc0100000
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("  mem_pool_init done\n");
}
/**
 * @brief pf表示的虚拟内存池中申请pg_cnt个虚拟页, 成功则返回虚拟页的起始地址，失败则返回NULL
 * 
 * @param pf 内存池标记
 * @param pg_cnt  申请的页数
 * @return void* 
 */
static void *vaddr_get(enum pool_flags pf, uint32_t pg_cnt)
{
    int vaddr_start = 0,    //存储分配的起始虚拟地址
        bit_idx_start = -1; //空闲位在位图内的下标
    uint32_t cnt = 0;
    if (pf == PF_KERNEL) //  PF_KERNEL=1 内核内存池 TODO: enmu 使用
    {
        // 在位图中连续申请cnt 个位置
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if (bit_idx_start == -1)
        {
            return NULL;
        }
        while (cnt < pg_cnt)
        {
            //将图btmp的bit_idx 位设置为1
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
        }
        // 用虚拟内存池的起始地址 + 加上起始位索引*8
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    }
    else
    {
        // 用户内存池
    }
    // 虚拟页的的起始地址
    return (void *)vaddr_start;
}
//得到虚拟地址 vaddr 对应的 pte (页表)指针
uint32_t *pte_ptr(uint32_t vaddr)
{
    // 先访问到页表自己
    // 再用页目录项 pde（页目录内页表的索引）作为pte的索引访问到页表
    // 再用pte的索引作为页内偏移
    uint32_t *pte = (uint32_t *)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
    //第一步：0xffc00000 是取出第1023个页目录项进行索引，其实就是页目录表的物理地址
    //第二步：((vaddr & 0xffc00000) >> 10) 是将原来vaddr的前10位取出，放在中间10位的位置上，用来获取 pte 的
    //第三步：PTE_IDX(vaddr) * 4 会被当作物理偏移直接加上，而不会像其前面10位会被cpu自动*4再加上，所以这里手动*4，获取PTE索引，得到PTE物理地址
    return pte;
}
/**
 * @brief 得到虚拟地址vaddr 对应的pde(页目录)的虚拟指针
 * 
 * @param vaddr  返回的也是虚拟地址
 * @return uint32_t* 
 */
uint32_t *pde_ptr(uint32_t vaddr)
{
    // 为啥是0xfffff000请看手绘图
    //在 loader.S 中创建页表的时候，在最后一个页目录项里写的是页目录表自己的物理地址，目的就是为了通过此页目录项编辑页表

    //0xfffff 用来访问到页表本身所在的地址
    //前10位是1023，是页目录表的物理地址
    //中10位是1023，索引到的还是页目录表的物理地址
    //后12位是addr的前10位*4，也就是页目录表的索引
    uint32_t *pde = (uint32_t *)((0xfffff000) + PDE_IDX(vaddr) * 4); //\*4 固定算法
    return pde;
}
/**
 * @brief 在m_pool 指向的物理内存池中分配一个物理页，
 * 成功返回页框的物理地址，失败则返回NULL
 * 
 * @param m_pool 
 * @return void* 
 */
static void *palloc(struct pool *m_pool)
{
    /* 扫描或设置位图要保证原子操作*/
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1); // 找到一个物理页面
    if (bit_idx == -1)
    {
        return NULL;
    }
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1); // 将次位置bit_idx置为1
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
    return (void *)page_phyaddr;
}

// 页表中添加虚拟地址_vaddr与物理地址_page_phyaddr的映射
static void page_table_add(void *_vaddr, void *_page_phyaddr)
{
    uint32_t vaddr = (uint32_t)_vaddr;
    uint32_t page_phyaddr = (uint32_t)_page_phyaddr;
    // 页目录项
    uint32_t *pde = pde_ptr(vaddr);
    // 页表项
    uint32_t *pte = pte_ptr(vaddr);

    //执行*pte，会访问到空的 pde。所以确保pde创建完成后才能执行*pte,否则 会引发page_fault。
    //因此在 *pde为0时，pte只能出现在下面 else语句块中的*pde后面。
    //总之，就是创建pte之前必须先创建好pde才行，不然没法通过pde访问到pte

    //先在页目录内判断目录项的p位， 若为1, 则表示该表已存在
    if (*pde & 0x00000001)
    {
        // 页目录项和页表项的第0位为P 此处哦按段目录项是否存在
        ASSERT(!(*pte & 0x00000001)); //此时pte应该不存在
        if (!(*pte & 0x00000001))
        {
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1); //创建pte
        }
        else
        { //目前执行不到这里
            PANIC("pte repeat");
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
    }
    else
    {
        //页表中用到的页框一律从内核空间分配
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);

        //分配到的物理页地址pde_phyaddr对应的物理 内存清0
        //避免里面的陈旧数据变成了页表项，从而让页表混乱。
        //访间到 pde对应的物理地址，用 pte取高20位便可。
        //因为pte基千该pde对应的物理地址内再寻址，把低12位置0便是该pde对应的物理页的起始。
        memset((void *)((int)pte & 0xfffff000), 0, PG_SIZE);

        ASSERT(!(*pte & 0x00000001));
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    }
}

static void *malloc_page(enum pool_flags pf, uint32_t pg_cnt)
{

    ASSERT(pg_cnt > 0 && pg_cnt < 3840);
    //malloc_page 的原理是三个动作的合成
    // 1 通过vadder_get 在虚拟内存池中申请虚拟地址
    // 2 通过 palloc 在物理内存池中申请物理页
    // 3. 通过page_table_add 将以上得到的虚拟地址和物理地址在页表中完成映射

    void *vaddr_start = vaddr_get(pf, pg_cnt);
    if (vaddr_start == NULL)
    {
        return NULL;
    }

    uint32_t vaddr = (uint32_t)vaddr_start;
    uint32_t cnt = pg_cnt;
    struct pool *mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;

    //因为虚拟地址是连续的，但物理地址不连续，所以逐个映射
    while (cnt-- > 0)
    {
        void *page_phyaddr = palloc(mem_pool);
        if (page_phyaddr == NULL)
        {
            //失败时要将曾经已申请的虚拟地址和
            //物理页全部回滚，在将来完成内存回收时再补充
            return NULL;
        }
        page_table_add((void *)vaddr, page_phyaddr); //在表中逐个做映射
        vaddr += PG_SIZE;
    }
    return vaddr_start;
}

// 从内核物理内存池中申请内存，成功返回虚拟地址，失败返回NULL
void *get_kernel_pages(uint32_t pg_cnt)
{
    void *vaddr = malloc_page(PF_KERNEL, pg_cnt);
    if (vaddr != NULL)
    {
        memset(vaddr, 0, pg_cnt * PG_SIZE);
    }
    return vaddr;
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