#ifndef MINIOS_V2_H
#define MINIOS_V2_H

/***********HNU HaiKou Mei Lan*************
####### ######   #####   #####   ##
    ##  ##   ## ##   ## ##   ## ###
   ##   ##   ## ##  ### ##  ###  ##
  ##    ######  ## # ## ## # ##  ##
 ##     ##      ###  ## ###  ##  ##
##      ##      ##   ## ##   ##  ##
####### ##       #####   #####  ####
------------------------------------------
      ——miniOS小型多任务程序框架设计——

功能：*适配Cortex M3,stm32f103rct6
致谢：感谢rt_thread的项目
*******copyright v2.0 2022.07.31 ********/

/****************************************
功能：* 适配Cortex M0,LPC1114,Board V3.2
致谢：感谢ucOS II项目, icegoly的blog
*******copyright v2.1 2022.08.04 ********/

/****************************************
功能：* 适配Cortex M4,stm32f401ceu6
            * .s文件和M3可以共用，请不要开启FPU
            * 移植oled,并且使用软件IIC驱动
致谢：感谢ucOS II项目, icegoly的blog
*******copyright v2.1 2022.08.09 ********/

/****************************************
功能：*增加信号量，现在是结构体数组的方式
            *有信号量初始化和take\give两个函数
            *可以进行单信号量在两个线程间的乒乓实验
*******copyright v2.2 2022.08.11 ********/

#define lpc1114 0
#define stm32f1 0
#define stm32f4 0
#define gd32f103 1

#include "stdint.h"
#include "stdlib.h"

#if stm32f4
/*todo :请将下面几行代码换成芯片对应的头文件*/
#include "stm32f4xx.h"
#include "core_cm4.h"
#endif

#if gd32f103
#include "gd32f10x.h"
#include "core_cm3.h"
#endif

#define __WEAKDEF __weak

typedef enum
{
    os_Task_sys = 0,
    os_Task_key,
    os_Task_led,
    os_Task_gui,
    os_Task_wifi,
    os_Task_uart,
    os_Task_test,
    os_Task_idle, //空闲线程永远在os_Task_sum计数之前，所有线程之后
    os_Task_sum
} miniOS_ID_t; //线程ID定义，ID越小优先级越高

typedef enum
{
    os_suspend = 0, //挂起
    os_sleep,       //睡眠或者说就绪
    os_run,         //运行
    os_wait_sem     //挂起等待信号量
} miniOS_TaskStatus_t;

typedef enum
{
    staticSTK = 0,
    dynamicSTK
} miniOS_STK_Type;

typedef enum
{
    semIdle = 0,
    semTaked
} miniOS_semStatus_Type;

//信号量定义
typedef enum
{
    os_sem_1 = 1,
    os_sem_2 = 2,
    os_sem_3 = 3,
    os_sem_4 = 4,
    os_sem_5 = 5,
    os_sem_6 = 6,
    os_sem_7 = 7,
    os_sem_8 = 8,
    os_sem_sum
} miniOS_semId;

/****************************************************************************************************/
/**************************************第二版本******************************************************/
/****************************************************************************************************/

#define idleStackSize 64             /*定义空闲线程栈*/
#define miniOS_CPU_ExceptStkSize 512 /*定义cpu主执行栈*/

typedef unsigned int miniOS_STK;        /*定义int类型为栈类型，32位原生数据类型*/
typedef void (*miniOS_TASK)(void *arg); /*任务线程类型*/

extern void miniOS_Hw_contextSw(void);                 /*汇编代码中的任务切换函数*/
extern void miniOS_Start_Asm(void);                    /*汇编代码中的用于启动第一个线程*/
extern int32_t miniOS_hw_interrupt_disable(void);      /*汇编代码中用于【关闭】全局中断的函数*/
extern void miniOS_hw_interrupt_enable(int32_t level); /*汇编代码中用于【开启】全局中断的函数*/

