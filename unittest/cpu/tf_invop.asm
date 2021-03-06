;--------------------------------------------------------------------------------------
; TF_INVOP.COM
; Invalid opcode tests. This is meant for 8088/8086 which doesn't have invalid opcodes,
; because some people say that it skips bytes quietly, but nobody knows exactly what it
; does. So we execute 0xFF,0xFF (an invalid code) which so far triggers an invalid opcode
; on modern hardware, but is said to be ignored by the 8086.
; Meant to run from within TFL8086.COM to log each instruction.
;--------------------------------------------------------------------------------------
		bits 16			; 16-bit real mode
		org 0x100		; DOS .COM executable starts at 0x100 in memory

		cli			; clear interrupts (will be skipped by TF log mechanism)

; take INT 06h. our prefix tests might trigger invalid opcodes
		push	es
		xor	ax,ax
		mov	es,ax
		mov	bx,[es:(0x06*4)+0]
		mov	cx,[es:(0x06*4)+2]
		mov	word [old_06h],bx
		mov	word [old_06h+2],cx
		mov	word [es:(0x06*4)+0],new_06h
		mov	word [es:(0x06*4)+2],cs
		pop	es

; =================== 0xFF,0xFF
test1:		mov	word [cs:ret_06h],.pass		; in case of Invalid Opcode, set jump location
		db	0xFF,0xFF			; invalid opcode 0xFF,0xFF
		nop
		nop
		nop
		nop
.pass:

; =================== CS: 0xFF,0xFF
test2:		mov	word [cs:ret_06h],.pass		; in case of Invalid Opcode, set jump location
		cs
		db	0xFF,0xFF			; invalid opcode 0xFF,0xFF
		nop
		nop
		nop
		nop
.pass:

; =================== ES: 0xFF,0xFF
test3:		mov	word [cs:ret_06h],.pass		; in case of Invalid Opcode, set jump location
		ds
		db	0xFF,0xFF			; invalid opcode 0xFF,0xFF
		nop
		nop
		nop
		nop
.pass:

; =================== REP 0xFF,0xFF
test4:		mov	word [cs:ret_06h],.pass		; in case of Invalid Opcode, set jump location
		mov	cx,32				; might affect CX?
		rep
		db	0xFF,0xFF			; invalid opcode 0xFF,0xFF
		nop
		nop
		nop
		nop
.pass:

; =================== REPZ 0xFF,0xFF
test5:		mov	word [cs:ret_06h],.pass		; in case of Invalid Opcode, set jump location
		mov	cx,32				; might affect CX?
		xor	ax,ax				; clear AX, setting ZF=1
		repz
		db	0xFF,0xFF			; invalid opcode 0xFF,0xFF
		nop
		nop
		nop
		nop
.pass:

; =================== LOCK 0xFF,0xFF
test6:		mov	word [cs:ret_06h],.pass		; in case of Invalid Opcode, set jump location
		db	0xF0				; lock
		db	0xFF,0xFF			; invalid opcode 0xFF,0xFF
		nop
		nop
		nop
		nop
.pass:

; =================== CS: 0xFF,0xFF
test7:		mov	word [cs:ret_06h],.pass		; in case of Invalid Opcode, set jump location
		cs
		ds
		es
		ss
		db	0xFF,0xFF			; invalid opcode 0xFF,0xFF
		nop
		nop
		nop
		nop
.pass:

; =================== CS: 0xFF,0xFF
test8:		mov	word [cs:ret_06h],.pass		; in case of Invalid Opcode, set jump location
		db	0xF0
		db	0xF0
		db	0xF0
		db	0xF0
		db	0xFF,0xFF			; invalid opcode 0xFF,0xFF
		nop
		nop
		nop
		nop
.pass:

; restore INT 06h
		push	es
		xor	ax,ax
		mov	es,ax
		mov	bx,word [old_06h]
		mov	cx,word [old_06h+2]
		mov	word [es:(0x06*4)+0],bx
		mov	word [es:(0x06*4)+2],cx
		pop	es

; =================== END
		ret

; int 06h vector.
; NTS: TFL8086.COM cannot trap-flag log execution during this fault handler.
;      Logging the fault handler happens to work for SANITY4.COM apparently because
;      UD2 (0x0F 0x0B) causes the CPU to single-step the INT 6 fault handler. A real
;      case of invalid opcode does not trigger that behavior and this handler would
;      not be logged at all.
new_06h:	add	sp,2		; discard offset
		mov	bp,.fault_jmp	; replace offset
		push	bp		; load it as the offset
		iret			; IRET to .fault_jmp
.fault_jmp:	jmp	word [cs:ret_06h] ; jump to fault routine.
		; NTS: Because of the way trap flags work, the CPU will execute this JMP
		;      first after the fault handler THEN log the first instruction at CS:IP.
		;      This JMP will not show up in the trap flag log.

		segment .data

junk:		db			"This program is meant to run in TRAPLOG"

		segment .bss

old_06h:	resw	2
ret_06h:	resw	1

