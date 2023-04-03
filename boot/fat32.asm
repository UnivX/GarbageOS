%define FAT32_END_OF_CHAIN 0x0FFFFFF8
%define FAT32_DIR_ATTR_OFFSET 11
%define ATTR_LONG_FILE_NAME 0x0F
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
	xor eax, eax
	mov al, [ds:fat32_number_of_fats]
	mul edx
	add eax, [ds:fat32_fat_start]
	mov [ds:fat32_data_area_start], eax

	popa
	ret

;in -> eax start_cluster
;in -> edi memory position(unreal mode)
load_file_in_memory:
	pusha

	mov [ds:lfim_clus], eax
.lfim_parse_cluster:

	mov eax, [ds:lfim_clus]
	call fat32_get_first_sector_of_cluster
	;edx:eax cluster

	mov cl, [ds:fat32_sectors_per_cluster]
.lfim_parse_sector:

	;load the sector
	push edx
	push eax
	push edi
	mov di, [ds:fat32_buffer_offset]
	call read_sector
	pop edi
	pop eax
	pop edx

	;copy
	push ebx
	push eax

	xor esi, esi
	mov si, [ds:fat32_buffer_offset]
	mov ebx, 0
.lfim_copy:
	mov eax, [ds:esi+ebx]
	mov [ds:edi+ebx], eax
	add ebx, 4
	cmp ebx, [ds:fat32_sector_size]
	jb .lfim_copy

	pop eax
	pop ebx


	;increment the LBA sector
	inc eax
	jnc .lfim_no_carry
	inc edx
.lfim_no_carry:
	loop .lfim_parse_sector

	mov eax, [ds:lfim_clus]
	call get_fat_entry
	mov [ds:lfim_clus], eax
	cmp eax, FAT32_END_OF_CHAIN
	jb .lfim_parse_cluster

	popa
	ret

;in -> esi ptr to string
;out -> eax cluster of file (0 = error)
;out -> edx size of file
search_file_in_root:
	pusha

	mov eax, [ds:fat32_root_cluster]
	mov [ds:sfir_clust], eax


	mov [ds:sfir_string_ptr], esi


	;eax cluster to parse
.parse_cluster:
	;parse cluster


	mov eax, [ds:sfir_clust]
	;in eax cluster, out edx:eax sector
	call fat32_get_first_sector_of_cluster
	mov [ds:sfir_sect_low], eax
	mov [ds:sfir_sect_high], edx



	xor ecx, ecx
	mov cl, [ds:fat32_sectors_per_cluster]
.parse_sector:
	;because there is a nested loop the ecx reg must be saved after the loop instruction
	;and restored before the loop instruction
	mov [ds:sfir_sector_counter], ecx

	;load sector
	mov eax, [ds:sfir_sect_low]
	mov edx, [ds:sfir_sect_high]
	mov di, [ds:fat32_buffer_offset]
	call read_sector

	;inner loop
	mov si, [ds:fat32_buffer_offset]
	xor ecx, ecx
	mov ecx, [ds:fat32_sector_size]
	;divide by the size of the directory entry(32 bytes or 2^5 bytes)
	shr ecx, 5
.parse_entry:
	;check the attr
	mov al, [ds:si+FAT32_DIR_ATTR_OFFSET]
	and al, ATTR_LONG_FILE_NAME
	cmp al, ATTR_LONG_FILE_NAME
	je .parse_entry_loop_end
	;check if it's used
	cmp byte [ds:si], 0
	je .error_exit

	cmp byte [ds:si], 0xE5
	je .parse_entry_loop_end

	;call fat32_print_dir_entry

	;check the name first first 4 bytes
	mov di, [ds:sfir_string_ptr]
	mov edx, [ds:si]
	cmp edx, [ds:di]
	jne .parse_entry_loop_end

	;check the name first 8 bytes
	mov edx, [ds:si+4]
	cmp edx, [ds:di+4]
	jne .parse_entry_loop_end
	
	;check the name last 3 bytes
	mov edx, [ds:si+8]
	and edx, 0x00FFFFFF
	mov ebx, [ds:di+8]
	and ebx, 0x00FFFFFF
	cmp ebx, edx
	jne .parse_entry_loop_end
	
	;if the name is the same
	xor eax, eax
	;high 2 bytes
	mov ax, [ds:si+20]
	shl eax, 16
	;low 2 bytes
	mov ax, [ds:si+26]

	mov edx, [ds:si+28];size of file

	;mov si, fat32_file_found_msg
	;call printc

	jmp .exit

.parse_entry_loop_end:
	add si, 32
	loop .parse_entry
	
	;increment sector
	mov eax, [ds:sfir_sect_low]
	mov edx, [ds:sfir_sect_high]
	inc eax
	jnc .no_carry
	inc edx
.no_carry:
	mov [ds:sfir_sect_low], eax
	mov [ds:sfir_sect_high], edx

	mov ecx, [ds:sfir_sector_counter]
	;loop not used because the jump is pretty long
	;loop .parse_sector
	dec ecx
	jnz .parse_sector
	
	;in eax -> value of parsed cluster
	mov eax, [ds:sfir_clust]
	call get_fat_entry
	mov [ds:sfir_clust], eax
	cmp eax, FAT32_END_OF_CHAIN
	jb .parse_cluster

	;if it doesn't find the entry return 0 as error
.error_exit:
	;mov si, fat32_file_not_found_msg
	;call printc
	xor eax, eax
.exit:
	mov [ds:fat32_temp], eax
	mov [ds:fat32_temp2], edx
	popa
	mov eax, [ds:fat32_temp]
	mov edx, [ds:fat32_temp2]
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


;in -> eax cluster sector
;out -> eax low lba sector, edx high lba sector
fat32_get_first_sector_of_cluster:
	push ebx
	sub eax, 2
	xor edx, edx
	mov dl, [ds:fat32_sectors_per_cluster]
	mul edx

	mov ebx, [ds:partition_offset]
	add ebx, [ds:fat32_data_area_start]


	add eax, ebx
	jnc .gfsof_exit
	inc edx
	;edx:eax
.gfsof_exit:
	pop ebx
	ret

;in -> esi, dir entry addr
fat32_print_dir_entry:
	push esi
	push cx
	push eax
	push ebx

	mov cx, 11
	mov ebx, 0
.copy:

	mov al, [ds:esi+ebx]
	cmp al, 0
	jz .copy_loop
	mov [ds:fat32_file_name_buffer+ebx], al
	inc ebx
.copy_loop:
	loop .copy

	mov si, fat32_file_print_msg
	call printc

	pop ebx
	pop eax
	pop cx
	pop esi
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
	fat32_file_found_msg db "file found", 13, 10, 0
	fat32_file_not_found_msg db "file not found", 13, 10, 0
	fat32_temp dd 0
	fat32_temp2 dd 0
	fat32_cluster_msg db "reading cluster: ", 0
	fat32_sector_msg db "reading sector: ", 0
	fat32_column_msg db ":", 0
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

	sfir_clust dd 0
	sfir_sect_low dd 4
	sfir_sect_high dd 8
	sfir_sector_counter dd 12 
	sfir_string_ptr dd 16
	lfim_clus dd 0
	lfim_sector dd 0

	fat32_file_print_msg db "Fat Dir Entry: "
	fat32_file_name_buffer times 11 db '?'
	fat32_endl db 13,10,0
