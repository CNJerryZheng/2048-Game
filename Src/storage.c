#include "storage.h"

FILE *storage_open_read(const char *file_path)
{
    if (file_path == NULL)
        return NULL;

    return fopen(file_path, "r");
}

FILE *storage_open_append(const char *file_path)
{
    if (file_path == NULL)
        return NULL;

    return fopen(file_path, "a");
}

bool storage_read_line(FILE *file, char *buffer, size_t buffer_size)
{
    if (file == NULL || buffer == NULL || buffer_size == 0)
        return false;

    return fgets(buffer, (int)buffer_size, file) != NULL;
}

bool storage_write_user(FILE *file, const char *username, const char *password)
{
    if (file == NULL || username == NULL || password == NULL)
        return false;

    return fprintf(file, "%s\t%s\n", username, password) >= 0;
}

bool storage_close(FILE *file)
{
    if (file == NULL)
        return false;

    return fclose(file) == 0;
}
