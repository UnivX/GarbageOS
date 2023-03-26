;input eax -> low lba sector
;input edx -> high lba sector
;input di -> destination offset
read_sector:
	pusha
	mov ax, di
	mov [ds:fat32dap_transfer_buffer_offset], ax
	mov [ds:fat32dap_lower_lba], eax
	mov [ds:fat32dap_upper_lba], edx

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
	xor eax, eax
	mov ax, [ds:fat32_buffer_offset]
	call read_sector

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




fat32_error:
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
	fat32_err_msg db "fat32 error", 13, 10, 0

FAT32_DAP:
	fat32dap_size db 16
	fat32dap_reserved db 0
	fat32dap_to_read dw 1
	fat32dap_transfer_buffer_offset dw third_stage_entry
	fat32dap_transfer_buffer_segment dw 0
	fat32dap_lower_lba dd 0
	fat32dap_upper_lba dd 0
