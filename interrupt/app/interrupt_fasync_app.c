#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include <sys/select.h>

#include <string.h>

#include <sys/ioctl.h>

#include <unistd.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <signal.h>
#include <string.h>




/*********************** UDP client  ***************************/

//int send_num;  
int recv_num;  
char send_buf[20] = "hey,who are you?";  
char recv_buf[20] ; 
socklen_t len;
int sockfd;
int fd;

#define UDP_TEST_PORT       2234

#define UDP_SERVER_IP       "192.168.0.23"

#define MAX_LINE             80 

struct sockaddr_in server;
socklen_t  server_len = sizeof(struct sockaddr_in); 

int Udp_ClientInit(void);
int Udp_Send(char * send_buf, int len);

int Udp_ClientInit(void)  
{   
  
  
  char *sendStr ="i am a client\n"; //默认发送串 
  
  //我们一般的英特网局域网用的就是这个，AF_INET只是一个标识而已。 
  //定义是这样的 #define AF_INET         2               // internetwork: UDP, TCP, etc.  
  /*-----------------------udp init---------------------*/
  
  /* setup a socket，attention: must be SOCK_DGRAM */
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    
    perror("socket");
    exit(1);
  }
  
  
  /*complete the struct: sockaddr_in*/
  bzero(&server, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(UDP_TEST_PORT);
  server.sin_addr.s_addr = inet_addr(UDP_SERVER_IP);
  
  
  if(sendto(sockfd, sendStr,  strlen(sendStr), 0, (struct sockaddr *)&server, server_len)<0) //sizeof
  {
    printf("sendto error\n");
    //   exit(1);
  }
  
  // sizeof : 返回定义数组时，编译器为其分配的数组空间大小，不关心里面存储了多少数据
  
  return 0;  
}  

int Udp_Send(char * send_buf, int Bufferlen)
{
  int send_num; 
  
  printf("len = %d !" , len);
  // send_num = sendto(sockfd,send_buf,strlen(send_buf),0,(struct sockaddr*)&address,len);  // strlen:只关心存储的数据内容，不关心空间的大小和类型
  send_num = sendto(sockfd,send_buf,Bufferlen,0,(struct sockaddr*)&server,server_len); 
  
  
  
  if(send_num < 0)
  {
    // perror 用 来 将 上 一 个 函 数 发 生 错 误 的 原 因 输 出 到 标 准 错误 (stderr)。
    perror("sendto error:");  
    return 1;
    //  exit(1);  // 表示程序异常退出
  }
  else
  {
    printf("send success !"); 
    return 0;
  }
}

//***********************************************************************************************************************************




/*********** UDP server ********/
/********************socket相关******************************************/
//socket参数
#define SERVER_PORT 1025                 /*监听端口*/
#define MAX_MSG_SIZE 256              /*recv缓冲器大小*/

#define UDP_TEST_PORT       2234

#define UDP_SERVER_IP       "192.168.0.23"

#define MAX_LINE             80 

/*接收缓冲区--------------------len =4-----------------*/
unsigned char message[MAX_MSG_SIZE];
int recv_len=0;
/* IP地址的存放缓冲区*/
char addr_p[INET_ADDRSTRLEN];
char *sndbuf = "Welcome! This is a UDP server, I can only received message from client and reply with same message.\n";


int sockfd;  //套接字描述符

/*客户机的地址信息及长度信息*/
struct sockaddr_in client;
socklen_t  client_len=sizeof(struct sockaddr_in); 

