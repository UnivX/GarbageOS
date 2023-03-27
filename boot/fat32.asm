;input eax -> low lba sector
;input edx -> high lba sector
;input di -> destination offset
read_sector:
	pusha

	mov bx, di
	mov [ds:fat32dap_transfer_buffer_offset], bx

	mov [ds:fat32dap_lower_lba], eax
	mov [ds:fat32dap_upper_lba], dx

	;call the BIOS int 13h extension
	mov si, FAT32_DAP
	mov ah, 0x42
	mov dl, [ds:fat32_drive_number]
	int 0x13;jc set if error

	jc fat32_error
	popa
	ret


;input dl -> drive_number
;input ax -> buffer offset, size=sector_size
init_fat32_driver:
	pusha
	mov [ds:fat32_drive_number], dl
	mov [ds:fat32_buffer_offset], ax

	xor edx, edx
	mov eax, [ds:partition_offset]
	mov di, [ds:fat32_buffer_offset]
	call read_sector

	mov ax, [ds:fat32_buffer_offset+510]

	mov ax, [ds:fat32_buffer_offset]
	mov si, ax

	mov ebx, [ds:si+44]
	mov [ds:fat32_root_cluster], ebx

	mov bl, [ds:si+13]
	mov [ds:fat32_sectors_per_cluster], bl

	mov bx, [ds:si+11]
	mov [ds:fat32_sector_size], bx

	mov bx, [ds:si+14]
	mov [ds:fat32_fat_start], bx

	mov bl, [ds:si+16]
	mov [ds:fat32_number_of_fats], bl

	mov ebx, [ds:si+36]
	mov [ds:fat32_fat_size], ebx

	;calc data area start
	mov edx, [ds:fat32_fat_size]
	mov eax, [ds:fat32_number_of_fats]
	mul edx
	add eax, [ds:fat32_fat_start]
	mov [ds:fat32_data_area_start], eax

	popa
	ret


;in -> eax fat entry
;out -> eax fat entry value

get_fat_entry:
	push di
	push si
	push edx
	push ecx
	push ebx
	push ebp
	mov ebp, esp
	add esp, 4

	;ss:ebp -> eax save
	shl eax, 2
	mov [ss:ebp], eax
	;now ax contains low and dx contains high
	mov dx, [ss:ebp+2]
	;cx = div
	mov cx, [ds:fat32_sector_size]
	div cx; dx:ax / cx
	;result in ax, remainder in dx
	;set to zero the high part of eax and edx
	and eax, 0x0000ffff
	and edx, 0x0000ffff
	add eax, [ds:fat32_fat_start]
	add eax, [ds:partition_offset];fat32 partition_offset
	mov ebx, edx

	;eax sector number
	;ebx sector offset

	xor edx, edx
	;eax sector number
	mov di, [ds:fat32_buffer_offset]
	push eax
	push ebx
	call read_sector
	pop ebx
	pop eax
	;get the fat_entry value
	add bx, [ds:fat32_buffer_offset]
	mov eax, [ds:ebx]

	
	mov esp, ebp
	pop ebp
	pop ebx
	pop ecx
	pop edx
	pop si
	pop di
	ret

fat32_error:
	shr ax, 8

	call print_al
	mov si, fat32_err_msg
	call printc

fat32_hang:
	jmp fat32_hang

fat32_data:
	fat32_root_cluster dd 0
	fat32_buffer_offset dw 0
	fat32_sectors_per_cluster db 0

	fat32_fat_start dd 0
	fat32_fat_size dd 0
	fat32_data_area_start dd 0
	fat32_number_of_fats db 0
	fat32_sector_size dd 0

	fat32_drive_number db 0
	fat32_err_msg db " fat32 error", 13, 10, 0

	align 2
FAT32_DAP:
	fat32dap_size db 16
	fat32dap_reserved db 0
	fat32dap_to_read dw 1
	fat32dap_transfer_buffer_offset dw 0
	fat32dap_transfer_buffer_segment dw 0
	fat32dap_lower_lba dd 0
	fat32dap_upper_lba dd 0

fat32_other_data:
	fat32_dbg_msg db "fat32 dbg ", 13, 10, 0
