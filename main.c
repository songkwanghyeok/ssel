#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>	//struct fd
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/delay.h>	//ssleep()

#include <linux/kthread.h>
#include <linux/mutex.h>

#include <linux/wait.h>	//wait inteur
#include <linux/unistd.h>

#include <linux/slab.h>	// kmalloc, kfree

#include <linux/types.h>

#include "include/queue.h"
#define DEVICE_FILE_NAME "mydev"
#define MAJOR_NUM 0

#define SSEL_READ 100
#define SSEL_WRITE 200
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SSEL");
MODULE_DESCRIPTION("KVM-based Intergrated I/O Driver in Kernel");


typedef struct VMMetaData
{ 
	int imgfd;
	struct file *file;
	char __user *buf;
	size_t count;
	loff_t* pos;
	int RDWR;
} VMMetaData;

typedef struct ThKimMutex
{
	struct mutex lock;
} ThKimMutex;

typedef struct ThreadPoolElement
{
	void *arg;
	int ret;

	QTAILQ_ENTRY(ThreadPoolElement) reqs;

}  ThreadPoolElement;

typedef struct ThreadPool
{
	ThKimMutex lock;
	struct task_struct *th_id;

	QTAILQ_HEAD(, ThreadPoolElement) request_list;

	int max_threads;
	int cur_threads;
	int idle_threads;
	int new_threads;
	int pending_threads;
	int pending_cancellations;
	bool stopping;

} ThreadPool;

static ThreadPool g_ssel_thp;
DECLARE_WAIT_QUEUE_HEAD(WaitQueue);
static int major;

static void ssel_mutex_init(ThKimMutex* mutex)
{
	mutex_init(&mutex->lock);
}

static void ssel_mutex_unlock(ThKimMutex* mutex)
{
	mutex_unlock(&mutex->lock);
}

static void ssel_mutex_lock(ThKimMutex* mutex)
{
	mutex_lock(&mutex->lock);
}

/*
static void ssel_mutex_destroy(ThKimMutex* mutex)
{
}

static void ssel_mutex_trylock(ThKimMutex* mutex)
{
	int err;
	err = mutex_trylock(&mutex->lock);
}
*/
static int kio_worker(VMMetaData *meta)
{
	struct fd img_fd;
	ssize_t ret = -1;

	printk(KERN_INFO"[SSEL]MODULE '%s()'\n", __FUNCTION__);

	if(NULL != meta)
	{
		printk(KERN_INFO"[SSEL]\tmeta->imgfd : %d\n",(int) meta->imgfd);
		printk(KERN_INFO"[SSEL]\tmeta->count : %d\n", (int)meta->count);
		printk(KERN_INFO"[SSEL]\tmeta->pos : %lld\n", (long long)meta->pos);
		printk(KERN_INFO"[SSEL]\tmeta->file : %p\n",meta->file);

		//img_fd = fdget(meta->imgfd);
		if(meta->file)
		//if(img_fd.file)
		{
			//printk(KERN_INFO"[SSEL]\tfile->f_op->read %x\n", img_fd.file->f_op->read);
			//printk(KERN_INFO"[SSEL]\tfile->f_op->aio_read %x\n", img_fd.file->f_op->aio_read);
			//printk(KERN_INFO"[SSEL]\timg_fd.file->f_op %p\n", img_fd.file->f_op);
			printk(KERN_INFO"[SSEL]\tfile->f_op %p\n", meta->file->f_op);
			printk(KERN_INFO"[SSEL]\tfile->f_op->read %p\n", meta->file->f_op->read);
			printk(KERN_INFO"[SSEL]\tfile->f_op->aio_read %p\n", meta->file->f_op->aio_read);

			if(SSEL_READ == meta->RDWR)
			{
				//if (img_fd.file->f_op->read)
				if (meta->file->f_op->read)
				{
					ret = meta->file->f_op->read(meta->file, meta->buf, meta->count, meta->pos);
					printk(KERN_INFO"[SSEL]\tf_op->read Success (ret %d)\n", (int)ret);
				}
				else
				{
					ret = do_sync_read(meta->file, meta->buf, meta->count, meta->pos);
					printk(KERN_INFO"[SSEL]\tdo_sync_read Success (ret %d)\n", (int)ret);
				}
				/*
				   if (ret > 0) {
					fsnotify_access(meta->file);
					add_rchar(current, ret);
				}
				inc_syscr(current);
				*/
			}
			else if(SSEL_WRITE == meta->RDWR)
			{
				//if (img_fd.file->f_op->read)
				file_start_write(meta->file);
				if (meta->file->f_op->write)
				{
					ret = meta->file->f_op->write(meta->file, meta->buf, meta->count, meta->pos);
					printk(KERN_INFO"[SSEL]\tf_op->write Success (ret %d)\n", (int)ret);
				}
				else
				{
					ret = do_sync_write(meta->file, meta->buf, meta->count, meta->pos);
					printk(KERN_INFO"[SSEL]\tdo_sync_write Success (ret %d)\n", (int)ret);
				}
				/*
				   if (ret > 0) {
					fsnotify_access(meta->file);
					add_wchar(current, ret);
				}
				inc_syscr(current);
				*/

				file_end_write(meta->file);
			}
			else
			{
				printk(KERN_INFO"[SSEL]\tRDWR not found\n");
			}
			fdput(img_fd);
		}
		else
		{
			printk(KERN_INFO"[SSEL]\tError\n");
		}

		kfree(meta);
	}
	else
	{
		printk(KERN_INFO"[SSEL]\tmeta data is NULL\n");
	}

	return ret;
}


