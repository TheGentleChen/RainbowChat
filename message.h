/*
 * @Author: HandsomeChen
 * @Date: 2019-12-17 15:23:32
 * @LastEditTime : 2019-12-18 17:48:20
 * @LastEditors  : HandsomeChen
 */
#include <stdint.h>
#include "endpoint.h"

#define MSG_MAGIC 0x3478 
#define LEN_MSG_MAGIC 2
#define LEN_MSG_TYPE 2
#define LEN_MSG_LENGTH 4
#define LEN_MSG_HEADER LEN_MSG_MAGIC + LEN_MSG_TYPE + LEN_MSG_LENGTH
/* a message is a UDP datagram with following structure:
   -----16bits--+---16bits--+-----32bits----------+---len*8bits---+
   --  0x3478   + msg type  + msg length(exclude) + message body  +
   -------------+-----------+---------------------+---------------+
*/
#define SIZE_SEND_BUF 1024
#define SIZE_RECV_BUF 1024

typedef struct _Message Message;
typedef struct _MessageHead MessageHead;

enum _MessageType {
    MSG_TYPE_LOGIN = 0,
    MSG_TYPE_LIST,
    MSG_TYPE_JOIN,
    MSG_TYPE_PEERS,
    MSG_TYPE_PEERS_REPLY,
    MSG_TYPE_PUNCH,
    MSG_TYPE_PING,
    MSG_TYPE_PONG,
    MSG_TYPE_REPLY,
    MSG_TYPE_TEXT,
    MSG_TYPE_QUIT,
    MSG_TYPE_QUIT_REPLY,
    MSG_TYPE_LOGOUT
};
typedef enum _MessageType MessageType;

struct _MessageHead {
    uint16_t magic;
    uint16_t type;
    uint32_t length;
}__attribute__((packed));

struct _Message {
    MessageHead head;
    char *body;
};

const char *msg_type_str(MessageType type);
int msg_unpack_msg(Message msg, char *buf, unsigned int buf_size);
Message msg_stream_pack(char *buf, unsigned int buf_size);

// replay a Message
int udp_send_msg(int sock, ENDPOINT *peer, Message msg);
// reply a buf with length
int udp_send_buf(int sock, ENDPOINT *peer, MessageType type,
        char *buf, unsigned int len);
// reply a NULL terminated text
int udp_send_text(int sock, ENDPOINT *peer, MessageType type, char *text);
