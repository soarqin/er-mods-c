default rel

section .data
global execute_action_button_param_proxy_return
execute_action_button_param_proxy_return: dq original_stub
filter_result: dd 0
original_called: dd 0
align 16
expected0: dd 1, 2, 3, 4
expected1: dd 5, 6, 7, 8
expected2: dd 9, 10, 11, 12

section .text
extern exec_action_button_param_proxy_hook
global test_autoloot_wrapper
global check_exec_action_button_param_filters

; Overwrite the callee's home area and all volatile arguments.
check_exec_action_button_param_filters:
    mov qword [rsp+8], 0
    mov qword [rsp+10h], 0
    mov qword [rsp+18h], 0
    mov qword [rsp+20h], 0
    xor ecx, ecx
    xor edx, edx
    xor r8d, r8d
    xor r9d, r9d
    pxor xmm0, xmm0
    pxor xmm1, xmm1
    pxor xmm2, xmm2
    mov eax, [filter_result]
    ret

original_stub:
    mov dword [original_called], 1
    mov eax, 7
    ret

test_autoloot_wrapper:
    sub rsp, 28h
    mov [filter_result], ecx
    mov dword [original_called], 0
    mov ecx, 11
    mov edx, 22
    mov r8d, 33
    mov r9d, 44
    movdqa xmm0, [expected0]
    movdqa xmm1, [expected1]
    movdqa xmm2, [expected2]
    call exec_action_button_param_proxy_hook
    cmp dword [filter_result], -1
    je .fallback
    cmp eax, 1
    jne .fail
    cmp dword [original_called], 0
    jne .fail
    jmp .registers
.fallback:
    cmp eax, 7
    jne .fail
    cmp dword [original_called], 1
    jne .fail
.registers:
    cmp rcx, 11
    jne .fail
    cmp rdx, 22
    jne .fail
    cmp r8, 33
    jne .fail
    cmp r9, 44
    jne .fail
    pcmpeqb xmm0, [expected0]
    pmovmskb eax, xmm0
    cmp eax, 0FFFFh
    jne .fail
    pcmpeqb xmm1, [expected1]
    pmovmskb eax, xmm1
    cmp eax, 0FFFFh
    jne .fail
    pcmpeqb xmm2, [expected2]
    pmovmskb eax, xmm2
    cmp eax, 0FFFFh
    jne .fail
    mov eax, 1
    add rsp, 28h
    ret
.fail:
    xor eax, eax
    add rsp, 28h
    ret
