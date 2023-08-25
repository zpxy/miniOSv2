    .cpu    cortex-m0
    .fpu    softvfp
    .syntax unified
    .thumb
    .text

@********************************************************************************************************
@                                          PUBLIC FUNCTIONS
@                                          ���������Լ�����
@********************************************************************************************************


  .extern  miniOS_CPU_ExceptStkBase          @����ջ
  .extern  miniOS_curTask                    @��ǰ������ƿ�
  .extern  miniOS_nextTask                   @��һ������ƿ�
  .extern  miniOS_SwHook                     @���Ӻ���

@********************************************************************************************************
@                                               EQUATES
@                                            һЩ�Ĵ�����ַ
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


@**************���ж�************************
@* int32_t miniOS_hw_interrupt_dSisable();
@*******************************************
  .global miniOS_hw_interrupt_disable               
  .type miniOS_hw_interrupt_disable, %function      
miniOS_hw_interrupt_disable:
   
    MRS     r0, PRIMASK
    CPSID   I
    BX      LR

	
@**************���ж�*****************************
@* void miniOS_hw_interrupt_enable(int32_t level);
@**************************************************
    .global miniOS_hw_interrupt_enable               
    .type miniOS_hw_interrupt_enable, %function
miniOS_hw_interrupt_enable:
   
    MSR     PRIMASK, r0
    BX      LR


@*************�л�������****************
@*    void miniOS_Hw_contextSw();
@**************************************
    .global miniOS_Hw_contextSw
    .type miniOS_Hw_contextSw, %function
miniOS_Hw_contextSw:
    LDR     R0, =NVIC_INT_CTRL
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR

@******ʹ�û�����������һ�ε���*******
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


@*******arm cortex оƬ�������ж�******
@*     void PendSV_Handler();
@* �����ε��û���PendSV_Handler����
@* push��pop����Ҫ
@* stmia ldmia ָ��
@*************************************
    .global PendSV_Handler
    .type PendSV_Handler, %function
PendSV_Handler:
   
	CPSID   I                            @**��ֹ�������л�ʱ�ж� *
    
    LDR     R1, =miniOS_curTask          @**��ȡ��ǰ��ջ *                                                
    LDR     R0, [R1]                     
    CMP     R0, #0                       @**��M3�ܹ���M0�ܹ���CBZָ��ĵ�Ч����*
    BEQ     PendSVHandler_nosave         @**����ǵ�һ�ν�����ȣ���ת��PendSVHandler_nosave*                      
    
    MRS     R0, PSP                      @**PSP�ǽ��̶�ջָ�� *
    
    SUBS    R0, R0, #0x20                @**׼��32 byte�ռ� *
    LDR     R1, [R1]
    STR     R0, [R1]

    STMIA   R0!, {R4-R7}                 @**���R4��R7������ *
    
    MOV     R4, R8
    MOV     R5, R9
    MOV     R6, R10
    MOV     R7, R11

    STMIA   R0!, {R4-R7}                 @**���R8��R11�����ݣ���ʱ���еļĴ������Ѿ�����*
                                                               
PendSVHandler_nosave:

    LDR     R0, =miniOS_curTask          @** miniOS_curTask = miniOS_nextTask *                       
    LDR     R1, =miniOS_nextTask       
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     R0, [R2]                                           

    LDMIA   R0!, {R4-R7}        
    PUSH    {R4 - R7}                     @* ��һ������Ҫ * 

    LDMIA   R0!, {R4-R7}
    MOV     R8,  R4                                           
    MOV     R9,  R5
    MOV     R10, R6
    MOV     R11, R7 
    POP     {R4 - R7}                     @** ��һ������Ҫ * 

    MSR     PSP, R0                       @** �ָ�psp *                           

    MOV     R0, LR                        @** PendSV�˳� *                        
    MOVS    R1, #0x04
    ORRS    R0,R0,R1
    MOV     LR, R0
    CPSIE   I
    BX      LR                                                  
.end