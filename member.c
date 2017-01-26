#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 50001  /* ポート番号 */

struct list{
  int num;
  char name[32];
  struct list *next;
};

/*** リストにデータを登録 ***/
struct list *add(int n,char *name, struct list *head)
{
  struct list *new;
  
  /* 記憶領域の確保 */
  if ((new = (struct list *) malloc(sizeof(struct list))) == NULL) {
    printf("member: malloc error\n");
    exit(1);
  }
  
  /* リストにデータを登録 */
  new->num = n;
  strcpy(new->name,name);
  
  /* ポインタのつなぎ換え */
  new->next = head; 
  head = new;	

  return head;
  
}

/*** リストからデータを削除 ***/
struct list *delete(int n,struct list *head,char *code)
{
  struct list *out;

  out=head;
  if(out!=NULL&&out->num == n){
    head = out->next;
    return head;
  }

  /*探索*/
  while(out->next != NULL){
    if(out->next->num == n){
      strcpy(code,out->next->name);
      out->next = out->next->next;
      return head;
    }
    out = out->next;
  }

  return head;

}

/*** リストの表示 ***/
void show(struct list *head)
{
  struct list *out;
  out = head;

  while (out != NULL) {	
    printf("%d\n",out->num);
    out = out->next;
  }
}

/*** 今のメンバーを知らせる ***/
int make(char *new,char *buf,struct list *head,int flag)
{
  struct list *out;
  out = head;

  buf[0]='\0';
  sprintf(buf," ーー %sさんが",new);
  if(flag==1){
    strcat(buf,"入室しました。\n");
  }else if(flag==2){
    strcat(buf,"退室しました。\n");
  }

  if(out!=NULL){

    strcat(buf,"今のメンバーは、");
    
    while (out != NULL) {	
      sprintf(buf,"%s%sさん,",buf,out->name);
      out = out->next;
    }
    strcat(buf,"\bです。\0");
   
  }else{

    strcat(buf,"＞＞ 現在チャットルームには誰もいません\0");

  }

  return strlen(buf)+1;

}

int main( void ) {
  int    i;
  int    connected_fd, listening_fd;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  int    len, buflen;
  char   buf[1024];
  char new[32];

  struct list *head;         /*リストの先頭ポインタ*/
  int flag,n;
  int count=0,j;


  /* リスニングソケット作成 */
  if ( ( listening_fd = socket(PF_INET, SOCK_STREAM, 0) ) < 0 ) {
    perror("*** member: socket ***");
    exit(1);
  }

  /* アドレスファミリ・ポート番号・IPアドレス設定 */
  bzero( (char *)&server_addr, sizeof(server_addr) );
  server_addr.sin_family = PF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(PORT);

  /* リスニングソケットにアドレスを割り当て */
  if ( bind( listening_fd, (struct sockaddr *)&server_addr, 
         sizeof(server_addr) ) < 0 ) {
    perror("*** member: bind ***");
    close(listening_fd);
    exit(1);
  }
    
  /* リスニングソケット待ち受け */
  if ( listen( listening_fd, 1 ) < 0 ) {
    perror("*** member: listen ***");
    close(listening_fd);
    exit(1);
  }
  printf( "member: Waiting for connections from a client.\n" );

  /* 接続要求受け付け */
  len = sizeof(client_addr);
  if ( ( connected_fd = accept(listening_fd, 
         (struct sockaddr *)&client_addr, &len) ) < 0 ) {
    perror("*** member: accept ***");
    close(listening_fd);
    exit(1);
  }
  close(listening_fd);
  printf( "member: Accepted connection.\n" );

  head =NULL;

  /* データの送受信 */
  while (1) {
    /*データ受け取り*/
    if ( ( buflen = read( connected_fd, buf, sizeof(buf) ) ) <= 0 ) {
      break;
    }

    /*受信データを分解*/
    buf[buflen]='\0';
    sscanf(buf,"%d %d %s",&flag,&n,new);
    /*
    flag = buf[0]-'0';
    n = atoi(&buf[1]);
    */

    switch(flag){
    /*リストに追加及び送信データの作成、送信*/
    case 1:
      head = add(n,new,head);
      count++;

      /*今のメンバーを送信*/
      buflen = make(new,buf,head,1);
      write( connected_fd, buf, buflen );
      break;

    /*リストから削除*/
    case 2:
      head = delete(n,head,new);
      count--;

      /*今のメンバーを送信*/
      buflen = make(new,buf,head,2);
      write( connected_fd, buf, buflen );
      break;

    /*デバック用*/
    case 3:
      printf("デバック\n");
      show(head);
      break;
      
      /*例外*/
    default:
      printf("member: flag error\n");
      break;

    }




  }

  /* ソケット切断 */
  close(connected_fd);
  printf("member: end\n");

}