static int worker_thread(void *arg)
{ 
	ThreadPool *pool;
	printk(KERN_INFO"[SSEL]MODULE_THREAD Online '%s()'\n", __FUNCTION__);

	if(NULL != arg)
	{
		pool = arg; //-> g_ssel_thp
	}
	else
	{
		pool = &g_ssel_thp;
	}

	ssel_mutex_lock(&pool->lock);
	pool->pending_threads--;
	//do_spawn_thread(pool);

	while(!kthread_should_stop() || pool->stopping)
	//while(!pool->stopping);
	{
		ThreadPoolElement *req;
		int ret;

		do
		{
			pool->idle_threads++;
			ssel_mutex_unlock(&pool->lock);
			printk(KERN_INFO"[SSEL]MODULE_THREAD '%s()' loop\n", __func__);
			ret = interruptible_sleep_on_timeout(&WaitQueue, 5000);
			ssel_mutex_lock(&pool->lock);
			pool->idle_threads--;
		}
		while(0 == ret && QTAILQ_EMPTY(&pool->request_list));
		printk(KERN_INFO"[SSEL]MODULE_THREAD interruptible_sleep_on_timeout() return value : %d\n", ret);

		if(!pool->stopping)
		{
			printk(KERN_INFO"[SSEL]MODULE_THREAD '%s()' loop breaking\n", __func__);
			break;
		}

		//mutex_lock
		req = QTAILQ_FIRST(&pool->request_list);
		QTAILQ_REMOVE(&pool->request_list, req, reqs);
		//mutex_unlock
		ssel_mutex_unlock(&pool->lock);
		req->ret = kio_worker((VMMetaData*)req->arg);

		ssel_mutex_lock(&pool->lock);
	}

	ssel_mutex_unlock(&pool->lock);
	printk(KERN_INFO"[SSEL]MODULE_THREAD '%s()' kthread_should_stop() return\n", __func__);
	return 0;
} 


static ssize_t thread_pool_submit_aio(ThreadPool* pool, void* arg)
{
	ThreadPoolElement* req = NULL;
	VMMetaData* meta = NULL;
	ssize_t ret = -EBADF;

	printk(KERN_INFO"[SSEL]MODULE '%s()'\n", __FUNCTION__);

	//req = kmalloc(sizeof(struct ThreadPoolElement), GFP_KERNEL);
	if(NULL != req)
	{ 
		req->arg = arg;
		meta = (VMMetaData*)arg;
		printk(KERN_INFO"[SSEL]\tmeta->imgfd : %d\n", (int)meta->imgfd);
		printk(KERN_INFO"[SSEL]\tmeta->count : %d\n", (int)meta->count);
		printk(KERN_INFO"[SSEL]\tmeta->pos : %lld\n", (long long)meta->pos);

		ssel_mutex_lock(&pool->lock);
		QTAILQ_INSERT_TAIL(&pool->request_list, req, reqs);
		ssel_mutex_unlock(&pool->lock);
		wake_up_interruptible(&WaitQueue);
		
		ret = -1;
	}
	else	//NULL == req
	{   
		printk(KERN_INFO"[SSEL]\tThreadPoolElement Address is NULL\n");
		ret = kio_worker((VMMetaData*)arg);
	}

	return ret;
}  

static ssize_t ssel_read(struct file* file, char __user* buf, size_t count, loff_t* pos)
{
	struct fd img_f;
	ssize_t ret = -EBADF;
	VMMetaData* meta = NULL;

	printk(KERN_INFO"[SSEL]MODULE '%s()'\n", __FUNCTION__);
	printk(KERN_INFO"[SSEL]\tfile address %p\n", file->f_op);


	if (pos < 0)
	{
		printk(KERN_INFO"[SSEL]\t (return -EINVAL) 'pos < 0'\n");
		return -EINVAL;
	}

	img_f = fdget(file->imgfd);

	printk(KERN_INFO"[SSEL]\timgfd : %d\n", file->imgfd);
	if(img_f.file)
	{
		ret = -ESPIPE;
		meta = kmalloc(sizeof(struct VMMetaData), GFP_KERNEL);
		if(NULL != meta)
		{
			meta->imgfd = file->imgfd;
			meta->file = img_f.file;
			printk(KERN_INFO"[SSEL]\tmeta->file address %p\n", meta->file->f_op);
			meta->buf = buf;
			meta->count = count;
			meta->pos = pos;
			meta->RDWR = SSEL_READ;

			ret = thread_pool_submit_aio(&g_ssel_thp, meta);
		}
		else	//NULL == meta
		{
			printk(KERN_INFO"[SSEL]\t 'NULL != meta'\n");
			if (img_f.file->f_mode & FMODE_PREAD)
			{
				ret = vfs_read(img_f.file, buf, count, pos);
				//ret = do_sync_read(img_f.file, buf, count, pos);
				printk(KERN_INFO"[SSEL]\t vfs_read Success (ret %d)\n", (int)ret);
			}
		}

		//now?? next??
		fdput(img_f);
	}
	else
	{
		printk(KERN_INFO"[SSEL]\t'fdget Error'\n");
	}

	printk(KERN_INFO"[SSEL]\tssel_read() return %d\n", (int)ret);
	return ret;
} 
EXPORT_SYMBOL(ssel_read);

