#include "storage.h"

#ifdef _WIN32
#include <windows.h>
#endif

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

FILE *storage_open_write(const char *file_path)
{
    if (file_path == NULL)
        return NULL;

    return fopen(file_path, "w");
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

bool storage_write_score(FILE *file, const char *username, int score)
{
    if (file == NULL || username == NULL || score < 0)
        return false;

    return fprintf(file, "%s\t%d\n", username, score) >= 0;
}

bool storage_close(FILE *file)
{
    if (file == NULL)
        return false;

    return fclose(file) == 0;
}

bool storage_remove_file(const char *file_path)
{
    if (file_path == NULL)
        return false;

    return remove(file_path) == 0;
}

bool storage_replace_file(const char *source_path, const char *target_path)
{
    if (source_path == NULL || target_path == NULL)
        return false;

#ifdef _WIN32
    return MoveFileExA(source_path,
                       target_path,
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(source_path, target_path) == 0;
#endif
}
