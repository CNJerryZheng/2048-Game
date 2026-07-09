#pragma once

typedef enum AuthResult
{
    AUTH_OK,
    AUTH_INVALID_USERNAME,
    AUTH_INVALID_PASSWORD,
    AUTH_USERNAME_EXISTS,
    AUTH_USER_NOT_FOUND,
    AUTH_WRONG_PASSWORD,
    AUTH_FILE_ERROR
} AuthResult;

AuthResult auth_register_user(const char *user_file,
                              const char *username,
                              const char *password);
AuthResult auth_login_user(const char *user_file,
                           const char *username,
                           const char *password);
AuthResult auth_change_password(const char *user_file,
                                const char *username,
                                const char *old_password,
                                const char *new_password);
AuthResult auth_delete_user(const char *user_file,
                            const char *username,
                            const char *password);
AuthResult auth_admin_delete_user(const char *user_file,
                                  const char *username);
AuthResult auth_rename_user(const char *user_file,
                            const char *old_username,
                            const char *new_username,
                            const char *password);
int auth_user_exists(const char *user_file, const char *username);
