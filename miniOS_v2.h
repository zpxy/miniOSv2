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
      ����miniOSС�Ͷ������������ơ���

���ܣ�*����Cortex M3,stm32f103rct6
��л����лrt_thread����Ŀ
*******copyright v2.0 2022.07.31 ********/

/****************************************
���ܣ�* ����Cortex M0,LPC1114,Board V3.2
��л����лucOS II��Ŀ, icegoly��blog
*******copyright v2.1 2022.08.04 ********/

/****************************************
���ܣ�* ����Cortex M4,stm32f401ceu6
            * .s�ļ���M3���Թ��ã��벻Ҫ����FPU
            * ��ֲoled,����ʹ�����IIC����
��л����лucOS II��Ŀ, icegoly��blog
*******copyright v2.1 2022.08.09 ********/

/****************************************
���ܣ�*�����ź����������ǽṹ������ķ�ʽ
            *���ź�����ʼ����take\give��������
            *���Խ��е��ź����������̼߳��ƹ��ʵ��
*******copyright v2.2 2022.08.11 ********/

#define lpc1114 0
#define stm32f1 0
#define stm32f4 0
#define gd32f103 1

#include "stdint.h"
#include "stdlib.h"

#if stm32f4
/*todo :�뽫���漸�д��뻻��оƬ��Ӧ��ͷ�ļ�*/
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
    os_Task_idle, //�����߳���Զ��os_Task_sum����֮ǰ�������߳�֮��
    os_Task_sum
} miniOS_ID_t; //�߳�ID���壬IDԽС���ȼ�Խ��

