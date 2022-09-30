#include "miniOS_v2.h"

/***********HNU HaiKou Mei Lan*************
####### ######   #####   #####   ##
    ##  ##   ## ##   ## ##   ## ###
   ##   ##   ## ##  ### ##  ###  ##
  ##    ######  ## # ## ## # ##  ##
 ##     ##      ###  ## ###  ##  ##
##      ##      ##   ## ##   ##  ##
####### ##       #####   #####  ####
��Ŀ������miniOSС�Ͷ�����ϵͳ��ơ���
���ܣ�����Cortex M3
��л����лrt_thread����Ŀ
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

/*�ź����ṹ��*/
/*
 * nowBind ����ʹ����Դ��taskID
 * isTaked ��Դ�Ƿ�ʹ�õı�־
 */
struct miniOS_sem_s
{
    uint8_t nowBind;
    uint8_t isTaked;
};

volatile static uint32_t miniOS_TimeTick = 0;                                /*��������*/
volatile static uint32_t miniOS_delay = 0;                                   /*ɵ�ȼ���*/
volatile static miniOS_ID_t miniOsCurTaskID = os_Task_sum;                   /*��ǰ����ID*/
volatile static miniOS_ID_t miniOsNextTaskID, miniOsTaskReady = os_Task_sum; /*��һ��Ҫ���е�����*/

/***********************/
/*       �ڶ��汾      */
/***********************/

/*
 * miniOS_curTask;   ��ǰ����ָ��
 * miniOS_nextTask;  ��һ����ָ��
 * miniOS_CurTaskID; ��ǰ����ID
 * miniOS_NextTaskID;��һ����ָ��
 */
miniOS_RtTask_p miniOS_curTask;
miniOS_ID_t miniOS_CurTaskID;

miniOS_RtTask_p miniOS_nextTask;
miniOS_ID_t miniOS_NextTaskID;

/*����������鷽ʽʵ��*/
miniOS_RtTask_t miniOS_RtTask_Table[os_Task_sum] = {0};

static miniOS_sem_t miniOS_sem_table[os_Task_sum] = {0};

/*idle�߳�ջ*/
miniOS_STK idleStack[idleStackSize];

/*cpuջ*/
miniOS_STK miniOS_CPU_ExceptStk[miniOS_CPU_ExceptStkSize];

/*cpuջ����ַ*/
miniOS_STK *miniOS_CPU_ExceptStkBase;

/*�������������*/
void Task_End(void)
{
    miniOS_RtTask_Delete((miniOS_ID_t)miniOS_curTask->thisID);
    /*�Ƿ����malloc��free���ͷ���Դ*/
    if (miniOS_curTask->sp != NULL)
        free(miniOS_curTask->sp);
    miniOS_RtTask_SW();
}

/*�����߳�*/
/*
 * __WFE ʹ��CPU����ȴ��жϻ��ߵȴ��¼���Ҳ���ǵ͹���ģʽ
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

/*miniOS ����ṹ ��ʼ������*/
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

/*miniOS�̴߳���������ע��*/
/*
 * miniOS_ID_t wantRegTaskId,  �̱߳��е�ID��
 * miniOS_TASK task,           �߳���ڵ�ַ
 * miniOS_STK_Type             �߳�ջ����
 * miniOS_STK *stackBaseptr    �߳�ջ����ַ
 * miniOS_STK *stackptr ,      �߳�ջ�׵�ַ�����õݼ�ջ�ռ�ʹ��
 * miniOS_TaskStatus_t runflag,�̵߳��ȵ�һ�α�־
 * uint16_t runPerid,          �߳�ӵ��ʱ��Ƭ
 * uint16_t runTime            �߳��ӳ�����ʱ�䣬���os_suspend��־
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
    /*��ȡ�߳̿��ƿ��ַ*/
    miniOS_RtTask_t *tcb = &miniOS_RtTask_Table[wantRegTaskId];

    //��һ�� ����ջ����ַ��ջ��ʼ��
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

    //�ڶ����������߳������Ϣ
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

/*������һ�����е��߳�ID*/
/*
 * id ; ��һ�����е�ID
 */
void setNext(miniOS_ID_t id)
{
    miniOS_nextTask = &miniOS_RtTask_Table[id];
    miniOsNextTaskID = id;
}

