#ifndef __SWRE_WTC_MODULE_MSG_H__
#define __SWRE_WTC_MODULE_MSG_H__

#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <asm/errno.h>
#include <linux/types.h>

#include "IPCmsgKern_mod.h"

static void copyUserData(char *pDist, char *pSrc, size_t len);

typedef struct
{
  struct list_head list;
  uint16_t chid;
} fdChannel_object;

typedef struct
{
  struct list_head list;
  uint16_t chid;
  uint16_t connectionNr;
} fdConnection_object;

typedef struct
{
  struct mutex fdHanle_mutex;
  bool dead;
  struct list_head channelList;
  struct list_head connectionList;
} fdHanle_object;


static ssize_t handle_msg_request(IPCmsgKern_sendHeader *pMsg, char *buffer, fdHanle_object *fdHandle);
static void handle_release(fdHanle_object *fdHandle);

typedef struct 
{
  struct list_head list;
  void *pConnection; // connection_object
} messageQue_object;

#define MAX_BUF_SIZE 1000000
typedef struct
{
  struct list_head list;
  uint16_t      connectionNr;
  int           receiveId;
  bool          dead;
  bool          messageInQue;
  messageQue_object messageQueObj;
  struct semaphore msgReadySem;
  uint8_t       messagePriority;
  long          status;
  size_t        dataLength;
  char          dataBuf[MAX_BUF_SIZE];
  size_t        writeBufCount;
  size_t        readBufCount;
  struct IPCmsgKern_iovec writeData[IPCmsgKern_MAX_BUF_COUNT];
  struct IPCmsgKern_iovec readData[IPCmsgKern_MAX_BUF_COUNT];
} connection_object;

typedef struct
{
  struct list_head list;
  uint16_t chid;
  int pid;
  bool dead;
  uint16_t nextConnectionId;
  struct mutex connectionMutex;
  struct list_head connectionList;
  size_t pendingMessages;
  struct mutex msgQueMutex;
  struct semaphore msgInQueSem;
  struct list_head messagePrioQue[256];
} channel_object;








int GetNextChannelId(void);
int msgSend(IPCmsgKern_sendHeader *pMsg);
int CreateChannel(int pid, fdHanle_object *fdHandle);
int AttachToChannel(int pid, int channelId, fdHanle_object *fdHandle);

void WriteMsg(char *mpData, struct IPCmsgKern_iovec *iov, int parts, int maxSize);
int ReadMsg(char *mpData, struct IPCmsgKern_iovec *iov, int parts, int maxSize, int offset);
int msgReceive(IPCmsgKern_sendHeader *pMsg);
int msgReply(IPCmsgKern_sendHeader *pMsg);
int CalculateMessageSize(struct IPCmsgKern_iovec *iov, int parts);

connection_object *__connection_find(channel_object *channelObj, uint16_t connectionId);

// Protects the cache, cache_num, and the objects within it
// Must be holding cache_lock
channel_object *__channel_find(int channelId);

// Must be holding cache_lock
void __channel_delete(channel_object *obj);

// Must be holding cache_lock 
void __channel_add(channel_object *obj);

void channel_delete(int channelId);

#endif
