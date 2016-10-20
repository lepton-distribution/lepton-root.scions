
/*============================================
| Compiler Directive
==============================================*/
#ifndef __UNISTD_H__
#define __UNISTD_H__


/*============================================
| Includes
==============================================*/
#include "kernel/core/kal.h"
#include "kernel/core/system.h"
/*============================================
| Declaration
==============================================*/

#ifdef __cplusplus
extern "C" {
#endif
int   mount(int fstype,const char* dev_path, const char* mount_path);
int   umount(const char* mount_path);
void  sync(void);
int   creat(const char *path,mode_t mode);
int   open(const char *path, int oflag,mode_t mode);
int   close(int fildes);
int   remove(const char *path);
int   rename(const char *old_name, const char *new_name);


off_t    lseek(int fildes, off_t offset, int whence);
ssize_t  read(int fildes, void *buf, size_t nbyte);
ssize_t  write(int fildes, const void *buf, size_t nbyte);


int ioctl(int fildes, int request, ... );

#ifdef __cplusplus
}
#endif

//posix compatiblity: symbolic link not yet supported on lepton.
//in future development syscall unlink must be implemented.
#define unlink(__path__) remove(__path__)

#define uname(__utsname__)_unistd_uname(__utsname__)

#endif
