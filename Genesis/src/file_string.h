#ifndef ANATHEMA_FILE_STRING_H
#define ANATHEMA_FILE_STRING_H
/* Include stdio.h first. Checked runtime reads do not exit the process.
 * The tilde primitive uses caller storage. Engine results may be interned;
 * release them with free_string(), never the C library's free(). */
enum file_string_status {
    FILE_STRING_OK, FILE_STRING_FORMAT, FILE_STRING_LIMIT, FILE_STRING_IO
};
#define FILE_STRING_MAX_READS 65536
enum file_string_status fread_tilde_checked(FILE *fp,char *out,size_t capacity);
enum file_string_status fread_string_checked(FILE *fp,char **out);
#endif