/*实时任务结构体*/
/*
  * !!! 重要信息 Attion
    * 正是因为结构体第一个变量是栈指针
    * 因此巧妙的现象出现了【线程栈指针 === 线程结构体指针】
    * 方便了汇编代码和内核代码的书写

  * miniOS_STK  *stackptr;            # 线程栈地址，stk数组尾部减一的地址，&xStack[XStackSize -1]
    * void (*task)(void *arg);          # 线程入口函数
    * void *taskentry;                  # 线程入口函数
    * void *sp;                         # 栈基地址 xStack
    * int  thisID;                      # 线程ID号

    * uint16_t stackSize;               # 线程栈大小

    * miniOS_TaskStatus_t runflag;      # 线程状态，os_suspend,os_sleep,os_run
    * miniOS_TaskStatus_t runState;     # 线程状态寄存器
    * uint16_t runPerid ;               # 线程时间片
    * uint16_t runTimer ;               # 线程时间片计数器
    * uint32_t nextTime ;               # 线程挂起时间
*/
// typedef struct miniOS_RtTask_s{
//	miniOS_STK  *stackptr;
//
//	void (*task)(void *arg);
//	void *taskentry;
//	void *sp;
//	int  thisID;
//	uint16_t stackSize;
//
//	miniOS_TaskStatus_t runflag;
//	miniOS_TaskStatus_t runState;
//	uint16_t runPerid ;
//	uint16_t runTimer ;
//	uint32_t nextTime ;
//
// }miniOS_RtTask_t,*miniOS_RtTask_p;

struct miniOS_RtTask_s;
typedef struct miniOS_RtTask_s miniOS_RtTask_t, *miniOS_RtTask_p;
struct miniOS_sem_s;
typedef struct miniOS_sem_s miniOS_sem_t, *miniOS_sem_p;
/*********************************************miniOS RT队列接口*****************************************************/
// struct queue16;
// struct queue32;
// struct queue64;
// struct queue128;
// struct queue256;

typedef struct queue16
{
    unsigned char buf[16];
    unsigned char *head;
    unsigned char *tail;
} queue16_t;

typedef struct queue32
{
    unsigned char buf[32];
    unsigned char *head;
    unsigned char *tail;
} queue32_t;

typedef struct queue64
{
    unsigned char buf[64];
    unsigned char *head;
    unsigned char *tail;
} queue64_t;

typedef struct queue128
{
    unsigned char buf[128];
    unsigned char *head;
    unsigned char *tail;
} queue128_t;

typedef struct queue255
{
    unsigned char buf[255];
    unsigned char *head;
    unsigned char *tail;
} queue256_t;

// typedef struct queue16 queue16_t;
// typedef struct queue32 queue32_t;
// typedef struct queue64 queue64_t;
// typedef struct queue128 queue128_t;
// typedef struct queue256 queue256_t;

extern void queue_in(unsigned char **head, unsigned char **tail, unsigned char *pbuff, unsigned char pbufflen, unsigned char *data, unsigned char datalen);
extern int queue_out(unsigned char **head, unsigned char **tail, unsigned char *pbuff, unsigned char pbufflen, unsigned char *data, unsigned char datalen);
extern void queue_init(unsigned char **head, unsigned char **tail, unsigned char *pbuff);
extern uint16_t queue_len(unsigned char **head, unsigned char **tail, unsigned char *pbuff, uint16_t pbufflen);
extern int queue_strstr(unsigned char **head, unsigned char **tail, unsigned char *pbuff, uint16_t pbufflen, const char *substr);
/*
 * x 结构体
 * y 存放数据的变量地址（&a）
 */
