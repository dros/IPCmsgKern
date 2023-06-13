#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/list.h>

#include "IPCmsgKernModMsg.h"

static void copyUserData(char *pDist, char *pSrc, size_t len)
{
   while(len)
   {
     get_user(*pDist,pSrc);
     pDist++;
     pSrc++;
     len--;
   }
}

static void copyToUserData(char *pDist, char *pSrc, size_t len)
{
   while(len)
   {
     put_user(*pSrc,pDist);
     pDist++;
     pSrc++;
     len--;
   }
}


static ssize_t handle_msg_request(IPCmsgKern_sendHeader *pMsg, char *buffer, fdHanle_object *fdHandle)
{
  IPCmsgKern_replyHeader reply;
  memset(&reply, 0, sizeof(reply));
  memcpy(&reply.magicMsgGuard, IPCmsgKern_MAGIC_CONST_REPLY, sizeof(reply.magicMsgGuard));
  reply.msgType = pMsg->msgType;
  reply.response = -1;
  debug(printk(KERN_INFO "IPCmsgKern pMsg->msgType id %d\n", pMsg->msgType);)

  switch(pMsg->msgType)
  {
    case IPCmsgKern_msg_type_create:
    {
      int chid = CreateChannel(pMsg->u.create.pid, fdHandle);
      debug(printk(KERN_INFO "IPCmsgKern created channel with id %d\n", chid);)
      if(chid > 0)
      {
        reply.response = 0;
        reply.u.create.channelId = chid;
      }
    }
      break;
    case IPCmsgKern_msg_type_destroy:
      break;
    case IPCmsgKern_msg_type_attach:
    {
      int connectionid = AttachToChannel(pMsg->u.attach.pid, pMsg->u.attach.channelId, fdHandle);
      debug(printk(KERN_INFO "IPCmsgKern attach to connection with id %d\n", connectionid);)
      if(connectionid > 0)
      {
        reply.response = 0;
        reply.u.attach.connectionId = connectionid;
      }
    }
      break;
    case IPCmsgKern_msg_type_ditach:
      break;
    case IPCmsgKern_msg_type_msg_send:
    {
      reply.response = 0;
      reply.u.msg_send.status = msgSend(pMsg);
    }
      break;
    case IPCmsgKern_msg_type_msg_read:
      break;
    case IPCmsgKern_msg_type_reply:
    {
      reply.response = 0;
      reply.u.msg_reply.retValue = msgReply(pMsg);
    }
      break;
    case IPCmsgKern_msg_type_receive:
    {
      reply.response = 0;
      reply.u.msg_receive.receiveId = msgReceive(pMsg);
    }
      break;
    case IPCmsgKern_msg_type_send_pulse:
      break;
    case IPCmsgKern_msg_type_timer_create:
    case IPCmsgKern_msg_type_timer_delete:
    case IPCmsgKern_msg_type_end:
      break;
  }

  if(reply.response == 0)
  {
    copyToUserData(buffer, (char *)&reply, sizeof(reply));
    return sizeof(reply);
  }
  return -1;
}

static void handle_release(fdHanle_object *fdHandle)
{
  printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);

  mutex_lock(&fdHandle->fdHanle_mutex);
  {
    fdConnection_object *i;
    printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);
    list_for_each_entry(i, &fdHandle->connectionList, list)
    {
      channel_object *obj = __channel_find(i->chid);
      if(obj != NULL)
      {
        connection_object *connectionObj = __connection_find(obj, i->connectionNr);
        if(connectionObj != NULL)
        {
          connectionObj->dead = true;
          up(&connectionObj->msgReadySem);
        }
      }
    }
  }
  {
    fdChannel_object *i;
  printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);
    list_for_each_entry(i, &fdHandle->channelList, list)
    {
      channel_object *obj = __channel_find(i->chid);
      if(obj != NULL)
      {
        connection_object *iCon;
        obj->dead = true;
        up(&obj->msgInQueSem);

        list_for_each_entry(iCon, &obj->connectionList, list)
        {
          iCon->dead = true;
          up(&iCon->msgReadySem);
        }
      }
    }
  }
  fdHandle->dead = true;
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
  mutex_unlock(&fdHandle->fdHanle_mutex);
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)

}


