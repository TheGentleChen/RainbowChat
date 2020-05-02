/*
 * @Author: HandsomeChen
 * @Date: 2019-12-15 19:58:50
 * @LastEditTime : 2019-12-18 10:25:06
 * @LastEditors  : HandsomeChen
 * @Description: 客户端服务
 */
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <assert.h>
#include "client.h"
#include "endpoint.h"
#include "message.h"

#define INTERVAL_PING 10

static int quited = 0;  /*  退出标志 */
static int g_clientfd;  /*  客户端本地sock */
static ENDPOINT *g_server;  /* 服务器节点信息 */
static ENDPOINT_GROUP *g_peers; /* 分组成员列表 */
void *keep_alive_loop();
void *receive_loop();
void *console_loop();

static void quit()
{
    quited = 1;
}

/**
 * @description: 子线程, 保持与已打通的主机及服务器的连接活性
 * @return: NULL
 */
void *keepalive_loop()
{
    Message ping;
    ping.head.magic = MSG_MAGIC;
    ping.head.type = MSG_TYPE_PING;
    ping.head.length = 0;
    ping.body = NULL;
    unsigned int i = 0;
    struct list_head *get = NULL;
    struct list_head *n = NULL;

    while(!quited) {
        /* 每10秒发送一次PING */
        if (i++ < INTERVAL_PING) {
            sleep(1);
            continue;
        }
        i = 0;
        udp_send_msg(g_clientfd, g_server, ping);

        if (NULL != g_peers)    /* 如果存在已打洞主机, 则分别发送PING消息, 保持活性 */
        {
            list_iterate_safe(get, n, &g_peers->list_endpoint)
            {
                ENDPOINT *tmp = list_get(get, ENDPOINT, list);
                udp_send_msg(g_clientfd, tmp, ping);
            }
        }
    }
    printf("quiting keepalive_loop\n");
    return NULL;
}

/**
 * @description: 消息解析函数
 * @param addr 源节点地址
 * @param msg 接收到的消息
 * @return: void
 */
void on_message(struct sockaddr_in addr, Message msg)
{
    struct list_head *get = NULL;
    struct list_head *n = NULL;

    /* 来自服务器的消息 */
    if (addr_equal(g_server->client_addr, addr))
    {
        printf("RECV %d bytes FROM %s: %s %s\n", msg.head.length,
                ep_tostring(g_server), msg_type_str(msg.head.type), msg.body);
        switch (msg.head.type) 
        {
            case MSG_TYPE_PUNCH:    /* 如果是打洞请求, 则对方主机回复打洞请求 */
                { 
                    ENDPOINT *peer = ep_fromstring(msg.body);
                    printf("%s on call, replying...\n", ep_tostring(peer));
                    udp_send_text(g_clientfd, peer, MSG_TYPE_REPLY, NULL);
                }
                break;
            case MSG_TYPE_TEXT:
                printf("SERVER: %s\n", msg.body);
                break;
            case MSG_TYPE_QUIT_REPLY:   /* 如果是退出分组的回复, 则释放资源, 置g_peers为NULL */
                {
                    printf("SERVER: %s\n", msg.body);
                    free(g_peers);
                    g_peers = NULL;
                }
                break;
            case MSG_TYPE_PEERS_REPLY:  /* 如果是请求分组成员的回复, 则解析消息将成员都加入g_peers中 */
                {
                    printf("SERVER: %s\n", msg.body);
                    char *user = NULL;
                    user = strtok(msg.body, ";");
                    while (NULL != user)
                    {
                        ENDPOINT *tmp = ep_fromstring(user);
                        LIST_ADD(&tmp->list, &g_peers->list_endpoint);
                        user = strtok(NULL, ";");
                    }
                }
                break;
            default:
                break;
        }
        return;
    }

    /* 来自客户端的消息 */
    if (NULL != g_peers)
    {   
        /* 搜索消息来自哪一个已打通客户端 */
        list_iterate_safe(get, n, &g_peers->list_endpoint)
        {
            ENDPOINT *tmp = list_get(get, ENDPOINT, list);
            if (addr_equal(tmp->client_addr, addr))
            {
                printf("RECV %d bytes FROM %s: %s %s\n", msg.head.length,
                    ep_tostring(tmp), msg_type_str(msg.head.type), msg.body);

                switch (msg.head.type)
                {
                    case MSG_TYPE_TEXT:
                        printf("Peer(%s): %s\n", ep_tostring(tmp), msg.body);
                        break;
                    case MSG_TYPE_REPLY:    /* 请求打洞回复, 说明打洞成功 */
                        printf("Peer(%s) replied, you can talk now\n", ep_tostring(tmp));
                        break;
                    case MSG_TYPE_PUNCH:    /* 可能出现的情况(已经打通, 但对方又请求打洞), 这里简单处理 */
                        udp_send_text(g_clientfd, tmp, MSG_TYPE_TEXT, "I SEE YOU");
                        break;
                    case MSG_TYPE_PING:     /* 对方的PING消息, 回复PONG */
                        udp_send_text(g_clientfd, tmp, MSG_TYPE_PONG, NULL);
                        break;
                    default:
                        break;
                }
                break;
            }
        }
    }
}

