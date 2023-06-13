/*
 * QNXMsgStub.cpp
 *
 *  Created on: 24 Mar 2012
 *      Author: KNUDS00A
 */
#include "../IPCkern/IPCmsgKern.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int channelId;
int pid;
int connectionId;

void server()
{
  char dataBuf[200];

  IPCmsgKern_iov iov[2];
  iov[0].iov_base = dataBuf;
  iov[0].iov_len = sizeof(dataBuf);

  IPCmsgKern_msgInfo info;

int run = 1;
  while(run)
  {
    memset(dataBuf,0,sizeof(dataBuf));
    int receiveId = IPCmsgKern_MsgReceive( channelId, iov, 1, &info );
  
    dataBuf[sizeof(dataBuf)-1] = 0;
    printf("receiveId : %d\n", receiveId);
    printf("dataBuf : %s\n", dataBuf);
    if(dataBuf[0] = 42)
    {
      printf("run = 0");
      run = 0;
    }
    memset(dataBuf,0,sizeof(dataBuf));
    strcpy(dataBuf, "hello world");


    int err = IPCmsgKern_MsgReply( receiveId, 0, iov, 1);
    printf("err : %d\n", err);
  }
  printf("Exit server");

}


int main(int argc, char *argv[])
{
  channelId = IPCmsgKern_ChannelCreate( 0 );
  pid = getpid();
  printf("channelId %d\n", channelId);
  printf("pid %d\n", pid);

  connectionId = IPCmsgKern_ConnectAttach( pid, channelId, 0,0);
  printf("connectionId %d\n", connectionId);


  if(0 == fork())
  {
    server();
  }
  else
  {
    IPCmsgKern_iov sendData[1];
    IPCmsgKern_iov receiveData[1];
  
    char sendDataBuf[200];
    char receiveDataBuf[100];
  
    sendData[0].iov_base = sendDataBuf;
    sendData[0].iov_len = sizeof(sendDataBuf);
    receiveData[0].iov_base = receiveDataBuf;
    receiveData[0].iov_len = sizeof(receiveDataBuf);
  
    memset(sendDataBuf,0,sizeof(sendDataBuf));
    strcpy(sendDataBuf, "It is nice 2");
    memset(receiveDataBuf,0,sizeof(receiveDataBuf));
  
    int err = IPCmsgKern_MsgSend(connectionId, sendData, 1, receiveData, 1 );
    printf("err : %d\n", err);
    receiveDataBuf[sizeof(receiveDataBuf) -1] = 0;
    printf("receiveDataBuf %s\n",receiveDataBuf);
  
  
    memset(sendDataBuf,0,sizeof(sendDataBuf));
    strcpy(sendDataBuf, "To be a cat 42");
    memset(receiveDataBuf,0,sizeof(receiveDataBuf));
  
    for(int i=0; i<1000; i++)
    {
      err = IPCmsgKern_MsgSend(connectionId, sendData, 1, receiveData, 1 );
    }
    printf("err : %d\n", err);
    receiveDataBuf[sizeof(receiveDataBuf) -1] = 0;
    printf("receiveDataBuf %s\n",receiveDataBuf);
  
    sendDataBuf[0] = 42;
    err = IPCmsgKern_MsgSend(connectionId, sendData, 1, receiveData, 1 );
    sleep(1);
    
    IPCmsgKern_ConnectDetach(connectionId );
    IPCmsgKern_ChannelDestroy(channelId);
    IPCmsgKern_CloseFileHanle();
  }
  return 0;
}


