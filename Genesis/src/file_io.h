#ifndef ANATHEMA_FILE_IO_H
#define ANATHEMA_FILE_IO_H
#include <stdio.h>
/* Include after merc.h for NULL_FILE. The reserve is supplementary, not data. */
extern FILE *fpReserve;
static inline void file_reserve_close(void)
{
    FILE *owned=fpReserve;
    fpReserve=NULL;
    if(owned!=NULL) fclose(owned);
}
static inline void file_reserve_open(void)
{
    /* A diagnostic may already have reopened it during a reader operation. */
    file_reserve_close();
    fpReserve=fopen(NULL_FILE,"r");
}
#endif
