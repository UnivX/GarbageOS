#include "interrupt_savedcontext.h"
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
