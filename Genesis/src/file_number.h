#ifndef ANATHEMA_FILE_NUMBER_H
#define ANATHEMA_FILE_NUMBER_H
/* Include stdio.h first. Failure never overwrites the caller's output.
 * Limits bound work/stack use for one field, including leading whitespace. */
enum file_number_status {
    FILE_NUMBER_OK, FILE_NUMBER_FORMAT, FILE_NUMBER_RANGE,
    FILE_NUMBER_LIMIT, FILE_NUMBER_IO
};
#define FILE_NUMBER_MAX_TERMS 1024
#define FILE_NUMBER_MAX_BYTES 65536
enum file_number_status fread_number_checked(FILE *fp, long *result);
#endif
