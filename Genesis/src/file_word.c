/* Bounded, caller-owned word parsing shared by boot and session readers. */
#include <ctype.h>
#include <stdio.h>
#include "file_word.h"

enum file_word_status fread_word_checked(FILE *fp, char *word, size_t capacity)
{
    size_t used=0, reads=0;
    int c, quote=0;
    enum file_word_status status=FILE_WORD_LIMIT;
    if (word!=NULL && capacity!=0) word[0]='\0';
    if (fp==NULL || word==NULL || capacity==0) return FILE_WORD_FORMAT;
    if (ferror(fp)) return FILE_WORD_IO;
    do {
        if (reads++==FILE_WORD_MAX_READS) goto failed;
        c=getc(fp);
        if (c==EOF) {
            status=ferror(fp)?FILE_WORD_IO:FILE_WORD_EOF;
            goto failed;
        }
    } while (isspace((unsigned char)c));
    if (c=='\'' || c=='"') quote=c;
    else {
        if (c==0) { status=FILE_WORD_FORMAT; goto failed; }
        if (capacity<2) goto failed;
        word[used++]=(char)c;
    }
    for (;;) {
        if (reads++==FILE_WORD_MAX_READS) goto failed;
        c=getc(fp);
        if (c==EOF) {
            if (ferror(fp)) { status=FILE_WORD_IO; goto failed; }
            if (quote) { status=FILE_WORD_FORMAT; goto failed; }
            word[used]='\0';
            return FILE_WORD_OK;
        }
        if (quote ? c==quote : isspace((unsigned char)c)) {
            /* Match the historical unquoted delimiter/stream position. */
            if (!quote && ungetc(c,fp)==EOF) { status=FILE_WORD_IO; goto failed; }
            word[used]='\0';
            return FILE_WORD_OK;
        }
        if (c==0) { status=FILE_WORD_FORMAT; goto failed; }
        if (used>=capacity-1) goto failed;
        word[used++]=(char)c;
    }
failed:
    word[0]='\0';
    return status;
}
