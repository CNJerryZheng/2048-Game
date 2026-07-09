#include "utils.h"
#include "config.h"

#include <stdio.h>
#include <string.h>

static const char XOR_KEY[] = "2048GameRankKey";
static const char HEX_DIGITS[] = "0123456789ABCDEF";

static void utils_hash_update(uint32_t *hash1, uint32_t *hash2, unsigned char character)
{
    *hash1 = (uint32_t)(((uint64_t)(*hash1) * HASH_BASE1 + character) % HASH_MOD1);
    *hash2 = (uint32_t)(((uint64_t)(*hash2) * HASH_BASE2 + character) % HASH_MOD2);
}

static int utils_hex_value(char character)
{
    if (character >= '0' && character <= '9')
        return character - '0';

    if (character >= 'A' && character <= 'F')
        return character - 'A' + 10;

    if (character >= 'a' && character <= 'f')
        return character - 'a' + 10;

    return -1;
}

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

bool utils_is_valid_username(char character)
{
    if (character >= '0' && character <= '9')
        return true;

    if (character >= 'A' && character <= 'Z')
        return true;

    if (character >= 'a' && character <= 'z')
        return true;

    if (character == '_')
        return true;

    return false;
}

bool utils_is_valid_password(char character)
{
    if (character == ' ')
        return false;

    if (character == '\t')
        return false;

    if (character == '\n')
        return false;

    if (character == '\r')
        return false;

    return true;
}

bool utils_password_hash(const char *salt, const char *password, uint32_t *hash1, uint32_t *hash2)
{
    uint32_t h1 = 0;
    uint32_t h2 = 0;
    size_t i;

    if (salt == NULL || password == NULL || hash1 == NULL || hash2 == NULL)
        return false;

    for (i = 0; salt[i] != '\0'; i = -~i)
        utils_hash_update(&h1, &h2, (unsigned char)salt[i]);

    utils_hash_update(&h1, &h2, (unsigned char)':');

    for (i = 0; password[i] != '\0'; i = -~i)
        utils_hash_update(&h1, &h2, (unsigned char)password[i]);

    *hash1 = h1;
    *hash2 = h2;
    return true;
}

bool utils_xor_encrypt_to_hex(const char *plain, char *encrypted_hex, size_t encrypted_hex_size)
{
    size_t plain_length;
    size_t key_length;
    size_t i;
    unsigned char encrypted_byte;

    if (plain == NULL || encrypted_hex == NULL || encrypted_hex_size == 0)
        return false;

    plain_length = strlen(plain);
    key_length = strlen(XOR_KEY);

    if (plain_length > (encrypted_hex_size - 1) / 2)
        return false;

    for (i = 0; i < plain_length; i = -~i)
    {
        encrypted_byte = (unsigned char)plain[i] ^ (unsigned char)XOR_KEY[i % key_length];
        encrypted_hex[i * 2] = HEX_DIGITS[encrypted_byte >> 4];
        encrypted_hex[i * 2 + 1] = HEX_DIGITS[encrypted_byte & 0x0F];
    }

    encrypted_hex[plain_length * 2] = '\0';
    return true;
}

bool utils_xor_decrypt_from_hex(const char *encrypted_hex, char *plain, size_t plain_size)
{
    size_t encrypted_length;
    size_t byte_length;
    size_t key_length;
    size_t i;
    int high_value;
    int low_value;
    unsigned char encrypted_byte;

    if (encrypted_hex == NULL || plain == NULL || plain_size == 0)
        return false;

    encrypted_length = strlen(encrypted_hex);
    if (encrypted_length % 2 != 0)
        return false;

    byte_length = encrypted_length / 2;
    if (byte_length >= plain_size)
        return false;

    key_length = strlen(XOR_KEY);

    for (i = 0; i < byte_length; i = -~i)
    {
        high_value = utils_hex_value(encrypted_hex[i * 2]);
        low_value = utils_hex_value(encrypted_hex[i * 2 + 1]);
        if (high_value < 0 || low_value < 0)
            return false;

        encrypted_byte = (unsigned char)((high_value << 4) | low_value);
        plain[i] = (char)(encrypted_byte ^ (unsigned char)XOR_KEY[i % key_length]);
    }

    plain[byte_length] = '\0';
    return true;
}
