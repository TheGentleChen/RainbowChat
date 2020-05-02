/*
 * @Author: HandsomeChen
 * @Date: 2019-12-15 18:34:20
 * @LastEditTime : 2019-12-18 10:12:45
 * @LastEditors  : HandsomeChen
 * @Description: 服务器流程
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "list.h"
#include "user.h"
#include "endpoint.h"
#include "message.h"

/* 回调函数, 用于解析消息 */
typedef void callback_t(int server_sock, struct sockaddr_in from, Message msg);

/* 分组列表, 包含所有分组, 至少包含一个分组(默认分组, 即 default_endpoint_group) */
static struct list_head list_endpoint_group;

/* 默认分组, 包含所有存活节点 */
static ENDPOINT_GROUP *default_endpoint_group;

void udp_receive_loop(int listen_sock, callback_t callback)
{
    struct sockaddr_in peer;
    socklen_t addrlen;
    char buf[SIZE_RECV_BUF];
    for(;;) {
        addrlen = sizeof(peer);
        memset(&peer, 0, addrlen);
        memset(buf, 0, SIZE_RECV_BUF);
        int rd_size;
        /* UDP isn't a "stream" protocol. once you do the initial recvfrom,
           the remainder of the packet is discarded */
        rd_size = recvfrom(listen_sock, buf, SIZE_RECV_BUF, 0,
                (struct sockaddr*)&peer, &addrlen);
        if (rd_size == -1) {
            perror("recvfrom");
            break;
        } else if (rd_size == 0) {
            printf("EOF from %s", inet_ntoa(peer.sin_addr));
            continue;
        }
        Message msg = msg_stream_pack(buf, rd_size);
        if (msg.head.magic != MSG_MAGIC || msg.body == NULL) {
            printf("Invalid message(%d bytes): {0x%x,%d,%d} %p", rd_size,
                    msg.head.magic, msg.head.type, msg.head.length, msg.body);
            continue;
        }

        /* 调用回调函数, 解析消息 */
        callback(listen_sock, peer, msg);
        continue;

    }
    printf("udp_receive_loop stopped.\n");
}

