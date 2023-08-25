    .cpu    cortex-m0
    .fpu    softvfp
    .syntax unified
    .thumb
    .text

@********************************************************************************************************
@                                          PUBLIC FUNCTIONS
@                                          公共变量以及函数
@********************************************************************************************************


  .extern  miniOS_CPU_ExceptStkBase          @主堆栈
  .extern  miniOS_curTask                    @当前任务控制块
  .extern  miniOS_nextTask                   @下一任务控制块
  .extern  miniOS_SwHook                     @钩子函数

@********************************************************************************************************
@                                               EQUATES
@                                            一些寄存器地址
@********************************************************************************************************

@ NVIC_INT_CTRL   EQU     0xE000ED04                              ; Interrupt control state register.
@ NVIC_SYSPRI14   EQU     0xE000ED22                              ; System priority register (priority 14).
@ NVIC_SYSPRI2    EQU     0xE000ED20                              ; system priority register (2)
@ NVIC_PENDSV_PRI EQU           0xFF                              ; PendSV priority value (lowest).
@ NVIC_PENDSVSET  EQU     0x10000000                              ; Value to trigger PendSV exception.

 
  .equ NVIC_INT_CTRL,     0xE000ED04                              @ Interrupt control state register.
  .equ NVIC_SYSPRI2,      0xE000ED20                              @ system priority register (2)
  .equ NVIC_PENDSV_PRI,   0xff                                    @ PendSV priority value (lowest).
  .equ NVIC_PENDSVSET,    0x10000000                              @ Value to trigger PendSV exception.


@**************关中断************************
@* int32_t miniOS_hw_interrupt_dSisable();
@*******************************************
  .global miniOS_hw_interrupt_disable               
  .type miniOS_hw_interrupt_disable, %function      
miniOS_hw_interrupt_disable:
   
    MRS     r0, PRIMASK
    CPSID   I
    BX      LR

	
@**************开中断*****************************
@* void miniOS_hw_interrupt_enable(int32_t level);
@**************************************************
    .global miniOS_hw_interrupt_enable               
    .type miniOS_hw_interrupt_enable, %function
miniOS_hw_interrupt_enable:
   
    MSR     PRIMASK, r0
    BX      LR


@*************切换上下文****************
@*    void miniOS_Hw_contextSw();
@**************************************
    .global miniOS_Hw_contextSw
    .type miniOS_Hw_contextSw, %function
miniOS_Hw_contextSw:
    LDR     R0, =NVIC_INT_CTRL
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR

@******使用汇编代码启动第一次调度*******
@*    void miniOS_Start_Asm();
@*************************************
    .global miniOS_Start_Asm                       
    .type miniOS_Start_Asm, %function              
miniOS_Start_Asm:
    LDR     R0, =NVIC_SYSPRI2                                    
    LDR     R1, =NVIC_PENDSV_PRI
    STR     R1, [R0]

    MOVS    R0, #0                                              
    MSR     PSP, R0

    LDR     R0, =miniOS_CPU_ExceptStkBase                       
    LDR     R1, [R0]
    MSR     MSP, R1

    LDR     R0, =NVIC_INT_CTRL                                 
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]

    CPSIE   I                                                   
	
miniOSStartHang:
    B       miniOSStartHang                                     


@*******arm cortex 芯片可悬浮中断******
@*     void PendSV_Handler();
@* 请屏蔽掉用户的PendSV_Handler函数
@* push和pop很重要
@* stmia ldmia 指令
@*************************************
    .global PendSV_Handler
    .type PendSV_Handler, %function
PendSV_Handler:
   
	CPSID   I                            @**防止上下文切换时中断 *
    
    LDR     R1, =miniOS_curTask          @**获取当前堆栈 *                                                
    LDR     R0, [R1]                     
    CMP     R0, #0                       @**从M3架构到M0架构，CBZ指令的等效代码*
    BEQ     PendSVHandler_nosave         @**如果是第一次进入调度，跳转到PendSVHandler_nosave*                      
    
    MRS     R0, PSP                      @**PSP是进程堆栈指针 *
    
    SUBS    R0, R0, #0x20                @**准备32 byte空间 *
    LDR     R1, [R1]
    STR     R0, [R1]

    STMIA   R0!, {R4-R7}                 @**存放R4到R7的数据 *
    
    MOV     R4, R8
    MOV     R5, R9
    MOV     R6, R10
    MOV     R7, R11

    STMIA   R0!, {R4-R7}                 @**存放R8到R11的数据，此时所有的寄存器都已经保存*
                                                               
PendSVHandler_nosave:

    LDR     R0, =miniOS_curTask          @** miniOS_curTask = miniOS_nextTask *                       
    LDR     R1, =miniOS_nextTask       
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     R0, [R2]                                           

    LDMIA   R0!, {R4-R7}        
    PUSH    {R4 - R7}                     @* 这一步很重要 * 

    LDMIA   R0!, {R4-R7}
    MOV     R8,  R4                                           
    MOV     R9,  R5
    MOV     R10, R6
    MOV     R11, R7 
    POP     {R4 - R7}                     @** 这一步很重要 * 

    MSR     PSP, R0                       @** 恢复psp *                           

    MOV     R0, LR                        @** PendSV退出 *                        
    MOVS    R1, #0x04
    ORRS    R0,R0,R1
    MOV     LR, R0
    CPSIE   I
    BX      LR                                                  
.end