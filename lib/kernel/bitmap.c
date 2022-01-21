#include "bitmap.h"
#include "stdint.h"
#include "string.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"
// 注意位图为0 代表可用 1 代表不可用
/**
 * @brief 将位图btmp初始化
 *  根据位图的字节大小 btmp_bytes_len 将位图的每一个字节用 0 来填充。
 * @param btmp 位图指针 
 */
void binmap_init(struct bitmap *btmp)
{
    // 内存数据的初始化 填充的是0
    memset(btmp->bits, 0, btmp->btmp_tytes_len);
}
/**
 * @brief 判断位图btmp中bit_idx 位是否为1 若为1 则返回true 否则返回false
 * 
 * @param btmp 位图指针
 * @param bit_idx  位索引
 * @return true 
 * @return false 
 */
bool bitmap_scan_test(struct bitmap *btmp, uint32_t bit_idx)
{
    uint32_t byte_idx = bit_idx / 8; // 向下取整用于索引数组下标
    uint32_t bit_odd = bit_idx % 8;  // 取余用于索引数组内的位
    return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd));
}
/**
 * @brief 在位图中连续申请cnt 个位置，成功，则返回其起始下标，失败返回-1
 * 
 * @param btmp 位图指针
 * @param cnt 位的个数
 * @return int 
 */
int bitmap_scan(struct bitmap *btmp, uint32_t cnt)
{
    uint32_t idx_byte = 0; // 用于记录第一个空闲位所在的字节
    // 每个字节进行判断 一个字节都被占用是 1111 1111（二进制） 用16进制 表示就是 0xff
    // 1 表示已经分配 若为0xff 则表示该字节已经没有空位， 向下一个字节查找
    while ((0xff == btmp->bits[idx_byte]) && (idx_byte < btmp->btmp_tytes_len))
    {
        idx_byte++;
    }
    // 超界判断
    ASSERT(idx_byte < btmp->btmp_tytes_len);
    // 若该内存池找不到可以使用的空间
    if (idx_byte == btmp->btmp_tytes_len)
    {
        return -1;
    }
    // 此时已经找到了第一个空闲的字节。 但是不确定在哪个比特位
    // 若在位图数组内找到了空闲位 在该字节内 逐位置的对比 返回空闲位置的索引
    int idx_bit = 0;
    while ((uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_byte])
    {
        idx_bit++;
    }
    int bit_idx_start = idx_byte * 8 + idx_bit; // 空闲位在位图内的下标

    if (cnt == 1)
    { // 申请一个空闲位, 直接返回地址
        return bit_idx_start;
    }
    // 如果申请的空闲位置大于1 还要继续查找空闲位置
    // 找到右边界
    uint32_t bit_left = (btmp->btmp_tytes_len * 8 - bit_idx_start);
    //  用于记录位图中下一个待查找的位，就是第一个空闲位置的下一个位置
    uint32_t next_bit = bit_idx_start + 1;
    uint32_t count = 1; // 用于找到空闲的位置

    bit_idx_start = -1; // 先将其置为-1 若找不到连续的位就直接返回

    while (bit_left-- > 0)
    {
        if (!(bitmap_scan_test(btmp, next_bit)))
        { // next_bit 为0 没有被使用
            count++;
        }
        else
        { // next_bit 为1 已经被使用 如果遇到比特位被占用 就从新开始
            count = 0;
        }
        if (count == cnt) // 若找到连续的cnt个空位
        {
            bit_idx_start = next_bit - cnt + 1;
            break;
        }
        // 下一比特位进行判断
        next_bit++;
    }
    // 如果没有找到的话 就为-1
    return bit_idx_start;
}
/**
 * @brief 将图btmp的bit_idx 位设置为value
 * 
 * @param btmp 位图指针
 * @param bit_idx 索引位
 * @param value 要填充的值
 */
void bitmap_set(struct bitmap *btmp, uint32_t bit_idx, int8_t value)
{
    ASSERT((value == 0) || (value == 1));
    uint32_t byte_idx = bit_idx / 8; // 向下取整用户索引素组下标
    uint32_t bit_odd = bit_idx % 8;  // 取余用于索引数组内的位
    // 一般都会使用这个0x1 这样的数对字节中的位操作 将1 任意移动后在取反，或者先取反在位移， 可用来对位置0 操作
    if (value)
    { // 如果为value 为1
        btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
    }
    else
    { // 如果为0 按字节进行处理 更改其中的一个值
        btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
    }
}

// 1. 字节指定位置数值为1
//  将23的3位值为1
//  采用左移和按位与的操作
// a:23(00010111)
// b:1(00000001) 左移3位 00001000
// a|b: 00010111|00001000 = 00011111
// 用c 语言表示 a |= (1<3)

// 2. 字节指定位置为0
//  将23的4位值为0
//  采用左移和取反 和按位与的操作
// a:23(00010111)
// b:1(00000001) 左移4位 00010000 在取反11101111
// a&b: 00010111|11101111 = 00010111
// 用c 语言表示 a&= ~(1<4)
