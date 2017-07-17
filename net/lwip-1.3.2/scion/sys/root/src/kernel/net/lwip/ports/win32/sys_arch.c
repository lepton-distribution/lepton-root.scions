
/*
Author: Adam Dunkels

The operating system emulation layer provides a common interface
between the lwIP code and the underlying operating system kernel. The
general idea is that porting lwIP to new architectures requires only
small changes to a few header files and a new sys_arch
implementation. It is also possible to do a sys_arch implementation
that does not rely on any underlying operating system.

The sys_arch provides semaphores and mailboxes to lwIP. For the full
lwIP functionality, multiple threads support can be implemented in the
sys_arch, but this is not required for the basic lwIP
functionality. Previous versions of lwIP required the sys_arch to
implement timer scheduling as well but as of lwIP 0.5 this is
implemented in a higher layer.

In addition to the source file providing the functionality of sys_arch,
the OS emulation layer must provide several header files defining
macros used throughout lwip.  The files required and the macros they
must define are listed below the sys_arch description.

Semaphores can be either counting or binary - lwIP works with both
kinds. Mailboxes are used for message passing and can be implemented
either as a queue which allows multiple messages to be posted to a
mailbox, or as a rendez-vous point where only one message can be
posted at a time. lwIP works with both kinds, but the former type will
be more efficient. A message in a mailbox is just a pointer, nothing
more.

Semaphores are represented by the type "sys_sem_t" which is typedef'd
in the sys_arch.h file. Mailboxes are equivalently represented by the
type "sys_mbox_t". lwIP does not place any restrictions on how
sys_sem_t or sys_mbox_t are represented internally.

The following functions must be implemented by the sys_arch:

- void sys_init(void)

  Is called to initialize the sys_arch layer.

- sys_sem_t sys_sem_new(u8_t count)

  Creates and returns a new semaphore. The "count" argument specifies
  the initial state of the semaphore.

- void sys_sem_free(sys_sem_t sem)

  Deallocates a semaphore.

- void sys_sem_signal(sys_sem_t sem)

  Signals a semaphore.

- u32_t sys_arch_sem_wait(sys_sem_t sem, u32_t timeout)

  Blocks the thread while waiting for the semaphore to be
  signaled. If the "timeout" argument is non-zero, the thread should
  only be blocked for the specified time (measured in
  milliseconds).

  If the timeout argument is non-zero, the return value is the number of
  milliseconds spent waiting for the semaphore to be signaled. If the
  semaphore wasn't signaled within the specified time, the return value is
  SYS_ARCH_TIMEOUT. If the thread didn't have to wait for the semaphore
  (i.e., it was already signaled), the function may return zero.

  Notice that lwIP implements a function with a similar name,
  sys_sem_wait(), that uses the sys_arch_sem_wait() function.

- sys_mbox_t sys_mbox_new(void)

  Creates an empty mailbox.

- void sys_mbox_free(sys_mbox_t mbox)

  Deallocates a mailbox. If there are messages still present in the
  mailbox when the mailbox is deallocated, it is an indication of a
  programming error in lwIP and the developer should be notified.

- void sys_mbox_post(sys_mbox_t mbox, void *msg)

  Posts the "msg" to the mailbox.

- u32_t sys_arch_mbox_fetch(sys_mbox_t mbox, void **msg, u32_t timeout)

  Blocks the thread until a message arrives in the mailbox, but does
  not block the thread longer than "timeout" milliseconds (similar to
  the sys_arch_sem_wait() function). The "msg" argument is a result
  parameter that is set by the function (i.e., by doing "*msg =
  ptr"). The "msg" parameter maybe NULL to indicate that the message
  should be dropped.

  The return values are the same as for the sys_arch_sem_wait() function:
  Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a
  timeout.

  Note that a function with a similar name, sys_mbox_fetch(), is
  implemented by lwIP.

- struct sys_timeouts *sys_arch_timeouts(void)

  Returns a pointer to the per-thread sys_timeouts structure. In lwIP,
  each thread has a list of timeouts which is repressented as a linked
  list of sys_timeout structures. The sys_timeouts structure holds a
  pointer to a linked list of timeouts. This function is called by
  the lwIP timeout scheduler and must not return a NULL value.

  In a single threadd sys_arch implementation, this function will
  simply return a pointer to a global sys_timeouts variable stored in
  the sys_arch module.

If threads are supported by the underlying operating system and if
such functionality is needed in lwIP, the following function will have
to be implemented as well:

- sys_thread_t sys_thread_new(void (* thread)(void *arg), void *arg, int prio)

  Starts a new thread with priority "prio" that will begin its execution in the
  function "thread()". The "arg" argument will be passed as an argument to the
  thread() function. The id of the new thread is returned. Both the id and
  the priority are system dependent.

- sys_prot_t sys_arch_protect(void)

  This optional function does a "fast" critical region protection and returns
  the previous protection level. This function is only called during very short
  critical regions. An embedded system which supports ISR-based drivers might
  want to implement this function by disabling interrupts. Task-based systems
  might want to implement this by using a mutex or disabling tasking. This
  function should support recursive calls from the same task or interrupt. In
  other words, sys_arch_protect() could be called while already protected. In
  that case the return value indicates that it is already protected.

  sys_arch_protect() is only required if your port is supporting an operating
  system.

- void sys_arch_unprotect(sys_prot_t pval)

  This optional function does a "fast" set of critical region protection to the
  value specified by pval. See the documentation for sys_arch_protect() for
  more information. This function is only required if your port is supporting
  an operating system.

*/
/*============================================
| Includes
==============================================*/
#include <string.h>

