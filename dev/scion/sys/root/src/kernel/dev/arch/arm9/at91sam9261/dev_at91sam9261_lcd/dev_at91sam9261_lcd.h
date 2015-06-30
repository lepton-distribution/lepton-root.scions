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


/*============================================
| Compiler Directive
==============================================*/
#ifndef __DEV_AT91SAM9261_LCD_H___
#define __DEV_AT91SAM9261_LCD_H___

/*============================================
| Includes
==============================================*/



/*===========================================
Declarations
=============================================*/
extern const char dev_at91sam9261_lcd_name[];

extern int dev_at91sam9261_lcd_load(dev_panel_info_t* p_dev_panel_info);
extern int dev_at91sam9261_lcd_open(desc_t desc, int o_flag);
extern int dev_at91sam9261_lcd_close(desc_t desc);
extern int dev_at91sam9261_lcd_isset_read(desc_t desc);
extern int dev_at91sam9261_lcd_isset_write(desc_t desc);
extern int dev_at91sam9261_lcd_read(desc_t desc, char* buf,int size);
extern int dev_at91sam9261_lcd_write(desc_t desc, const char* buf,int size);
extern int dev_at91sam9261_lcd_seek(desc_t desc,int offset,int origin);
extern int dev_at91sam9261_lcd_ioctl(desc_t desc,int request,va_list ap);


#endif //end of DEV_AT91SAM9261_LCD_H_
/*============================================
| End of Header  : dev_at91sam9261_lcd.h
==============================================*/

/*--------------------------------------------
| Created:  13 sept. 2010
| Revision/Date: $Revision: 1.3 $  $Date: 2010-02-08 09:17:04 $
| Description:
|---------------------------------------------
| Historic:
|---------------------------------------------
| Authors     | Date     | Comments
| $Log: not supported by cvs2svn $
|
|---------------------------------------------*/


