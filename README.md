# STM32MP157_tslib
# 多点触摸屏

实现目标：识别用户在触摸屏上的操作，如:单击、双击、长按、移动、放大、缩小

## tslib库简介

触摸屏是现代电子设备中常见的输入方式之一，tslib库是一个软件工具，可以帮助开发者在Linux系统上方便地处理触摸屏的输入数据。

tslib库提供了一组API函数，可以让开发者轻松地获取触摸屏输入的坐标、压力等信息，并进行相应的处理。使用tslib库，开发者不需要直接与底层硬件交互，而是通过标准的接口进行操作，从而提高了代码的可移植性和可维护性。

除了常见的输入处理功能外，tslib库还提供了一些额外的功能，如校准、手势识别等，可以帮助开发者更好地适配不同类型的触摸屏设备。

总之，tslib库是一个非常实用的工具，可以方便地处理触摸屏输入数据，是开发Linux平台下触摸屏应用程序的重要组成部分。

## tslib库移植

正点STM32MP157开发板出厂已移植(非广告!)，需要请参考其他教程。

tslib官方网站：<http://www.tslib.org/> 

## tslib库函数使用

### 打开触摸屏设备

函数：

```c
函数原型：
struct tsdev *ts_open(const char *dev_name, int nonblock)

参数：
dev_name -- 触摸屏的设备节点
nonblock -- 是否以阻塞的方式打开，1为非阻塞，0为阻塞

作用：
打开触摸屏设备，返回触摸屏设备句柄
```

### 配置触摸屏设备

函数：

```c
函数原型：
int ts_config(struct tsdev *ts)

参数：
ts -- 触摸屏设备句柄

作用：
调用该函数对触摸屏进行配置
```

### 打开并配置触摸屏设备

函数（本项目使用）：

```c
函数原型：
struct tsdev *ts_setup(const char *dev_name, int nonblock)

参数：
dev_name -- 触摸屏的设备节点，该参数设置为NULL时，ts_setup()函数内部会读取
TSLIB_TSDEVICE环境变量，获取该环境变量的内容以得知触摸屏的设备节点。
nonblock -- 是否以阻塞的方式打开，1为非阻塞，0为阻塞

作用：
打开并配置触摸屏设备
```

### 读取触摸屏设备

函数：

```c
函数原型：
int ts_read(struct tsdev *ts, struct ts_sample *samp, int nr)

参数：
ts -- 触摸屏设备句柄
samp -- 保存触点信息
nr -- 对一个触摸点的采样数，设置为1即可

作用：
读取触摸屏设备数据
```

struct ts_sample 结构体：

```c
struct ts_sample {
    int x; //X 坐标
    int y; //Y 坐标
    unsigned int pressure; //按压力大小
    struct timeval tv; //时间
};
```

 函数（本项目使用）：

```c
函数原型：
int ts_read_mt(struct tsdev *ts, struct ts_sample_mt **samp, int max_slots, int nr)

参数：
ts -- 触摸屏设备句柄
samp -- 保存触点信息
max_slots -- 触摸屏支持最大触点数
nr -- 对一个触摸点的采样数，设置为1即可

作用：
读取触摸屏设备数据，一次读取多个触点数据
```

struct ts_sample_mt结构体（主要用到的）：

```c
struct ts_sample_mt {
    int x; //触点X坐标
    int y; //触点Y坐标
    unsigned int pressure; //按压力大小 255/0
    int slot; //触摸点slot
    int tracking_id; //ID
    ......
    struct timeval tv; //时间戳
    short pen_down; //BTN_TOUCH 的状态
    short valid; //触摸点数据是否发生更新
};
```

## 多点触摸屏程序编写

本项目编写程序要实现触摸屏数据的**非阻塞读取**和判断，并且**外部调用**该函数时能判断用户执行了怎样的操作。
当用户操作为（**单击/双击/长按**）时，外部能获取**触点的坐标数据**。
当用户操作为（**移动**）时，外部能获取**触点坐标数据以及坐标偏移量**。
当用户操作位（**放大/缩小**）时，外部能获取**两指间中心点坐标以及间距的偏移量**。

代码大致的流程是：
**==以非阻塞方式打开触摸屏设备->读取触摸屏设备数据->判断触点数量->根据触点数量不同执行不同操作==**

外部调用的大致流程是：
**==初始化触摸屏设备->循环执行触点数据处理函数并传入外部处理函数->在用户操作后在外部处理函数对用户的行为进行判断进而进行相关操作==**

### 触点数据结构体定义

结构体内数据包括触摸事件、单个触点数据结构体以及多触点数据结构体。

```c
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
```

### 事件定义

使用不同位上的1代表不同事件。