#include "lwip/opt.h"
#include "arch/sys_arch.h"
#include "lwip/sys.h"
#include "lwip/def.h"

#include "kernel/core/kernel_clock.h"
#include "kernel/core/kernel_pthread_mutex.h"

/*============================================
| Global Declaration
==============================================*/
//FIXME use CYG_HWR_whatever for RTC


#define SYS_THREADS     2       /* polling thread and tcpip_thread */

/* List of threads: associate eCos thread info with lwIP timeout info */
struct lwip_thread {
   struct lwip_thread * next;
   //struct sys_timeouts to;
   kernel_pthread_t t;
} *threads;


/*
* Timeout for threads which were not created by sys_thread_new
* usually "main"
*/
//struct sys_timeouts to;



static kernel_pthread_mutex_t kernel_mutex_lwip_protect;

/*============================================
| Implementation
==============================================*/

#if 0
void
sys_timeout(u16_t msecs, sys_timeout_handler h, void *arg)
{
}
/*
* Return current thread's timeout info
*/

struct sys_timeouts *sys_arch_timeouts(void)
{
   struct lwip_thread *t;
   kernel_pthread_t* pthread;

   pthread = kernel_pthread_self();

   //p_task = OS_GetpCurrentTask();

   for (t = threads; t; t = t->next)
      if (&t->t == pthread)
         return &(t->to);

   //to.next=(struct sys_timeout *)0;//lwip 1.1.0
   to.next = (struct sys_timeo *)0; //lwip 1.2.0
   return &to;
}
#endif

/*
/* Time 
*/
u32_t sys_now(void){
   struct timespec ts;
   kernel_clock_gettime(CLOCK_MONOTONIC, &ts);
   return (u32_t)(ts.tv_sec * 1000L + ts.tv_nsec / 1000000L);
}

u32_t sys_jiffies(void) {
   struct timespec ts;
   kernel_clock_gettime(CLOCK_MONOTONIC, &ts);
   return (u32_t)(ts.tv_sec * 1000000000L + ts.tv_nsec);
}

/*
 * Set up memory pools and threads
 */
void sys_init(void){
   pthread_mutexattr_t mutex_attr=0;
   kernel_pthread_mutex_init(&kernel_mutex_lwip_protect,&mutex_attr);
   /*
cyg_mempool_var_create(memvar, sizeof(memvar), &var_mempool_h, &var_mempool);
   threads = NULL;
   to.next = NULL;
*/
}

/*
- sys_prot_t sys_arch_protect(void)

  This optional function does a "fast" critical region protection and returns
  the previous protection level. This function is only called during very short
  critical regions. An embedded system which supports ISR-based drivers might
  want to implement this function by disabling interrupts. Task-based systems
  might want to implement this by using a mutex or disabling tasking. This
  function should support recursive calls from the same task or interrupt. In
  other words, sys_arch_protect() could be called while already protected. In
  that case the return value indicates that it is already protected.

  sys_arch_protect() is only required if your port is supporting an operating
  system.
*/

sys_prot_t sys_arch_protect(void){
   kernel_pthread_mutex_lock(&kernel_mutex_lwip_protect);
   return 0;
}

