#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <linux/input.h>
#include "touch.h"

#define dev_fileName "/dev/input/event0"    /*触摸屏设备节点*/
static touch_struct *touch_data;                   /*触摸点数据结构体*/
static touch_opt_handle_t touch_opt_handle;        /*外部处理函数指针*/
static uint8_t click_flag = 0;      /*多触点置1 移动置2*/

/*定时器信号处理函数*/
void click_timer_handler(int signo) {
    // if(touch_data->click_num == 0)
    // {
    //     /*标志位置3*/
    //     click_flag = 3;
    //     /*长按事件*/
    //     touch_data->type = TOUCH_PRESS;
    //     /*向外部传递数据*/
    //     touch_opt_handle(touch_data);
    // }
    if(touch_data->click_num == 1)
    {
        /*单击事件*/
        touch_data->type = TOUCH_CLICK;
        /*向外部传递数据*/
        touch_opt_handle(touch_data);
    }
    else
    {
        /*双击事件*/
        touch_data->type = TOUCH_DOUBLECLICK; 
        /*向外部传递数据*/
        touch_opt_handle(touch_data);
    }

    touch_data->click_num = 0;
}

int touch_process(touch_opt_handle_t handle)
{
    /**
     * 1.结构体定义
     * 2.打开设备节点
     * 3.阻塞读取
     * 4.根据读取到的数据type code value执行不同操作
    */

    int fd;
    struct input_event in_event = {0};  /*设备节点数据结构体*/
    static int down = 0;                /*1为按下 0为松开 -1为移动*/
    static int valid = 0;               /*数据是否有效*/
    static int finger = 0;              /*触点数量*/
    static int slot = 0;                /*当前触点*/
    static int data_new[5][2];          /*保存触点最新数据*/
    static int data_old[5][2];          /*保存触点上一次数据*/
    static int distance_new = 0;        /*两点间当前距离*/
    static int distance_old = 0;        /*两点间上一次距离*/
    struct itimerval timer;             /*定时器结构体*/

    /*触点数据结构体初始化*/
    touch_data = malloc(sizeof(struct touch_struct));
    touch_data->click_num = 0;

    /*指针指向外部处理函数*/
    touch_opt_handle = handle;

    /*触点数据清零*/
    memset(data_new, 0, 5*2*sizeof(int));
    memset(data_old, 0, 5*2*sizeof(int));

    /*设定定时器信号处理函数*/
    signal(SIGALRM, click_timer_handler);
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 200000;//200ms
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;//定时器一次调用仅触发一次

    /*打开设备节点*/
    fd = open(dev_fileName, O_RDONLY);
    if(fd < 0) {
        perror("open");
        return -1;
    }

    while(1)
    {
        /*阻塞读取设备数据*/
        if(read(fd, &in_event, sizeof(struct input_event)) != sizeof(struct input_event))
        {
            perror("read");
            return -1;
        }

        // printf("input event:type:%d code:%d value:%d\n", in_event.type,
        //     in_event.code,in_event.value);

        switch(in_event.type)
        {
            /*按键事件：第一根手指触摸、最后一根手指离开*/
            case EV_KEY:
                if(in_event.code == BTN_TOUCH)
                {
                    down = in_event.value;
                    valid = 1;
                }
                break;

            /*绝对位移事件：X坐标、Y坐标、新触点关联slot、触点ID创建、替换和撤销*/
            case EV_ABS:
                switch (in_event.code)
                {
                    /*X轴坐标*/
                    case ABS_MT_POSITION_X:
                        data_old[slot][0] = data_new[slot][0];
                        data_new[slot][0] = in_event.value;
                        valid = 1;
                        break;

                    /*Y轴坐标*/
                    case ABS_MT_POSITION_Y:
                        data_old[slot][1] = data_new[slot][1];
                        data_new[slot][1] = in_event.value;
                        valid = 1;
                        break;  

                    /*新触碰的手指关联slot*/
                    case ABS_MT_SLOT:
                        slot = in_event.value;
                        break;

                    /*触摸点的创建、替换和销毁*/
                    case ABS_MT_TRACKING_ID:
                        if(in_event.value == -1)
                            finger--;
                        else
                            finger++;
                        break;
                }
                break;

            /*同步事件：告知一轮数据上报完成*/
            case EV_SYN:
                if(in_event.code == SYN_REPORT)
                {
                    /*数据有效*/
                    if(valid == 1)
                    {
                        /*检测到第一个触点*/
                        if(down == 1)
                        {
                            touch_data->one_clickData.x = data_new[0][0];
                            touch_data->one_clickData.y = data_new[0][1];
                            // /*开启定时器*/
                            // setitimer(ITIMER_REAL, &timer, NULL);
                        }
                        /*最后一个触点离开*/
                        else if(down == 0)
                        {
                            /*如果仅为点击操作*/
                            if(click_flag == 0)
                            {
                                touch_data->click_num++;
                                /*开启定时器*/
                                setitimer(ITIMER_REAL, &timer, NULL); 
                            }

                            click_flag = 0;

                            /*触点数据清零*/
                            memset(data_new, 0, 5*2*sizeof(int));
                            memset(data_old, 0, 5*2*sizeof(int));
                            distance_new = distance_old = 0;
                        }
                        /*触点移动*/
                        else if(down == -1)
                        {
                            /*单个手指移动*/
                            if(finger == 1)
                            {
                                /*标志位置1*/
                                click_flag = 2;

                                /*保存最新坐标、坐标偏移量*/
                                touch_data->one_clickData.x = data_new[slot][0];
                                touch_data->one_clickData.y = data_new[slot][1];
                                touch_data->one_clickData.offset_x = data_new[slot][0]-data_old[slot][0];
                                touch_data->one_clickData.offset_y = data_new[slot][1]-data_old[slot][1];

                                touch_data->type = TOUCH_MOVE;

                                /*向外部传递数据*/
                                touch_opt_handle(touch_data);
                            }
                            /*双手指：放大、缩小*/
                            else
                            {
                                /*标志位置1*/
                                click_flag = 1;

                                /*求两点间距离*/
                                distance_new = sqrt(
                                (pow(fabs(data_new[1][0]-data_new[0][0]),2))+
                                (pow(fabs(data_new[1][1]-data_new[0][1]),2)));

                                /*上一次两点间距离不为0*/
                                if(distance_old != 0)
                                {
                                   if(distance_new > distance_old)
                                    {
                                        touch_data->type = TOUCH_MAGNIFY;
                                    }
                                    else
                                    {
                                        touch_data->type = TOUCH_SHRINK;
                                    }
                                    /*保存中心点坐标、两点间距离偏移量*/
                                    touch_data->more_clickData.centre_x = fabs(data_new[1][0]-data_new[0][0])/2;
                                    touch_data->more_clickData.centre_y = fabs(data_new[1][1]-data_new[0][1])/2;
                                    touch_data->more_clickData.offset_distance = distance_new - distance_old;

                                    /*向外部传递数据*/
                                    touch_opt_handle(touch_data);
                                }
                                
                                /*将本次距离保存至distance_old*/
                                distance_old = distance_new;
                            }
                        }

                        down = -1;
                        valid = 0;
                    }
                }
                break;

            default:
                break;
        }
    }
    close(fd);
    return 0;
}