/*rt�汾�߳��л�*/
void miniOS_RtTask_SW()
{
    int i = 0;
    miniOS_RtTask_p tcb = NULL;
    int32_t level = 0;
    level = miniOS_hw_interrupt_disable();
    /*�����������os_run��־�������ҳ���*/
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
    /*����Ҳ����κ�os_run ��־��	�̣߳�������һ������ ָ��idle�̣߳���һ�ε��ȣ�*/
    if (i == os_Task_sum)
        i = os_Task_idle;

    miniOS_nextTask = &miniOS_RtTask_Table[i];
    miniOS_hw_interrupt_enable(level);
    miniOS_Hw_contextSw();
}

/*miniOS�߳�ɾ��*/
/*
 * taskID��Ҫɾ�����߳�ID
 */
void miniOS_RtTask_Delete(miniOS_ID_t taskID)
{
    if (taskID >= os_Task_sum)
        return;
    miniOS_RtTask_Table[taskID].taskentry = NULL;
    miniOS_RtTask_Table[taskID].task = NULL;
    miniOS_RtTask_Table[taskID].stackptr = NULL;
}

/*miniOS�߳̾���*/
/*
 * taskID��Ҫ�����������߳�ID
 */
void miniOS_RtTask_AddReady(miniOS_ID_t taskID)
{
}

/*systick �̷߳�����*/
/*
 * �뽫�˺�����ӽ�sysTick Handler������
 * �˺�����ɣ�tick����
 * ����������ж��߳�״̬������״̬ת��
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
        /*��ȡ�߳̿��ƿ��ַ*/
        tcb = &miniOS_RtTask_Table[i];

        /*�߳���ں���û��ע��Ͳ鿴��һ��ID*/
        if (tcb->taskentry == NULL)
            continue;

        /*�����ǰ�߳��ǹ���״̬�����ȴ�ʱ���1 tick,ֱ���ȴ�ʱ�䵽�������߳�*/
        if (tcb->runflag == os_suspend)
        {
            tcb->nextTime--;
            if (tcb->nextTime == 0)
            {
                tcb->runflag = os_run;
            }
            continue;
        }
        /*����߳��ǵȴ��ź�����״̬�����鱾�̵߳ȴ����ź����Ƿ�ռ�ã��ȴ��ź����ͷź����߳�*/
        else if (tcb->runflag == os_wait_sem)
        {
            if (miniOS_sem_table[tcb->waitSemID].isTaked == semIdle)
                tcb->runflag = os_run;
            continue;
        }

        /*�����ǰ�߳�������״̬�������߳�ʱ��Ƭ�Ƿ����꣬�������߳�˯�ߣ�����cpu����Ȩ������һ�����񣬲��ȴ���һ�λ���*/
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

        //�������ַ���������˳��ִ����ȥ
        /*�����ǰ�߳���˯��״̬�������������Ի���*/
        else if (tcb->runflag == os_sleep)
        {
            tcb->runflag = os_run;
            continue;
        }

        /*�����ǰ�߳���˯��̬������˯��һ��ʱ��Ƭ֮����*/
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

/*miniOS��ʱ����*/
/*
 * delayCount ����ʱ���ٸ�sysTick
 * �ڵ�ǰ����û��ɾ����ǰ���£�����delayʱ��(nextTime)
 * ������һ��ϵͳ����
 * Attention:����ʱ��������׼,Ҫʵ��������ʱ��������systick��ʱ������ѯ����ռCPU��
   �½�static uint32_t delay = 0;��sysTick Handler������ݼ�����whileѭ���в�ѯdelay��ֵ
 * todo : ���˺�������void SysTick_Handler(void)
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

/*miniOS �������ȣ���ʼ��main����ת�����Ե�����*/
void miniOS_Rt_Start()
{
    miniOS_Start_Asm();
}

/*������ʱ����*/
/*
 * nTick ,�����ȴ���ʱ
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

/**************������Ľ�����*****************/
/*
 * 1 ���������ȳ����У���Ϊ�����б�����ݴ��䷽��
 * 2 �����ź�������
   ����һ��״̬��os_wait_sem,������һ��sem�ļ�ֻ���õ��ź�����ʱ��ſ��Ա�Ϊos_run,
*/

/*��ֵ�ź���*/
/*
 * һ���ź�������ṩ�������߳�ʹ�ã������п��ܷ�������
 *
 *
 */

