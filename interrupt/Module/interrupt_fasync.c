#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#include <linux/semaphore.h>
#include <linux/cdev.h>

#include <linux/types.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/gpio.h>
#include <linux/poll.h>
#include <linux/sched.h> 



#define MAJOR_NUM 203 //主设备号
#define MINOR_NUM  0  


#define ROM_BASE 0xc0000000  // 挂载在 hps to fpga 线上,偏移量为0
#define ROM_SIZE 64
void *fpga_rom_mem;

//bool data_read_flag = false;  /*表明设备有足够数据可供读*/


static struct fasync_struct *button_async;

static struct class *keydrv_class;

/*等待队列，应用层调用读接口，如果没有中断来，它将休眠*/
static DECLARE_WAIT_QUEUE_HEAD(mem_waitq); 


//char fpga_rom_mem[ROM_SIZE];
static int mem_open(struct inode * inode , struct file * filp);
static int mem_fasync(int fd, struct file *filp, int mode);
ssize_t mem_read(struct file *, char *, size_t, loff_t*);
ssize_t mem_write(struct file *, const char *, size_t, loff_t*);
static int mem_poll(struct file *filp, struct poll_table_struct *wait);
//volatile unsigned long get_MMU_table_addr(void);




static struct class *mem_dev_class;
struct cdev *mem_cdev;


struct mem_dev
{
  struct cdev dev;
  char mem[ROM_SIZE];
  struct fasync_struct *async_queue;
};

static struct mem_dev *mem_dev_p;
static dev_t mem_devno;
static struct class *mem_class;


int gpio_number;
int key_irq;
char tmp[64];


/*中断事件标识*/
static volatile int env_irq = 0;

//初始化字符设备驱动的file_operations 结构体
struct file_operations mem_fops =
{
  .owner = THIS_MODULE,
  .open = mem_open,
  //  .release = mem_release,
  .read = mem_read,
  .write = mem_write,
  .fasync = mem_fasync,
  
};





/*******************************************/
//中断处理函数：
int i;
static irqreturn_t key_interrupt (int irq, void *dev_id)
{
  
  printk("\n=============key_interrupt=============\n");
  
  for (i=0;i < ROM_SIZE;i++){
    tmp[i] = ioread8(fpga_rom_mem + i);
    mem_dev_p->mem = tmp;
    printk("%12x \n",tmp[i]);
  }
  
  printk("\n"); 
  
  /* 表示中断发生了 */ 
  env_irq = 1;  
  /*唤醒等待队列*/
  wake_up_interruptible(&mem_waitq);
  
  //发送信号SIGIO信号给fasync_struct 结构体所描述的PID，触发应用程序的SIGIO信号处理函数 
  kill_fasync(&button_async, SIGIO, POLL_IN);
  
  return IRQ_RETVAL(IRQ_HANDLED);
  //return 0;
}




unsigned int test_data;
ssize_t mem_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
  
  unsigned long err;
  int i;
  
  
  struct mem_dev *dev_p = filp->private_data;
  
  
  /* 如果没有按键动作, 休眠 */  
  wait_event_interruptible(mem_waitq, env_irq); // env_irq = 1 ，往下执行，env_irq=0，一直在此处等待
  
  /* 如果有按键动作, 返回键值 */
  if (len > ROM_SIZE )
    len = ROM_SIZE ;
  
  if (copy_to_user(buf, dev_p->mem, len))
  {
    return -EFAULT;
  }
  
  /*
  if (copy_to_user(buf, &tmp, ROM_SIZE))
  {
  return - EFAULT;
}
  */
  
  for (i = 0; i < ROM_SIZE; i++) {
    printk("KER_INFO: [loop] key_values[%d] = %d\n", i+1, tmp[i]);
  }
  
  
  //  memset(tmp, 0, sizeof(tmp))
  env_irq = 0;
  
  return sizeof(int);
}

ssize_t mem_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
  //将用户空间的数据复制到内核空间的global_var
  //if (copy_from_user(&fpga_rom_mem, buf, sizeof(int)))
  
  struct mem_dev *dev_p = filp->private_data;
  
  if (len > ROM_SIZE )
    len = ROM_SIZE ;
  
  if (copy_from_user(dev_p->mem, buf, ROM_SIZE))
    
  {
    return - EFAULT;
  }
  
  if (dev_p->async_queue)
  {
    kill_fasync(&dev_p->async_queue, SIGIO, POLL_IN);
  }
  return sizeof(int);
}



static int mem_fasync(int fd, struct file *filp, int mode)
{
  struct mem_dev *dev_p = filp->private_data;
  printk("driver: mem_fasync\n");
  
  //return fasync_helper(fd, filp, mode, &dev_p->async_queue);
  return fasync_helper (fd, filp, mode, &button_async);
}