/* Protects the cache, cache_num, and the objects within it */
static DEFINE_MUTEX(channel_lock);
static LIST_HEAD(channel_list);
static int nextChannelId = 100;

int GetNextChannelId(void)
{
  return nextChannelId++;
}


int msgSend(IPCmsgKern_sendHeader *pMsg)
{
  uint16_t channelId = (pMsg->u.msg_send.connectionId >> 16 ) & 0xFFFF;
  uint16_t connectionId = pMsg->u.msg_send.connectionId & 0xFFFF;
  channel_object *obj = __channel_find(channelId);
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)

  if((pMsg->u.msg_send.sendBufCount > IPCmsgKern_MAX_BUF_COUNT) || (pMsg->u.msg_send.receiveBufCount > IPCmsgKern_MAX_BUF_COUNT))
  {
    printk(KERN_INFO "IPCmsgKern to many read or write bufferes\n");
    return -1;
  }


  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
  if(obj != NULL)
  {
    connection_object *connectionObj = __connection_find(obj, connectionId);
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
    if(connectionObj != NULL)
    {
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
      if(connectionObj->dead) return -1;
      debug(printk(KERN_INFO "IPCmsgKern msgSend found connectio\n");)
      connectionObj->messageInQue = true;
      connectionObj->readBufCount = pMsg->u.msg_send.sendBufCount;
      copyUserData((char*)connectionObj->readData, (char*)pMsg->u.msg_send.sendData, pMsg->u.msg_send.sendBufCount * sizeof(struct IPCmsgKern_iovec));
      connectionObj->writeBufCount = pMsg->u.msg_send.receiveBufCount;
      copyUserData((char*)connectionObj->writeData, (char*)pMsg->u.msg_send.receiveData, pMsg->u.msg_send.receiveBufCount * sizeof(struct IPCmsgKern_iovec));

  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
      WriteMsg(connectionObj->dataBuf, connectionObj->writeData, connectionObj->writeBufCount, MAX_BUF_SIZE);
      connectionObj->dataLength = CalculateMessageSize(connectionObj->writeData, connectionObj->writeBufCount);

      mutex_lock(&obj->msgQueMutex);
      list_add_tail(&connectionObj->messageQueObj.list,&obj->messagePrioQue[pMsg->u.msg_send.priority]);
      up(&obj->msgInQueSem);
      mutex_unlock(&obj->msgQueMutex);
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
      down(&connectionObj->msgReadySem);
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
      if(connectionObj->dead) return -1;
      connectionObj->messageInQue = false;
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
  debug(printk(KERN_INFO "IPCmsgKern dataLength : %ld \n", connectionObj->dataLength);)
      ReadMsg(connectionObj->dataBuf, connectionObj->readData, connectionObj->readBufCount, connectionObj->dataLength, 0);
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
      return connectionObj->status;
    }
  }
  return -1;
}

int msgReceive(IPCmsgKern_sendHeader *pMsg)
{
  uint16_t channelId = (pMsg->u.msg_receive.channelId & 0xFFFF);
  channel_object *obj = __channel_find(channelId);
  size_t        readBufCount = 0;
  struct IPCmsgKern_iovec readData[IPCmsgKern_MAX_BUF_COUNT];
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)

  if(pMsg->u.msg_receive.receiveBufCount > IPCmsgKern_MAX_BUF_COUNT)
  {
    printk(KERN_INFO "IPCmsgKern to many read bufferes\n");
    return -1;
  }
  readBufCount = pMsg->u.msg_receive.receiveBufCount;
  copyUserData((char*)readData, (char*)pMsg->u.msg_receive.receiveData, pMsg->u.msg_receive.receiveBufCount * sizeof(struct IPCmsgKern_iovec));
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
  debug(printk(KERN_INFO "IPCmsgKern readBufCount : %ld \n", readBufCount);)

  while(obj != NULL)
  {
    int i = 255;
    bool found = false;
    int prio = 0;
    if(obj->dead) return -1;
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
    down(&obj->msgInQueSem);
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
    if(obj->dead) return -1;
    mutex_lock(&obj->msgQueMutex);
    for(i=255; i>=0 && !found; i--)
    {
      if(! list_empty(&obj->messagePrioQue[i]))
      {
        found = true;
        prio = i;
      }
    }
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
    if (found)
    {
      messageQue_object *pMsg = (messageQue_object*)obj->messagePrioQue[prio].next;
      connection_object *connectionObj = (connection_object *)pMsg->pConnection;
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
        list_del(&pMsg->list);
      mutex_unlock(&obj->msgQueMutex);
      if(!connectionObj->dead)
      {
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
  debug(printk(KERN_INFO "IPCmsgKern connectionObj->dataLength : %ld \n", connectionObj->dataLength);)
        ReadMsg(connectionObj->dataBuf, readData, readBufCount, connectionObj->dataLength, 0);
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
        return connectionObj->receiveId;
      }
    }
    else
    {
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
      mutex_unlock(&obj->msgQueMutex);
    }
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
  }
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
  return -1;
}


