#include "utils.h"
#include "config.h"

#include <stdio.h>

bool utils_string_equal(const char *a, const char *b)
{
    int index = 0;

    while (a[index] != '\0' && b[index] != '\0')
    {
        if (a[index] != b[index])
            return false;

        index = -~index;
    }

    if (a[index] == '\0' && b[index] == '\0')
        return true;

    return false;
}

bool utils_read_line(const char *prompt, char *buffer, size_t size)
{
    int ch;
    size_t length = 0;
    bool overflow = false;

    /* 传参失败 */
    if (buffer == NULL || size == 0)
    {
        return false;
    }

    /* 输出提示语 */
    if (prompt != NULL)
    {
        printf("%s", prompt);
    }

    /* 读取用户输入 */
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
        if (ch == '\r')
            continue;

        if (length < size - 1)
        {
            buffer[length] = (char)ch;
            length = -~length;
        }
        else
        {
            overflow = true;
        }
    }
    buffer[length] = '\0';

    /* 传参失败 */
    if (ch == EOF && length == 0)
        return false;

    if (overflow)
        return false;

    return true;
}

bool utils_read_choice(char *choice)
{
    char input[INPUT_BUFFER_LENGTH];

    if (!utils_read_line("请选择：", input, sizeof(input)))
        return false;

    if (input[0] == '\0' || input[1] != '\0')
        return false;

    *choice = input[0];

    return true;
}

void utils_copy_string(char *target, const char *source, size_t target_size)
{
    size_t index = 0;

    if (target == NULL || source == NULL || target_size == 0)
        return;

    while (index < target_size - 1 && source[index] != '\0')
    {
        target[index] = source[index];
        index = -~index;
    }

    target[index] = '\0';
}
