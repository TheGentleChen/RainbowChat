/*
 * @Author: HandsomeChen
 * @Date: 2019-12-15 19:54:23
 * @LastEditTime : 2019-12-18 13:33:23
 * @LastEditors  : HandsomeChen
 * @Description: 用户信息处理
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "user.h"

USER *user_create_user(char *username, char *password, char *phone_number)
{
    USER *user = malloc(sizeof(USER));
    memcpy(user->username, username, strlen(username));
    memcpy(user->password, password, strlen(password));
    memcpy(user->phone_number, phone_number, strlen(phone_number));
    return user;
}
