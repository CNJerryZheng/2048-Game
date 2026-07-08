#include "auth.h"

#include "config.h"
#include "storage.h"
#include "utils.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int auth_username_valid(const char *username)
{
    size_t index;
    size_t length;

    if (username == NULL)
        return 0;

    length = strlen(username);
    if (length < USER_NAME_LENGTH_MIN || length >= USER_NAME_LENGTH_MAX)
        return 0;

    for (index = 0; index < length; index = -~index)
    {
        if (!utils_is_valid_username(username[index]))
            return 0;
    }
    return 1;
}

static int auth_password_valid(const char *password)
{
    size_t index;
    size_t length;

    if (password == NULL)
        return 0;

    length = strlen(password);
    if (length < USER_PASSWORD_LENGTH_MIN || length >= USER_PASSWORD_LENGTH_MAX)
        return 0;

    for (index = 0; index < length; index = -~index)
    {
        if (!utils_is_valid_password(password[index]))
            return 0;
    }
    return 1;
}

static int auth_read_record(FILE *file,
                            char *username,
                            uint32_t *hash1,
                            uint32_t *hash2)
{
    char line[INPUT_BUFFER_LENGTH];
    unsigned int parsed_hash1;
    unsigned int parsed_hash2;

    while (storage_read_line(file, line, sizeof(line)))
    {
        if (sscanf(line, "%31[^\t]\t%u\t%u",
                   username,
                   &parsed_hash1,
                   &parsed_hash2) == 3)
        {
            *hash1 = (uint32_t)parsed_hash1;
            *hash2 = (uint32_t)parsed_hash2;
            return 1;
        }
    }
    return 0;
}

AuthResult auth_register_user(const char *user_file,
                              const char *username,
                              const char *password)
{
    FILE *file;
    char stored_username[USER_NAME_LENGTH_MAX];
    uint32_t stored_hash1;
    uint32_t stored_hash2;
    uint32_t hash1;
    uint32_t hash2;

    if (!auth_username_valid(username))
        return AUTH_INVALID_USERNAME;
    if (!auth_password_valid(password))
        return AUTH_INVALID_PASSWORD;

    file = storage_open_read(user_file);
    if (file != NULL)
    {
        while (auth_read_record(file,
                                stored_username,
                                &stored_hash1,
                                &stored_hash2))
        {
            if (utils_string_equal(username, stored_username))
            {
                (void)storage_close(file);
                return AUTH_USERNAME_EXISTS;
            }
        }
        (void)storage_close(file);
    }

    if (!utils_password_hash(username, password, &hash1, &hash2))
        return AUTH_FILE_ERROR;

    file = storage_open_append(user_file);
    if (file == NULL)
        return AUTH_FILE_ERROR;

    if (!storage_write_user(file, username, hash1, hash2))
    {
        (void)storage_close(file);
        return AUTH_FILE_ERROR;
    }

    if (!storage_close(file))
        return AUTH_FILE_ERROR;

    return AUTH_OK;
}

AuthResult auth_login_user(const char *user_file,
                           const char *username,
                           const char *password)
{
    FILE *file;
    char stored_username[USER_NAME_LENGTH_MAX];
    uint32_t stored_hash1;
    uint32_t stored_hash2;
    uint32_t input_hash1;
    uint32_t input_hash2;
    int username_found = 0;

    if (username == NULL || password == NULL)
        return AUTH_USER_NOT_FOUND;
    if (!utils_password_hash(username,
                             password,
                             &input_hash1,
                             &input_hash2))
        return AUTH_FILE_ERROR;

    file = storage_open_read(user_file);
    if (file == NULL)
        return AUTH_USER_NOT_FOUND;

    while (auth_read_record(file,
                            stored_username,
                            &stored_hash1,
                            &stored_hash2))
    {
        if (!utils_string_equal(username, stored_username))
            continue;

        username_found = 1;
        if (input_hash1 == stored_hash1 && input_hash2 == stored_hash2)
        {
            (void)storage_close(file);
            return AUTH_OK;
        }
    }

    (void)storage_close(file);
    return username_found ? AUTH_WRONG_PASSWORD : AUTH_USER_NOT_FOUND;
}

static AuthResult auth_rewrite_user(const char *user_file,
                                    const char *username,
                                    const char *new_password,
                                    int delete_user)
{
    FILE *source;
    FILE *target;
    char cache_file[512];
    char stored_username[USER_NAME_LENGTH_MAX];
    uint32_t stored_hash1;
    uint32_t stored_hash2;
    uint32_t new_hash1 = 0;
    uint32_t new_hash2 = 0;
    int found = 0;

    if (!delete_user &&
        !utils_password_hash(username, new_password, &new_hash1, &new_hash2))
        return AUTH_FILE_ERROR;

    if (snprintf(cache_file, sizeof(cache_file), "%s.tmp", user_file) < 0)
        return AUTH_FILE_ERROR;

    source = storage_open_read(user_file);
    if (source == NULL)
        return AUTH_USER_NOT_FOUND;
    target = storage_open_write(cache_file);
    if (target == NULL)
    {
        (void)storage_close(source);
        return AUTH_FILE_ERROR;
    }

    while (auth_read_record(source, stored_username,
                            &stored_hash1, &stored_hash2))
    {
        if (utils_string_equal(stored_username, username))
        {
            found = 1;
            if (delete_user)
                continue;
            stored_hash1 = new_hash1;
            stored_hash2 = new_hash2;
        }
        if (!storage_write_user(target, stored_username,
                                stored_hash1, stored_hash2))
        {
            (void)storage_close(source);
            (void)storage_close(target);
            (void)storage_remove_file(cache_file);
            return AUTH_FILE_ERROR;
        }
    }

    if (!storage_close(source) || !storage_close(target) || !found ||
        !storage_replace_file(cache_file, user_file))
    {
        (void)storage_remove_file(cache_file);
        return found ? AUTH_FILE_ERROR : AUTH_USER_NOT_FOUND;
    }
    return AUTH_OK;
}

AuthResult auth_change_password(const char *user_file,
                                const char *username,
                                const char *old_password,
                                const char *new_password)
{
    AuthResult result = auth_login_user(user_file, username, old_password);
    if (result != AUTH_OK)
        return result;
    if (!auth_password_valid(new_password))
        return AUTH_INVALID_PASSWORD;
    return auth_rewrite_user(user_file, username, new_password, 0);
}

AuthResult auth_delete_user(const char *user_file,
                            const char *username,
                            const char *password)
{
    AuthResult result = auth_login_user(user_file, username, password);
    if (result != AUTH_OK)
        return result;
    return auth_rewrite_user(user_file, username, NULL, 1);
}
