#include <linux/module.h>

#include <linux/init.h>

#include <linux/fs.h>

#include <asm/uaccess.h>

#include <linux/device.h>

#include <linux/cdev.h>

#include <asm/io.h> 



MODULE_LICENSE("GPL");



#define MAJOR_NUM 256 //主设备号

#define MINOR_NUM  0 

#define DEVICE_NAME "my_buttons" 



#define MAP_SIZE             (0x20000)

#define MAP_BASE_ADDR        (0xFF200000) //lwAxiMaster Base Addr



#define Button_PIO_BASE        	 (0x100c0)

#define Button_PIO_BASE_OFFSET  (0x0)

#define PWM_DUTY_OFFSET  	 (0x0)



/* declared in /asm/io.h file

void iounmap(void * addr);

void* ioremap_nocache(unsigned long phys_addr, unsigned long size);

void* ioremap(unsigned long phys_addr, unsigned long size);

*/

ssize_t globalvar_read(struct file *, char *, size_t, loff_t*);

ssize_t globalvar_write(struct file *, const char *, size_t, loff_t*);

volatile unsigned long get_MMU_table_addr(void);



//初始化字符设备驱动的file_operations 结构体

struct file_operations globalvar_fops =

{

	read: globalvar_read, write: globalvar_write,

};



static int global_var = 0; //“globalvar”设备的全局变量

static struct class *globalvar_dev_class;

struct cdev *globalvar_cdev;



unsigned int * LWH2F_bridge_addr=0;



int __init globalvar_init(void)

{

	int ret;





	dev_t mychardev_num;



	LWH2F_bridge_addr=(unsigned int *)ioremap_nocache(MAP_BASE_ADDR+Button_PIO_BASE,0x1000);

	if(LWH2F_bridge_addr ==0)

	{

		printk("Failed to allocate virtual space for LWH2F bridge space.\n");

		return -1;

	}

	/*  注册设备驱动*/

	mychardev_num=MKDEV (MAJOR_NUM, MINOR_NUM);

	ret = register_chrdev_region (mychardev_num, 1, "mychardev");  // 在 proc的devices中察看

	if (ret)

	{

		printk("my chardev -> button register failure\n");

		return ret;

	}

	

        //动态初始化

	globalvar_cdev = cdev_alloc();



	if (globalvar_cdev != NULL)

	{

		  cdev_init (globalvar_cdev, &globalvar_fops);

		  globalvar_cdev->ops = &globalvar_fops;

		  globalvar_cdev->owner = THIS_MODULE;

		  if (cdev_add (globalvar_cdev, mychardev_num, 1) )

		  {

		  	printk (KERN_NOTICE "Someting wrong when adding globalvar_cdev!\n");

		  }

		  else

		  {

		  	 printk ("Success adding mychardev!\n");

		  }

	}

	/* 创建设备文件 */

	globalvar_dev_class= class_create(THIS_MODULE, "button_driver");     // 将放于/sysfs       查看#ls /sys/class

	if (IS_ERR(globalvar_dev_class))

	{

		printk ("Failed to invoke class_create() \n");

		return -1;

	}

	device_create( globalvar_dev_class,NULL, mychardev_num, NULL,DEVICE_NAME);   //将存放于/dev  查看#ls /dev  open时的文件名

	printk (KERN_INFO "Registered character driver\n");

	

	return 0;

}





int __exit globalvar_exit(void)

{



	iounmap((void *)LWH2F_bridge_addr);

	cdev_del (globalvar_cdev);

	device_destroy(globalvar_dev_class, MKDEV (MAJOR_NUM, MINOR_NUM));

	class_destroy(globalvar_dev_class);

	unregister_chrdev_region (MKDEV (MAJOR_NUM, MINOR_NUM), 1);

	printk (KERN_INFO "char driver cleaned up\n");



}



unsigned int test_data;

ssize_t globalvar_read(struct file *filp, char *buf, size_t len, loff_t *off)

{

	//将global_var 从内核空间复制到用户空间

	

	global_var=*(LWH2F_bridge_addr+PWM_DUTY_OFFSET);

	printk("get_MMU_table_addr's value is 0x%x\n", global_var);

	

	if (copy_to_user(buf, &global_var, sizeof(int)))

	{

		return - EFAULT;

	}

	return sizeof(int);

}



ssize_t globalvar_write(struct file *filp, const char *buf, size_t len, loff_t *off)

{

	//将用户空间的数据复制到内核空间的global_var

	if (copy_from_user(&global_var, buf, sizeof(int))) //global_var 内核空间，buf用户空间

	{

		return - EFAULT;

	}



	//*(LWH2F_bridge_addr+PWM_DIV_OFFSET) = 100;  // PWM 周期

	*(LWH2F_bridge_addr+PWM_DUTY_OFFSET) = global_var; //PWM 占空比

       // *（LWH2F_bridge_addr) = global_var；

	return sizeof(int);

}



module_init(globalvar_init);

module_exit(globalvar_exit);

