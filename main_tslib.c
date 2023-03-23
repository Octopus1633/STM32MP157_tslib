/***************************************************************************
文件名 : main_tslib.c
作者 : Octopus
博客 : https://blog.csdn.net/Octopus1633?
描述 : 识别用户在触摸屏上的操作，如:单击、双击、长按、移动、放大、缩小
交叉编译示例 : $ {CC} touch_tslib.c main_tslib.c -o touch_tslib -lts
程序运行示例 : $ ./voice touch_tslib
***************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include "touch_tslib.h"

void touch_event_handler(touch_struct *touch_data)
{
    uint16_t m_event = touch_data->event;

    /*单击*/
    if(0 != (m_event & TOUCH_CLICK))
    {
        printf("操作:单击,坐标:(%d,%d)\n",
        touch_data->one_clickData.x,
        touch_data->one_clickData.y);
    }
    /*双击*/
    else if(0 != (m_event & TOUCH_DOUBLECLICK))
    {
        printf("操作:双击,坐标:(%d,%d)\n",
        touch_data->one_clickData.x,
        touch_data->one_clickData.y);
    }
    /*长按*/
    else if(0 != (m_event & TOUCH_LONGPRESS))
    {
        printf("操作:长按,坐标:(%d,%d)\n",
        touch_data->one_clickData.x,
        touch_data->one_clickData.y);
    }
    /*长按松开*/
    else if(0 != (m_event & TOUCH_LOOSE))
    {
        printf("操作:长按松开,坐标:(%d,%d)\n",
        touch_data->one_clickData.x,
        touch_data->one_clickData.y);
    }
    /*移动*/
    else if(0 != (m_event & TOUCH_MOVE))
    {
        printf("操作:移动,坐标:(%d,%d),坐标偏移:(%d,%d)\n",
        touch_data->one_clickData.x,
        touch_data->one_clickData.y,
        touch_data->one_clickData.offset_x,
        touch_data->one_clickData.offset_y);
    }
    /*放大*/
    else if(0 != (m_event & TOUCH_MAGNIFY))
    {
        printf("操作:放大,中心坐标:(%d,%d),距离偏移:%d\n",
        touch_data->more_clickData.centre_x,
        touch_data->more_clickData.centre_y,
        touch_data->more_clickData.offset_distance);
    }
    /*缩小*/
    else if(0 != (m_event & TOUCH_SHRINK))
    {
        printf("操作:缩小,中心坐标:(%d,%d),距离偏移:%d\n",
        touch_data->more_clickData.centre_x,
        touch_data->more_clickData.centre_y,
        touch_data->more_clickData.offset_distance);
    }
}

void *touch_thread(void *arg)
{
    while(1)
    {
        touch_process(touch_event_handler);
    }
}

int main()
{
    pthread_t pid;

    /*创建屏幕数据处理线程*/
    pthread_create(&pid,NULL,touch_thread,NULL);
    /*分离线程*/
    pthread_detach(pid);
    /*初始化*/
    touch_init();

    while(1)
    {
        sleep(1);
    }

    /*释放*/
    touch_free();
}

