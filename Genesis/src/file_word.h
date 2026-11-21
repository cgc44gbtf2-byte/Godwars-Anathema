#ifndef ANATHEMA_FILE_WORD_H
#define ANATHEMA_FILE_WORD_H
/* Include stdio.h first. A failed read clears a nonempty output buffer.
 * Clean EOF before a token is distinct from a malformed/partial word.
 * The caller owns storage; no allocation or static buffer is used here. */
enum file_word_status {
    FILE_WORD_OK, FILE_WORD_EOF, FILE_WORD_FORMAT, FILE_WORD_LIMIT, FILE_WORD_IO
};
#define FILE_WORD_MAX_READS 65536
enum file_word_status fread_word_checked(FILE *fp, char *word, size_t capacity);
#endif
