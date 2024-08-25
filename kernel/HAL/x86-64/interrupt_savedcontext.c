#include "interrupt_savedcontext.h"
#include "../../kio.h"
#include "../../hal.h"

x64SavedContext* getx64SavedContext(void* interrupt_frame){
	return (x64SavedContext*)(interrupt_frame-sizeof(struct x64PostSavedContext));
}

InterruptArgs get_interrupt_args_from_saved_context(void* interrupt_savedcontext){
	InterruptArgs args;
	// rdi, rsi, rdx, rcx
	const x64SavedContext* regs = getx64SavedContext(interrupt_savedcontext);
	args.arg0 = regs->post_rbp.rdi;
	args.arg1 = regs->post_rbp.rsi;
	args.arg2 = regs->post_rbp.rdx;
	args.arg3 = regs->post_rbp.rcx;
	return args;
}

void print_interrupt_savedcontext(void* interrupt_savedcontext){
	const x64SavedContext* ctx = getx64SavedContext(interrupt_savedcontext);
	uint32_t ds = ctx->post_rbp.ds;
	uint32_t es = ctx->post_rbp.es;
	uint32_t fs = ctx->post_rbp.fs;
	uint32_t gs = ctx->post_rbp.gs;
	uint32_t cs = ctx->pre_rbp.cs;
	uint64_t rip = ctx->pre_rbp.rip;
	uint32_t ss = ctx->pre_rbp.ss;
	uint64_t rsp = ctx->pre_rbp.rsp;
	uint64_t rflags = ctx->pre_rbp.rflags;
	uint64_t rbp = ctx->pre_rbp.rbp;
	printf(
			"[SAVED INTERRUPT CONTEXT]\n"
			"cs:rip=%x32:%h64, ss:rsp=%x32:%h64, ds=%x32, es=%x32, fs=%x32, gs=%x64\n"
			"rflags=%h64, rbp=%h64\n"
			,cs, rip, ss, rsp, ds, es, fs, gs
			,rflags, rbp
			);
}
