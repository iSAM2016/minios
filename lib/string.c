#include "string.h"
#include "global.h"
#include "debug.h"
// typedef unsigned char uint8_t;
// void * 是一个跳跃力未定的指针
/**
 * @brief  将dst_起始size 个字节为value
 *  内存数据的初始化
 * @param dst_  目标字符
 * @param value 替换值
 * @param size  要替换的长度
 */
void memset(void *dst_, uint8_t value, uint32_t size)
{
    ASSERT(dst_ == null);
    uint8_t *des = (uint8_t *)dst_;
    while (size-- > 0)
    {
        *dst++ = value;
    }
}
/**
 * @brief 数据拷贝 将src_起始的size个字节 复制到dst_ 
 *  拷贝
 * @param des_ 目标内存
 * @param src_  起始内存
 * @param size  个数
 */
void memcpy(void *des_, const void *src_, uint32_t size)
{
    ASSERT(des_ != NULL && src_ != NULL);
    uint8_t *dst = des_;
    const uint8_t *src = src_;
    while (size-- > 0)
    {
        *dst++ = *src++;
    }
}

/**
 * @brief 
 * 连续比较以地址 a_和地址 b_开头的 size 个字节，若相等则返回 0， 若 a_大于 b_，返回+1，否则返回−1 
 * 
 * @param a_ 
 * @param b_ 
 * @param size 
 * @return int  
 */
int memcmp(const void *a_, const void *b_, uint32_t size)
{
    const char *a = a_;
    const char *b = b_;
    ASSERT(a != NULL || b != NULL);
    while (size-- > 0)
    {
        if (*a != *b)
        {
            return *a > *b ? 1 : -1;
        }
        a++;
        b++;
    }
    return 0;
}
/**
 * @brief 将字符串常src 复制到dst_
 * 
 * @param dst_   目标数据
 * @param src_  源数据
 * @return char*  返回一个新的字符
 */
char *strcp(char *dst_, const char *src_)
{
    ASSERT(dst_ != NULL && src_ != NULL);
    char *r = dst_; // 用来返回目的的字符串起始地址
    // 按照字节地址顺序进行拷贝
    while ((*dst_++ = *src_++))
        ;
    return r;
}
/**
 * @brief 字符拷贝
 * 
 * @param str 源字符
 * @return uint32_t 
 */
uint32_t strlen(const char *str)
{
    ASSERT(str != NULL);
    const char *p = str;
    // 遍历到最后一个字符
    while (*p++)
        ;
    return p - str - 1; // 因为 *p++ 在最后一次会多加一次1
}
/**
 * @brief 比较两个字符串 若a_中的字符大于b_的字符返回1，相等时返回0 否则返回-1
 * 
 * @param a 
 * @param b 
 * @return int8_t 
 */
int8_t strcmp(const char *a, const char *b)
{
    ASSERT(a != NULL && b != NULL);
    // 直到 找到不相同的字符
    while (*a != 0 && *a == *b)
    {
        a++;
        b++;
    }
    /* 如果*a 小于*b 就返回−1，否则就属于*a 大于等于*b 的情况。
在后面的布尔表达式"*a > *b"中，* 若*a 大于*b，表达式就等于 1， 否则表达式不成立，也就是布尔值为 0，恰恰表示*a 等于*b */
    return *a < *b ? -1 : *a > *b;
}
/**
 * @brief  从左往右查询字符串
 * 
 * @param str 
 * @param ch 
 * @return char* 
 */
char *strchr(const char *str, const uint8_t ch)
{
    ASSERT(str != NULL);
    while (*str != 0)
    {
        if (*str == ch)
        {
            return (char *)str;
        }
        str++;
    }
    return NULL;
}
/* 从后往前查找字符串 str 中首次出现字符 ch 的地址*/
char *strrchr(const char *str, const uint8_t ch)
{
    ASSERT(str != NULL);
    const char *last_char = NULL;
    /* 从头到尾遍历一次，若存在 ch 字符，last_char 总是该字符最后一次      出现在串中的地址(不是下标，是地址)*/
    while (*str != 0)
    {
        if (*str == ch)
        {
            last_char = str;
        }
        str++;
    }
    return (char *)last_char;
}

/* 将字符串 src_拼接到 dst_后，返回拼接的串地址 */
char *strcat(char *dst_, const char *src_)
{
    ASSERT(dst_ != NULL && src_ != NULL);
    char *str = dst_;
    while (*str++)
        ;
    --str; // 别看错了，--str 是独立的一句，并不是 while 的循环体
    while ((*str++ = *src_++))
        ; //当*str 被赋值为 0 时           //也就是表达式不成立，正好添加了字符串结尾的 0
    return dst_;
}
/* 在字符串 str 中查找字符 ch 出现的次数 */ uint32_t strchrs(const char *str, uint8_t ch)
{
    ASSERT(str != NULL);
    uint32_t ch_cnt = 0;
    const char *p = str;
    while (*p != 0)
    {
        if (*p == ch)
        {
            ch_cnt++;
        }
        p++;
    }
    return ch_cnt;
}