//udp服务器监听函数初始化
void udpser_init()
{
  /*服务器的地址信息*/
  struct sockaddr_in server;
  
  /*服务器填充sockaddr server结构*/
  bzero(&server,sizeof(server));
  server.sin_family=AF_INET;
  server.sin_addr.s_addr=htonl(INADDR_ANY);
  server.sin_port=htons(SERVER_PORT);
  
  /*服务器建立socket描述符*/
  if(-1 == (sockfd=socket(AF_INET, SOCK_DGRAM,0)))    
  {
    perror("create socket failed");
    exit (1);
  }
  /*捆绑socket描述符sockfd*/
  if(-1 == ( bind( sockfd, ( struct sockaddr * )&server, sizeof(server) )) )
  {
    perror("bind error");
    exit (1);    
  }
  
  
  /********************************接收数据并打印*************************/
  recv_len=recvfrom(sockfd,message,sizeof(message),0,(struct sockaddr *)&client,&client_len); //阻塞式
  if(recv_len <0)
  {
    printf("recvfrom error\n");
    exit(1);
  }
  /*打印客户端地址和端口号*/
  inet_ntop(AF_INET,&client.sin_addr,addr_p,sizeof(addr_p));
  printf("client IP is %s, port is %d\n",addr_p,ntohs(client.sin_port));
  message[recv_len]='\0';
  /*显示消息长度*/
  printf("server received %d:%s\n", recv_len, message);
  
  /********************************回发数据*************************/ 
  if(sendto(sockfd,sndbuf,strlen(sndbuf),0,(struct sockaddr*)&client,client_len)<0)
  {
    printf("sendto error\n");
    exit(1);
  }
  
  if(sendto(sockfd, message,strlen(sndbuf),0,(struct sockaddr*)&client,client_len)<0)
  {
    printf("sendto error\n");
    exit(1);
  }
}



int UdpServer_Send(char * send_buf, int Bufferlen)
{
  int send_num; 
  
  printf("len = %d !" , len);
  // send_num = sendto(sockfd,send_buf,strlen(send_buf),0,(struct sockaddr*)&address,len);  // strlen:只关心存储的数据内容，不关心空间的大小和类型
  send_num = sendto(sockfd,send_buf,Bufferlen,0,(struct sockaddr*)&client,client_len); 
  
  
  
  if(send_num < 0)
  {
    // perror 用 来 将 上 一 个 函 数 发 生 错 误 的 原 因 输 出 到 标 准 错误 (stderr)。
    perror("sendto error:");  
    return 1;
    //  exit(1);  // 表示程序异常退出
  }
  else
  {
    printf("send success !"); 
    return 0;
  }
}


//信号处理函数
void my_signal_fun(int signum)
{
  char frd[64];
  unsigned char s[64];
  read(fd, s, 64);
  printf("open my_signal_fun() ............\n");
  printf("read : %d\n", frd);//is 2
}

int main(int argc, char **argv)
{
  int i,ret;
  int Oflags;
  char val[64];
  
  
  char test[64];
  
  //在应用程序中捕捉SIGIO信号（由驱动程序发送）  
  signal(SIGIO, my_signal_fun);
  fd = open("/dev/fpga_mem", O_RDWR);
  if (fd < 0)
  {
    printf("cannot open\n");
    return 1;
  }
  else
  {
    printf("success open\n");
  }
  
  //将当前进程PID设置为fd文件所对应驱动程序将要发送SIGIO,SIGUSR信号进程PID 
  ret = fcntl(fd, F_SETOWN, getpid());
  
  if(ret < 0)
  {
    printf("can't fcntl_1!\n");
  }
  else
  {
    printf("can fcntl_1!\n");
  }
  
  //获取fd的打开方式 
  Oflags = fcntl(fd, F_GETFL); 
  printf("Oflags is : %d\n", Oflags);//is 2
  
  if(Oflags < 0)
  {
    printf("can't fcntl_2!\n");
  }
  
  printf("read!\n");
  //将fd的打开方式设置为FASYNC --- 即 支持异步通知  
  //该行代码执行会触发 驱动程序中 file_operations->fasync 函数 ------fasync函数调用fasync_helper初始化一个fasync_struct结构体，该结构体描述了将要发送信号的进程PID (fasync_struct->fa_file->f_owner->pid)
  ret = fcntl(fd, F_SETFL, Oflags | FASYNC);
  printf("read!\n");
  if(ret < 0)
  {
    printf("can't fcntl_3!\n");
  }
  
  //Udp_ClientInit();
  //udp服务器监听函数初始化
  // udpser_init();
  
  read(fd, test, 64);
  printf("read!\n");
  
  while(1)
  {
    sleep(1000);
  }
  
  
  
  close(sockfd);
  
  return 0;
  
  
  while(1);
}