static ssize_t ssel_write(struct file* file, const char __user* buf, size_t count, loff_t* pos)
{ 
	struct fd img_f;
	ssize_t ret = -EBADF;
	VMMetaData* meta = NULL;
	
	printk(KERN_INFO"[SSEL]MODULE Called '%s()'\n", __FUNCTION__);

	if (pos < 0)
	{
		printk(KERN_INFO"[SSEL]\t (return -EINVAL) 'pos < 0'\n");
		return -EINVAL;
	}

	img_f = fdget(file->imgfd);
	printk(KERN_INFO"[SSEL]\t imgfd : %d\n", file->imgfd);
	if(img_f.file)
	{
		ret = -ESPIPE;
		meta = kmalloc(sizeof(struct VMMetaData), GFP_KERNEL);
		if(NULL != meta)
		{
			meta->imgfd = file->imgfd;
			meta->file = img_f.file;
			meta->buf = buf;
			meta->count = count;
			meta->pos = pos;
			meta->RDWR = SSEL_WRITE;

			ret = thread_pool_submit_aio(&g_ssel_thp, meta);
		}
		else	//NULL == meta
		{
			printk(KERN_INFO"[SSEL]\t 'NULL != meta'\n");
			if(img_f.file->f_mode & FMODE_PWRITE)
			{
				//ret = do_sync_write(img_f.file, buf, count, pos);
				ret = vfs_write(img_f.file, buf, count, pos);
				printk(KERN_INFO"[SSEL]\t vfs_write Success (ret %d)\n", (int)ret);
			}
		}

		//now?? next??
		fdput(img_f);
	}
	else
	{
		printk(KERN_INFO"[SSEL]\t'fdget Error'\n");
	}

	printk(KERN_INFO"[SSEL]\tssel_write() return %d\n", (int)ret);
	return ret;
}  
EXPORT_SYMBOL(ssel_write);

static int ssel_open(struct inode* node, struct file* file)
{
	int num = MINOR(node->i_rdev);

	printk(KERN_INFO"[SSEL]'%s' Device Driver - call open (minor : %d)\n", DEVICE_FILE_NAME, num);
	
	return 0;
}
EXPORT_SYMBOL(ssel_open);

static int ssel_release(struct inode* inode, struct file* file)
{
	printk(KERN_INFO"[SSEL]Called '%s()'\n", __FUNCTION__);

	return 0;
}	
EXPORT_SYMBOL(ssel_release);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = ssel_read,
	.write = ssel_write,
	.open = ssel_open,
	.release = ssel_release,
};

static int __init ssel_init(void)
{
	printk(KERN_INFO"[SSEL]MODULE Called '%s()'\n", __FUNCTION__);

	//init mutex
	ssel_mutex_init(&g_ssel_thp.lock);
	g_ssel_thp.th_id = NULL;

	if(NULL == g_ssel_thp.th_id)
	{
		g_ssel_thp.th_id = (struct task_struct*)kthread_run(worker_thread, &g_ssel_thp, "worker_thread");
		g_ssel_thp.stopping = true;
	}

	major = register_chrdev(MAJOR_NUM, DEVICE_FILE_NAME, &fops);

	if(major < 0)
	{
		printk(KERN_INFO"[SSEL]Can't Create Device Driver : %d\n", major);
		return major;
	}
	else
	{
		QTAILQ_INIT(&g_ssel_thp.request_list);
		printk(KERN_INFO"[SSEL]MODULE Create Device Driver\n");
		printk(KERN_INFO"[SSEL]MODULE mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, major);
	}
	
	return 0;
}
EXPORT_SYMBOL(ssel_init);

static void __exit ssel_cleanup(void)
{
	printk(KERN_INFO"[SSEL]MODULE Cleaning up module.\n");

	//if(g_ssel_thp.th_id)
	if(g_ssel_thp.stopping)
	{
		g_ssel_thp.stopping = false;
		wake_up_interruptible(&WaitQueue);
		kthread_stop(g_ssel_thp.th_id);
		g_ssel_thp.th_id = NULL;
	}

	unregister_chrdev(major, DEVICE_FILE_NAME);
	printk(KERN_INFO"[SSEL]MODULE Destroy Device Driver\n");
}

module_init(ssel_init);
module_exit(ssel_cleanup);
