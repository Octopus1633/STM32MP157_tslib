/***************************************************************************
文件名 : touch_tslib.c
作者 : Octopus
博客 : https://blog.csdn.net/Octopus1633?
描述 : 识别用户在触摸屏上的操作，如:单击、双击、长按、移动、放大、缩小
交叉编译示例 : $ {CC} touch_tslib.c main_tslib.c -o touch_tslib -lts
程序运行示例 : $ ./voice touch_tslib
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <unistd.h>
#include <tslib.h>
#include <math.h>
#include <time.h>
#include "touch_tslib.h"

int max_slots;                          //触摸屏支持最大触点数
struct tsdev *ts = NULL;                //触摸屏设备文件指针
struct input_absinfo slot;              //触点信息结构体
struct ts_sample_mt *mt_ptr = NULL;     //触点数据结构体（每次读取）
static touch_opt_handle_t touch_handle; //回调函数指针，指向外部用户自定义处理函数
static touch_struct *touch_data;        //触点数据结构体（传递给外部处理函数）

/*触摸屏初始化：打开触摸屏设备、相关变量、结构体初始化*/
void touch_init()
{
     /*打开并配置触摸屏设备，设置为非阻塞*/
    ts = ts_setup(NULL, 1);
    if (NULL == ts) {
        fprintf(stderr, "ts_setup error");
        exit(EXIT_FAILURE);
    }

    /*获取触摸屏支持的最大触摸点数*/
    if (0 > ioctl(ts_fd(ts), EVIOCGABS(ABS_MT_SLOT), &slot)) {
        perror("ioctl error");
        ts_close(ts);
        exit(EXIT_FAILURE);
    }

    /*计算得到最大触摸点数量*/
    max_slots = slot.maximum + 1 - slot.minimum;
    printf("max_slots: %d\n", max_slots);

    /*内存分配*/
    mt_ptr = calloc(max_slots, sizeof(struct ts_sample_mt));

    /*触点数据结构体初始化*/
    touch_data = malloc(sizeof(struct touch_struct));
}

/*结束使用触摸屏：关闭设备、释放内存*/
void touch_free()
{
    /* 关闭设备、释放内存 */
    ts_close(ts);
    free(mt_ptr);
    free(touch_data);
}

