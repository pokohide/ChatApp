/*
 * chat_server.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#define PORT 59630  /* ポート番号 */
#define PORT1 50001
#define PORT2 50002
#define PRINTF(...)  {printf("server: ");printf(__VA_ARGS__);}
#define WRITE(...)  {printf("server: ");write(__VA_ARGS__);printf("\n");}

void request (int req);
void end();

int *commSock;

int member=0;
int listenfd,fd1,fd2;
int len, buflen;
char buf[1024],buf2[1024];

int main(int argc, char *argv[]) 
{

  int i,j,max,maxflg=0;
  int req,reqflg=0;
  struct sockaddr_in server;
  struct sockaddr_in client,addr1,addr2;
  fd_set readfds;

  if(argc < 3){
    PRINTF("Usage:chat member_address db_address\n");
    exit(1);
  }
  
  commSock = (int *)malloc( sizeof(int)*(member+1) );
  commSock[0] = -1;
 
  /***************************************************************/
  /* リスニングソケット作成 */
  if ( ( listenfd = socket(PF_INET, SOCK_STREAM, 0) ) < 0 ) {
    perror("*** server: listen socket make error***");
    exit(1);
  }

  /* アドレスファミリ・ポート番号・IPアドレス設定 */
  bzero( (char *)&server, sizeof(server) );
  server.sin_family = PF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(PORT);

  /* リスニングソケットにアドレスを割り当て */
  if ( bind( listenfd, (struct sockaddr *)&server, 
         sizeof(server) ) < 0 ) {
    perror("*** server: listen address error ***");
    close(listenfd);
    exit(1);
  }
    
  /* リスニングソケット待ち受け */
  if ( listen( listenfd, 5 ) < 0 ) {
    perror("*** server: listen wait error***");
    close(listenfd);
    exit(1);
  }
  PRINTF("Waiting for connections from a client.\n" );
  /***************************************************************/
  /*memberソケット作成 */
  if ( ( fd1 = socket( PF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
    perror("*** server: [member] socket make error***");
    exit(1);
  }
  
  bzero( (char *)&addr1, sizeof(addr1) );
  addr1.sin_family = PF_INET;
  addr1.sin_addr.s_addr = inet_addr(argv[1]);
  addr1.sin_port = htons(PORT1);

  /*memberソケット接続要求 */
  if ( connect( fd1, (struct sockaddr *)&addr1, sizeof(addr1) ) < 0 ) {
    perror("*** server: [member] connect error***");
    close(fd1);
    exit(1);
  }else
    PRINTF("Connected to [member]\n" );
  /***************************************************************/
  /*dbソケット作成 */
  if ( ( fd2 = socket( PF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
    perror("*** server: [db] socket make error***");
    exit(1);
  }
  
  bzero( (char *)&addr2, sizeof(addr2) );
  addr2.sin_family = PF_INET;
  addr2.sin_addr.s_addr = inet_addr(argv[2]);
  addr2.sin_port = htons(PORT2);

  /*dbソケット接続要求 */
  if ( connect( fd2, (struct sockaddr *)&addr2, sizeof(addr2) ) < 0 ) {
    perror("*** server: [db] connect error***");
    close(fd2);
    exit(1);
  }else
    PRINTF("Connected to [db]\n" );
  /***************************************************************/

  //一番大きいソケット番号をmaxに
  max=0;
  if(max < listenfd)max = listenfd;
  if(max < fd1)max = fd1;
  if(max < fd2)max = fd2;

  /* データの送受信 */
  while (1) {

    FD_ZERO(&readfds);
    
    FD_SET(listenfd, &readfds);
    FD_SET(fd1, &readfds);
    FD_SET(fd2, &readfds);

    for(i = 0; i < member+1; i++){
      if(commSock[i] != -1){
        FD_SET(commSock[i], &readfds);
	if(max < commSock[i])max = commSock[i]; 
      }
    }
    
    select(max+1, &readfds, NULL, NULL, NULL);
    //new client
    /***************************************************************/
    if(FD_ISSET(listenfd, &readfds)){
      int s;
      // クライアントからの接続要求受付
      len = sizeof(client);
 
      PRINTF("クライアントからの接続要求受付\n");
      s = accept(listenfd, (struct sockaddr *)&client, &len);
 
      for(i = 0; i < member+1; i++){
	if(maxflg){
	  close(s);
	  break;
	}
        if(commSock[i] == -1){
          PRINTF("クライアント[%d]の接続を受付完了\n", i);
          commSock[i] = s;
	  
	  //名前の受け付け
	  if( (buflen = read( commSock[i], buf, sizeof(buf) ) ) <= 0){
	    PRINTF("name read error\n");
	    close(commSock[i]);
	    commSock[i] = -1;
	    break;
	  }
	  buf[buflen++]='\0';
	  //名前の表示
	  PRINTF("name is %s\n",buf);
	  //memberに報告 入室	  
	  //メッセージを整形
	  sprintf(buf2,"1 %d %s",i,buf);
	  //メッセージの表示
	  WRITE(1, buf2, strlen(buf2));
	  //memberに送信
	  PRINTF("<<< Sending to [member]...\n", j);
	  write(fd1,buf2,strlen(buf2));	  
	  
	  //ログの送信
	  req = i;
	  PRINTF("メンバー %d にログを送信します\n",req);
	  write(fd2,"REQUEST",8);
	  request(req);
	  
	  
	  if(i == member){
	    member++;
	    int *tmp = (int *)realloc(commSock, sizeof(int)*(member+1) );
	    if(tmp == NULL){
	      PRINTF("これ以上メンバーを追加できません\n");
	      maxflg = 1;//
	    }else{
	      commSock = tmp;
	      commSock[member] = -1;
	    }
	  }
          break;
        }
      }
      if(i == member){
        //接続中クライアントがいっぱいなので
        //受け付けたがすぐ切断
        close(s);
      }    
    }
    //client
    /***************************************************************/
    for(i = 0; i < member+1 && !reqflg; i++){
      if(commSock[i] == -1)continue;
      if(FD_ISSET(commSock[i], &readfds)){
	if ( ( buflen = read( commSock[i], buf, sizeof(buf) ) ) <= 0 ) {
	  PRINTF("fail read [%d]\n",i);
	  //memberに報告 退室
	  sprintf(buf2,"2 %d",i);
	  //メッセージの表示
	  WRITE(1, buf2, strlen(buf2));
	  //memberに送信
	  PRINTF("<<< Sending to [member]...\n", j);
	  write(fd1,buf2,strlen(buf2));
	  close(commSock[i]);
	  commSock[i] = -1;
	  continue;
	}
        PRINTF(">>> Received from [%d](size:%d).\n", i, buflen );

	buf[buflen++] = '\0';
	//メッセージの表示
	WRITE(1, buf, buflen);
	//dbに送信
	PRINTF("<<< Sending to [db]...\n", j);
	write(fd2, buf, buflen);

	//みんなにおくる
	for(j = 0; j < member+1; j++){
	  if(commSock[j] == -1 || i==j)continue;//送り主以外
	  PRINTF("<<< Sending to [%d]...\n", j);
	  if ( write( commSock[j], buf, buflen ) < 0 ) {
	    PRINTF("fail write [%d]\n",j);
	    //memberに報告 退室
	    sprintf(buf2,"2 %d",j);
	    //メッセージの表示
	    WRITE(1, buf2, strlen(buf2));
	    //memberに送信
	    PRINTF("<<< Sending to [member]...\n", j);
	    write(fd1,buf2,strlen(buf2));
	    close(commSock[j]);
	    commSock[j] = -1;
	    continue;
	  }
	}
      }
    }
    //member
    /***************************************************************/
    if(FD_ISSET(fd1, &readfds)){
      if ( ( buflen = read( fd1, buf, sizeof(buf) ) ) <= 0 ) {
	PRINTF("[member] error\n");
	break;
      }
      PRINTF(">>> Received from [member](size:%d).\n", buflen );

      buf[buflen++] = '\0';
      //メッセージの表示
      WRITE(1, buf, buflen );
      //dbに送信
      PRINTF("<<< Sending to [db]...\n", j);
      write(fd2, buf, buflen);
      //みんなにおくる
      for(j = 0; j < member+1; j++){
	if(commSock[j] == -1)continue;
	PRINTF("<<< Sending to [%d]...\n", j);
	if ( write( commSock[j], buf, buflen ) < 0 ) {
	  PRINTF("fail write [%d]\n",j);
	  //memberに報告　退室
	  sprintf(buf2,"2 %d",j);
	  //メッセージの表示
	  WRITE(1, buf2, strlen(buf2));
	  //memberに送信
	  PRINTF("<<< Sending to [member]...\n", j);
	  write(fd1,buf2,strlen(buf2));
	  close(commSock[j]);
	  commSock[j] = -1;
	  continue;
	}
      }
    }
  }

  end();
  return 0;
}

void request (int req)
{

  while(1){
    if ( ( buflen = read( fd2, buf, sizeof(buf) ) ) <= 0 ) {
      PRINTF("request error\n");
      end();
    }
    
    PRINTF(">>> Received from [db](size:%d).\n", buflen );
    
    buf[buflen++]='\0';
    if(!strcmp(buf,"END"))
      break;
    if(!strcmp(buf,"no data"))
      break;
    //メッセージの表示
    WRITE(1, buf, buflen ); 
    //入室者に送信
    PRINTF("<<< Sending to [%d]...\n", req);
    write(commSock[req], buf, buflen );
    
  }
  PRINTF("request end\n");
  return;
  
}

void end()
{
  int i;

  /* ソケット切断 */
  PRINTF("切断\n");
  for(i = 0; i < member+1; i++){
    if(commSock[i] != -1){
      close(commSock[i]);
    }
  }
  close(listenfd);
  close(fd1);
  close(fd2);
  PRINTF("サーバーを終了します\n");
  exit(1);
}
