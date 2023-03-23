#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "touch.h"

void touch_handle(touch_struct *touch_data)
{
    uint16_t m_event = touch_data->type;

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
    else if(0 != (m_event & TOUCH_PRESS))
    {
        printf("操作:长按,坐标:(%d,%d)\n",
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

void handle()
{
    printf("test\n");
}

int main()
{
    while(1)
    {
        touch_process(touch_handle);
    }
}