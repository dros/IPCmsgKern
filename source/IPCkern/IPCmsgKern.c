/*
 * QNXMsgStub.cpp
 *
 *  Created on: 24 Mar 2012
 *      Author: KNUDS00A
 */
#include "IPCmsgKern.h"
#include "../kernelModule/IPCmsgKern_mod.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int IPCmsgKern_fileid = -1;

  int IPCmsgKern_GetFileHandle()
  {
    if(IPCmsgKern_fileid == -1)
    {
      printf("open(/dev/IPCmsgKern)\n");
      IPCmsgKern_fileid = open("/dev/IPCmsgKern", O_RDWR);
      printf("IPCmsgKern_fileid %d \n",IPCmsgKern_fileid);
    }
    return IPCmsgKern_fileid;
  }  

  void IPCmsgKern_FillHeader(IPCmsgKern_message *pMsg, enum IPCmsgKern_msg_type msgType)
  {
    memcpy(pMsg->sendHeader.magicMsgGuard,IPCmsgKern_MAGIC_CONST_SEND, sizeof(pMsg->sendHeader.magicMsgGuard));
    pMsg->sendHeader.msgType = msgType;
  }

  int IPCmsgKern_replyMsgValid(IPCmsgKern_message *pMsg, enum IPCmsgKern_msg_type msgType, int rc)
  {
    if(0 == memcmp(pMsg->replyHeader.magicMsgGuard, IPCmsgKern_MAGIC_CONST_REPLY, sizeof(pMsg->replyHeader.magicMsgGuard)))
    {
      if((rc == sizeof(pMsg->replyHeader)) && (pMsg->replyHeader.msgType == msgType) )
      {
        return 1;
      }
    }
    return 0;
  }

  void* IOCmsgKern_copyBuffer(struct IPCmsgKern_iovec **toBuffer, size_t *toParts, const IPCmsgKern_iov* fromData, int fromParts)
  {
    *toBuffer = NULL;
    if(fromParts == 0)
    {
      *toParts = fromParts;
    }
    else if(fromParts > 0 && (fromParts <= IPCmsgKern_MAX_BUF_COUNT))
    {
      *toParts = fromParts;
      *toBuffer = (struct IPCmsgKern_iovec *)malloc(sizeof(struct IPCmsgKern_iovec) * fromParts);
      for(int i=0; i<fromParts; i++)
      {
        (*toBuffer)[i].iov_base = fromData[i].iov_base;
        (*toBuffer)[i].iov_len = fromData[i].iov_len;
      }
    }
    else
    {
      *toParts = 0;
    }
    return *toBuffer;
  }


  int IPCmsgKern_CloseFileHanle()
  {
    if(IPCmsgKern_fileid != -1)
    {
      close(IPCmsgKern_fileid);
      IPCmsgKern_fileid = -1;
    }
    return IPCmsgKern_fileid;
  }


  int IPCmsgKern_ChannelCreate( unsigned int flags )
  {
    IPCmsgKern_message message;
    IPCmsgKern_FillHeader(&message,IPCmsgKern_msg_type_create);
    message.sendHeader.u.create.pid = getpid();

    int rc = read(IPCmsgKern_GetFileHandle(),&message,sizeof(message.sendHeader));
    if(IPCmsgKern_replyMsgValid(&message,IPCmsgKern_msg_type_create,rc))
    {
      return message.replyHeader.u.create.channelId;
    }
    return -1;
  }

  int IPCmsgKern_ChannelDestroy(int chid)
  {
    IPCmsgKern_message message;
    IPCmsgKern_FillHeader(&message,IPCmsgKern_msg_type_destroy);
    message.sendHeader.u.destroy.channelId = chid;

    int rc = read(IPCmsgKern_GetFileHandle(),&message,sizeof(message.sendHeader));
    if(IPCmsgKern_replyMsgValid(&message,IPCmsgKern_msg_type_destroy,rc))
    {
      return message.replyHeader.u.destroy.channelId;
    }
    return -1;
  }
  int IPCmsgKern_MsgInfo( int rcvid, IPCmsgKern_msgInfo* info )
  {
    return 0;
  }

  int IPCmsgKern_ConnectAttach( int processId, int channelId, unsigned index, int flags )
  {
    IPCmsgKern_message message;
    IPCmsgKern_FillHeader(&message,IPCmsgKern_msg_type_attach);
    message.sendHeader.u.attach.pid = processId;
    message.sendHeader.u.attach.channelId = channelId;

    int rc = read(IPCmsgKern_GetFileHandle(),&message,sizeof(message.sendHeader));
    if(IPCmsgKern_replyMsgValid(&message,IPCmsgKern_msg_type_attach,rc))
    {
      return message.replyHeader.u.attach.connectionId;
    }
    return -1;
  }

  int IPCmsgKern_ConnectDetach( int connectionId )
  {
    IPCmsgKern_message message;
    IPCmsgKern_FillHeader(&message,IPCmsgKern_msg_type_ditach);
    message.sendHeader.u.ditach.connectionId = connectionId;

    int rc = read(IPCmsgKern_GetFileHandle(),&message,sizeof(message.sendHeader));
    if(IPCmsgKern_replyMsgValid(&message,IPCmsgKern_msg_type_attach,rc))
    {
      return 0;
    }
    return -1;
  }

  int IPCmsgKern_MsgSend( int connectionId, const IPCmsgKern_iov* sendData, int sendParts, const IPCmsgKern_iov* receiveData, int receiveParts )
  {
    IPCmsgKern_message message;
    IPCmsgKern_FillHeader(&message,IPCmsgKern_msg_type_msg_send);
    message.sendHeader.u.msg_send.connectionId = connectionId;
    void *sendPtr = IOCmsgKern_copyBuffer(&message.sendHeader.u.msg_send.sendData,&message.sendHeader.u.msg_send.sendBufCount,sendData,sendParts);
    void *receivePtr = IOCmsgKern_copyBuffer(&message.sendHeader.u.msg_send.receiveData,&message.sendHeader.u.msg_send.receiveBufCount,receiveData,receiveParts);
    message.sendHeader.u.msg_send.priority = 10;

    int rc = read(IPCmsgKern_GetFileHandle(),&message,sizeof(message.sendHeader));
    if(sendPtr != 0) free(sendPtr);
    if(receivePtr != 0) free(receivePtr);
    if(IPCmsgKern_replyMsgValid(&message,IPCmsgKern_msg_type_msg_send,rc))
    {
      return message.replyHeader.u.msg_send.status;
    }
    return -1;
  }

  int IPCmsgKern_MsgSendPulse(int connectionId, int priority, int code, int value)
  {
    return 0;
  }
  int IPCmsgKern_MsgError( int receiveId, int error )
  {
    return 0;
  }

  int IPCmsgKern_MsgReceive( int channelId, const IPCmsgKern_iov * receiveData, int receiveParts, IPCmsgKern_msgInfo * info )
  {
    IPCmsgKern_message message;
    IPCmsgKern_FillHeader(&message,IPCmsgKern_msg_type_receive);
    message.sendHeader.u.msg_receive.channelId = channelId;
    void *receivePtr = IOCmsgKern_copyBuffer(&message.sendHeader.u.msg_receive.receiveData,&message.sendHeader.u.msg_receive.receiveBufCount,receiveData,receiveParts);

    int rc = read(IPCmsgKern_GetFileHandle(),&message,sizeof(message.sendHeader));
    if(receivePtr != 0) free(receivePtr);
    if(IPCmsgKern_replyMsgValid(&message,IPCmsgKern_msg_type_receive,rc))
    {
      if(info != NULL)
      {
        info->srcmsglen = message.replyHeader.u.msg_receive.readLength;
      }
      return message.replyHeader.u.msg_receive.receiveId;
    }
    return -1;
  }

  int IPCmsgKern_MsgRead( int receiveId, const IPCmsgKern_iov* receiveData, int receiveParts, int offset )
  {
    IPCmsgKern_message message;
    IPCmsgKern_FillHeader(&message,IPCmsgKern_msg_type_msg_read);
    message.sendHeader.u.msg_read.receiveId = receiveId;
    message.sendHeader.u.msg_read.offset = offset;
    void *receivePtr = IOCmsgKern_copyBuffer(&message.sendHeader.u.msg_read.receiveData,&message.sendHeader.u.msg_read.receiveBufCount,receiveData,receiveParts);

    int rc = read(IPCmsgKern_GetFileHandle(),&message,sizeof(message.sendHeader));
    if(receivePtr != 0) free(receivePtr);
    if(IPCmsgKern_replyMsgValid(&message,IPCmsgKern_msg_type_msg_read,rc))
    {
      return message.replyHeader.u.msg_read.readLength;
    }
    return -1;
  }
  int IPCmsgKern_MsgReply( int receiveId, int status, const IPCmsgKern_iov* replyData, int replyParts )
  {
    IPCmsgKern_message message;
    IPCmsgKern_FillHeader(&message,IPCmsgKern_msg_type_reply);
    message.sendHeader.u.msg_reply.receiveId = receiveId;
    message.sendHeader.u.msg_reply.status = status;
    void *replyPtr = IOCmsgKern_copyBuffer(&message.sendHeader.u.msg_reply.replyData,&message.sendHeader.u.msg_reply.replyBufCount,replyData,replyParts);

    int rc = read(IPCmsgKern_GetFileHandle(),&message,sizeof(message.sendHeader));
    if(replyPtr != 0) free(replyPtr);
    if(IPCmsgKern_replyMsgValid(&message,IPCmsgKern_msg_type_reply,rc))
    {
      return message.replyHeader.u.msg_reply.retValue;
    }
    return -1;
  }

  int IPCmsgKern_timerCreate( IPCmsgKern_ClockType clock_id, IPCmsgKern_sigevent * evp, size_t * timerid )
  {
    return 0;
  }
  int IPCmsgKern_timerDelete( size_t timerid )
  {
    return 0;
  }
  int IPCmsgKern_timerSettime( size_t timerid, int flags, const IPCmsgKern_itimerspec * value, IPCmsgKern_itimerspec * ovalue )
  {
    return 0;
  }
  int IPCmsgKern_timerGettime( size_t timerid, IPCmsgKern_itimerspec *value )
  {
    return 0;
  }


  int IPCmsgKern_InterruptAttachEvent(int interruptNr, const IPCmsgKern_sigevent *__event, unsigned int __flags)
  {
    return 0;
  }

