/*
 * @Author: HandsomeChen
 * @Date: 2019-12-18 17:39:22
 * @LastEditTime : 2019-12-18 17:39:22
 * @LastEditors  : HandsomeChen
 * @Description: This is description
 * @FilePath: /stun frame/useful structure/tests/test_endpoint.c
 */
#include "../endpoint.h"
#include "../user.h"
#include "../list.h"
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

void test_create()
{
    struct sockaddr_in fd;
    fd.sin_addr.s_addr = inet_addr("127.0.0.1");
    fd.sin_family = AF_INET;
    fd.sin_port = htons(10000);
    USER *user = user_create_user("chen", "123", "123");
    ENDPOINT *ep = ep_create_endpoint(user, NAT_RESTRICTED_CONE, fd);
    printf("%s\n", ep_tostring(ep));
}

void test_get_remove()
{
    LIST_HEAD(list_group);
    INIT_LIST(list_group);
    struct sockaddr_in fd;
    fd.sin_addr.s_addr = inet_addr("127.0.0.1");
    fd.sin_family = AF_INET;
    fd.sin_port = htons(10000);
    USER *user = user_create_user("chen", "123", "123");
    ENDPOINT *ep = ep_create_endpoint(user, NAT_RESTRICTED_CONE, fd);
    ENDPOINT *ep2 = ep_create_endpoint(user, NAT_RESTRICTED_CONE, fd);
    ENDPOINT_GROUP *epg0 = ep_create_endpoint_group();
    assert(epg0->dsc == 0);
    ENDPOINT_GROUP *epg1 = ep_create_endpoint_group();
    assert(epg1->dsc == 1);

    LIST_ADD(&epg0->list, &list_group);
    LIST_ADD(&epg1->list, &list_group);

    ep->group_dsc = epg1->dsc;
    ep2->group_dsc = epg1->dsc;

    LIST_ADD(&ep->list, &epg1->list_endpoint);
    LIST_ADD(&ep2->list, &epg1->list_endpoint);
    LIST_ADD(&ep->default_list, &epg0->list_endpoint);
    LIST_ADD(&ep2->default_list, &epg0->list_endpoint);
    ENDPOINT_GROUP *tmp_epg = ep_get_endpoint_group_with_endpoint(ep, &list_group);
    // assert(tmp_epg == NULL);
    assert(tmp_epg->dsc == epg1->dsc);
    ENDPOINT *tmp_ep = ep_get_endpoint_with_sock(fd, &epg1->list_endpoint, LIST_JOINED);
    assert(addr_equal(tmp_ep->client_addr, ep->client_addr));

    ENDPOINT *tmp_ep2 = ep_get_endpoint_with_user(user, &epg0->list_endpoint, LIST_DEFAULT);
    assert(addr_equal(tmp_ep2->client_addr, ep->client_addr));
    printf("%s", ep_get_peers_from_group(ep, &list_group));
    ep_remove_endpoint_from_group(ep, &epg1->list_endpoint, LIST_JOINED);
    assert(1 == list_size(&epg1->list_endpoint));
    ep_remove_group(epg1, &list_group);
    assert(1 == list_size(&list_group));
}

void test_str()
{
    char text[50];
    sprintf(text, "%s:%s:%d:%d", "Server", "127.0.0.1", 3478, 0);
    ENDPOINT *ep = ep_fromstring(text);
    printf("%s", ep_tostring(ep));
}

int main()
{
    test_create();
    test_get_remove();
    test_str();
}