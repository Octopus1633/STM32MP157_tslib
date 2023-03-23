/***************************************************************************
文件名 : touch_tslib.h
作者 : Octopus
博客 : https://blog.csdn.net/Octopus1633?
描述 : 识别用户在触摸屏上的操作，如:单击、双击、长按、移动、放大、缩小
交叉编译示例 : $ {CC} touch_tslib.c main_tslib.c -o touch_tslib -lts
程序运行示例 : $ ./voice touch_tslib
***************************************************************************/

#ifndef _TOUCH_TSLIB_H_
#define _TOUCH_TSLIB_H_

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
    uint16_t event;                 /*触摸事件*/
    one_clickStruct one_clickData;  /*单个触点数据*/
    more_clickStruct more_clickData;/*多触点数据*/
}touch_struct;

#define TOUCH_CLICK         0X0001  /*单击事件*/
#define TOUCH_DOUBLECLICK   0X0002  /*双击事件*/
#define TOUCH_LONGPRESS     0X0004  /*长按事件*/
#define TOUCH_LOOSE         0X0008  /*长按松开事件*/
#define TOUCH_MOVE          0X0010  /*移动事件*/
#define TOUCH_MAGNIFY       0X0020  /*放大事件*/
#define TOUCH_SHRINK        0X0040  /*缩小事件*/

void touch_init(void);/*触摸屏初始化*/
void touch_free(void);/*触摸屏释放*/

typedef void (*touch_opt_handle_t)(touch_struct *touch_data);/*指向外部处理函数指针结构定义*/
int touch_process(touch_opt_handle_t handle);/*外部调用接口函数*/

#endif // !_TOUCH_TSLIB_H_
