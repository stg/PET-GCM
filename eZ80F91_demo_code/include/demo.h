/*                                                   Copyright (c) ZiLOG, 2003
 *****************************************************************************
 *  demo.h (1.0)
 *
 *****************************************************************************
 */

/* LCD user interface elements */
extern void intro_lcd( void );
extern void write_to_LCD( void );

/* Console user interface elements */
extern char console_welcome[];
extern char console_menu[];

/* misc. */
extern char device_name[];
extern char features[];
extern char version[];

/* RTC user interface elements */
extern void Display_RTC(void);
extern void rtc_run_switch( void );

extern char am_flag;
extern int lcd_update;
extern int rtc_buff[20];	// RTC ascii buff