/*触点数据处理函数*/
int touch_process(touch_opt_handle_t handle)
{
    /*变量定义*/
    static int i;                           //循环变量
    static int finger_num = 0;              //触点数量
    static uint8_t touch_statue = 0;        //触摸状态（长按/移动/放大/缩小）
    static uint8_t click_num = 0;           //点击次数（单击/双击）
    static int distance_old;                //上一次两指间距离（用于判断放大/缩小）
    static int distance_new;                //当前两指间距离（用于判断放大/缩小）
    static int x_old;                       //上一次触点x坐标（用于判断移动偏移）
    static int y_old;                       //上一次触点y坐标（用于判断移动偏移）
    static __time_t sec_press;              //第一个触点触摸时间戳的秒数（用于判断长按）
    static __suseconds_t usec_press;        //第一个触点触摸时间戳的微秒数（用于判断长按）
    static __time_t sec_click;              //触点离开时间戳的秒数（用于判断单击/双击）
    static __suseconds_t usec_click;        //触点离开时间戳的微妙数（用于判断单击/双击）
    static int ret;                         //读取触摸屏数据函数返回值（用于判断是否读到数据）
    static int pressure_data[5] = {0};      //保存所有触点压力大小（255按下/0松开）
    static uint8_t click_index_new[2] = {0};//用于保存最新触点下标（判断放大/缩小时的触点下标）
    static uint8_t click_index_old[2] = {0};//用于保存上次触点下标（判断放大/缩小时的触点下标）

    /*不加延时一直读取会导致段错误（实际应用不能加延时，正在解决）*/
    usleep(10000);
    /*指向外部处理函数*/
    touch_handle = handle;

    /*判断长按：当一个手指按下，并且触点状态为0时继续判断按下时长是否满足长按标准*/
    if((finger_num == 1) && (touch_statue == 0))
    {
        /*获取时间戳*/
        struct timeval tv;
        gettimeofday(&tv, NULL);

        /*按下时间大于500ms小于3s判断为长按*/
        if(((tv.tv_sec - sec_press) * 1000000 + tv.tv_usec - usec_press > 500000) && (tv.tv_sec - sec_press < 3))
        {
            /*长按事件*/
            touch_data->event |= TOUCH_LONGPRESS;
            /*调用外部事件处理函数*/
            touch_handle(touch_data);
            /*事件位清零*/
            touch_data->event ^= TOUCH_LONGPRESS;
            /*当前状态为长按*/
            touch_statue |= TOUCH_LONGPRESS;
        }
    }

    /*判断单击/双击：点击次数大于0，并且触点状态为0时继续判断*/
    if(click_num > 0 && touch_statue == 0)
    {
        /*获取时间戳*/
        struct timeval tv;
        gettimeofday(&tv, NULL);

        /*按下时间小于200ms时不断判断click_num数值*/
        if(((tv.tv_sec - sec_click) * 1000000 + tv.tv_usec - usec_click < 200000))
        {
            /*双击*/
            if(click_num > 1)
            {
                /*次数清零*/
                click_num = 0;
                /*双击事件*/
                touch_data->event |= TOUCH_DOUBLECLICK;
                /*调用外部事件处理函数*/
                touch_handle(touch_data);
                /*事件位清零*/
                touch_data->event ^= TOUCH_DOUBLECLICK;
            }
        }
        /*超过200ms && click_num==1为单击*/
        else
        {
            /*次数清零*/
            click_num = 0;
            /*单击事件*/
            touch_data->event |= TOUCH_CLICK;
            /*调用外部事件处理函数*/
            touch_handle(touch_data);
            /*事件位清零*/
            touch_data->event ^= TOUCH_CLICK;
        }
    }

    /*读取触摸屏数据*/
    ret = ts_read_mt(ts, &mt_ptr, max_slots, 1);
    /*如果读取到数据*/
    if (0 < ret) 
    {
        /*保存触点压力数据*/
        for(i = 0; i < max_slots; i++) 
        {
            if(mt_ptr[i].valid) 
            {
               pressure_data[i] = mt_ptr[i].pressure;
            }
        }

        /*计算触点数量*/
        finger_num = 0;
        for(i = 0; i < max_slots; i++) 
        {
            if(pressure_data[i] == 255) 
            {
               finger_num++;
            }
        }

        /*如果触点数量大于1：放大/缩小*/
        if(finger_num > 1)
        {
            /*用队列保存最新触点下标*/
            for(i = 0; i < max_slots; i++) 
            {
                if(mt_ptr[i].valid && (i != click_index_new[0]) && (i != click_index_new[1])) 
                {
                    click_index_new[0] = click_index_new[1];
                    click_index_new[1] = i;
                }
            }

            /*当前两触点下标与上次两触点下标相同：多指接触时用最新移动的两个触点作为判断*/
            if((click_index_new[0] == click_index_old[0] || click_index_new[0] == click_index_old[1]) && (click_index_new[1] == click_index_old[0] || click_index_new[1] == click_index_old[1]))
            {
                /*计算两点间距离*/
                distance_new = sqrt(pow(fabs(mt_ptr[click_index_new[0]].y-mt_ptr[click_index_new[1]].y),2)+
                                    pow(fabs(mt_ptr[click_index_new[0]].x-mt_ptr[click_index_new[1]].x),2));

                /*如果上次两指间距离不为0*/
                if(distance_old != 0)
                {
                    /*判断距离如何变化*/
                    if(distance_new > distance_old)
                    {
                        /*放大事件*/
                        touch_data->event |= TOUCH_MAGNIFY;
                        /*调用外部事件处理函数*/
                        touch_handle(touch_data);
                        /*事件位清零*/
                        touch_data->event ^= TOUCH_MAGNIFY;
                        /*当前状态为放大*/
                        touch_statue |= TOUCH_MAGNIFY;
                    }
                    else
                    {
                        /*缩小事件*/
                        touch_data->event |= TOUCH_SHRINK;
                        /*调用外部事件处理函数*/
                        touch_handle(touch_data);
                        /*事件位清零*/
                        touch_data->event ^= TOUCH_SHRINK;
                        /*当前状态为缩小*/
                        touch_statue |= TOUCH_SHRINK;
                    }

                    /*保存中心点坐标、两点间距离偏移量*/
                    touch_data->more_clickData.centre_x = fabs(mt_ptr[click_index_new[0]].x-mt_ptr[click_index_new[1]].x)/2;
                    touch_data->more_clickData.centre_y = fabs(mt_ptr[click_index_new[0]].y-mt_ptr[click_index_new[1]].y)/2;
                    touch_data->more_clickData.offset_distance = fabs(distance_new - distance_old);
                }

                /*保存当前两指间距离*/
                distance_old = distance_new;
            }
            /*当前两触点下标与上次两触点下标不同*/
            else
            {
                click_index_old[0] = click_index_new[0];
                click_index_old[1] = click_index_new[1];
                distance_old = 0;
            }  
        }

        /*如果触点数量等于1：移动/长按*/
        else if(finger_num == 1)
        {   
            /*用队列保存最新两个触点下标*/
            for(i = 0; i < max_slots; i++) 
            {
                if(mt_ptr[i].valid) 
                {
                    /*触点状态为触点按下：首个触点*/
                    if(mt_ptr[i].pen_down == 1)
                    {
                        touch_data->one_clickData.x = mt_ptr[i].x;
                        touch_data->one_clickData.y = mt_ptr[i].y;
                        sec_press = mt_ptr[i].tv.tv_sec; 
                        usec_press = mt_ptr[i].tv.tv_usec; 
                        x_old = mt_ptr[i].x;
                        y_old = mt_ptr[i].y;
                    }
                    /*触点状态为触点离开：多指变单指*/
                    else if(mt_ptr[i].pressure == 0)
                    {
                        /*坐标清零：单指->多指->单指*/
                        x_old = 0;
                    }
                    /*触点状态为移动*/
                    else if(mt_ptr[i].pen_down == -1)
                    {
                        /*如果上次触点坐标不为0*/
                        if(x_old != 0)
                        {
                            /*保存坐标和偏移量*/
                            touch_data->one_clickData.x = mt_ptr[i].x;
                            touch_data->one_clickData.y = mt_ptr[i].y;
                            touch_data->one_clickData.offset_x = mt_ptr[i].x - x_old;
                            touch_data->one_clickData.offset_y = mt_ptr[i].y - y_old;

                            /*移动事件*/
                            touch_data->event |= TOUCH_MOVE;
                            /*调用外部事件处理函数*/
                            touch_handle(touch_data);
                            /*事件位清零*/
                            touch_data->event ^= TOUCH_MOVE;
                        }

                        /*当前状态为移动*/
                        touch_statue |= TOUCH_MOVE;
                        x_old = mt_ptr[i].x;
                        y_old = mt_ptr[i].y;
                    }
                }
            }
        }

        /*如果触点数量为0：手指松开*/
        else if(finger_num == 0)
        {
            /*如果触摸状态不为放大、缩小、长按、移动 这些状态下touch_statue会被赋值*/
            if(touch_statue == 0)
            {
                click_num ++;
                sec_click = mt_ptr[0].tv.tv_sec;
                usec_click = mt_ptr[0].tv.tv_usec;
            }
            /*如果触摸状态为长按*/
            else if(touch_statue & TOUCH_LONGPRESS)
            {
                /*长按松开事件*/
                touch_data->event |= TOUCH_LOOSE;
                /*调用外部事件处理函数*/
                touch_handle(touch_data);
                /*事件位清零*/
                touch_data->event ^= TOUCH_LOOSE;
            }

            /*触点缓存数据清零*/
            touch_statue = 0;
            distance_old = 0;
            sec_press = 0;
            usec_press = 0;
            x_old = 0;
            y_old = 0;
        }
    }
}
