#ifndef OAK_STDARG_H
#define OAK_STDARG_H

typedef char *va_list;

/**
 *  @brief  获取参数列表的第二个参数
 *  @param  ap  用于指向参数的指针
 *  @param  v  参数列表的第一个参数
 */
#define va_start(ap, v) (ap = (va_list) & v + sizeof(char *))

/**
 *  @brief  获取参数列表下一个参数
 *  @param  ap  用于指向参数的指针
 *  @param  t  要获取的参数的类型
 */
#define va_arg(ap, t) (*(t *)((ap += sizeof(char *)) - sizeof(char *)))

/**
 *  @brief  清空指向参数的指针
 *  @param  ap  用于指向参数的指针
 */
#define va_end(ap) (ap = (va_list)0)

#endif // !OAK_STDARG_H
