/* ***************************************
* Name: btn_test.c
* Proj: 简单按键驱动程序查询方式实现(Mini2440)
* Desc: 驱动测试程序
* Auth & Date: 
*************************/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

int main(void)
{
  int fd, i,fd_char;
  int count = 0;
  unsigned char s[1];
  unsigned int char_dev_read_data;
  
  /* 打开设备文件 */
  fd = open("/dev/my_buttons", O_RDONLY);
  if (fd < 0) {
    printf("Open buttons error.\n");
    
    return -1;
  }
  
  
  
  /* 循环方式查询并打印按键信息 */
  while (true) {
    i = read(fd, s, 1);
    /*
    if (i != sizeof(s)) {
    printf("Read buttons error.\n");
    printf("i's value is %x\n", i);
    printf("sizeof(s)'s value is %x\n", sizeof(s));
    
    return -1;
  }
    */  
    
    char_dev_read_data = s[0] ;
    //   write(fd_char,&char_dev_read_data,sizeof(int));
    printf("Button's value is %x\n", s[0] );
    
    sleep(1);
  }
  
  close(fd);
  return 0;
}

