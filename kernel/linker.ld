ENTRY(_start)
 
SECTIONS
{
	. = 0xffffc00000000000;
 
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.text)
	}
 
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	.rdata BLOCK(4K) : ALIGN(4K)
	{
		*(.rdata)
	}
 
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}
 
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
}
