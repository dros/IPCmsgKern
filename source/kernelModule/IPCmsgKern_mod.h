#ifndef __IPCMSGKERN_MOD_H__
#define __IPCMSGKERN_MOD_H__

enum IPCmsgKern_msg_type
{
  IPCmsgKern_msg_type_create = 10, // make sure that it is a valid request
  IPCmsgKern_msg_type_destroy,
  IPCmsgKern_msg_type_attach,
  IPCmsgKern_msg_type_ditach,
  IPCmsgKern_msg_type_msg_send,
  IPCmsgKern_msg_type_msg_read,
  IPCmsgKern_msg_type_reply,
  IPCmsgKern_msg_type_receive,
  IPCmsgKern_msg_type_send_pulse,
  IPCmsgKern_msg_type_timer_create,
  IPCmsgKern_msg_type_timer_delete,
  IPCmsgKern_msg_type_end
};

enum IPCmsgKern_constants
{
  IPCmsgKern_MAX_BUF_COUNT = 100
};

#pragma pack(push, 1)
struct IPCmsgKern_iovec
  {
    void *iov_base;	/* Pointer to data.  */
    size_t iov_len;	/* Length of data.  */
  };

static const char IPCmsgKern_MAGIC_CONST_SEND[]  = {"IPCmsgS_0.01"};
static const char IPCmsgKern_MAGIC_CONST_REPLY[] = {"IPCmsgR_0.01"};

typedef struct
{
 char               magicMsgGuard[12]; // make sure that it is a valid request
 enum IPCmsgKern_msg_type msgType;
 union {
  struct { int pid;} create;
  struct { int channelId;} destroy;
  struct { int pid; int channelId;} attach;
  struct { int connectionId;} ditach;
  struct { int readLength;
    int                connectionId;
    uint8_t            priority;
    size_t             sendBufCount;
    struct IPCmsgKern_iovec  *sendData;
    size_t             receiveBufCount;
    struct IPCmsgKern_iovec  *receiveData;
  } msg_send;
  struct {
    int                receiveId;
    int                readOffset;
    int                offset;
    size_t             receiveBufCount;
    struct IPCmsgKern_iovec  *receiveData;
  } msg_read;
  struct { 
    int                receiveId;
    long               status;
    size_t             replyBufCount;
    struct IPCmsgKern_iovec  *replyData;
  } msg_reply;
  struct { 
    int                channelId;
    size_t             receiveBufCount;
    struct IPCmsgKern_iovec  *receiveData;
    } msg_receive;
  struct {
    int                connectionId;
    uint8_t            priority;
    int                code;
    int                value;
  } msg_send_pulse;
 } u;
} IPCmsgKern_sendHeader;

typedef struct
{
 char                     magicMsgGuard[12]; // make sure that it is a valid request
 enum IPCmsgKern_msg_type msgType;
 int                      response; // 0 = OK , <0 error
 union {
  struct { int channelId;} create;
  struct { int channelId;} destroy;
  struct { int connectionId;} attach;
  struct { int connectionId;} ditach;
  struct { long status;} msg_send;
  struct { int readLength;} msg_read;
  struct { int retValue;} msg_reply;
  struct { int readLength; int receiveId;} msg_receive;
  struct { int status;} msg_send_pulse;
 } u;
} IPCmsgKern_replyHeader;


typedef union
{
  IPCmsgKern_sendHeader sendHeader;
  IPCmsgKern_replyHeader replyHeader;
} IPCmsgKern_message;


#pragma pack(pop)

#endif