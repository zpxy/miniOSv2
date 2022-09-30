#include "miniOS_v2.h"

/***********HNU HaiKou Mei Lan*************
####### ######   #####   #####   ##
    ##  ##   ## ##   ## ##   ## ###
   ##   ##   ## ##  ### ##  ###  ##
  ##    ######  ## # ## ## # ##  ##
 ##     ##      ###  ## ###  ##  ##
##      ##      ##   ## ##   ##  ##
####### ##       #####   #####  ####
项目：——miniOS小型多任务系统设计——
功能：适配Cortex M3
致谢：感谢rt_thread的项目
*****************************************
@ 海南大学 信息与通信工程学院
   嵌入式智能系统实验室
*******copyright v2.0 2022.07.31 ********/

struct miniOS_RtTask_s
{
    miniOS_STK *stackptr;

    void (*task)(void *arg);
    void *taskentry;
    void *sp;
    int thisID;
    uint16_t stackSize;

    miniOS_TaskStatus_t runflag;
    miniOS_TaskStatus_t runState;
    uint16_t runPerid;
    uint16_t runTimer;
    uint32_t nextTime;
    uint8_t waitSemID;
};

// struct miniOS_sem_s{
// 	uint8_t sem[os_Task_sum];
// 	uint8_t head;
// 	uint8_t tail;
// 	unsigned char index0:1;
// 	unsigned char index1:1;
// 	unsigned char index2:1;
// 	unsigned char index3:1;
// 	unsigned char index4:1;
// 	unsigned char index5:1;
// 	unsigned char index6:1;
// 	unsigned char index7:1;
// };

/*信号量结构体*/
/*
 * nowBind 现在使用资源的taskID
 * isTaked 资源是否被使用的标志
 */
struct miniOS_sem_s
{
    uint8_t nowBind;
    uint8_t isTaked;
};

volatile static uint32_t miniOS_TimeTick = 0;                                /*心跳计数*/
volatile static uint32_t miniOS_delay = 0;                                   /*傻等计数*/
volatile static miniOS_ID_t miniOsCurTaskID = os_Task_sum;                   /*当前运行ID*/
volatile static miniOS_ID_t miniOsNextTaskID, miniOsTaskReady = os_Task_sum; /*下一个要运行的任务*/

/***********************/
/*       第二版本      */
/***********************/

/*
 * miniOS_curTask;   当前任务指针
 * miniOS_nextTask;  下一任务指针
 * miniOS_CurTaskID; 当前任务ID
 * miniOS_NextTaskID;下一任务指针
 */
miniOS_RtTask_p miniOS_curTask;
miniOS_ID_t miniOS_CurTaskID;

miniOS_RtTask_p miniOS_nextTask;
miniOS_ID_t miniOS_NextTaskID;

/*任务表：以数组方式实现*/
miniOS_RtTask_t miniOS_RtTask_Table[os_Task_sum] = {0};

static miniOS_sem_t miniOS_sem_table[os_Task_sum] = {0};

/*idle线程栈*/
miniOS_STK idleStack[idleStackSize];

/*cpu栈*/
miniOS_STK miniOS_CPU_ExceptStk[miniOS_CPU_ExceptStkSize];

/*cpu栈基地址*/
miniOS_STK *miniOS_CPU_ExceptStkBase;

/*任务结束清理函数*/
void Task_End(void)
{
    miniOS_RtTask_Delete((miniOS_ID_t)miniOS_curTask->thisID);
    /*是否可以malloc和free来释放资源*/
    if (miniOS_curTask->sp != NULL)
        free(miniOS_curTask->sp);
    miniOS_RtTask_SW();
}

/*空闲线程*/
/*
 * __WFE 使得CPU进入等待中断或者等待事件，也就是低功耗模式
 */
void miniOS_RtTask_Idle(void *arg)
{
    int idle_tick = 0;
    while (1)
    {
        //__WFE;
        __ASM("wfe");
        idle_tick++;
        if (idle_tick == 0x7F00 - 1)
        {
            idle_tick = 0;
        }

        miniOS_RtTask_SW();
    }
}

