#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H
void painc_spin(char *filename, int line, const char *func, const char *condition);
/***************************  __VA_ARGS__  *******************************  * __VA_ARGS__ 是预处理器所支持的专用标识符
 * 代表所有与省略号相对应的参数。
 * ... 代表定义 的红其参数可变
 * 将
 */
#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef NDEBUG
#define ASSERT(CONDITION) ((void)0)
#else
// 定义多行洪使用\ 换行
#define ASSERT(CONDITION)  \
    \if (CONDITION) {}     \
    else                   \
    { \ 
/* 符号#让编译器将宏的参数转化为字符串字面量  跳转到 panic_spin*/                   \
        PANIC(#CONDITION); \
        \  
                    \
    \   
}
#endif /*NDEBUF*/
#endif //__KERNEL_DEBUG_H