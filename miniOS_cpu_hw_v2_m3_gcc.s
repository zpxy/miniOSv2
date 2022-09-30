@********************************************************************************************************
@                                          PUBLIC FUNCTIONS
@                                          公共变量以及函数
@********************************************************************************************************


  .extern  miniOS_CPU_ExceptStkBase          @主堆栈
  .extern  miniOS_curTask                    @当前任务控制块
  .extern  miniOS_nextTask                   @下一任务控制块


@********************************************************************************************************
@                                               EQUATES
@                                            一些寄存器地址
@********************************************************************************************************

@ NVIC_INT_CTRL   EQU     0xE000ED04                              ; Interrupt control state register.
@ NVIC_SYSPRI14   EQU     0xE000ED22                              ; System priority register (priority 14).
@ NVIC_SYSPRI2    EQU     0xE000ED20                              ; system priority register (2)
@ NVIC_PENDSV_PRI EQU           0xFF                              ; PendSV priority value (lowest).
@ NVIC_PENDSVSET  EQU     0x10000000                              ; Value to trigger PendSV exception.

  .equ SCB_VTOR,          0xE000ED08                                    /* Vector Table Offset Register */
  .equ NVIC_INT_CTRL,     0xE000ED04                              @ Interrupt control state register.
  .equ NVIC_SYSPRI14,     0xE000ED22                              @ System priority register (priority 14).
  .equ NVIC_SYSPRI2,      0xE000ED20                              @ system priority register (2)
  .equ NVIC_PENDSV_PRI,   0xFF                                    @ PendSV priority value (lowest).
  .equ NVIC_PENDSVSET,    0x10000000                              @ Value to trigger PendSV exception.
  


@********************************************************************************************************
@                                     CODE GENERATION DIRECTIVES
@                                     代码生成指令
@********************************************************************************************************

@  .cpu    cortex-m0                  @ arm m0内核 lpc1114
@  .cpu    cortex-m3                  @ arm m3内核 gd32f103 st32f103
@  .cpu    cortex-m4                  @ arm m4内核 n32g430  st32f401
   .fpu    softvfp                    @选择软件浮点运算
   .align  2                          @半字对齐
   .syntax unified                    @通用语法
   .thumb                             @thumb 指令模式
   .text                              @代码区

@ AREA |.text|, CODE, READONLY, ALIGN=2
@ THUMB
@ REQUIRE8
@ PRESERVE8


  
@********************************************************************************************************
@                                     FUNCTION IN ASSEMBLY
@                                         汇编函数
@********************************************************************************************************


@**********函数使用方法*********************
@  int32_t miniOS_hw_interrupt_disable();
@  关闭cpu总中断
@******************************************

  .global miniOS_hw_interrupt_disable                @声明为全局函数，外部可调用
  .type miniOS_hw_interrupt_disable, %function       @声明为函数类型
miniOS_hw_interrupt_disable:
    MRS     r0, PRIMASK
    CPSID   I
    BX      LR

	
@**********函数使用方法***************************
@  void miniOS_hw_interrupt_enable(int32_t level)
@  开启cpu总中断
@************************************************

  .global miniOS_hw_interrupt_enable                @声明为全局函数，外部可调用
  .type miniOS_hw_interrupt_enable, %function      @声明为函数类型
miniOS_hw_interrupt_enable:
    MSR     PRIMASK, r0
    BX      LR

@*************************************
@ level = int32_t miniOS_Cpu_SR_Save();
@*************************************

@ .global miniOS_Cpu_SR_Save
@ .type miniOS_Cpu_SR_Save, %function
@ miniOS_Cpu_SR_Save:
@	MRS r0,PRIMASK
@	CPSID I
@	BX LR
	

@*******************************************
@ void miniOS_Cpu_SR_Recover(int32_t level);
@*******************************************

@ .global miniOS_Cpu_SR_Recover
@ .type miniOS_Cpu_SR_Recover, %function
@ miniOS_Cpu_SR_Recover:
@   MSR     PRIMASK, r0
@    BX      LR
 
 

@**********函数使用方法********
@  void miniOS_Start_Asm(void)
@  第一次启动系统
@*****************************

 .global miniOS_Start_Asm                        @声明为全局函数，外部可调用
 .type miniOS_Start_Asm, %function               @声明为函数类型
miniOS_Start_Asm:

    LDR     R0, =NVIC_SYSPRI2                                   @ Set the PendSV exception priority
    LDR     R1, =NVIC_PENDSV_PRI
    STRB    R1, [R0]

    MOVS    R0, #0                                              @ Set the PSP to 0 for initial context switch call
    MSR     PSP, R0

    LDR     R0, =miniOS_CPU_ExceptStkBase                       @ Initialize the MSP to the OS_CPU_ExceptStkBase
    LDR     R1, [R0]
    MSR     MSP, R1    

    LDR     R0, =NVIC_INT_CTRL                                  @ Trigger the PendSV exception (causes context switch)
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]

    CPSIE   I	                                                @ Enable interrupts at processor level

miniOSStartHang:
    B       miniOSStartHang                                     @ Should never get here,应不会运行到这里


@**********函数使用方法***********
@  void miniOS_Hw_contextSw(void)
@  触发PendSV异常
@********************************
  .global miniOS_Hw_contextSw
  .type miniOS_Hw_contextSw, %function
miniOS_Hw_contextSw:
	LDR     R0, =NVIC_INT_CTRL
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR



@*************** 函数说明  ******************
@   PendSV_Handler
@   请在xxx_it.c中屏蔽原生 PendSV_Handler函数
@*******************************************
  .global PendSV_Handler
  .type PendSV_Handler, %function
PendSV_Handler:
    CPSID   I                                                   @ Prevent interruption during context switch
    MRS     R0, PSP                                             @ PSP is process stack pointer
    CBZ     R0, OS_CPU_PendSVHandler_nosave                     @ Skip register save the first time
   
    SUBS    R0, R0, #0x20                                       @ Save remaining regs r4-11 on process stack
    STM     R0, {R4-R11}

    LDR     R1, =miniOS_curTask                                 @ OSTCBCur->OSTCBStkPtr = SP;
    LDR     R1, [R1]
    STR     R0, [R1]                                            @ R0 is SP of process being switched out
                                                                @ At this point, entire context of process has been saved
OS_CPU_PendSVHandler_nosave:
    LDR     R0, =miniOS_curTask                                 @ OSTCBCur  = OSTCBHighRdy;
    LDR     R1, =miniOS_nextTask
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     R0, [R2]                                            @ R0 is new process SP; SP = OSTCBHighRdy->OSTCBStkPtr;
  
    LDM     R0, {R4-R11}                                        @ Restore r4-11 from new process stack
    ADDS    R0, R0, #0x20
            
    MSR     PSP, R0                                             @ Load PSP with new process SP
    ORR     LR, LR, #0x04                                       @ Ensure exception return uses process stack
    
    CPSIE   I
    BX      LR                                                  @ Exception return will restore remaining context

.end