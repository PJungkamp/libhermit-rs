/*
 * Copyright (c) 2010, Stefan Lankes, RWTH Aachen University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the University nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <hermit/stddef.h>
#include <hermit/stdio.h>
#include <hermit/tasks.h>
#include <hermit/errno.h>
#include <hermit/syscall.h>
#include <hermit/spinlock.h>
#include <hermit/semaphore.h>
#include <hermit/time.h>

#include <lwip/sockets.h>
#include <lwip/err.h>
#include <lwip/stats.h>

//TODO: don't use one big kernel lock to comminicate with all proxies
static spinlock_t lwip_lock = SPINLOCK_INIT;

static tid_t sys_getpid(void)
{
	task_t* task = per_core(current_task);

	return task->id;
}

static int sys_getprio(void)
{
	task_t* task = per_core(current_task);

	return task->prio;
}

static void sys_yield(void)
{
	reschedule();
}

void NORETURN do_exit(int arg);

typedef struct {
	int sysnr;
	int arg;
} __attribute__((packed)) sys_exit_t;

/** @brief To be called by the systemcall to exit tasks */
void NORETURN sys_exit(int arg)
{
	task_t* task = per_core(current_task);
	sys_exit_t sysargs = {__NR_exit, arg};

	if (task->sd >= 0)
	{
		spinlock_lock(&lwip_lock);
		write(task->sd, &sysargs, sizeof(sysargs));
		spinlock_unlock(&lwip_lock);

		closesocket(task->sd);
		task->sd = -1;
	}

	do_exit(arg);
}

typedef struct {
	int sysnr;
	int fd;
	size_t len;
} __attribute__((packed)) sys_read_t;

static ssize_t sys_read(int fd, char* buf, size_t len)
{
	task_t* task = per_core(current_task);
	sys_read_t sysargs = {__NR_read, fd, len};
	ssize_t j, ret;

	if (task->sd < 0)
		return -ENOSYS;

	spinlock_lock(&lwip_lock);
	write(task->sd, &sysargs, sizeof(sysargs));

	read(task->sd, &j, sizeof(j));
	if (j > 0)
	{
		ssize_t i = 0;

		while(i < j)
		{
			ret = read(task->sd, buf+i, j-i);
			if (ret < 0) {
				spinlock_unlock(&lwip_lock);
				return ret;
			}

			i += ret;
		}
	}

	spinlock_unlock(&lwip_lock);

	return j;
}

typedef struct {
	int sysnr;
	int fd;
	size_t len;
} __attribute__((packed)) sys_write_t;

static ssize_t sys_write(int fd, const char* buf, size_t len)
{
	task_t* task = per_core(current_task);
	ssize_t i, ret;
	int flag;
	sys_write_t sysargs = {__NR_write, fd, len};

	if (BUILTIN_EXPECT(!buf, 0))
		return -1;

	if (task->sd < 0)
	{
		for(i=0; i<len; i++)
			kputchar(buf[i]);

		return len;
	}

	spinlock_lock(&lwip_lock);

	flag = 0;
	setsockopt(task->sd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(flag));

	write(task->sd, &sysargs, sizeof(sysargs));

	i=0;
	while(i < len)
	{
		ret = write(task->sd, (char*)buf+i, len-i);
		if (ret < 0) {
			spinlock_unlock(&lwip_lock);
			return ret;
		}

		i += ret;
	}

	flag = 1;
	setsockopt(task->sd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(flag));

	if (fd > 2) {
		ret = read(task->sd, &i, sizeof(i));
		if (ret < 0)
			i = ret;
	} else i = len;

	spinlock_unlock(&lwip_lock);

	return i;
}

static ssize_t sys_sbrk(int incr)
{
	task_t* task = per_core(current_task);
	vma_t* heap = task->heap;
	ssize_t ret;

	spinlock_lock(&task->vma_lock);

	if (BUILTIN_EXPECT(!heap, 0)) {
		kprintf("sys_sbrk: missing heap!\n");
		abort();
	}

	ret = heap->end;
	heap->end += incr;
	if (heap->end < heap->start)
		heap->end = heap->start;

	// allocation and mapping of new pages for the heap
	// is catched by the pagefault handler

	spinlock_unlock(&task->vma_lock);

	return ret;
}

static int sys_open(const char* name, int flags, int mode)
{
	task_t* task = per_core(current_task);
	int i, ret, sysnr = __NR_open;
	size_t len;

	if (task->sd < 0)
		return 0;

	len = strlen(name)+1;

	spinlock_lock(&lwip_lock);

	i = 0;
	setsockopt(task->sd, IPPROTO_TCP, TCP_NODELAY, (char *) &i, sizeof(i));

	ret = write(task->sd, &sysnr, sizeof(sysnr));
	if (ret < 0)
		goto out;

	ret = write(task->sd, &len, sizeof(len));
	if (ret < 0)
		goto out;

	i=0;
	while(i<len)
	{
		ret = write(task->sd, name+i, len-i);
		if (ret < 0)
			goto out;
		i += ret;
	}

	ret = write(task->sd, &flags, sizeof(flags));
	if (ret < 0)
		goto out;

	ret = write(task->sd, &mode, sizeof(mode));
	if (ret < 0)
		goto out;

	i = 1;
	setsockopt(task->sd, IPPROTO_TCP, TCP_NODELAY, (char *) &i, sizeof(i));

	read(task->sd, &ret, sizeof(ret));

out:
	spinlock_unlock(&lwip_lock);

	return ret;
}

typedef struct {
	int sysnr;
	int fd;
} __attribute__((packed)) sys_close_t;