static int mem_open(struct inode * inode , struct file * filp)
{
  
  filp->private_data = mem_dev_p;
  
  printk("....file_open...\n");
  
  return 0;
}

static int mem_release(struct inode * inode, struct file *filp)
{
  mem_fasync(-1, filp, 0);
  return 0;
}



int button_release(struct platform_device *pdev)
{
  return 0;    
}

int button_probe(struct platform_device *pdev)
{
  int result;
  
  int req;
  int ret;
  
  struct device_node *node = pdev->dev.of_node;
  
  gpio_number = of_get_named_gpio(node,"inter_gpios",0);
  
  printk("============= gpio_number = %d=============\n",gpio_number);  
  
  key_irq = gpio_to_irq(gpio_number);
  
  
  printk("=============key_irq = %d=============\n",key_irq);        
  
  //  result = request_irq(key_irq, key_interrupt,IRQF_TRIGGER_FALLING, "key_int", NULL);
  result = request_irq(key_irq, key_interrupt,0, "key_int", NULL);
  
  if(result)
  {
    printk(KERN_INFO"[FALLED: Cannot register Key  Interrupt!]\n");
  }
  
  dev_t mychardev_num;
  
  /*  注册设备驱动*/
  mychardev_num=MKDEV (MAJOR_NUM, MINOR_NUM);
  ret = register_chrdev_region (mychardev_num, 1, "mychardev");
  if (ret)
  {
    printk("mychardev register failure\n");
    return ret;
  }
  
  mem_cdev = cdev_alloc();
  if (mem_cdev != NULL)
  {
    cdev_init (mem_cdev, &mem_fops);
    mem_cdev->ops = &mem_fops;
    mem_cdev->owner = THIS_MODULE;
    if (cdev_add (mem_cdev, mychardev_num, 1) )
    {
      printk (KERN_NOTICE "Someting wrong when adding mem_cdev!\n");
    }
    else
    {
      printk ("Success adding my_mem!\n");
    }
  }
  
  
  /* 创建设备文件 */
  mem_dev_class= class_create(THIS_MODULE, "mem_driver");
  if (IS_ERR(mem_dev_class))
  {
    printk ("Failed to invoke class_create() \n");
    return -1;
  }
  device_create( mem_dev_class,NULL, mychardev_num, NULL,"fpga_mem");
  printk (KERN_INFO "Registered character driver\n");
  
  
  return 0;
}

static struct of_device_id altera_gpio_of_match[] = {
  // { .compatible = "gpio-buttons", },
  { .compatible = "altr,button", },
  {},
};

MODULE_DEVICE_TABLE(of, altera_gpio_of_match);

static struct platform_driver button_fops = {
  .driver = {
    .name        = "altr,pio-14.0",
    .owner        = THIS_MODULE,
    .of_match_table = of_match_ptr(altera_gpio_of_match),
  },
  .probe = button_probe,
  .remove = button_release,
};

// 模块加载函数
static int __init button_init(void)
{
  printk(KERN_ALERT "\n key modules altr,pio-button is install\n");  
  
  
  if (!request_mem_region(ROM_BASE,ROM_SIZE,"rom_test"))  // 请求分配指定起始位置/长度的IO内存资源
  {
    printk( KERN_INFO"rom_test Memory region busy\n");
    return  -EBUSY;
    
  }
  fpga_rom_mem = ioremap(ROM_BASE,ROM_SIZE);
  //fpga_rom_mem = (unsigned int *)ioremap_nocache(ROM_BASE,ROM_SIZE);
  
  printk("=============fpga_rom_mem = %d=============\n",fpga_rom_mem);  
  if(!fpga_rom_mem) {
    printk( KERN_INFO" rom_test ioremap failed\n");
    return -EIO;
  }
  
  return    platform_driver_register(&button_fops);   
  
}

// 模块卸载函数
static void  __exit button_cleanup(void)
{
  printk("Goodbye,chenzhufly!\n");
  free_irq(key_irq,0);  
  gpio_free(gpio_number);
  release_mem_region(ROM_BASE,ROM_SIZE);  //用于释放不再需要的IO内存区
  iounmap((void *)fpga_rom_mem);  // 用于指定
  platform_driver_unregister(&button_fops);   
  
  cdev_del (mem_cdev);
  device_destroy(mem_dev_class, MKDEV (MAJOR_NUM, MINOR_NUM));
  class_destroy(mem_dev_class);
  unregister_chrdev_region (MKDEV (MAJOR_NUM, MINOR_NUM), 1);
  printk (KERN_INFO "char driver cleaned up\n");
}

module_init(button_init);
module_exit(button_cleanup);

MODULE_AUTHOR("chenlz");
MODULE_LICENSE("Dual BSD/GPL");
