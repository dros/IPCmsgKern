/*
 * IPCmsgKern.h
 *
 *  Created on: 23 Mar 2012
 *      Author: Andreas Egebjerg
 */

#ifndef IPC_MSG_KERN_H_
#define IPC_MSG_KERN_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stddef.h>

typedef struct 
  {
    void *iov_base;	/* Pointer to data.  */
    size_t iov_len;	/* Length of data.  */
  } IPCmsgKern_iov;


typedef struct {              /* _msg_info  _server_info */
  int         processId;    /*  client    server */
  int           threadId;    /*  thread    n/a */
  int           channelId;   /*  server    server */
  int           scoid;    /*  server    server */
  int           connectionId;   /*  client    client */
  int           msglen;   /*  msg     n/a */
  int           srcmsglen;  /*  thread    n/a */
  int           dstmsglen;  /*  thread    n/a */
  int           priority; /*  thread    n/a */
  int           flags;    /*  n/a     client */
} IPCmsgKern_msgInfo;

union IPCmsgKern_sigval {
	size_t   sival_int;
    void     * sival_ptr;
};

typedef struct {
  int       sigev_notify;
  int       sigev_signo;
  int       sigev_coid;
  int       sigev_id;
  short     sigev_code;
  short     sigev_priority;
  union IPCmsgKern_sigval    sigev_value;
} IPCmsgKern_sigevent;


typedef struct
{
  long        tv_sec;         ///< Seconds 
  long        tv_nsec;        ///< Nanoseconds 
} IPCmsgKern_timespec;

/// Define the itimerspec structure
typedef struct
{
  IPCmsgKern_timespec it_interval;    ///< Timer period
  IPCmsgKern_timespec it_value;       ///< Timer expiration
} IPCmsgKern_itimerspec;

typedef enum
{
  IPCmsgKern_ClockMonocronic,
  IPCmsgKern_ClockReal
} IPCmsgKern_ClockType;


  int IPCmsgKern_CloseFileHanle();
  int IPCmsgKern_ChannelCreate( unsigned int flags );
  int IPCmsgKern_ChannelDestroy(int channelId);
  int IPCmsgKern_MsgInfo( int rcvid, IPCmsgKern_msgInfo* info );

  int IPCmsgKern_ConnectAttach( int processId, int channelId, unsigned index, int flags );
  int IPCmsgKern_ConnectDetach( int connectionId );

  int IPCmsgKern_MsgSend( int connectionId, const IPCmsgKern_iov* sendData, int sendParts, const IPCmsgKern_iov* receiveData, int receiveParts );
  int IPCmsgKern_MsgSendPulse(int connectionId, int priority, int code, int value);
  int IPCmsgKern_MsgError( int receiveId, int error );

  int IPCmsgKern_MsgReceive( int channelId, const IPCmsgKern_iov * receiveData, int receiveParts, IPCmsgKern_msgInfo * info );

  int IPCmsgKern_MsgRead( int receiveId, const IPCmsgKern_iov* receiveData, int receiveParts, int offset );
  int IPCmsgKern_MsgReply( int receiveId, int status, const IPCmsgKern_iov* replyData, int replyParts );

  int IPCmsgKern_timerCreate( IPCmsgKern_ClockType clock_id, IPCmsgKern_sigevent * evp, size_t * timerid );
  int IPCmsgKern_timerDelete( size_t timerid );
  int IPCmsgKern_timerSettime( size_t timerid, int flags, const IPCmsgKern_itimerspec * value, IPCmsgKern_itimerspec * ovalue );
  int IPCmsgKern_timerGettime( size_t timerid, IPCmsgKern_itimerspec *value );


  int IPCmsgKern_InterruptAttachEvent(int interruptNr, const IPCmsgKern_sigevent *__event, unsigned int __flags);

#ifdef __cplusplus
}
#endif

#endif /* IPC_MSG_KERN_H_ */
