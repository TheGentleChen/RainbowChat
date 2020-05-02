/*
 * @Author: HandsomeChen
 * @Date: 2019-12-15 19:57:42
 * @LastEditTime : 2019-12-18 10:35:59
 * @LastEditors  : HandsomeChen
 * @Description: 节点及分组管理
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "endpoint.h"
static int cnt_group = 0;

ENDPOINT *ep_create_endpoint(USER *user, NAT_CLASS nat_class, struct sockaddr_in client_addr)
{
    ENDPOINT *ep = malloc(sizeof(ENDPOINT));
    memcpy(&ep->user, user, sizeof(USER));
    ep->group_dsc = DEFAULT_GROUP;
    ep->client_addr = client_addr;
    ep->nat_class = nat_class;
    INIT_LIST(ep->list);
    INIT_LIST(ep->default_list);
    return ep;
}

int addr_equal(struct sockaddr_in addr1, struct sockaddr_in addr2)
{
    return ((addr1.sin_family == addr2.sin_family) && 
            (addr1.sin_addr.s_addr == addr2.sin_addr.s_addr) && 
            (addr1.sin_port == addr2.sin_port));
}

ENDPOINT_GROUP *ep_create_endpoint_group()
{
    ENDPOINT_GROUP *epg = malloc(sizeof(ENDPOINT_GROUP));
    epg->cnt_endpoint = 0;
    epg->dsc = cnt_group++;
    INIT_LIST(epg->list);
    INIT_LIST(epg->list_endpoint);
    return epg;
}

int ep_remove_endpoint_from_group(ENDPOINT *ep, struct list_head *list, LIST_CLASS list_class)
{
    struct list_head *get = NULL;
    struct list_head *n = NULL;
    
    list_iterate_safe(get, n, list)
    {
        ENDPOINT *tmp;

        if (list_class == LIST_JOINED)
            tmp = list_get(get, ENDPOINT, list);
        else
            tmp = list_get(get, ENDPOINT, default_list);

        if (addr_equal(tmp->client_addr, ep->client_addr))
        {
            LIST_DEL(&tmp->list);
            tmp->group_dsc = DEFAULT_GROUP;
            return 1;
        }
    }
    return 0;
}

int ep_remove_group(ENDPOINT_GROUP *epg, struct list_head *list)
{
    struct list_head *get = NULL;
    struct list_head *n = NULL;
    
    list_iterate_safe(get, n, list)
    {
        ENDPOINT_GROUP *tmp = list_get(get, ENDPOINT_GROUP, list);
        if (epg->dsc == tmp->dsc)
        {
            LIST_DEL(&tmp->list);
            return 1;
        }
    }
    return 0;
}

ENDPOINT *ep_get_endpoint_with_user(USER *user, struct list_head *list, LIST_CLASS list_class)
{
    struct list_head *get = NULL;
    struct list_head *n = NULL;
    
    list_iterate_safe(get, n, list)
    {
        ENDPOINT *tmp;

        if (list_class == LIST_JOINED)
            tmp = list_get(get, ENDPOINT, list);
        else
            tmp = list_get(get, ENDPOINT, default_list);
        
        if (0 == strcmp(tmp->user.phone_number, user->phone_number))
        {
            return tmp;
        }
    }
    return NULL;
}

ENDPOINT *ep_get_endpoint_with_sock(struct sockaddr_in sock, struct list_head *list, LIST_CLASS list_class)
{
    struct list_head *get = NULL;
    struct list_head *n = NULL;

    list_iterate_safe(get, n, list)
    {
        ENDPOINT *tmp;

        if (list_class == LIST_JOINED)
            tmp = list_get(get, ENDPOINT, list);
        else
            tmp = list_get(get, ENDPOINT, default_list);

        if (addr_equal(sock, tmp->client_addr))
        {
            return tmp;
        }
    }
    return NULL;
}

ENDPOINT_GROUP *ep_get_endpoint_group_with_endpoint(ENDPOINT *ep, struct list_head *list)
{
    struct list_head *get = NULL;
    struct list_head *n = NULL;

    list_iterate_safe(get, n, list)
    {
        ENDPOINT_GROUP *tmp = list_get(get, ENDPOINT_GROUP, list);
        if (tmp->dsc != DEFAULT_GROUP && tmp->dsc == ep->group_dsc)
        {
            return tmp;
        }
    }
    return NULL;
}

ENDPOINT_GROUP *ep_get_endpoint_group_with_dsc(int dsc, struct list_head *list)
{
    struct list_head *get = NULL;
    struct list_head *n = NULL;

    list_iterate_safe(get, n, list)
    {
        ENDPOINT_GROUP *tmp = list_get(get, ENDPOINT_GROUP, list);
        if (tmp->dsc != DEFAULT_GROUP && tmp->dsc == dsc)
        {
            return tmp;
        }
    }
    return NULL;
}

char *ep_tostring(ENDPOINT *ep)
{
    char *tuple = malloc(SIZE_ENDPOINT_STR);
    snprintf(tuple, SIZE_ENDPOINT_STR, "%s:%s:%d:%d",
                                        ep->user.username,
                                        inet_ntoa(ep->client_addr.sin_addr),
                                        ntohs(ep->client_addr.sin_port),
                                        ep->nat_class);
    return tuple;
}

ENDPOINT *ep_fromstring(const char *tuple)
{
    char _tuple[SIZE_ENDPOINT_STR];
    sprintf(_tuple, "%s", tuple);
    ENDPOINT *ep = malloc(sizeof(ENDPOINT));
    char *username = NULL;
    struct sockaddr_in client_addr;

    username = strtok(_tuple, ":");
    memcpy(ep->user.username, username, strlen(username));

    client_addr.sin_addr.s_addr = inet_addr(strtok(NULL, ":"));
    client_addr.sin_port = htons(atoi(strtok(NULL, ":")));
    client_addr.sin_family = AF_INET;
    ep->client_addr = client_addr;
    
    ep->nat_class = atoi(strtok(NULL, ":"));
    return ep;
}

char *ep_get_peers_from_group(ENDPOINT *ep, struct list_head *list)
{
    char *peers = NULL;
    struct list_head *get = NULL;
    struct list_head *n = NULL;

    ENDPOINT_GROUP *epg = ep_get_endpoint_group_with_endpoint(ep, list);
    if (NULL == epg)
    {
        return NULL;
    }
    peers = malloc(list_size(&epg->list_endpoint)*SIZE_ENDPOINT_STR);
    list_iterate_safe(get, n, &epg->list_endpoint)
    {
        char *str_tmp = NULL;
        ENDPOINT *tmp = list_get(get, ENDPOINT, list);

        if (!addr_equal(tmp->client_addr, ep->client_addr))
        {
            str_tmp = ep_tostring(tmp);
            strcat(peers, str_tmp);
            free(str_tmp);
            if (n != list)
            {
                strcat(peers, ";");
            }
        }
    }
    return peers;
}
