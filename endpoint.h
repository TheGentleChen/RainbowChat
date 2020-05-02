/*
 * @Author: HandsomeChen
 * @Date: 2019-12-15 18:14:15
 * @LastEditTime : 2019-12-18 10:34:15
 * @LastEditors  : HandsomeChen
 * @Description: 节点及分组管理
 */
#ifndef _ENDPOINT_H 
#define _ENDPOINT_H 
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include "user.h"
#include "list.h"

#define DEFAULT_GROUP 0 /* 默认分组描述符 */

#define SIZE_ENDPOINT_STR 64

/* NAT类型 */
enum _NAT_CLASS
{
    NAT_PUBLIC = 0,
    NAT_FULL_CONE,
    NAT_RESTRICTED_CONE,
    NAT_PORT_RESTRICTED_CONE,
    NAT_SYMMETRIC
};
typedef enum _NAT_CLASS NAT_CLASS;

enum _LIST_CLASS
{
    LIST_DEFAULT = 0,
    LIST_JOINED
};
typedef enum _LIST_CLASS LIST_CLASS;

/* 节点 */
typedef struct _ENDPOINT ENDPOINT;
struct _ENDPOINT
{
    USER user;  /* 对应用户 */
    struct sockaddr_in client_addr; /* 用户地址信息 */
    int group_dsc;  /* 节点属于的分组描述符, 默认为0 */
    NAT_CLASS nat_class;    /* 节点NAT类型 */
    struct list_head list;  /* 用于加入的分组管理 */
    struct list_head default_list;  /* 用于默认分组管理 */
};

/* 节点分组 */
typedef struct _ENDPOINT_GROUP ENDPOINT_GROUP;
struct _ENDPOINT_GROUP
{
    int dsc;    /* 分组描述符 */
    int cnt_endpoint;   /* 组中成员个数 */
    struct list_head list_endpoint; /* 组中成员列表 */
    struct list_head list;  /* 用于链表管理 */
};


/**
 * @description: 根据参数创建新节点
 * @param user 用户描述
 * @param nat_class NAT类型
 * @param client_addr 用户地址信息 
 * @return: ENDPOINT 新节点, 错误返回NULL
 */
ENDPOINT *ep_create_endpoint(USER *user, NAT_CLASS nat_class, struct sockaddr_in client_addr);

/**
 * @description: 比较俩Address是否相等
 * @param ep1 第一个Address
 * @param ep2 第二个Address
 * @return: 1 相等, 0 不相等
 */
int addr_equal(struct sockaddr_in addr1, struct sockaddr_in addr2);

/**
 * @description: 创建新分组, 分组描述符为上一分组描述符+1
 * @return: ENDPOINT_GROUP 新分组, 错误返回NULL
 */
ENDPOINT_GROUP *ep_create_endpoint_group();

/**
 * @description: 从分组中去除节点
 * @param ep 要去除的节点
 * @param list 对应分组中的成员列表
 * @return: 1 成功, 0 失败
 */
int ep_remove_endpoint_from_group(ENDPOINT *ep, struct list_head *list, LIST_CLASS list_class);

/**
 * @description: 从分组列表中去除对应分组
 * @param epg 要去除的分组
 * @param list 分组列表
 * @return: 1 成功, 0 失败
 */
int ep_remove_group(ENDPOINT_GROUP *epg, struct list_head *list);

/**
 * @description: 根据user获得默认成员分组中的节点
 * @param user 用户信息
 * @param list 默认分组中的成员列表
 * @return: ENPOINT 对应于user的ENDPOINT, 错误返回NULL
 */
ENDPOINT *ep_get_endpoint_with_user(USER *user, struct list_head *list, LIST_CLASS list_class);

/**
 * @description: 根据端点值获得分组中对应的节点
 * @param sock 端点值
 * @param list 分组中的成员列表
 * @return: ENPOINT 对应于sock的节点, 错误返回NULL
 */
ENDPOINT *ep_get_endpoint_with_sock(struct sockaddr_in sock, struct list_head *list, LIST_CLASS list_class);

/**
 * @description: 获得节点所在的分组
 * @param ep 要获得分组的节点
 * @param list 分组列表 
 * @return: ENDPOINT_GROUP ep节点的分组, 错误返回NULL
 */
ENDPOINT_GROUP *ep_get_endpoint_group_with_endpoint(ENDPOINT *ep, struct list_head *list);

/**
 * @description: 根据分组描述符, 得到相应分组
 * @param dsc 分组描述符
 * @param list 分组列表 
 * @return: 描述符为dsc的分组, 不存在返回NULL
 */
ENDPOINT_GROUP *ep_get_endpoint_group_with_dsc(int dsc, struct list_head *list);

/**
 * @description: 节点转字符串('username:addr_ip:port:nat_class'), 用完需free()
 * @param ep 节点
 * @return: char * 转换后的字符串
 */
char *ep_tostring(ENDPOINT *ep);

/**
 * @description: 字符串('username:addr_ip:port:nat_class')转节点, 用完需free()
 * @param tuple 要转换的字符串
 * @return: ENDPOINT 包含字符串信息的新节点
 */
ENDPOINT *ep_fromstring(const char *tuple);

/**
 * @description: 获得与节点同一分组的其他节点, 用完需free()
 * @param ep 节点
 * @param list 分组列表
 * @return: char * 同一分组的其他节点组成的字符串
 *          如: 'username1:addr_ip1:port1:nat_class1;username2:addr_ip2:port2:nat_class2'
 */
char *ep_get_peers_from_group(ENDPOINT *ep, struct list_head *list);

#ifdef __cplusplus
}
#endif
#endif	// _ENDPOINT_H