/*
- void sys_arch_unprotect(sys_prot_t pval)

  This optional function does a "fast" set of critical region protection to the
  value specified by pval. See the documentation for sys_arch_protect() for
  more information. This function is only required if your port is supporting
  an operating system.
*/
void sys_arch_unprotect(sys_prot_t pval){
   kernel_pthread_mutex_unlock(&kernel_mutex_lwip_protect);
}


/*
 * Create a new mbox.If no memory is available return NULL
 */
err_t sys_mbox_new(sys_mbox_t *mbox, int size){
   //
   if(!size)
      size= MAX_QUEUE_ENTRIES;
   //alloc mailbox buffer
   mbox->p_buf = _sys_malloc(size*sizeof(void*));
   /* out of memory? */
   if(!mbox->p_buf)
      return ERR_MEM;
   OS_CreateMB(&mbox->os_mailbox,sizeof(void*),size, mbox->p_buf);
   //
   mbox->is_valid = 1;
   return ERR_OK;
}

/*
 * Destroy the mbox and release the space it took up in the pool
 */
void sys_mbox_free(sys_mbox_t* mbox){
   OS_DeleteMB(&mbox->os_mailbox);
   _sys_free(mbox->p_buf);
   mbox->is_valid = 0;
}

/*
 * cyg_mbox_put should not be passed a NULL otherwise the cyg_mbox_get will not
 * know if it's real data or error condition. But lwIP does pass NULL on occasion
 * in cases when maybe using a semaphore would be better. So this dummy_msg replaces
 * NULL data
 */

int dummy_msg = 1;

/*
 * Post data to a mbox.
 */
void sys_mbox_post(sys_mbox_t* mbox, void *data)
{
   long addr = (long)data;
   if (!data)
      data = &dummy_msg;

   //printf("post mbox:0x%x = 0x%x\r\n",mbox,addr);

   OS_PutMail(&mbox->os_mailbox,&addr);
}


/*
 * Try post data to a mbox. new since lwip 1.3.0
 */
err_t sys_mbox_trypost(sys_mbox_t* mbox, void *data)
{
   long addr = (long)data;
   if (!data)
      data = &dummy_msg;

   //printf("post mbox:0x%x = 0x%x\r\n",mbox,addr);

   if(OS_PutMailCond(&mbox->os_mailbox,&addr))
      return ERR_MEM;

   return ERR_OK;
}

/*
 * Fetch data from a mbox.Wait for at most timeout millisecs
 * Return -1 if timed out otherwise time spent waiting.
 */
u32_t sys_arch_mbox_fetch(sys_mbox_t* mbox, void **data, u32_t timeout)
{
   long addr=0L;
   u32_t end_time = 0, start_time = 0;
   int err=0;

   if (timeout) {
      start_time = OS_GetTime();
      if(OS_GetMailTimed (&mbox->os_mailbox, &addr,timeout)) {
         if (data != NULL) {
            *data = NULL;
         }
         return SYS_ARCH_TIMEOUT;
      }
      end_time = OS_GetTime();

   } else {
      OS_GetMail(&mbox->os_mailbox,&addr);
   }

   if(data != NULL) {
      if (addr == (long)&dummy_msg)
         *data = NULL;
      else
         *data=(void*)addr;

      //printf("fetch mbox:0x%x = 0x%x\r\n",mbox,*data);
   }else{
      //printf("fetch mbox:0x%x\r\n",mbox);
   }

   //
   if((end_time - start_time)<0)
      return 0;
   //
   return (end_time - start_time);
}

/*
 * Try fetch data from a mbox. new since lwip 1.3.0
 */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t* mbox, void **data){
   long addr=0L;
   int err=0;
   //
   if(OS_GetMailCond(&mbox->os_mailbox,&addr)) {
      if (data != NULL) {
         *data = NULL;
      }
      //return SYS_MBOX_EMPTY;
      return SYS_ARCH_TIMEOUT;
   }
   //
   if (data != NULL) {
      if (addr == (long)&dummy_msg)
         *data = NULL;
      else
         *data=(void*)addr;
   }
   //
   return 0;
}

/*
 * Create a new semaphore and initialize it.
 * If no memory is available return NULL
 */
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
  
   /* out of memory? */
   if(!sem)
      return ERR_MEM;
   //
   OS_CreateCSema(&sem->os_csema,count);
   //
   sem->is_valid = 1;
   //
   return ERR_OK;
}