```c
#define TOUCH_CLICK         0X0001  /*单击事件*/
#define TOUCH_DOUBLECLICK   0X0002  /*双击事件*/
#define TOUCH_LONGPRESS     0X0004  /*长按事件*/
#define TOUCH_LOOSE         0X0008  /*长按松开事件*/
#define TOUCH_MOVE          0X0010  /*移动事件*/
#define TOUCH_MAGNIFY       0X0020  /*放大事件*/
#define TOUCH_SHRINK        0X0040  /*缩小事件*
```

### 计算触点数量

在程序中创建一个压力数组保存各个触点的压力数据，当触点数据更新时，将该触点压力数据保存至数组对应位置。再通过判断该数组的值来确定当前的触点数量。

代码：

```c
/*保存所有触点压力大小（255按下/0松开）*/
static int pressure_data[5] = {0};     

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
}
```

### 判断单击、双击

思路：

大致的判断依据是在用户点击并离开屏幕后的200ms内点击的次数（包括这一次，即点击再离开后200ms内无再次点击为单击操作，若再次点击则为双击操作）。

大致实现步骤：

在用户点击离开时若click_num为0则记录下当前时间戳（触点结构体中有），使click_num++。在外部（这个外部是指触点数据处理函数内，读取触点数据判断外）循环获取当前时间戳，若间隔超过200ms时click_num依然为1则为单击，200ms内click_num>1则为双击。

代码：

```c
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
```

### 判断长按、移动

思路：

长按的判断：当用户点击且未离开屏幕时长达500ms即为长按。
移动的判断：触点坐标发生改变则为移动。

大致实现步骤：

在用户按下时记录当前时间戳，在外部（这个外部是指触点数据处理函数内，读取触点数据判断外）循环读取当前时间戳，判断时间间隔是否大于500ms，是则为长按。
用户按下后记录下当前触点坐标和触点数量，若触点数量为1、触点BTN_TOUCH状态为-1（移动、除第一触点外新增触点）、且触点坐标发生改变则为移动。

判断长按代码：

```c
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
```

判断移动代码：

```c
/*读取触摸屏数据*/
ret = ts_read_mt(ts, &mt_ptr, max_slots, 1);
/*如果读取到数据*/
if (0 < ret) 
{
    /*保存触点压力数据*/
   ...
    /*计算触点数量*/
   ...
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
}
```

### 判断放大、缩小

思路：

用户执行放大、缩小操作时，根据俩触点坐标计算两点间距离，保存该值，在下一次触点坐标变化时，根据判断该值是增大还是减小决定用户操作时放大或缩小。

大致实现步骤：

用户执行放大或缩小操作时，可能是两指也可能是多指，这时候需要用一个队列来存储最新变化的两个触点的数据。若上次间距不为0，则根据这两个触点的数据来进行放大或缩小的判断，并得到两次间距的偏移量和两点间中心坐标；若为0则保存当前两点间间距。若用来控制放大或缩小的两指发生变化，则重置上次间距为0。

判断放大、缩小代码：

```c
/*读取触摸屏数据*/
ret = ts_read_mt(ts, &mt_ptr, max_slots, 1);
/*如果读取到数据*/
if (0 < ret) 
{
    /*保存触点压力数据*/
    ...
    /*计算触点数量*/
    ...
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
            distance_new = sqrt(pow(fabs(mt_ptr[click_index_new[0]].y-mt_ptr[click_index_new[1]].y),2)+                                    pow(fabs(mt_ptr[click_index_new[0]].x-mt_ptr[click_index_new[1]].x),2));

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
}
```

### 外部调用

在外部执行触点数据处理函数时需要传递一个函数地址（函数名）作为参数，以便在处理函数内部回调传入的函数。

初始化以及释放函数：

```c
/*触摸屏初始化*/
void touch_init(void);
/*触摸屏释放*/
void touch_free(void);
```

触点数据处理函数：

```c
/*外部调用接口函数*/
int touch_process(touch_opt_handle_t handle);
```

函数指针结构定义：

```c
/*指向外部处理函数指针结构定义*/
typedef void (*touch_opt_handle_t)(touch_struct *touch_data);
结构名为touch_opt_handle_t，参数为触点数据结构体指针。
```

外部调用示例：

```c
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
```

## 代码流程图（草图）

![tslib](https://user-images.githubusercontent.com/97655921/227143417-f96545b2-0ed2-46f5-a643-0f959f4ef6ce.png)


## 实现效果

单击、双击：

![1678284965645](https://user-images.githubusercontent.com/97655921/227143454-0ea97099-389c-4404-9131-f23878c4fb10.png)

长按、长按松开：

![1678285000026](https://user-images.githubusercontent.com/97655921/227143492-ab1021a1-b2ad-4113-bd7f-073c20fcbe96.png)

移动：

![1678285024520](https://user-images.githubusercontent.com/97655921/227143549-ccae6aef-924f-4f2f-8da0-d514f57457b8.png)

放大、缩小：

![1678285070206](https://user-images.githubusercontent.com/97655921/227143565-a8c6d056-3823-4fc0-b006-0c844b02cf84.png)
