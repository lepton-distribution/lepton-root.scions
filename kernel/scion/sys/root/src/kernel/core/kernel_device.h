/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Chauvin-Arnoux.
Portions created by Chauvin-Arnoux are Copyright (C) 2011. All Rights Reserved.

Alternatively, the contents of this file may be used under the terms of the eCos GPL license
(the  [eCos GPL] License), in which case the provisions of [eCos GPL] License are applicable
instead of those above. If you wish to allow use of your version of this file only under the
terms of the [eCos GPL] License and not to allow others to use your version of this file under
the MPL, indicate your decision by deleting  the provisions above and replace
them with the notice and other provisions required by the [eCos GPL] License.
If you do not delete the provisions above, a recipient may use your version of this file under
either the MPL or the [eCos GPL] License."
*/

/*===========================================
Compiler Directive
=============================================*/
#ifndef __KERNEL_DEVICE_H__
#define __KERNEL_DEVICE_H__


/*===========================================
Includes
=============================================*/



/*===========================================
Declaration
=============================================*/

//console device interface
extern desc_t __g_kernel_desc_tty;
#define __set_kernel_tty_desc(__kernel_desc_tty__) __g_kernel_desc_tty = __kernel_desc_tty__
#define __get_kernel_tty_desc() (__g_kernel_desc_tty)

//cpu device interface
extern fdev_map_t*   __g_kernel_cpu;
extern desc_t __g_kernel_desc_cpu;

#define __set_cpu(__p_kernel_cpu__) __g_kernel_cpu = __p_kernel_cpu__
#define __get_cpu() __g_kernel_cpu

#define __set_cpu_desc(__desc_kernel_cpu__) __g_kernel_desc_cpu = __desc_kernel_cpu__
#define __get_cpu_desc() __g_kernel_desc_cpu

//i2c device interface
extern fdev_map_t*   __g_kernel_if_i2c_master;
extern desc_t __g_kernel_desc_if_i2c_master;

#define __set_if_i2c_master(__p_if_i2c_master__) __g_kernel_if_i2c_master = __p_if_i2c_master__
#define __get_if_i2c_master() __g_kernel_if_i2c_master

#define __set_if_i2c_master_desc(__desc_if_i2c_master__) __g_kernel_desc_if_i2c_master = __desc_if_i2c_master__
#define __get_if_i2c_master_desc() __g_kernel_desc_if_i2c_master

extern kernel_pthread_mutex_t _i2c_core_mutex;

#define _i2c_lock() kernel_pthread_mutex_lock(&_i2c_core_mutex);
#define _i2c_unlock() kernel_pthread_mutex_unlock(&_i2c_core_mutex);

//spi device interface
extern fdev_map_t*   __g_kernel_if_spi_master;
extern desc_t __g_kernel_desc_if_spi_master;

#define __set_if_spi_master(__p_if_spi_master__) __g_kernel_if_spi_master = __p_if_spi_master__
#define __get_if_spi_master() __g_kernel_if_spi_master

#define __set_if_spi_master_desc(__desc_if_spi_master__) __g_kernel_desc_if_spi_master = __desc_if_spi_master__
#define __get_if_spi_master_desc() __g_kernel_desc_if_spi_master

extern kernel_pthread_mutex_t _spi_core_mutex;

#define _spi_lock()   kernel_pthread_mutex_lock  (&_spi_core_mutex);
#define _spi_unlock() kernel_pthread_mutex_unlock(&_spi_core_mutex);
#endif