/*miniOS 程序结构 初始化函数*/
void miniOS_Rt_Init()
{
    int i = 0;
    for (i = 0; i < os_Task_sum; i++)
    {
        miniOS_RtTask_Table[i].stackptr = NULL;
        miniOS_RtTask_Table[i].taskentry = NULL;
        miniOS_RtTask_Table[i].task = NULL;
        miniOS_RtTask_Table[i].sp = NULL;
        miniOS_RtTask_Table[i].thisID = -1;
        miniOS_RtTask_Table[i].stackSize = 0;
        miniOS_RtTask_Table[i].runflag = os_sleep;
        miniOS_RtTask_Table[i].runState = os_sleep;
        miniOS_RtTask_Table[i].runPerid = 0;
        miniOS_RtTask_Table[i].runTimer = 0;
        miniOS_RtTask_Table[i].nextTime = 0;
        if (i == os_Task_idle)
        {
            miniOS_task_create(os_Task_idle,
                               miniOS_RtTask_Idle,
                               0,
                               staticSTK,
                               idleStack,
                               &idleStack[idleStackSize - 1],

                               os_run, 1, 0);
        }
    }
    miniOS_CPU_ExceptStkBase = miniOS_CPU_ExceptStk + miniOS_CPU_ExceptStkSize - 1;

    return;
}

/*miniOS线程创建与任务注册*/
/*
 * miniOS_ID_t wantRegTaskId,  线程表中的ID号
 * miniOS_TASK task,           线程入口地址
 * miniOS_STK_Type             线程栈类型
 * miniOS_STK *stackBaseptr    线程栈基地址
 * miniOS_STK *stackptr ,      线程栈底地址，采用递减栈空间使用
 * miniOS_TaskStatus_t runflag,线程调度第一次标志
 * uint16_t runPerid,          线程拥有时间片
 * uint16_t runTime            线程延迟启动时间，配合os_suspend标志
 */
void miniOS_task_create(miniOS_ID_t wantRegTaskId,
                        miniOS_TASK task,
                        void *arg,

                        miniOS_STK_Type stkTtpe,
                        miniOS_STK *stackBaseptr,
                        miniOS_STK *stackptr,
                        // uint16_t   stacksize,

                        miniOS_TaskStatus_t runflag,
                        uint16_t runPerid,
                        uint16_t runTime)
{
    /*获取线程控制块地址*/
    miniOS_RtTask_t *tcb = &miniOS_RtTask_Table[wantRegTaskId];

    //第一步 设置栈基地址与栈初始化
    miniOS_STK *p_stackkptr;
    p_stackkptr = stackptr;
    p_stackkptr = (miniOS_STK *)((miniOS_STK)(p_stackkptr)&0xFFFFFFF8);
    *(--p_stackkptr) = (miniOS_STK)0x01000000uL;
    *(--p_stackkptr) = (miniOS_STK)task;         // Entry Point
    *(--p_stackkptr) = (miniOS_STK)Task_End;     // R14 (LR)
    *(--p_stackkptr) = (miniOS_STK)0x12121212uL; // R12
    *(--p_stackkptr) = (miniOS_STK)0x03030303uL; // R3
    *(--p_stackkptr) = (miniOS_STK)0x02020202uL; // R2
    *(--p_stackkptr) = (miniOS_STK)0x01010101uL; // R1
    *(--p_stackkptr) = (miniOS_STK)arg;          // R0(arg)

    *(--p_stackkptr) = (miniOS_STK)0x11111111uL; // R11
    *(--p_stackkptr) = (miniOS_STK)0x10101010uL; // R10
    *(--p_stackkptr) = (miniOS_STK)0x09090909uL; // R9
    *(--p_stackkptr) = (miniOS_STK)0x08080808uL; // R8
    *(--p_stackkptr) = (miniOS_STK)0x07070707uL; // R7
    *(--p_stackkptr) = (miniOS_STK)0x06060606uL; // R6
    *(--p_stackkptr) = (miniOS_STK)0x05050505uL; // R5
    *(--p_stackkptr) = (miniOS_STK)0x04040404uL; // R4
    tcb->stackptr = p_stackkptr;

    //第二步，设置线程相关信息
    tcb->runflag = os_sleep;
    tcb->runState = os_sleep;
    tcb->runPerid = 10;
    tcb->stackSize = sizeof(stackptr);
    tcb->taskentry = task;
    tcb->task = task;
    tcb->nextTime = runTime;
    tcb->runTimer = 0;
    if (stkTtpe == dynamicSTK)
        tcb->sp = stackBaseptr;
    else
        tcb->sp = NULL;
    tcb->thisID = wantRegTaskId;
}

/*设置下一次运行的线程ID*/
/*
 * id ; 下一次运行的ID
 */
void setNext(miniOS_ID_t id)
{
    miniOS_nextTask = &miniOS_RtTask_Table[id];
    miniOsNextTaskID = id;
}

