```c
 \* Copyright (c) 2023, RT-Thread Development Team
 *
 \* SPDX-License-Identifier: Apache-2.0
 *
 \* Change Logs:
 \* Date      Author    Notes
 \* 2023-07-06   Supperthomas first version
 \* 2023-12-03   Meco Man   support nano version
 */\#include <board.h>
\#include <rtthread.h>
\#include <drv_gpio.h>
\#ifndef RT_USING_NANO
\#include <rtdevice.h>
\#include <rtdbg.h>
\#endif /* RT_USING_NANO */
rt_thread_t th1,th2 = RT_NULL;
 
void th1_entry(void){
  while (1)
  {
    rt_kprintf("th1 is running...\n");
    rt_thread_mdelay(500);
  }
  
}
void th2_entry(void){
  while (1)
  {
    rt_kprintf("th2 is running...\n");
    rt_thread_mdelay(500);
  }
}

int main(void)
{
  th1 = rt_thread_create("th1", th1_entry, RT_NULL, 1024, 20, 5);
  if(th1 == RT_NULL){
    LOG_E("thread th1 create failed...\n");
  }
  else{
    rt_thread_startup(th1);
    LOG_D("thread th1 create success!\n");
  }
  th2 = rt_thread_create("th2", th2_entry, RT_NULL, 1024, 21, 5);
  if(th2 == RT_NULL){
    LOG_E("thread th2 create failed...\n");
  }
  else{
    rt_thread_startup(th2);
    LOG_D("thread th2 create success!\n");
  }
}
```