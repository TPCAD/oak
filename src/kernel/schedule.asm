[bits 32]

section .text

global task_switch

task_switch:

	; 保存栈帧
	push ebp
	mov ebp, esp

	; ABI
	push ebx
	push esi
	push edi

	; 获取当前页地址
	mov eax, esp
	and eax, 0xfffff000

	; 保存栈顶地址到 PCB
	mov [eax], esp

	; 获取下一任务的栈顶地址
	mov eax, [ebp + 8]
	mov esp, [eax]

	; 恢复寄存器，此时已是新的任务堆栈
	pop edi
	pop esi
	pop ebx
	pop ebp

	; 跳转到新任务的代码
	ret
