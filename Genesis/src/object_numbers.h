#ifndef ANATHEMA_OBJECT_NUMBERS_H
#define ANATHEMA_OBJECT_NUMBERS_H
#include <limits.h>
/* number_fuzzy has an int API. Make wide template/level conversion explicit. */
static inline int object_fuzzy_input(long value)
{
    return value<INT_MIN?INT_MIN:value>INT_MAX?INT_MAX:(int)value;
}
/* Positive factory factors; never wrap an excessive price into a cheap item. */
static inline long object_cost_bounded(long base,long first,long second)
{
    long product;
    if(base<0 || first<0 || second<0 ||
       __builtin_mul_overflow(base,first,&product) ||
       __builtin_mul_overflow(product,second,&product)) return LONG_MAX;
    return product;
}
#endif
