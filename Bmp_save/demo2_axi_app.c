#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <fcntl.h>

#include <sys/mman.h>

#include <sys/types.h>

#include <dirent.h>

#include <inttypes.h>

#include <sys/time.h>

#include <stdbool.h>

#include <pthread.h>

#include <sys/ioctl.h> 

#include "math.h"

#include "hwlib.h"

#include "socal/socal.h"

#include "socal/hps.h"	

#include  "vip_capture.h"

#include  "lib_bitmap.h" 



//实际挂在LWFPGASLAVES 上， 映射到的地址是 0xFF200000  ，只是写成HW_REGS_BASE+ALT_LWFPGASLVS_OFST & HW_REGS_MASK 格式（规范化）。
//STM是外设区域的第一个模块，所以#define HW_REGS_BASE ( ALT_STM_OFST )，其实跟STM本身没有半毛钱关系



#define HW_REGS_BASE (ALT_STM_OFST )   //0xfc00 0000 STM基地址 ，即外设基地址 在 hps.h 文件里定义

#define HW_REGS_SPAN (0x04000000 )

#define HW_REGS_MASK (HW_REGS_SPAN - 1 )



#define BMP_WIDTH 640

#define BMP_HEIGHT 480

#define WRITE_COUNT_PILXEL 4

#define BMP_COUNT_PILXEL 3

#define BUFFER_SIZE BMP_WIDTH*BMP_HEIGHT*WRITE_COUNT_PILXEL

#define BMP_BUFFER_SIZE BMP_WIDTH*BMP_HEIGHT*BMP_COUNT_PILXEL



static volatile unsigned long *h2p_vip_capture_addr=NULL;



static unsigned int  DEMO_VGA_FRAME0_ADDR = 0x3f200000;



#define ALT_VIP_CAPTURE_BASE  0x00000000



void Capture_Start(uint32_t FRAME_BASE){   // 写寄存器 （qsys 中的 VIP_CAPTURE 模块，自定义）
  
  
  
  // stop
  
  h2p_vip_capture_addr[REG_CONTROL]=0x00;  // 只写，控制 VIP_CAPTURE 抓取数据开始和停止。  1：开始抓取数据 ， 0： 停止抓取
  
  // set base addr
  
  h2p_vip_capture_addr[REG_BASE_ADDRESS]=FRAME_BASE; //只写，数据写区域的起始地址
  
  // start
  
  h2p_vip_capture_addr[REG_CONTROL]=0x01;
  
  return ;
  
}



bool Capture_Get( uint32_t Width, uint32_t Height){
  
  bool bSuccess = false;
  
  uint32_t Status32, PixelCnt;
  
  do {
    
    usleep(10000);
    
    Status32=h2p_vip_capture_addr[REG_STATUS];   //只读，VIP_CAPTURE的工作状态寄存器，  一共6个工作状态
    
    
    
  } while (Status32 != ST_SUCCESS && Status32 != ST_FIFO_OVERFLOW && Status32 != ST_BAD_FRAME );
  
  
  
  // stop
  
  // sleep(1);
  
  h2p_vip_capture_addr[REG_CONTROL]=0x00;
  
  if (Status32 == ST_SUCCESS){
    
    PixelCnt=h2p_vip_capture_addr[REG_PIXEL_CNT];   //只读，已写入像素个数
    
    if (PixelCnt == Width*Height){
      
      printf("%d*%d pixel get \n",Width,Height); 
      
      bSuccess = true;
      
    }else{
      
      printf("[VIP_FC]invalid pixel count %d\r\n", (int)PixelCnt);
      
      bSuccess = false;
      
    }
    
  }else{
    
    bSuccess = false;
    
  }
  
  return bSuccess;
  
}

int main(int argc,char ** argv)

{
  
  int fd;
  
  unsigned char read_buffer[BUFFER_SIZE]={0};
  
  unsigned char bmp_buffer[ BMP_BUFFER_SIZE]={0};
  
  int retval;  
  
  int i,j;     
  
  void *lw_axi_virtual_base;
  
  if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
    
    printf( "ERROR: could not open \"/dev/mem\"...\n" );
    
    return( 1 );
    
  }
  
  lw_axi_virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );	
  
  if( lw_axi_virtual_base == MAP_FAILED ) {
    
    printf( "ERROR: mmap() failed...\n" );
    
    close( fd );
    
    return( 1 );
    
  }
  
  
  
  h2p_vip_capture_addr= lw_axi_virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + ALT_VIP_CAPTURE_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
  
  
  Capture_Start(DEMO_VGA_FRAME0_ADDR);
  
  Capture_Get(BMP_WIDTH,BMP_HEIGHT);  
  
  close( fd);
  
  
  fd = open("/dev/socfpgademo", O_RDWR);  
  
  if (fd == 1) {  
    
    perror("open error\n");  
    
    exit(-1);  
    
  }    
  
  retval = read(fd, read_buffer, BUFFER_SIZE);  
  
  if (retval == -1) {  
    
    perror("read error\n");  
    
    exit(-1);  
    
  }  
  
  j=0;
  
  for(i=0;i<BUFFER_SIZE;i++)
    
  {	
    
    if((i+1)%4)
      
    {
      
      bmp_buffer[j]=read_buffer[i];
      
      j=j+1;
      
    }
    
  } 
  
  GenBmpFile(bmp_buffer,24,BMP_WIDTH,BMP_HEIGHT,"axi_demo2.bmp");
  
  printf("axi_demo2.bmp file save over\n");
  
  close(fd);   
  
  return 0;
  
}