static int sys_close(int fd)
{
	int ret;
	task_t* task = per_core(current_task);
	sys_close_t sysargs = {__NR_close, fd};

	if (task->sd < 0)
		return 0;

	spinlock_lock(&lwip_lock);

	ret = write(task->sd, &sysargs, sizeof(sysargs));
	if (ret != sizeof(sysargs))
		goto out;
	read(task->sd, &ret, sizeof(ret));

out:
	spinlock_unlock(&lwip_lock);

	return ret;
}

static int sys_msleep(unsigned int msec)
{
	timer_wait(msec*TIMER_FREQ/1000);

	return 0;
}

static int sys_sem_init(sem_t** sem, unsigned int value)
{
	int ret;

	if (BUILTIN_EXPECT(!sem, 0))
		return -EINVAL;

	*sem = (sem_t*) kmalloc(sizeof(sem_t));
	if (BUILTIN_EXPECT(!(*sem), 0))
		return -ENOMEM;

	ret = sem_init(*sem, value);
	if (ret) {
		kfree(*sem);
		*sem = NULL;
	}

	return ret;
}

static int sys_sem_destroy(sem_t* sem)
{
	int ret;

	if (BUILTIN_EXPECT(!sem, 0))
		return -EINVAL;

	ret = sem_destroy(sem);
	if (!ret)
		kfree(sem);

	return ret;
}

static int sys_sem_wait(sem_t* sem)
{
	if (BUILTIN_EXPECT(!sem, 0))
		return -EINVAL;

	return sem_wait(sem, 0);
}

static int sys_sem_post(sem_t* sem)
{
	if (BUILTIN_EXPECT(!sem, 0))
		return -EINVAL;

	return sem_post(sem);
}

static int sys_sem_timedwait(sem_t *sem, unsigned int ms)
{
	if (BUILTIN_EXPECT(!sem, 0))
		return -EINVAL;

	return sem_wait(sem, ms);
}

static int sys_clone(tid_t* id, void* ep, void* argv)
{
	return clone_task(id, ep, argv, per_core(current_task)->prio);
}

typedef struct {
	int sysnr;
	int fd;
	off_t offset;
	int whence;
} __attribute__((packed)) sys_lseek_t;

static off_t sys_lseek(int fd, off_t offset, int whence)
{
	off_t off;
	task_t* task = per_core(current_task);
	sys_lseek_t sysargs = {__NR_lseek, fd, offset, whence};

	if (task->sd < 0)
		return -ENOSYS;

	spinlock_lock(&lwip_lock);

	write(task->sd, &sysargs, sizeof(sysargs));
	read(task->sd, &off, sizeof(off));

	spinlock_unlock(&lwip_lock);

	return off;
}

static int default_handler(void)
{
#if 0
	kprintf("Invalid system call\n");
#else
	uint64_t rax;

	asm volatile ("mov %%rax, %0" : "=m"(rax) :: "memory");
	kprintf("Invalid system call: %zd\n", rax);
#endif
	return -ENOSYS;
}

size_t syscall_table[] = {
	(size_t) sys_exit,		/* __NR_exit 	*/
	(size_t) sys_write,		/* __NR_write 	*/
	(size_t) sys_open, 		/* __NR_open 	*/
	(size_t) sys_close,		/* __NR_close 	*/
	(size_t) sys_read,		/* __NR_read 	*/
	(size_t) sys_lseek,		/* __NR_lseek	*/
	(size_t) default_handler, 	/* __NR_unlink	*/
	(size_t) sys_getpid, 		/* __NR_getpid	*/
	(size_t) default_handler,	/* __NR_kill	*/
	(size_t) default_handler,	/* __NR_fstat	*/
	(size_t) sys_sbrk,		/* __NR_sbrk	*/
	(size_t) default_handler,	/* __NR_fork	*/
	(size_t) default_handler,	/* __NR_wait	*/
	(size_t) default_handler,	/* __NR_execve	*/
	(size_t) default_handler,	/* __NR_times	*/
	(size_t) default_handler,	/* __NR_accept	*/
	(size_t) default_handler,	/* __NR_bind	*/
	(size_t) default_handler,	/* __NR_closesocket	*/
	(size_t) default_handler,	/* __NR_connect	*/
	(size_t) default_handler,	/* __NR_listen	*/
	(size_t) default_handler,	/* __NR_recv	*/
	(size_t) default_handler,	/* __NR_send	*/
	(size_t) default_handler,	/* __NR_socket	*/
	(size_t) default_handler,	/* __NR_getsockopt	*/
	(size_t) default_handler,	/* __NR_setsockopt	*/
	(size_t) default_handler, 	/* __NR_gethostbyname	*/
	(size_t) default_handler,	/* __NR_sendto	*/
	(size_t) default_handler,	/* __NR_recvfrom	*/
	(size_t) default_handler,	/* __NR_select	*/
	(size_t) default_handler,	/* __NR_stat	*/
	(size_t) default_handler,	/* __NR_dup	*/
	(size_t) default_handler,	/* __NR_dup2	*/
	(size_t) sys_msleep,		/* __NR_msleep	*/
	(size_t) sys_yield,		/* __NR_yield	*/
	(size_t) sys_sem_init,	 	/* __NR_sem_init	*/
	(size_t) sys_sem_destroy, 	/* __NR_sem_destroy	*/
	(size_t) sys_sem_wait,	 	/* __NR_sem_wait	*/
	(size_t) sys_sem_post,	 	/* __NR_sem_post	*/
	(size_t) sys_sem_timedwait, 	/* __NR_sem_timedwait	*/
	(size_t) sys_getprio,		/* __NR_getprio	*/
	(size_t) default_handler,	/* __NR_setprio	*/
	(size_t) sys_clone,		/* __NR_clone	*/
	(size_t) sys_sem_timedwait	/*  __NR_sem_cancelablewait	*/
};
