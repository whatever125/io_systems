#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "var1"

static dev_t first;
static struct cdev c_dev;
static struct class *cl;

static size_t total_chars = 0;      // Счетчик символов
static size_t *char_counts;         // Массив для хранения последовательности результатов
static size_t count_size = 0;       // Размер массива
static size_t count_capacity = 10;  // Начальная емкость массива
static size_t history_str_size = 0; // Длина строки с историей

static int my_open(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Driver: open()\n");
  return 0;
}

static int my_close(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Driver: close()\n");
  return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
  size_t i;
  char *output;
  size_t output_len = 0;

  if (*off > 0)
  {
    return 0;
  }

  printk(KERN_INFO "Driver: read()\n");

  for (i = 0; i < count_size; i++)
  {
    output_len += snprintf(NULL, 0, "%zd\n", char_counts[i]);
  }

  output = kmalloc(output_len + 1, GFP_KERNEL);
  if (!output)
  {
    return -ENOMEM;
  }

  // Формируем строку для вывода
  output_len = 0;
  for (i = 0; i < count_size; i++)
  {
    output_len += sprintf(output + output_len, "%zd\n", char_counts[i]);
  }

  // Копируем данные в пользовательский буфер
  if (copy_to_user(buf, output, output_len))
  {
    kfree(output);
    return -EFAULT;
  }

  *off += output_len;
  kfree(output);
  return output_len;
}

static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
  char *data;

  printk(KERN_INFO "Driver: write()\n");

  data = kmalloc(len + 1, GFP_KERNEL);
  if (!data)
  {
    return -ENOMEM;
  }

  if (copy_from_user(data, buf, len))
  {
    kfree(data);
    return -EFAULT;
  }

  data[len] = '\0';

  // Увеличиваем счетчик символов
  total_chars += len - 1;

  // Увеличиваем массив, если необходимо
  if (count_size >= count_capacity)
  {
    count_capacity *= 2;
    char_counts = krealloc(char_counts, count_capacity * sizeof(int), GFP_KERNEL);
    if (!char_counts)
    {
      kfree(data);
      return -ENOMEM;
    }
  }

  // Сохраняем текущее количество символов в массив
  char_counts[count_size++] = len - 1;

  // history_str_size += snprintf(NULL, 0, "%zd\n", len) - 1;

  kfree(data);
  return len;
}

static struct file_operations mychdev_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_close,
    .read = my_read,
    .write = my_write};

static int __init ch_drv_init(void)
{
  printk(KERN_INFO "Hello!\n");

  // Инициализация массива
  char_counts = kmalloc(count_capacity * sizeof(size_t), GFP_KERNEL);
  if (!char_counts)
  {
    return -ENOMEM;
  }

  if (alloc_chrdev_region(&first, 0, 1, "ch_dev") < 0)
  {
    return -1;
  }
  if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
  {
    unregister_chrdev_region(first, 1);
    return -1;
  }
  if (device_create(cl, NULL, first, NULL, DEVICE_NAME) == NULL)
  {
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    return -1;
  }
  cdev_init(&c_dev, &mychdev_fops);
  if (cdev_add(&c_dev, first, 1) == -1)
  {
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    return -1;
  }
  return 0;
}

static void __exit ch_drv_exit(void)
{
  cdev_del(&c_dev);
  device_destroy(cl, first);
  class_destroy(cl);
  unregister_chrdev_region(first, 1);
  kfree(char_counts);
  printk(KERN_INFO "Bye!!!\n");
}

module_init(ch_drv_init);
module_exit(ch_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DnB");
MODULE_DESCRIPTION("The first kernel module");
