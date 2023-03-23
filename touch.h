#ifndef _TOUCH_H_
#define _TOUCH_H_

#define TOUCH_CLICK         0X0001  /*单击事件*/
#define TOUCH_DOUBLECLICK   0X0002  /*双击事件*/
#define TOUCH_PRESS         0X0004  /*长按事件*/
#define TOUCH_MOVE          0X0008  /*移动事件*/
#define TOUCH_MAGNIFY       0X0010  /*放大事件*/
#define TOUCH_SHRINK        0X0020  /*缩小事件*/

typedef struct one_clickStruct
{
    int x;          /*当前触点x坐标*/
    int y;          /*当前触点y坐标*/
    int offset_x;   /*x坐标偏移量*/
    int offset_y;   /*y坐标偏移量*/
}one_clickStruct;

typedef struct more_clickStruct
{
    int centre_x;       /*中心点x坐标*/
    int centre_y;       /*中心点y坐标*/
    int offset_distance;/*两点间距离偏移量*/
}more_clickStruct;

typedef struct touch_struct
{
    uint16_t type;                  /*操作类型*/
    uint16_t click_num;             /*一定时间内点击次数*/
    one_clickStruct one_clickData;  /*单个触点数据*/
    more_clickStruct more_clickData;/*多触点数据*/
}touch_struct;

typedef void (*touch_opt_handle_t)(touch_struct *touch_data);/*指向外部处理函数指针*/
int touch_process(touch_opt_handle_t handle);/*外部调用接口函数*/

#endif // !_TOUCH_H_

