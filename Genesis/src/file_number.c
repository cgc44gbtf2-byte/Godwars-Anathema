#include <stdio.h>
#include <ctype.h>
#include "file_number.h"

/* Distinguish EOF from every byte value and the work-limit sentinel. */
static int next_number_byte(FILE *fp, unsigned *remaining)
{
    if (!*remaining) return -2;
    --*remaining;
    return getc(fp);
}

enum file_number_status fread_number_checked(FILE *fp, long *result)
{
    long terms[FILE_NUMBER_MAX_TERMS], value, next;
    unsigned count=0, remaining=FILE_NUMBER_MAX_BYTES;
    int c, negative;
    if (!fp || !result) return FILE_NUMBER_FORMAT;
    if (ferror(fp)) return FILE_NUMBER_IO;
    for (;;) {
        do { c=next_number_byte(fp,&remaining); }
        while(c>=0 && isspace((unsigned char)c));
        negative=c=='-';
        if(c=='+' || c=='-') c=next_number_byte(fp,&remaining);
        if(c==-2) return FILE_NUMBER_LIMIT;
        if(c==EOF || !isdigit((unsigned char)c))
            return ferror(fp)?FILE_NUMBER_IO:FILE_NUMBER_FORMAT;
        value=0;
        do {
            long digit=c-'0';
            if(__builtin_mul_overflow(value,10L,&next) ||
               (negative?__builtin_sub_overflow(next,digit,&value):
                         __builtin_add_overflow(next,digit,&value)))
                return FILE_NUMBER_RANGE;
            c=next_number_byte(fp,&remaining);
        } while(c>=0 && isdigit((unsigned char)c));
        if(ferror(fp)) return FILE_NUMBER_IO;
        if(c==-2) return FILE_NUMBER_LIMIT;
        terms[count++]=value;
        if(c=='|') {
            if(count==FILE_NUMBER_MAX_TERMS) return FILE_NUMBER_LIMIT;
            continue;
        }
        /* Preserve the legacy stream position: consume one ASCII space,
         * put every other delimiter back, and never push EOF back. */
        if(c!=EOF && c!=' ' && ungetc(c,fp)==EOF) return FILE_NUMBER_IO;
        break;
    }
    /* Old recursion was right-associative, not a left-fold or bitwise OR. */
    value=terms[--count];
    while(count) {
        if(__builtin_add_overflow(terms[--count],value,&next))
            return FILE_NUMBER_RANGE;
        value=next;
    }
    *result=value;
    return FILE_NUMBER_OK;
}