/*�ź�����ʼ������*/
/*
 * ���ź�����һЩ�������ó�Ĭ��ֵ
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

/*�ź����ȴ�����*/
/*
 * semID �ź���ID
 * ����һ�Բ��Ϲ��ж�
 * Ȼ����ID�Ƿ�Ϸ�
 * �ټ���ź����Ƿ�ռ�ã��类ռ�ã�������̡߳����óɵȴ��ź�����״̬��
   ����һ���������
 * ����ź���û�б�ռ����������У�
 * �̻߳��ѽ��ӱ���������Ĵ������ִ��
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
                miniOS_curTask->runflag = miniOS_curTask->runflag; //��������²����л��̣߳��õ����ź���������ִ��
            }
            else if (miniOS_sem_table[semID].isTaked == semTaked) //���������Ҫ�л��̣߳��ȴ��ź���
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

/*�ź����ͷź���*/
/*
 * �ź����ͷź󣬽����߳��޸�Ϊ����̬(os_sleep),
   �ȴ�ʱ��Ƭ��������
 * semID �ź���ID����ϵͳ���ṩ������������ͬ������
*/
void miniOS_sem_give(uint8_t semID)
{
    int32_t level = miniOS_hw_interrupt_disable();
    if (semID < os_Task_sum)
    {
        miniOS_sem_table[semID].isTaked = semIdle;
    }
    miniOS_curTask->runflag = os_sleep; //�޸�Ϊ����̬
    miniOS_hw_interrupt_enable(level);
    miniOS_RtTask_SW(); //Ȼ����һ�������л�
}

/*************************************************************/

/*����Ҫ�ľ��Ƕ��л��桢���С�����*/
/*���У�λ��/������ô��*/
/*�����Ҷ�β��һ��β+1*/
/*�����Ҷ�ͷ��һ��ͷ+1*/
/*��Ҫ�Ƕ�ͷ��β�Ĵ�����ж�*/

/*��Բ�ͬ�Ķ��е�ͨ���㷨*/

// void queue_in(unsigned char **head,unsigned char **tail,unsigned char *pbuff,unsigned char pbufflen,unsigned char *data,unsigned char datalen);
// int  queue_out(unsigned char **head,unsigned char **tail,unsigned char *pbuff,unsigned char pbufflen,unsigned char *data,unsigned char datalen);
// void queue_init(unsigned char **head,unsigned char **tail,unsigned char *pbuff);
// uint16_t queue_len(unsigned char **head,unsigned char **tail,unsigned char *pbuff,unsigned char pbufflen);

/*����*/
/*
 * head ��ͷ tail ��β pbuff ���������� pbufflen ���泤��
 * data���������ַ��datalen�������鳤��
 */
void queue_in(unsigned char **head, unsigned char **tail, unsigned char *pbuff, unsigned char pbufflen, unsigned char *data, unsigned char datalen)
{
    uint8_t num = 0;
    int32_t level = miniOS_hw_interrupt_disable();
    for (num = 0; num < datalen; num++, data++)
    {
        // TODO
        **tail = *data; //����������
        (*tail)++;      //��β+1
        if (*tail == (pbuff + pbufflen))
        {
            *tail = pbuff; //�������pbufβ��������ָ��pbuf�׵�ַ
        }
        if (*tail == *head)
        { //��βָ�� == ��ͷָ��
            // TODO
            if (++(*head) == (pbuff + pbufflen))
            {
                //�������pbuffβ��������ָ��pbuf�׵�ַ
                *head = pbuff;
            }
        }
    }
    miniOS_hw_interrupt_enable(level);
}

/*����*/
/*
 * head ��ͷ tail ��β pbuff ���������� pbufflen ���泤��
 * data���У�����ֵ����datalen�������鳤�ȣ�Ŀǰֻ����1�ֽڳ���
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

//����ĳ�int����
int queue_strstr(unsigned char **head, unsigned char **tail, unsigned char *pbuff, uint16_t pbufflen, const char *substr)
{
    uint8_t num = 0;
    int32_t level = miniOS_hw_interrupt_disable();
    unsigned char *phead = *head; //������ʱָ�룬���ı�ԭʼֵ
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

        //������ʱָ��
        unsigned char *tmpstr = phead;
        char *tmpsubstr = (char *)substr;

        while (*tmpsubstr != '\0')
        {
            if (*(tmpstr) != *tmpsubstr)
            {
                //ƥ��ʧ��
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
        if (*tmpsubstr == '\0') //ƥ��ɹ�
        {
            miniOS_hw_interrupt_enable(level);
            return num; //���ص����±�
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
