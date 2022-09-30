NVIC_INT_CTRL   EQU     0xE000ED04                              ; Interrupt control state register.
NVIC_SYSPRI14   EQU     0xE000ED22                              ; System priority register (priority 14).
NVIC_SYSPRI2    EQU     0xE000ED20                              ; system priority register (2)
NVIC_PENDSV_PRI EQU           0xFF                              ; PendSV priority value (lowest).
NVIC_PENDSVSET  EQU     0x10000000                              ; Value to trigger PendSV exception.

  
  AREA |.text|, CODE, READONLY, ALIGN=2
  THUMB
  REQUIRE8
  PRESERVE8

  EXTERN  miniOS_CPU_ExceptStkBase
  EXTERN  miniOS_curTask
  EXTERN  miniOS_nextTask
;  this is Suitable for arm -M3
;/*
; * int32_t miniOS_hw_interrupt_disable();
; */
miniOS_hw_interrupt_disable    PROC
    EXPORT  miniOS_hw_interrupt_disable
    MRS     r0, PRIMASK
    CPSID   I
    BX      LR
    ENDP
	
;/*
; * void miniOS_hw_interrupt_enable(int32_t level);
; */
miniOS_hw_interrupt_enable    PROC
    EXPORT  miniOS_hw_interrupt_enable
    MSR     PRIMASK, r0
    BX      LR
    ENDP

;/*
; * level = int32_t miniOS_Cpu_SR_Save();
; */
miniOS_Cpu_SR_Save PROC
	EXPORT miniOS_Cpu_SR_Save
	MRS r0,PRIMASK
	CPSID I
	BX LR
	ENDP

;/*
; * void miniOS_Cpu_SR_Recover(int32_t level);
; */
miniOS_Cpu_SR_Recover PROC
	EXPORT miniOS_Cpu_SR_Recover
	MSR     PRIMASK, r0
    BX      LR
    ENDP



miniOS_Hw_contextSw PROC
	EXPORT  miniOS_Hw_contextSw
	LDR     R0, =NVIC_INT_CTRL
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR
	ENDP


miniOS_Start_Asm PROC
	EXPORT  miniOS_Start_Asm
	LDR     R0, =NVIC_SYSPRI2                                   ; Set the PendSV exception priority
    LDR     R1, =NVIC_PENDSV_PRI
    STRB    R1, [R0]

    MOVS    R0, #0                                              ; Set the PSP to 0 for initial context switch call
    MSR     PSP, R0

    LDR     R0, =miniOS_CPU_ExceptStkBase                       ; Initialize the MSP to the OS_CPU_ExceptStkBase
    LDR     R1, [R0]
    MSR     MSP, R1    

    LDR     R0, =NVIC_INT_CTRL                                  ; Trigger the PendSV exception (causes context switch)
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]

    CPSIE   I	; Enable interrupts at processor level
	ENDP

miniOSStartHang PROC
    B       miniOSStartHang                                     ; Should never get here
    ENDP
		
PendSV_Handler PROC
    EXPORT  PendSV_Handler
    CPSID   I                                                   ; Prevent interruption during context switch
    MRS     R0, PSP                                             ; PSP is process stack pointer
    CBZ     R0, OS_CPU_PendSVHandler_nosave                     ; Skip register save the first time
   
    SUBS    R0, R0, #0x20                                       ; Save remaining regs r4-11 on process stack
    STM     R0, {R4-R11}

    LDR     R1, =miniOS_curTask                                       ; OSTCBCur->OSTCBStkPtr = SP;
    LDR     R1, [R1]
    STR     R0, [R1]                                            ; R0 is SP of process being switched out

                                                                ; At this point, entire context of process has been saved
OS_CPU_PendSVHandler_nosave
    LDR     R0, =miniOS_curTask                                       ; OSTCBCur  = OSTCBHighRdy;
    LDR     R1, =miniOS_nextTask
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     R0, [R2]                                            ; R0 is new process SP; SP = OSTCBHighRdy->OSTCBStkPtr;
  
    LDM     R0, {R4-R11}                                        ; Restore r4-11 from new process stack
    ADDS    R0, R0, #0x20
            
    MSR     PSP, R0                                             ; Load PSP with new process SP
    ORR     LR, LR, #0x04                                       ; Ensure exception return uses process stack
    
    CPSIE   I
    BX      LR                                                  ; Exception return will restore remaining context
  
    ENDP
		
    END