typedef enum
{
    os_suspend = 0, //����
    os_sleep,       //˯�߻���˵����
    os_run,         //����
    os_wait_sem     //����ȴ��ź���
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

//�ź�������
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
/**************************************�ڶ��汾******************************************************/
/****************************************************************************************************/

#define idleStackSize 64             /*��������߳�ջ*/
#define miniOS_CPU_ExceptStkSize 512 /*����cpu��ִ��ջ*/

typedef unsigned int miniOS_STK;        /*����int����Ϊջ���ͣ�32λԭ����������*/
typedef void (*miniOS_TASK)(void *arg); /*�����߳�����*/

extern void miniOS_Hw_contextSw(void);                 /*�������е������л�����*/
extern void miniOS_Start_Asm(void);                    /*�������е�����������һ���߳�*/
extern int32_t miniOS_hw_interrupt_disable(void);      /*�����������ڡ��رա�ȫ���жϵĺ���*/
extern void miniOS_hw_interrupt_enable(int32_t level); /*�����������ڡ�������ȫ���жϵĺ���*/

/*ʵʱ����ṹ��*/
/*
  * !!! ��Ҫ��Ϣ Attion
    * ������Ϊ�ṹ���һ��������ջָ��
    * ����������������ˡ��߳�ջָ�� === �߳̽ṹ��ָ�롿
    * �����˻�������ں˴������д

  * miniOS_STK  *stackptr;            # �߳�ջ��ַ��stk����β����һ�ĵ�ַ��&xStack[XStackSize -1]
    * void (*task)(void *arg);          # �߳���ں���
    * void *taskentry;                  # �߳���ں���
    * void *sp;                         # ջ����ַ xStack
    * int  thisID;                      # �߳�ID��

    * uint16_t stackSize;               # �߳�ջ��С

    * miniOS_TaskStatus_t runflag;      # �߳�״̬��os_suspend,os_sleep,os_run
    * miniOS_TaskStatus_t runState;     # �߳�״̬�Ĵ���
    * uint16_t runPerid ;               # �߳�ʱ��Ƭ
    * uint16_t runTimer ;               # �߳�ʱ��Ƭ������
    * uint32_t nextTime ;               # �̹߳���ʱ��
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
/*********************************************miniOS RT���нӿ�*****************************************************/
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
 * x �ṹ��
 * y ������ݵı�����ַ��&a��
 */
#define queueInit(x) queue_init((unsigned char **)&x.head, (unsigned char **)&x.tail, (unsigned char *)x.buf)
#define queueSet(x, y) queue_in((unsigned char **)&x.head, (unsigned char **)&x.tail, (unsigned char *)x.buf, sizeof(x.buf), y, 1)
#define queueSets(x, y, z) queue_in((unsigned char **)&x.head, (unsigned char **)&x.tail, (unsigned char *)x.buf, sizeof(x.buf), y, z)
#define queueOut(x, y) queue_out((unsigned char **)&x.head, (unsigned char **)&x.tail, (unsigned char *)x.buf, sizeof(x.buf), y, 1)
#define queueLen(x) queue_len((unsigned char **)&x.head, (unsigned char **)&x.tail, (unsigned char *)x.buf, (sizeof(x.buf)))
#define queueStrstr(x, y) queue_strstr((unsigned char **)&x.head, (unsigned char **)&x.tail, (unsigned char *)x.buf, (sizeof(x.buf)), y);

/*********************************************miniOS RT���s�ļ�ʹ�ñ���*****************************************************/
extern miniOS_RtTask_p miniOS_curTask;
extern miniOS_RtTask_p miniOS_nextTask;
extern miniOS_STK *miniOS_CPU_ExceptStkBase;
/*********************************************miniOS RT����ӿ�*****************************************************/

/*miniOS RT �����������*/
extern void Task_End(void);

/*miniOS RT �̴߳���(ע��)����*/
/*
    * miniOS_ID_t wantRegTaskId,  # �̱߳��е�ID��
    * miniOS_TASK task,           # �߳���ڵ�ַ void (*taskEntry)(void *arg)
  * taskEntry sample:
            void taskEntry(void *arg)
            {
                    NEW  Variables;
                    SOME init;
          WHILE(1){
              TO DO YOUR THINGS;
                    }
            }

  * miniOS_STK_Type stkTtpe,    # �߳�ջ����
  * miniOS_STK *stackBaseptr    # �߳�ջ����ַ
    * miniOS_STK *stackptr ,      # �߳�ջ�׵�ַ�����õݼ�ջ�ռ�ʹ��

    * miniOS_TaskStatus_t runflag,# �̵߳��ȵ�һ�α�־
    * uint16_t runPerid,          # �߳�ӵ��ʱ��Ƭ
    * uint16_t runTime            # �߳��ӳ�����ʱ�䣬���os_suspend��־
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

/*miniOS RTϵͳ��ʼ��*/
extern void miniOS_Rt_Init(void);

/*miniOS RT������һ���̵߳���ָ��*/
extern void setNext(miniOS_ID_t id);

/*miniOS RT �߳�ɾ������*/
extern void miniOS_RtTask_Delete(miniOS_ID_t taskID);

/*miniOS RT �߳��л�����*/
extern void miniOS_RtTask_SW(void);

/*miniOS RT ϵͳ������������*/
extern void miniOS_Rt_Start(void);

/*miniOS RT ϵͳ��ʱ����*/
extern void miniOS_Rt_Delay(uint16_t delayCount);

/*������ʱ����ms*/
extern void miniOS_delay_ms(uint16_t nTick);

/*������ʱ����us*/
extern void miniOS_delay_us(uint16_t nus);

/*��ѡ��ʱ����us*/
extern void miniOS_opt_delay(uint16_t delaytime, int rt);
/*miniOS RT �δ�ʱ��������*/
/*todo : ���˺�������void SysTick_Handler(void)
 *sample:
     void SysTick_Handler(void)
            {
                miniOS_RtTask_sysTickHandle();
            }
*/
extern void miniOS_RtTask_sysTickHandle(void);

/*���û��Լ�ʵ�ֵδ�ʱ����ʼ��*/
extern void sysTick_Init(void);

/*���û��Լ�ʵ��Ӳ����ʼ��*/
extern void miniOS_HW_Init(void);

extern void miniOS_SwHook(void);

extern void miniOS_sem_init(void);

extern void miniOS_sem_take(uint8_t semID);

extern void miniOS_sem_give(uint8_t semID);

#endif