/*rt版本线程切换*/
void miniOS_RtTask_SW()
{
    int i = 0;
    miniOS_RtTask_p tcb = NULL;
    int32_t level = 0;
    level = miniOS_hw_interrupt_disable();
    /*遍历任务表，将os_run标志的任务找出来*/
    for (i = 0; i < os_Task_sum; i++)
    {
        tcb = &miniOS_RtTask_Table[i];
        if (tcb->taskentry == NULL)
            continue;
        if (tcb->runflag == os_suspend)
            continue;
        if (tcb->runflag == os_sleep)
            continue;
        if (tcb->runflag == os_wait_sem)
            continue;
        if (tcb->runflag == os_run)
            break;
    }
    // printf("sw:%d",i);
    /*if can't find any 'os_run' task, let next task become os_Task_idle*/
    /*如果找不到任何os_run 标志的	线程，就让下一个任务 指向idle线程（下一次调度）*/
    if (i == os_Task_sum)
        i = os_Task_idle;

    miniOS_nextTask = &miniOS_RtTask_Table[i];
    miniOS_hw_interrupt_enable(level);
    miniOS_Hw_contextSw();
}

/*miniOS线程删除*/
/*
 * taskID，要删除的线程ID
 */
void miniOS_RtTask_Delete(miniOS_ID_t taskID)
{
    if (taskID >= os_Task_sum)
        return;
    miniOS_RtTask_Table[taskID].taskentry = NULL;
    miniOS_RtTask_Table[taskID].task = NULL;
    miniOS_RtTask_Table[taskID].stackptr = NULL;
}

/*miniOS线程就绪*/
/*
 * taskID，要加入就绪表的线程ID
 */
void miniOS_RtTask_AddReady(miniOS_ID_t taskID)
{
}

/*systick 线程服务函数*/
/*
 * 请将此函数添加进sysTick Handler函数中
 * 此函数完成：tick计数
 * 遍历任务表，判断线程状态并进行状态转换
 */
void miniOS_RtTask_sysTickHandle()
{
    int i = 0; // tasktable
    int j = 0; // semtabl
    miniOS_RtTask_p tcb = NULL;
    int32_t level = miniOS_hw_interrupt_disable();

    if (miniOS_delay != 0)
        miniOS_delay--;

    miniOS_TimeTick++;
    if (miniOS_TimeTick == UINT32_MAX)
        miniOS_TimeTick = 0;

    for (i = 0; i < os_Task_sum; i++)
    {
        /*获取线程控制块地址*/
        tcb = &miniOS_RtTask_Table[i];

        /*线程入口函数没有注册就查看下一个ID*/
        if (tcb->taskentry == NULL)
            continue;

        /*如果当前线程是挂起状态，将等待时间减1 tick,直到等待时间到达则唤醒线程*/
        if (tcb->runflag == os_suspend)
        {
            tcb->nextTime--;
            if (tcb->nextTime == 0)
            {
                tcb->runflag = os_run;
            }
            continue;
        }
        /*如果线程是等待信号量的状态，则检查本线程等待的信号量是否被占用，等待信号量释放后唤醒线程*/
        else if (tcb->runflag == os_wait_sem)
        {
            if (miniOS_sem_table[tcb->waitSemID].isTaked == semIdle)
                tcb->runflag = os_run;
            continue;
        }

        /*如果当前线程是运行状态，则检查线程时间片是否用完，用完则将线程睡眠，交出cpu控制权，给下一个任务，并等待下一次唤醒*/
        else if (tcb->runflag == os_run)
        {
            tcb->runTimer++;
            if (tcb->runTimer == tcb->runPerid)
            {
                tcb->runflag = os_sleep;
                tcb->runTimer = 0;
            }
            continue;
        }

        //以下两种方案都可以顺利执行下去
        /*如果当前线程是睡眠状态，则立即将线性唤醒*/
        else if (tcb->runflag == os_sleep)
        {
            tcb->runflag = os_run;
            continue;
        }

        /*如果当前线程是睡眠态，则在睡眠一个时间片之后唤醒*/
        //	 else if (tcb->runflag == os_sleep)
        //		{
        //			tcb->runTimer++;
        //			if(tcb->runTimer == tcb->runPerid)
        //				{
        //					tcb->runflag = os_run;
        //					tcb->runTimer = 0;
        //				}
        //			continue ;
        //		}
    }

    miniOS_hw_interrupt_enable(level);
}

