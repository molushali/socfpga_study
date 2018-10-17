#include <linux/init.h>

#include <linux/module.h>

#include <linux/miscdevice.h>

#include <linux/fs.h>

#include <asm/io.h>

#include <linux/ioport.h>

#include <linux/cdev.h>

#include <linux/slab.h>

#include <asm/uaccess.h>

#include <linux/dma-mapping.h>



#define MY_CDEV_MAGIC 'k'

#define MY_CDEV_CMD  _IO(MY_CDEV_MAGIC,0x1a)

//#define DEVICE_MAJOR 255  

#define MAJOR_NUM 255 //主设备号

#define MINOR_NUM  0 

//static int device_major = DEVICE_MAJOR;
static int device_major = MAJOR_NUM;



struct cdev cdev;

//define two buffer for frame reader buffer

void *my_kernel_buffer=NULL;  

unsigned long my_kernel_buffer_phy;  

//

int my_cdev_open( struct inode *node, struct file *filp )

{
  
  return 0;
  
}



int my_cdev_release( struct inode *node, struct file *filp )

{
  
  return 0;
  
}



ssize_t my_cdev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)  

{  
  
  //    int result;   
  
  if (copy_to_user(buf, my_kernel_buffer, count)) {  
    
    count = -EFAULT;  
    
    goto out;     
    
  }  
  
out:  
  
  return count;  
  
}  





static long my_cdev_ioctl( struct file *filp, unsigned int cmd, unsigned long arg)

{
  
  int ret = 0;
  
  unsigned long temp =0;
  
  switch (cmd){
    
  case MY_CDEV_CMD:
    
    {
      
      temp=(unsigned int)(virt_to_phys(my_kernel_buffer));
      
      if(copy_to_user((unsigned int *)arg, &temp,sizeof(unsigned int))) return -EFAULT;
      
      break;
      
    }
    
  default:
    
    printk(KERN_INFO "invalid value\n");
    
    ret = -EINVAL;
    
    break;
    
  }
  
  return ret;
  
}







static const struct file_operations my_cdev_fops =

{
  
  .owner = THIS_MODULE,
  
  .open = my_cdev_open,
  
  .release = my_cdev_release,
  
  //.unlocked_ioctl = my_cdev_ioctl,
  
  .read=my_cdev_read,
  
};

static struct class *globalvar_dev_class;



static int __init my_cdev_init(void)

{ 
  
  int result;
  
  int error;
  
  
  
  
  /*  注册设备驱动*/
  
  // dev_t devno = MKDEV(DEVICE_MAJOR, 0);
  dev_t devno = MKDEV(MAJOR_NUM, MINOR_NUM);
  
  
  unsigned char *buffer_temp;
  
  if ( device_major )
    
  {
    
    result = register_chrdev_region(devno, 1, "de1_soc_demo");
    
  }
  
  else
    
  {
    
    result = alloc_chrdev_region( &devno, 0, 1, "de1_soc_demo");
    
    device_major = MAJOR(devno);
    
  }
  
  if ( result < 0 )
    
  {
    
    return result;
    
  }
  
  pr_info ("\nmodule loading... \n");
  
  my_kernel_buffer = dma_alloc_coherent(NULL,
                                        
					640*480*4,
                                        
					(void *)&(my_kernel_buffer_phy),
                                        
					GFP_KERNEL);
  
  if(!my_kernel_buffer)
    
  {	
    
    pr_info("alloc buffer1 memory failed \n");
    
    goto fail_malloc1; 	
    
  }
  
  pr_info("buffer1 virtual address 0x%x\n",(uint32_t)(my_kernel_buffer));
  
  pr_info("buffer1 physical address  0x%x\n",my_kernel_buffer_phy);
  
  buffer_temp=(unsigned char *)my_kernel_buffer;
  
  
  cdev_init(&cdev,&my_cdev_fops);
  
  cdev.owner=THIS_MODULE;
  
  cdev.ops = &my_cdev_fops;
  
  error=cdev_add(&cdev,devno,1);
  
  if(error)
    
  {
    
    pr_info("add cdev error\n");
    
    goto add_error;
    
  }
  
  
  /* 创建设备文件 */
  // class_create动态创建设备的逻辑类，并完成部分字段的初始化，然后将其添加到内核中。创建的逻辑类位于/sys/class/。
  //一旦创建好了这个类，再调用device_create(…)函数来在/dev目录下创建相应的设备节点。
  // 参数：
  //        owner, 拥有者。一般赋值为THIS_MODULE。
  //        name, 创建的逻辑类的名称。
  
  globalvar_dev_class= class_create(THIS_MODULE, "socfpga_demo");
  
  if (IS_ERR(globalvar_dev_class))
    
  {
    
    printk ("Failed to invoke class_create() \n");
    
    return -1;
    
  }
  
  
  device_create( globalvar_dev_class,NULL, devno, NULL,"socfpgademo");
  
  printk (KERN_INFO "Registered character driver\n");
  
  
  
  
  
  return 0;
  
add_error:
  
  kfree(my_kernel_buffer);	
  
fail_malloc1: 
  
  unregister_chrdev_region(devno, 1);
  
  return result;
  
}



static void __exit my_cdev_exit(void)

{	
  
  
  
  cdev_del(&cdev);
  
  // kfree(my_kernel_buffer);
  dma_free_coherent(NULL, 640*480*4,my_kernel_buffer, (my_kernel_buffer_phy) );
  
  
  
  device_destroy(globalvar_dev_class, MKDEV (MAJOR_NUM, MINOR_NUM));
  
  class_destroy(globalvar_dev_class);
  unregister_chrdev_region (MKDEV (MAJOR_NUM, MINOR_NUM), 1);
  
  // unregister_chrdev_region(MKDEV(DEVICE_MAJOR, 0),1);   
  
  
  printk(KERN_INFO "module lab2_axi exit\n");  
  
}





MODULE_LICENSE("Dual BSD/GPL");

MODULE_AUTHOR("Terasic");

module_init(my_cdev_init);

module_exit(my_cdev_exit);
