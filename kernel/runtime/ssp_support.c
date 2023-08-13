//stack smashing protection support
#include <stdint.h>
#include "../hal.h"
#include "../kdefs.h"
#define STACK_CHK_GUARD 0x595e9fbd94fda766
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;
 
__attribute__((noreturn))
void __stack_chk_fail(void)
{
	kpanic(STACK_SMASHING);
	while(1) ;
}