void on_message(int sock, struct sockaddr_in from, Message msg) {
    printf("RECV %d bytes FROM %s:%d: %s %s\n", msg.head.length,
            inet_ntoa(from.sin_addr), ntohs(from.sin_port), msg_type_str(msg.head.type), msg.body);
    
    /* 根据sock获得对方节点 */
    ENDPOINT *peer = ep_get_endpoint_with_sock(from, &default_endpoint_group->list_endpoint, LIST_DEFAULT);

    switch(msg.head.type) {
        case MSG_TYPE_LOGIN:    /* 登录请求 */
            {
                if (NULL != peer)
                {
                    udp_send_text(sock, peer, MSG_TYPE_TEXT, "You are already logged!");
                    return;
                }

                /* 解析参数 */
                char *username = strtok(msg.body, ":");
                char *password = strtok(NULL, ":");
                char *phone_number = strtok(NULL, ":");
                USER *user = user_create_user(username, password, phone_number);

                /* 创建新节点 */
                peer = ep_create_endpoint(user, NAT_PUBLIC, from);
                free(user);

                /* 将新节点加入默认分组列表 */
                LIST_ADD(&peer->default_list, &default_endpoint_group->list_endpoint);
                printf("%s logged in\n", ep_tostring(peer));

                udp_send_text(sock, peer, MSG_TYPE_TEXT, "Login success!");
            }
            break;
        case MSG_TYPE_LIST: /* 展示分组请求 */
            {
                struct list_head *get = NULL;
                struct list_head *n = NULL;

                printf("%s quering list\n", ep_tostring(peer));
                char text[SIZE_SEND_BUF];
                memset(text, 0, SIZE_SEND_BUF);

                /* 遍历分组列表 */
                list_iterate_safe(get, n, &list_endpoint_group)
                {
                    ENDPOINT_GROUP *tmp = list_get(get, ENDPOINT_GROUP, list);
                    char tmp_str[10];

                    /* Int(tmp->dsc)转String(tmp_str) */
                    sprintf(tmp_str, "%d", tmp->dsc);
                    strcat(text, tmp_str);
                    if (n != &list_endpoint_group)
                    {
                        strcat(text, ":");
                    }
                }

                /* text最终形式如下: 1:2:3:4, 其中1、2、3、4为分组描述符 */
                udp_send_text(sock, peer, MSG_TYPE_TEXT, text);
            }
            break;
        case MSG_TYPE_JOIN: /* 加入分组 */
            {
                ENDPOINT_GROUP *group = ep_get_endpoint_group_with_endpoint(peer, &list_endpoint_group);

                /* 如果已经在一个分组内, 返回提示信息 */
                if (NULL != group)
                {
                    udp_send_text(sock, peer, MSG_TYPE_TEXT, "You already joined a group");
                    return;
                }

                /* 否则, 根据要加入的分组描述符进行判断 */
                int group_dsc;
                sscanf(msg.body, "%d", &group_dsc);
                group = ep_get_endpoint_group_with_dsc(group_dsc, &list_endpoint_group);
                if (group == NULL)  /* 如果是不存在的分组 */
                {
                    /* 为其创建新分组 */
                    group = ep_create_endpoint_group();
                    /* 将新分组加入分组列表 */
                    LIST_ADD(&group->list, &list_endpoint_group);
                    /* 将此成员加入新分组 */
                    LIST_ADD(&peer->list, &group->list_endpoint);
                    /* 新分组组内成员+1 */
                    group->cnt_endpoint++;
                    /* 修改此成员的分组描述符为此分组 */
                    peer->group_dsc = group_dsc;
                    udp_send_text(sock, peer, MSG_TYPE_TEXT, "Create a group Successfully");
                    return;
                }
                else    /* 此分组存在 */
                {
                    /* 直接加入到已存在分组中 */
                    LIST_ADD(&peer->list, &group->list_endpoint);
                    /* 分组组内成员+1 */
                    group->cnt_endpoint++;
                    /* 修改此成员的分组描述符为此分组 */
                    peer->group_dsc = group->dsc;
                    udp_send_text(sock, peer, MSG_TYPE_TEXT, "Joined Successfully");
                    return;
                }
                
            }
            break;
        case MSG_TYPE_PEERS:    /* 组内成员列表请求 */
            {
                char *text;
                ENDPOINT_GROUP *group = ep_get_endpoint_group_with_endpoint(peer, &list_endpoint_group);

                /* 对方未加入分组, 返回提示信息 */ 
                if (NULL == group)
                {
                    udp_send_text(sock, peer, MSG_TYPE_TEXT, "You don't join a group");
                    return;
                }

                printf("%s quering peers\n", ep_tostring(peer));
                /* 获得并发送回组内成员信息 */
                text = ep_get_peers_from_group(peer, &list_endpoint_group);
                udp_send_text(sock, peer, MSG_TYPE_PEERS_REPLY, text);
                free(text);
            }
            break;
        case MSG_TYPE_PUNCH:    /* 打洞协助请求 */
            {
                struct list_head *get = NULL;
                struct list_head *n = NULL;

                ENDPOINT_GROUP *group = ep_get_endpoint_group_with_endpoint(peer, &list_endpoint_group);
                /* 对方未加入分组, 返回提示信息 */ 
                if (NULL == group)
                {
                    udp_send_text(sock, peer, MSG_TYPE_TEXT, "You don't join a group");
                    return;
                }
                
                /* 遍历请求方的分组列表 */
                list_iterate_safe(get, n, &group->list_endpoint)
                {
                    ENDPOINT *tmp = list_get(get, ENDPOINT, list);

                    /* 要求除请求方的其他成员都对请求方的打洞进行回复 */
                    if (!addr_equal(tmp->client_addr, from))
                    {
                        printf("Punching to %s\n", ep_tostring(tmp));
                        udp_send_text(sock, tmp, MSG_TYPE_PUNCH, ep_tostring(peer));
                    }
                }
                
                udp_send_text(sock, peer, MSG_TYPE_TEXT, "Punch request sent");
            }
            break;
        case MSG_TYPE_QUIT: /* 退出分组请求 */
            {
                ENDPOINT_GROUP *group = ep_get_endpoint_group_with_endpoint(peer, &list_endpoint_group);

                /* 对方未加入分组, 返回提示信息 */ 
                if (NULL == group)
                {
                    udp_send_text(sock, peer, MSG_TYPE_TEXT, "You haven't join a group!");
                    return;
                }

                /* 从分组中移除 */
                if (0 == ep_remove_endpoint_from_group(peer, &group->list_endpoint, LIST_JOINED))
                {
                    udp_send_text(sock, peer, MSG_TYPE_QUIT_REPLY, "Quit Failed");
                    return;
                }

                /* 分组成员-1 */
                group->cnt_endpoint--;
                printf("cnt = %d\n", group->cnt_endpoint);
                /* 如果分组中已经没有成员, 就从分组列表中删除此分组 */
                if (0 == group->cnt_endpoint)
                {
                    if (0 == ep_remove_group(group, &list_endpoint_group))
                    {
                        udp_send_text(sock, peer, MSG_TYPE_TEXT, "Quit Failed");
                        return;
                    }
                }
                
                udp_send_text(sock, peer, MSG_TYPE_QUIT_REPLY, "Quit Success");
            }
            break;
        case MSG_TYPE_LOGOUT:
            {
                ENDPOINT_GROUP *group = ep_get_endpoint_group_with_endpoint(peer, &list_endpoint_group);
                /* 如果退出时还在某一分组内 */
                if (NULL != group)
                {
                    /* 则从分组中移除此成员 */
                    if (0 == ep_remove_endpoint_from_group(peer, &group->list_endpoint, LIST_JOINED))
                    {
                        udp_send_text(sock, peer, MSG_TYPE_TEXT, "Logout Failed");
                        return;
                    }

                    /* 分组成员-1 */
                    group->cnt_endpoint--;

                    /* 如果分组中已经没有成员, 就从分组列表中删除此分组 */
                    if (0 == group->cnt_endpoint)
                    {
                        if (0 == ep_remove_group(group, &list_endpoint_group))
                        {
                            udp_send_text(sock, peer, MSG_TYPE_TEXT, "Logout Failed");
                            return;
                        }
                    }
                }

                /* 从默认分组中移除此节点 */
                if (0 == ep_remove_endpoint_from_group(peer, &default_endpoint_group->list_endpoint, LIST_DEFAULT))
                {
                    udp_send_text(sock, peer, MSG_TYPE_REPLY, "Logout Failed");
                    return;
                }
                udp_send_text(sock, peer, MSG_TYPE_REPLY, "Logout success");
                free(peer);
            }
            break;
        case MSG_TYPE_PING: /* 来自客户端的PING */
            udp_send_text(sock, peer, MSG_TYPE_PONG, NULL);
            break;
        case MSG_TYPE_PONG: /* 来自客户端的PONG, 不处理 */
            break;
        default:
            udp_send_text(sock, peer, MSG_TYPE_REPLY, "Unkown command");
            break;
    }
}

int main(int argc, char **argv) {
    
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    /* 添加默认分组 */
    INIT_LIST(list_endpoint_group);
    default_endpoint_group = ep_create_endpoint_group();
    LIST_ADD(&default_endpoint_group->list, &list_endpoint_group);

    const char *host = "127.0.0.1";
    int port = atoi(argv[1]);
    int ret;
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(host);
    server.sin_port = htons(port);
    server.sin_family = AF_INET;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    ret = bind(sock, (const struct sockaddr*)&server, sizeof(server));
    if (ret == -1) { 
        perror("bind");
        exit(EXIT_FAILURE);
    }

    printf("server start on %s\n", inet_ntoa(server.sin_addr));

    /* 循环接收消息 */
    udp_receive_loop(sock, on_message);

    return 0;
}
