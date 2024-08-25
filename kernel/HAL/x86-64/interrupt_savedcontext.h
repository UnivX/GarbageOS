#pragma once
#include <stdint.h>

//we have a Pre and Post saved context because the isr_stub_n pass us the value of rsp after 
//the creation of the stack frame(aka after the push of rbp
typedef struct x64SavedContext{
	//the post saved context are the values/register pushed by the isr_stub_n
	//after the creation of the stack frame
	struct x64PostSavedContext{
		uint16_t es;
		uint16_t ds;
		uint16_t fs;
		uint16_t gs;
		uint64_t r11;
		uint64_t r10;
		uint64_t r9;
		uint64_t r8;
		uint64_t rcx;
		uint64_t rdx;
		uint64_t rsi;
		uint64_t rdi;
		uint64_t rax;
	}__attribute__((__packed__)) post_rbp;
	//the pre saved context are the values/register that were pushed by the cpu
	//and the rbp pushed to create the stack frame
	struct x64PreSavedContext{
		uint64_t rbp;
		uint64_t error_code;
		uint64_t rip;
		uint64_t cs;
		uint64_t rflags;
		uint64_t rsp;
		uint64_t ss;
	} __attribute__((packed)) pre_rbp;
}__attribute__((packed)) x64SavedContext;

x64SavedContext* getx64SavedContext(void* interrupt_savedcontext);