/*miniOS延时函数*/
/*
 * delayCount ，延时多少个sysTick
 * 在当前线性没被删除的前提下，设置delay时间(nextTime)
 * 并引起一次系统调度
 * Attention:此延时函数不精准,要实现阻塞延时，请利用systick定时器按查询法抢占CPU，
   新建static uint32_t delay = 0;在sysTick Handler中令其递减，在while循环中查询delay的值
 * todo : 将此函数放入void SysTick_Handler(void)
 * sample:
       void SysTick_Handler(void)
                {
                    miniOS_RtTask_sysTickHandle();
                }
*/
void miniOS_Rt_Delay(uint16_t delayCount)
{
    int32_t level = miniOS_hw_interrupt_disable();
    if (miniOS_curTask->taskentry != NULL)
    {
        // printf("NOW ID IS %d",miniOS_curTask->thisID);
        miniOS_curTask->nextTime = delayCount;
        miniOS_curTask->runflag = os_suspend;
    }
    miniOS_hw_interrupt_enable(level);
    miniOS_RtTask_SW();
}

/*miniOS 启动调度，开始从main函数转向线性调度器*/
void miniOS_Rt_Start()
{
    miniOS_Start_Asm();
}

/*阻塞延时函数*/
/*
 * nTick ,阻塞等待延时
 */
void miniOS_delay_ms(uint16_t nTick)
{
    miniOS_delay = nTick;
    while (miniOS_delay)
    {
        ;
    }
}

void miniOS_SwHook()
{
    ;
}

/**************第三版改进建议*****************/
/*
 * 1 增加先入先出队列，作为就绪列表和数据传输方案
 * 2 增加信号量机制
   增加一个状态：os_wait_sem,并增加一个sem文件只有拿到信号量的时候才可以变为os_run,
*/

/*二值信号量*/
/*
 * 一个信号量最多提供给两个线程使用，否则有可能发生错误
 *
 *
 */

/*信号量初始化函数*/
/*
 * 将信号量的一些参数设置成默认值
 */
void miniOS_sem_init()
{
    int i = 0;
    for (i = 0; i < os_sem_sum; i++)
    {
        miniOS_sem_table[i].nowBind = 0x78;
        miniOS_sem_table[i].isTaked = semIdle;
    }
}

// void miniOS_sem_bind(miniOS_ID_t taskID)
// {
// 	miniOS_sem_table[taskID].nowBind = taskID;
// }
/**/

/*信号量等待函数*/
/*
 * semID 信号量ID
 * 首先一言不合关中断
 * 然后检测ID是否合法
 * 再检测信号量是否被占用，如被占用，则挂起本线程。设置成等待信号量的状态，
   进行一次任务调度
 * 如果信号量没有被占用则继续进行，
 * 线程唤醒将从本函数后面的代码进行执行
 *
*/
void miniOS_sem_take(uint8_t semID)
{
    int ifSW = 0;
    int32_t level = miniOS_hw_interrupt_disable();
    if (miniOS_curTask->taskentry != NULL)
    {

        if (semID < os_Task_sum)
        {
            if (miniOS_sem_table[semID].isTaked == semIdle)
            {
                miniOS_sem_table[semID].isTaked = semTaked;
                miniOS_curTask->runflag = miniOS_curTask->runflag; //这种情况下不用切换线程，拿到了信号量就往下执行
            }
            else if (miniOS_sem_table[semID].isTaked == semTaked) //这种情况下要切换线程，等待信号量
            {
                miniOS_curTask->runflag = os_wait_sem;
                miniOS_curTask->waitSemID = semID;
                ifSW = 1;
            }
        }
    }
    miniOS_hw_interrupt_enable(level);
    if (ifSW == 1)
        miniOS_RtTask_SW();
}

/*信号量释放函数*/
/*
 * 信号量释放后，将本线程修改为就绪态(os_sleep),
   等待时间片到来唤醒
 * semID 信号量ID，本系统现提供与任务数量相同的信量
*/
void miniOS_sem_give(uint8_t semID)
{
    int32_t level = miniOS_hw_interrupt_disable();
    if (semID < os_Task_sum)
    {
        miniOS_sem_table[semID].isTaked = semIdle;
    }
    miniOS_curTask->runflag = os_sleep; //修改为就绪态
    miniOS_hw_interrupt_enable(level);
    miniOS_RtTask_SW(); //然后发生一次任务切换
}

/*************************************************************/

/*最主要的就是队列缓存、入列、出列*/
/*入列：位置/满了怎么办*/
/*入列找队尾，一入尾+1*/
/*出列找对头，一出头+1*/
/*主要是队头队尾的处理和判断*/

/*针对不同的队列的通用算法*/

// void queue_in(unsigned char **head,unsigned char **tail,unsigned char *pbuff,unsigned char pbufflen,unsigned char *data,unsigned char datalen);
// int  queue_out(unsigned char **head,unsigned char **tail,unsigned char *pbuff,unsigned char pbufflen,unsigned char *data,unsigned char datalen);
// void queue_init(unsigned char **head,unsigned char **tail,unsigned char *pbuff);
// uint16_t queue_len(unsigned char **head,unsigned char **tail,unsigned char *pbuff,unsigned char pbufflen);

