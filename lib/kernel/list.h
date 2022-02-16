#ifndef __LIB_KERNEL_LIST_LIST_H
#define __LIB_KERNEL_LIST_LIST_H
#include "global.h"

#define offset (struct_type, member)(int)(&((struct type *)0)->member)
#define elem2entry (struct_type, struct_member_name, elem_ptr)(struct_type *)((int)elem_ptr - offset(struct_type, struct_member_name))

/**** 定义链表结构成员结构***
 * 节点中不需要数成员， 只要求前驱和后继节点指针
 */
struct list_elem
{
    struct list_elem *prev; // 前驱节点
    struct list_elem *next; // 后续节点
};
/* 链表结构，用来实现队列*/
struct list
{
    /* head 是队首，是固定不变的，不是第一个元素，第一个元素为haed.next*/
    struct list_elem head;
    /* tail 是队尾 同样是固定不变的*/
    struct list_elem tail;
};
/*定义函数类型 function ，用于在 list traversal 中做回调函数*/

typedef bool(function)(struct list_elem *, int arg);

void list_init(struct list *);
void list_insert_before(struct list_elem *before, struct list_elem *elem);
void list__push(struct list plist, struct list_elem *ele_m);
void list_iterate(struct list *plist);
void list_append(struct list *plist, struct list_elem *elem);
void list_remove(struct list_elem *pelem);
struct list_elem list__pop(struct list *plist);
bool list_empt(struct list *plist);
uint32_t list_len(struct list *plist);
struct list_elem *list_traversal(struct list *plist, function func, int arg);
bool elem_find(struct list *plist, struct list_elem *obj_elem);
#endif