int msgReply(IPCmsgKern_sendHeader *pMsg)
{
  uint16_t channelId = (pMsg->u.msg_reply.receiveId >> 16 ) & 0xFFFF;
  uint16_t connectionId = pMsg->u.msg_reply.receiveId & 0xFFFF;
  channel_object *obj = __channel_find(channelId);
  size_t        writeBufCount;
  struct IPCmsgKern_iovec writeData[IPCmsgKern_MAX_BUF_COUNT];
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)

  if(pMsg->u.msg_reply.replyBufCount > IPCmsgKern_MAX_BUF_COUNT)
  {
    debug(printk(KERN_INFO "IPCmsgKern to many write bufferes\n");)
    return -1;
  }
  writeBufCount = pMsg->u.msg_reply.replyBufCount;
  copyUserData((char*)writeData, (char*)pMsg->u.msg_reply.replyData, pMsg->u.msg_reply.replyBufCount * sizeof(struct IPCmsgKern_iovec));

  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)

  if(obj != NULL)
  {
    if(obj->dead) return -1;
  }

  if(obj != NULL)
  {
    connection_object *connectionObj = __connection_find(obj, connectionId);
    debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
  if(connectionObj != NULL)
    {
      if(connectionObj->dead) return -1;
      debug(printk(KERN_INFO "IPCmsgKern msgSend found connectio\n");)

      WriteMsg(connectionObj->dataBuf, writeData, writeBufCount, MAX_BUF_SIZE);
      connectionObj->dataLength = CalculateMessageSize(writeData, writeBufCount);
  debug(printk(KERN_INFO "IPCmsgKern dataLength : %ld \n", connectionObj->dataLength);)
      connectionObj->status = pMsg->u.msg_reply.status;
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
      up(&connectionObj->msgReadySem);
  debug(printk(KERN_INFO "IPCmsgKern %s : %d \n", __FUNCTION__ , __LINE__);)
      return 0;
    }
  }
  return -1;
}


int CreateChannel(int pid, fdHanle_object *fdHandle)
{
  channel_object *obj = NULL;
  if ((obj = kmalloc(sizeof(*obj), GFP_KERNEL)) != NULL)
  {
    obj->dead = false;
    obj->pendingMessages = 0;
    {
    	static struct lock_class_key __key;
  	__mutex_init(&(obj->connectionMutex), "connectionMutex", &__key);
    }
    {
    	static struct lock_class_key __key;
  	__mutex_init(&(obj->msgQueMutex), "connectionMutex", &__key);
    }
    INIT_LIST_HEAD(&(obj->connectionList));
    {
      int i = 0;
      for(i=0; i<256; i++)
      {
        INIT_LIST_HEAD(&(obj->messagePrioQue[i]));
      }
    }
    sema_init(&obj->msgInQueSem, 0);
    mutex_lock(&channel_lock);
    obj->chid = GetNextChannelId();
    obj->pid = pid;
    __channel_add(obj);
    mutex_unlock(&channel_lock);
    {
      fdChannel_object *ch_obj = NULL;
      if ((ch_obj = kmalloc(sizeof(*ch_obj), GFP_KERNEL)) != NULL)
      {
        ch_obj->chid = obj->chid;
        mutex_lock(&fdHandle->fdHanle_mutex);
        list_add(&ch_obj->list, &fdHandle->channelList);
        mutex_unlock(&fdHandle->fdHanle_mutex);
      }
    }
    return obj->chid;
  }
  return 0;
}