/*入列*/
/*
 * head 队头 tail 队尾 pbuff 缓存区数组 pbufflen 缓存长度
 * data入列数组地址，datalen入列数组长度
 */
void queue_in(unsigned char **head, unsigned char **tail, unsigned char *pbuff, unsigned char pbufflen, unsigned char *data, unsigned char datalen)
{
    uint8_t num = 0;
    int32_t level = miniOS_hw_interrupt_disable();
    for (num = 0; num < datalen; num++, data++)
    {
        // TODO
        **tail = *data; //把数据入列
        (*tail)++;      //队尾+1
        if (*tail == (pbuff + pbufflen))
        {
            *tail = pbuff; //如果到达pbuf尾部，重新指向pbuf首地址
        }
        if (*tail == *head)
        { //队尾指针 == 队头指针
            // TODO
            if (++(*head) == (pbuff + pbufflen))
            {
                //如果到达pbuff尾部，重新指向pbuf首地址
                *head = pbuff;
            }
        }
    }
    miniOS_hw_interrupt_enable(level);
}

/*出列*/
/*
 * head 队头 tail 队尾 pbuff 缓存区数组 pbufflen 缓存长度
 * data出列（返回值），datalen入列数组长度，目前只做了1字节出列
 */
int queue_out(unsigned char **head, unsigned char **tail, unsigned char *pbuff, unsigned char pbufflen, unsigned char *data, unsigned char datalen)
{

    int status = 0;
    int32_t level = miniOS_hw_interrupt_disable();
    if (*head != *tail)
    {
        // TODO
        *data = *(*head);
        status = 1;
        if (++(*head) == (pbuff + pbufflen))
        {
            // TODO
            *head = pbuff;
        }
    }
    miniOS_hw_interrupt_enable(level);
    return status;
}

void queue_init(unsigned char **head, unsigned char **tail, unsigned char *pbuff)
{
    *head = pbuff;
    *tail = pbuff;
}

uint16_t queue_len(unsigned char **head, unsigned char **tail, unsigned char *pbuff, uint16_t pbufflen)
{
    uint16_t len = 0;
    // printf("==%d==",pbufflen);
    int32_t level = miniOS_hw_interrupt_disable();
    if (*tail > *head)
    {
        len = (*tail - *head);
        // return (*tail - *head);
    }
    else if (*tail < *head)
    {
        len = (*tail + pbufflen - *head);
        //	len = ((*tail + pbufflen) - (*head))

        // return (*tail + pbufflen - *head);
    }
    else if (*tail == *head)
    {
        len = 0;
        // return 0;
    }
    miniOS_hw_interrupt_enable(level);
    return len;
}

//建议改成int类型
int queue_strstr(unsigned char **head, unsigned char **tail, unsigned char *pbuff, uint16_t pbufflen, const char *substr)
{
    uint8_t num = 0;
    int32_t level = miniOS_hw_interrupt_disable();
    unsigned char *phead = *head; //创建临时指针，不改变原始值
    while (phead != *tail)
    {
        if (*phead != *substr)
        {
            (phead)++;
            if (phead == (pbuff + pbufflen))
                phead = pbuff;
            num++;
            continue;
        }

        //创建临时指针
        unsigned char *tmpstr = phead;
        char *tmpsubstr = (char *)substr;

        while (*tmpsubstr != '\0')
        {
            if (*(tmpstr) != *tmpsubstr)
            {
                //匹配失败
                (phead)++;
                if (phead == (pbuff + pbufflen))
                    phead = pbuff;
                num++;
                break;
            }
            (tmpstr)++;
            if (tmpstr == (pbuff + pbufflen))
                tmpstr = pbuff;
            tmpsubstr++;
        }
        if (*tmpsubstr == '\0') //匹配成功
        {
            miniOS_hw_interrupt_enable(level);
            return num; //返回的是下标
        }
    }
    miniOS_hw_interrupt_enable(level);
    return -1;
}

void miniOS_delay_us(uint16_t nus)
{

    uint32_t start, now, delta, reload, us_tick;
    start = SysTick->VAL;
    reload = SysTick->LOAD;
    us_tick = SystemCoreClock / 1000000UL;
    do
    {
        now = SysTick->VAL;
        delta = start > now ? start - now : reload + start - now;
    } while (delta < us_tick * nus);
}

void miniOS_opt_delay(uint16_t delaytime, int rt)
{
    if (rt == 1)
        miniOS_Rt_Delay(delaytime);
    else
    {
        miniOS_delay_ms(delaytime);
    }
}
