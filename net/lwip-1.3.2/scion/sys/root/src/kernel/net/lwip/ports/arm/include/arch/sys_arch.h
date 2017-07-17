
#ifndef __SYS_ARM_ARCH_H__
#define __SYS_ARM_ARCH_H__


#include "kernel/core/kernel_pthread.h"
#include "kernel/core/kernel_pthread_mutex.h"


/* Use this pragma instead of the one below to disable all
warnings     */
#ifdef WIN32
#pragma warning(disable:4761;disable:4244)
#endif


#ifndef MAX_QUEUE_ENTRIES
   #define MAX_QUEUE_ENTRIES 100
#endif

#define SYS_MBOX_NULL (sys_mbox_t)NULL
#define SYS_SEM_NULL  (sys_sem_t)NULL


typedef struct sys_mbox_st {
   OS_MAILBOX os_mailbox;
   unsigned char is_valid;
   char* p_buf;
}sys_mbox_t;

//
typedef struct sys_sem_st{
    OS_CSEMA  os_csema;
    unsigned char is_valid;
}sys_sem_t;

//
typedef struct sys_mutex_st {
   kernel_pthread_mutex_t  kernel_mutex;
   unsigned char is_valid;
}sys_mutex_t;

typedef kernel_pthread_t *       sys_thread_t;
typedef OS_RSEMA*                sys_prot_t;

///
#define sys_sem_valid(__sema__) (((__sema__) != NULL) && ((__sema__)->is_valid != 0) )
#define sys_sem_set_invalid(__sema__) ((__sema__)->is_valid = 0)
//
#define sys_mutex_valid(__mutex__) (((__mutex__) != NULL) && ((__mutex__)->is_valid != 0) )
#define sys_mutex_set_invalid(__mutex__) ((__mutex__)->is_valid = 0)
//
#define sys_mbox_valid(__mbox__) ((__mbox__ != NULL) && ((__mbox__)->is_valid != 0) )
#define sys_mbox_set_invalid(__mbox__) ((__mbox__)->is_valid = 0)

//
sys_sem_t* sys_arch_netconn_sem_get(void);
void sys_arch_netconn_sem_alloc(void);
void sys_arch_netconn_sem_free(void);

#define LWIP_NETCONN_THREAD_SEM_GET()   sys_arch_netconn_sem_get()
#define LWIP_NETCONN_THREAD_SEM_ALLOC() sys_arch_netconn_sem_alloc()
#define LWIP_NETCONN_THREAD_SEM_FREE()  sys_arch_netconn_sem_free()
#endif

