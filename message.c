/*
 * @Author: HandsomeChen
 * @Date: 2019-12-17 15:22:24
 * @LastEditTime : 2019-12-18 17:53:59
 * @LastEditors  : HandsomeChen
 */
#include "message.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

const char *msg_type_str(MessageType type) {
    switch(type) {
        case MSG_TYPE_LOGIN:        return "LOGIN";
        case MSG_TYPE_LIST:         return "LIST";
        case MSG_TYPE_JOIN:         return "JOIN";
        case MSG_TYPE_PEERS:        return "PEERS";
        case MSG_TYPE_PEERS_REPLY:  return "PEERS_REPLY";
        case MSG_TYPE_PUNCH:        return "PUNCH";
        case MSG_TYPE_QUIT:         return "QUIT";
        case MSG_TYPE_QUIT_REPLY:   return "QUIT_REPLY";
        case MSG_TYPE_LOGOUT:       return "LOGOUT";
        case MSG_TYPE_PING:         return "PING";
        case MSG_TYPE_PONG:         return "PONG";
        case MSG_TYPE_REPLY:        return "REPLY";
        case MSG_TYPE_TEXT:         return "TEXT";
        default:            return "UNKNOW";
    }
}

/* return bytes serialized */
int msg_unpack_msg(Message msg, char *buf, unsigned int buf_size) {
    if (buf_size < LEN_MSG_HEADER + msg.head.length) {
        printf("buf too small");
        return 0;
    }
    int16_t msg_magic = htons(msg.head.magic);
    int16_t msg_type = htons(msg.head.type);
    int32_t msg_length = htonl(msg.head.length);
    int index = 0;
    memcpy(buf + index, &msg_magic, LEN_MSG_MAGIC);
    index += LEN_MSG_MAGIC;
    memcpy(buf + index, &msg_type, LEN_MSG_TYPE);
    index += LEN_MSG_TYPE;
    memcpy(buf + index, &msg_length, LEN_MSG_LENGTH);
    index += LEN_MSG_LENGTH;
    memcpy(buf + index, msg.body, msg.head.length);
    index += msg.head.length;
    return index;
}

/*
    Message body is a pointer to buf + len(head)
*/
Message msg_stream_pack(char *buf, unsigned int buf_len) {
    Message m;
    memset(&m, 0, sizeof(m));
    if (buf_len < LEN_MSG_HEADER) {
        // at least we won't get an overflow
        return m;
    }
    int index = 0;
    m.head.magic = ntohs(*(uint16_t *)(buf + index));
    index += sizeof(uint16_t);
    if (m.head.magic != MSG_MAGIC) {
        return m;
    }
    m.head.type = ntohs(*(uint16_t *)(buf + index));
    index += sizeof(uint16_t);
    m.head.length = ntohl(*(uint32_t *)(buf + index));
    index += sizeof(uint32_t);
    if (index + m.head.length > buf_len) {
        printf("message declared body size(%d) is larger than what's received (%d), truncating\n",
                m.head.length, buf_len - LEN_MSG_HEADER);
        m.head.length = buf_len - index;
    }
    m.body = buf + index;
    return m;
}

// send a Message
int udp_send_msg(int sock, ENDPOINT *peer, Message msg) {
    char buf[SIZE_SEND_BUF] = {0};
    int wt_size = msg_unpack_msg(msg, buf, SIZE_SEND_BUF);
    return sendto(sock, buf, wt_size,
            MSG_DONTWAIT, (struct sockaddr *)&peer->client_addr, sizeof(peer->client_addr));
}
// send a buf with length
int udp_send_buf(int sock, ENDPOINT *peer,
        MessageType type, char *buf, unsigned int len) {
    Message m;
    m.head.magic = MSG_MAGIC;
    m.head.type = type;
    m.head.length = len;
    m.body = buf;
    return udp_send_msg(sock, peer, m);
}
// send a NULL terminated text
int udp_send_text(int sock, ENDPOINT *peer,
        MessageType type, char *text) {
    unsigned int len = text == NULL ? 0 : strlen(text);
    return udp_send_buf(sock, peer, type, text, len);
}