#define queueInit(x) queue_init((unsigned char **)&x.head, (unsigned char **)&x.tail, (unsigned char *)x.buf)
#define queueSet(x, y) queue_in((unsigned char **)&x.head, (unsigned char **)&x.tail, (unsigned char *)x.buf, sizeof(x.buf), y, 1)
#define queueSets(x, y, z) queue_in((unsigned char **)&x.head, (unsigned char **)&x.tail, (unsigned char *)x.buf, sizeof(x.buf), y, z)
#define queueOut(x, y) queue_out((unsigned char **)&x.head, (unsigned char **)&x.tail, (unsigned char *)x.buf, sizeof(x.buf), y, 1)
#define queueLen(x) queue_len((unsigned char **)&x.head, (unsigned char **)&x.tail, (unsigned char *)x.buf, (sizeof(x.buf)))
#define queueStrstr(x, y) queue_strstr((unsigned char **)&x.head, (unsigned char **)&x.tail, (unsigned char *)x.buf, (sizeof(x.buf)), y);

/*********************************************miniOS RT汇编s文件使用变量*****************************************************/
extern miniOS_RtTask_p miniOS_curTask;
extern miniOS_RtTask_p miniOS_nextTask;
extern miniOS_STK *miniOS_CPU_ExceptStkBase;
/*********************************************miniOS RT任务接口*****************************************************/

/*miniOS RT 任务结束函数*/
extern void Task_End(void);

/*miniOS RT 线程创建(注册)函数*/
/*
    * miniOS_ID_t wantRegTaskId,  # 线程表中的ID号
    * miniOS_TASK task,           # 线程入口地址 void (*taskEntry)(void *arg)
  * taskEntry sample:
            void taskEntry(void *arg)
            {
                    NEW  Variables;
                    SOME init;
          WHILE(1){
              TO DO YOUR THINGS;
                    }
            }

  * miniOS_STK_Type stkTtpe,    # 线程栈类型
  * miniOS_STK *stackBaseptr    # 线程栈基地址
    * miniOS_STK *stackptr ,      # 线程栈底地址，采用递减栈空间使用

    * miniOS_TaskStatus_t runflag,# 线程调度第一次标志
    * uint16_t runPerid,          # 线程拥有时间片
    * uint16_t runTime            # 线程延迟启动时间，配合os_suspend标志
*/
extern void miniOS_task_create(miniOS_ID_t wantRegTaskId,
                               miniOS_TASK task,
                               void *arg,

                               miniOS_STK_Type stkTtpe,
                               miniOS_STK *stackBaseptr,
                               miniOS_STK *stackptr,
                               // uint16_t   stacksize,

                               miniOS_TaskStatus_t runflag,
                               uint16_t runPerid,
                               uint16_t runTime);

/*miniOS RT系统初始化*/
extern void miniOS_Rt_Init(void);

/*miniOS RT设置下一次线程调度指针*/
extern void setNext(miniOS_ID_t id);

/*miniOS RT 线程删除函数*/
extern void miniOS_RtTask_Delete(miniOS_ID_t taskID);

/*miniOS RT 线程切换函数*/
extern void miniOS_RtTask_SW(void);

/*miniOS RT 系统调度启动函数*/
extern void miniOS_Rt_Start(void);

/*miniOS RT 系统延时函数*/
extern void miniOS_Rt_Delay(uint16_t delayCount);

/*阻塞延时函数ms*/
extern void miniOS_delay_ms(uint16_t nTick);

/*阻塞延时函数us*/
extern void miniOS_delay_us(uint16_t nus);

/*可选延时函数us*/
extern void miniOS_opt_delay(uint16_t delaytime, int rt);
/*miniOS RT 滴答定时器服务函数*/
/*todo : 将此函数放入void SysTick_Handler(void)
 *sample:
     void SysTick_Handler(void)
            {
                miniOS_RtTask_sysTickHandle();
            }
*/
extern void miniOS_RtTask_sysTickHandle(void);

/*请用户自己实现滴答定时器初始化*/
extern void sysTick_Init(void);

/*请用户自己实现硬件初始化*/
extern void miniOS_HW_Init(void);

extern void miniOS_SwHook(void);

extern void miniOS_sem_init(void);

extern void miniOS_sem_take(uint8_t semID);

extern void miniOS_sem_give(uint8_t semID);

#endif
