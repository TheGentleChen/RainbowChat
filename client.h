/*
 * @Author: HandsomeChen
 * @Date: 2019-12-16 19:08:33
 * @LastEditTime: 2019-12-16 22:54:38
 * @LastEditors: HandsomeChen
 * @Description: 客户端服务
 */
#ifndef _CLIENT_H 
#define _CLIENT_H 
#ifdef __cplusplus
extern "C" {
#endif


#include "user.h"

/**
 * @description: 用户注册, 注册后在将信息添加至数据库
 * @param user 用户信息
 * @return: 1 成功, 0 失败
 */
int user_register(USER *user);

/**
 * @description: 用户登录, 登录后在服务器添加对应节点, 加入至默认分组
 * @param user 用户信息
 * @return: 1 成功, 0 失败
 */
int user_login(USER *user);

/**
 * @description: 用户注销, 注销后在服务器删除对应节点
 * @param user 用户信息
 * @return: 1 成功, 0 失败
 */
int user_logout(USER *user);

#ifdef __cplusplus
}
#endif
#endif	// _CLIENT_H