int AttachToChannel(int pid, int channelId, fdHanle_object *fdHandle)
{
  int connectionId = -1;
  channel_object *obj = __channel_find(channelId);
  if(obj != NULL)
  {
    if(obj->pid == pid)
    {
      connection_object *con_obj = NULL;
      if ((con_obj = kmalloc(sizeof(*con_obj), GFP_KERNEL)) != NULL)
      {
        con_obj->dead = false;
        con_obj->messageInQue = false;
        con_obj->messageQueObj.pConnection = con_obj;
        sema_init(&con_obj->msgReadySem, 0);
        mutex_lock(&obj->connectionMutex);
        con_obj->connectionNr = obj->nextConnectionId++;
        con_obj->receiveId = (channelId << 16) | con_obj->connectionNr;
        list_add(&con_obj->list, &obj->connectionList);
        mutex_unlock(&obj->connectionMutex);
        connectionId = con_obj->receiveId;
        {
          fdConnection_object *fd_con_obj = NULL;
          if ((fd_con_obj = kmalloc(sizeof(*fd_con_obj), GFP_KERNEL)) != NULL)
          {
            fd_con_obj->chid = obj->chid;
            fd_con_obj->connectionNr = con_obj->connectionNr;
  
            mutex_lock(&fdHandle->fdHanle_mutex);
            list_add(&fd_con_obj->list, &fdHandle->connectionList);
            mutex_unlock(&fdHandle->fdHanle_mutex);
          }
        }
      }
    }
  }
  return connectionId;
}


  void WriteMsg(char *mpData, struct IPCmsgKern_iovec *iov, int parts, int maxSize)
  {
    int messageSize = CalculateMessageSize(iov, parts);
    int dataLength = (messageSize < maxSize) ? messageSize : maxSize;
    int index=0;

    int datapointer = 0;
    for (index=0; index<parts; index++)
    {
      int length = ((int)iov[index].iov_len < (dataLength - datapointer)) ? iov[index].iov_len : (dataLength - datapointer);
      memcpy(&(mpData[datapointer]), iov[index].iov_base, length);
      datapointer += iov[index].iov_len;
    }
  }

  int ReadMsg(char *mpData, struct IPCmsgKern_iovec *iov, int parts, int maxSize, int offset)
  {
    int messageSize = CalculateMessageSize(iov, parts) + offset;
    int dataLength = (messageSize < maxSize) ? messageSize : maxSize;
    int index=0;

    int datapointer = offset;
    for (index=0; index<parts; index++)
    {
      int length = ((int)iov[index].iov_len < (dataLength - datapointer)) ? iov[index].iov_len : (dataLength - datapointer);
      memcpy(iov[index].iov_base, &(mpData[datapointer]), length);
      datapointer += length;
    }
    return datapointer - offset;
  }

  int CalculateMessageSize(struct IPCmsgKern_iovec *iov, int parts)
  {
    int size = 0;
    int i = 0;
    for (i=0; i< parts; i++)
    {
      size += iov[i].iov_len;
    }
    return size;
  }


connection_object *__connection_find(channel_object *channelObj, uint16_t connectionId)
{
    connection_object *i;
    mutex_lock(&channelObj->connectionMutex);

    list_for_each_entry(i, &channelObj->connectionList, list)
      if (i->connectionNr == connectionId) {
        mutex_unlock(&channelObj->connectionMutex);
        return i;
      }
    mutex_unlock(&channelObj->connectionMutex);
    return NULL;
}
/* Must be holding cache_lock */
channel_object *__channel_find(int channelId)
{
    channel_object *i;
    mutex_lock(&channel_lock);

    list_for_each_entry(i, &channel_list, list)
      if (i->chid == channelId) {
        mutex_unlock(&channel_lock);
        return i;
      }
    mutex_unlock(&channel_lock);
    return NULL;
}

// Must be holding cache_lock 
void __channel_delete(channel_object *obj)
{
        BUG_ON(!obj);
        list_del(&obj->list);
        kfree(obj);
}

// Must be holding cache_lock
void __channel_add(channel_object *obj)
{
  list_add(&obj->list, &channel_list);
}

void channel_delete(int channelId)
{
  mutex_lock(&channel_lock);
  __channel_delete(__channel_find(channelId));
  mutex_unlock(&channel_lock);
}
