#include "kio.h"

KioState kio_state = {false, {{0,0,0,0},NULL, NULL, NULL}};

void init_kio(DisplayInterface display){
	kio_state.is_initialized = true;
	kio_state.display = display;
}

void finalize_kio(){
	if(!kio_state.is_initialized)
		kpanic(KIO_ERROR);
	kio_state.display.finalize();
}