/**
 * @description: 子线程, 接收消息函数
 * @return: NULL
 */
void *receive_loop()
{
    struct sockaddr_in peer;
    socklen_t addrlen;
    char buf[SIZE_RECV_BUF];
    int nfds;
    fd_set readfds;
    struct timeval timeout;

    nfds = g_clientfd + 1;
    while(!quited) {
        FD_ZERO(&readfds);
        FD_SET(g_clientfd, &readfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        int ret = select(nfds, &readfds, NULL, NULL, &timeout);
        if (ret == 0) {
            /* timeout */
            continue;
        }
        else if (ret == -1) {
            perror("select");
            continue;
        }
        assert(FD_ISSET(g_clientfd, &readfds));
        addrlen = sizeof(peer);
        memset(&peer, 0, addrlen);
        memset(buf, 0, SIZE_RECV_BUF);
        int rd_size = recvfrom(g_clientfd, buf, SIZE_RECV_BUF, 0,
                (struct sockaddr*)&peer, &addrlen);
        if (rd_size == -1) {
            perror("recvfrom");
            continue;
        } else if (rd_size == 0) {
            printf("EOF from %s:%d\n", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
            continue;
        }
        Message msg = msg_stream_pack(buf, rd_size);
        if (msg.head.magic != MSG_MAGIC || msg.body == NULL) {
            printf("Invalid message(%d bytes): {0x%x,%d,%d} %p\n", rd_size,
                    msg.head.magic, msg.head.type, msg.head.length, msg.body);
            continue;
        }

        on_message(peer, msg);
    }
    printf("quiting receive_loop\n");
    return NULL;
}

/**
 * @description: 操作提示
 * @return: void
 */
static void print_help()
{
    const static char *help_message = ""
        "Usage:"
        "\n\n login username password phone_number"
        "\n     login to server so that other peer(s) can see you"
        "\n\n list"
        "\n     list all groups"
        "\n\n join group"
        "\n     join in a group"
        "\n\n peers"
        "\n     list peers in group"
        "\n\n punch"
        "\n     punch a hole through UDP to [host:port]s of group"
        "\n\n send data"
        "\n     send [data] to peer [host:port]s in the group through UDP protocol "
        "\n     the other peers could receive your message if UDP hole punching succeed"
        "\n\n quit"
        "\n     quit the group"
        "\n\n logout"
        "\n     logout from server"
        "\n\n help"
        "\n     print this help message";
    printf("%s\n", help_message);
}

/**
 * @description: 命令读取
 * @return: NULL
 */
void *console_loop()
{
    char *line = NULL;
    size_t len;
    ssize_t read;
    while(fprintf(stdout, ">>> ") && (read = getline(&line, &len, stdin)) != -1) {

        /* 空行忽略 */
        if (read == 1) continue;

        /* 解析命令 */
        char *cmd = strtok(line, " ");
        if (strncmp(cmd, "login", 5) == 0) {
            char text[SIZE_SEND_BUF];

            /* 解析参数, 即 username password phone_number */
            char *username = strtok(NULL, " ");
            char *password = strtok(NULL, " ");
            char *phone_number = strtok(NULL, "\n");

            /* 以 username:password:phone_number 格式发送 */
            sprintf(text, "%s:%s:%s", username, password, phone_number);
            udp_send_text(g_clientfd, g_server, MSG_TYPE_LOGIN, text);
        } else if (strncmp(cmd, "list", 4) == 0) {
            udp_send_text(g_clientfd, g_server, MSG_TYPE_LIST, NULL);
        } else if (strncmp(cmd, "join", 4) == 0) {
            int dsc;

            /* 解析参数 分组描述符(group) */
            char *group = strtok(NULL, "\n");

            /* 现在以加入分组, g_peers初始化 */
            g_peers = ep_create_endpoint_group();
            sscanf(group, "%d", &dsc);  /* 字符串(group)转整数(dsc) */
            g_peers->dsc = dsc;
            udp_send_text(g_clientfd, g_server, MSG_TYPE_JOIN, group);
        } else if (strncmp(cmd, "peers", 5) == 0) {
            udp_send_text(g_clientfd, g_server, MSG_TYPE_PEERS, NULL);
        } else if (strncmp(cmd, "punch", 5) == 0) {
            if (NULL == g_peers)    /* g_peers为NULL, 则还未加入分组, 不可 punch */
            {
                printf("You haven't joined a group\n");
                continue;
            }

            struct list_head *get = NULL;
            struct list_head *n = NULL;
            /* 对所有分组内的发打洞消息 */
            list_iterate_safe(get, n, &g_peers->list_endpoint)
            {
                ENDPOINT *tmp = list_get(get, ENDPOINT, list);
                printf("punching %s\n", ep_tostring(tmp));
                udp_send_text(g_clientfd, tmp, MSG_TYPE_PUNCH, NULL);
            }

            /* 请求服务器协助打洞, 即让对方主机对本机回复 */
            udp_send_text(g_clientfd, g_server, MSG_TYPE_PUNCH, NULL);
        } else if (strncmp(cmd, "send", 4) == 0) {
            if (NULL == g_peers)
            {
                printf("You haven't joined a group\n");
                continue;
            }

            char *data = strtok(NULL, "\n");
            struct list_head *get = NULL;
            struct list_head *n = NULL;

            /* 对分组内所有主机发送消息 */
            list_iterate_safe(get, n, &g_peers->list_endpoint)
            {
                ENDPOINT *tmp = list_get(get, ENDPOINT, list);
                udp_send_text(g_clientfd, tmp, MSG_TYPE_TEXT, data);
            }
        } else if (strncmp(cmd, "quit", 4) == 0) {
            udp_send_text(g_clientfd, g_server, MSG_TYPE_QUIT, NULL);
        } else if (strncmp(cmd, "logout", 6) == 0) {
            udp_send_text(g_clientfd, g_server, MSG_TYPE_LOGOUT, NULL);
            quit();
            break;
        } else if (strncmp(cmd, "help", 4) == 0) {
            print_help();
        } else {
            printf("Unknown command %s\n", cmd);
            print_help();
        }
    }
    free(line);
    printf("quiting console_loop\n");
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s server:port\n", argv[0]);
        return 1;
    }
    int ret;
    pthread_t keepalive_pid, receive_pid, console_pid;

    char text[SIZE_ENDPOINT_STR];
    sprintf(text, "%s:%s:%d:%d", "Server", "127.0.0.1", 3478, 0);

    /* 初始化服务器地址 */
    g_server = ep_fromstring(text);
    /* 打通主机列表初始化为NULL */
    g_peers = NULL;
    g_clientfd = socket(AF_INET, SOCK_DGRAM, 0);

    printf("setting server to %s\n", ep_tostring(g_server));

    /* 创建线程 */
    if (g_clientfd == -1) { perror("socket"); goto clean; }
    ret = pthread_create(&keepalive_pid, NULL, &keepalive_loop, NULL);
    if (ret != 0) { perror("keepalive"); goto clean; }
    ret = pthread_create(&receive_pid, NULL, &receive_loop, NULL);
    if (ret != 0) { perror("receive"); goto clean; }
    ret = pthread_create(&console_pid, NULL, &console_loop, NULL);
    if (ret != 0) { perror("console"); goto clean; }

    pthread_join(console_pid, NULL);
    pthread_join(receive_pid, NULL);
    pthread_join(keepalive_pid, NULL);
clean:  /* 程序结束清理 */
    close(g_clientfd);
    return 0;
}
