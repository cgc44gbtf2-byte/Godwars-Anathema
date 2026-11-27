#include <ctype.h>
#include <stdio.h>
#include "file_string.h"

/* Preserve the legacy leading-space, CR and LF->LFCR wire format. */
enum file_string_status fread_tilde_checked(FILE *fp,char *out,size_t capacity)
{
    size_t length=0,reads=0;
    int c;
    if(out&&capacity)out[0]='\0';
    if(!fp||!out||!capacity)return FILE_STRING_FORMAT;
    if(ferror(fp))return FILE_STRING_IO;
    do {
        if(reads++==FILE_STRING_MAX_READS)return FILE_STRING_LIMIT;
        c=getc(fp);
        if(c==EOF)return ferror(fp)?FILE_STRING_IO:FILE_STRING_FORMAT;
    } while(isspace((unsigned char)c));
    for(;;) {
        if(c=='~'){out[length]='\0';return FILE_STRING_OK;}
        if(c==0){out[0]='\0';return FILE_STRING_FORMAT;}
        if(c!='\r'){
            size_t needed=c=='\n'?2:1;
            if(needed>=capacity-length){out[0]='\0';return FILE_STRING_LIMIT;}
            out[length++]=(char)c;
            if(c=='\n')out[length++]='\r';
        }
        if(reads++==FILE_STRING_MAX_READS){out[0]='\0';return FILE_STRING_LIMIT;}
        c=getc(fp);
        if(c==EOF){out[0]='\0';return ferror(fp)?FILE_STRING_IO:FILE_STRING_FORMAT;}
    }
}