/*
 * Wait on a semaphore for at most timeout millisecs
 * Return -1 if timed out otherwise time spent waiting.
 */
u32_t sys_arch_sem_wait(sys_sem_t* sem, u32_t timeout)
{
   u32_t end_time = 0, start_time = 0;
   int err=0;

   if (timeout) {
      start_time = OS_GetTime();
      if(!OS_WaitCSemaTimed(&sem->os_csema, timeout))
         return SYS_ARCH_TIMEOUT;
      end_time = OS_GetTime();

   } else {
      OS_WaitCSema(&sem->os_csema);
   }

   if((end_time - start_time)<0)
      return 0;

   return (end_time - start_time);
}

/*
 * Signal a semaphore
 */
void sys_sem_signal(sys_sem_t* sem)
{
   OS_SignalCSema(&sem->os_csema);
}

/*
 * Destroy the semaphore and release the space it took up in the pool
 */
void sys_sem_free(sys_sem_t* sem){
   OS_DeleteCSema(&sem->os_csema);
   sem->is_valid = 0;
}


/*
 * Create new thread
 */
sys_thread_t sys_thread_new(const char *name, void (*function)(void *arg), void *arg, int stacksize, int prio)
{

   pthread_attr_t thread_attr;
   struct lwip_thread * nt;
   //void * stack;
   static int thread_count = 0;

   char * pstack = (char*)_sys_malloc(stacksize);
   if(pstack<=0)
      return (sys_thread_t)0;
   //
   nt = (struct lwip_thread *)_sys_malloc(sizeof(struct lwip_thread));
   //
   nt->next = threads;
   //nt->to.next = NULL;
   threads = nt;
   //
   thread_attr.stacksize = stacksize;
   thread_attr.stackaddr = (void*)pstack;
   thread_attr.priority  = 100;
   thread_attr.timeslice = 1;

   if(!name)
      thread_attr.name= "lwip_tread";
   else
      thread_attr.name= (char*)name;

   kernel_pthread_create(&nt->t,&thread_attr,(start_routine_t)function,(char*)0);

   return &nt->t;
}

err_t sys_mutex_new(sys_mutex_t *mutex)
{
   pthread_mutexattr_t mutex_attr = 0;
   //
   if (kernel_pthread_mutex_init(&mutex->kernel_mutex, &mutex_attr) < 0) {
      return ERR_MEM;
   }
   //
   mutex->is_valid = 1;
   //
   return ERR_OK;
}

void sys_mutex_free(sys_mutex_t *mutex)
{
   kernel_pthread_mutex_destroy(&mutex->kernel_mutex);
   mutex->is_valid = 0;
}

void sys_mutex_lock(sys_mutex_t *mutex){
   kernel_pthread_mutex_lock(&mutex->kernel_mutex);
}

void sys_mutex_unlock(sys_mutex_t *mutex){
   kernel_pthread_mutex_unlock(&mutex->kernel_mutex);
}

#if LWIP_NETCONN_SEM_PER_THREAD
sys_sem_t* sys_arch_netconn_sem_get(void)
{
   LPVOID tls_data = TlsGetValue(netconn_sem_tls_index);
   return (sys_sem_t*)tls_data;
}

void sys_arch_netconn_sem_alloc(void)
{
   sys_sem_t *sem;
   err_t err;
   BOOL done;

   sem = (sys_sem_t*)malloc(sizeof(sys_sem_t));
   LWIP_ASSERT("failed to allocate memory for TLS semaphore", sem != NULL);
   err = sys_sem_new(sem, 0);
   LWIP_ASSERT("failed to initialise TLS semaphore", err == ERR_OK);
   done = TlsSetValue(netconn_sem_tls_index, sem);
   LWIP_UNUSED_ARG(done);
   LWIP_ASSERT("failed to initialise TLS semaphore storage", done == TRUE);
}

void sys_arch_netconn_sem_free(void)
{
   LPVOID tls_data = TlsGetValue(netconn_sem_tls_index);
   if (tls_data != NULL) {
      BOOL done;
      free(tls_data);
      done = TlsSetValue(netconn_sem_tls_index, NULL);
      LWIP_UNUSED_ARG(done);
      LWIP_ASSERT("failed to de-init TLS semaphore storage", done == TRUE);
   }
}
#endif /* LWIP_NETCONN_SEM_PER_THREAD */
/*============================================
| End of Source  : sys_arch.c
==============================================*/
