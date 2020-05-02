/*
 * @Author: HandsomeChen
 * @Date: 2019-12-16 22:45:13
 * @LastEditTime: 2019-12-16 22:51:55
 * @LastEditors: HandsomeChen
 * @Description: 用户信息处理
 */
#ifndef _USER_H 
#define _USER_H 
#ifdef __cplusplus
extern "C" {
#endif

#define SIZE_USER_STR 30    /* 最大用户信息长度 */

/* 用户信息 */
typedef struct _USER USER;
struct _USER
{
    int id; /* id 键值, 数据库用 */
    char username[SIZE_USER_STR];    /* 用户名 */
    char password[SIZE_USER_STR];    /* 密码 */
    char phone_number[SIZE_USER_STR];    /* 手机号 */
};

/**
 * @description: 根据参数创建新用户信息结构体, 用完需free()
 * @param username 用户名
 * @param password 密码
 * @param phone_number 手机号
 * @return: USER 新用户信息结构体
 */
USER *user_create_user(char *username, char *password, char *phone_number);

#ifdef __cplusplus
}
#endif
#endif	// _USER_H
