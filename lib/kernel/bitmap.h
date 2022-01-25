//  位图的结构
#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_Ha
#include "global.h"
#define BITMAP_MASK 1 // 常量 f用来在位图中逐位判断。 通过按位与&来判断是否为1
// 位图的结构
struct bitmap
{
    uint32_t btmp_tytes_len; // 位图的字节长度-在遍历位图的时候，整体上以字节为单位，细节上是以位为单位， unsigned int 4个字节 32位
    uint8_t *bits;           // 位图指针 unsigned char 1个字节8位
};
void bitmap_init(struct bitmap *bitmap);
bool bitmap_scan_test(struct bitmap *bimp, uint32_t bit_idx);
int bitmap_scan(struct bitmap *btmp, uint32_t cnt);
void bitmap_set(struct bitmap *btmp, uint32_t bit_idx, int8_t value);
#endif
