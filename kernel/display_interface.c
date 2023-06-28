#include "display_interface.h"

Pixel color_to_pixel(Color c){
	Pixel p = {c.b, c.g, c.r, c.a};
	return p;
}

Color pixel_to_color(Pixel p){
	Color c = {p.r, p.g, p.b, p.a};
	return c;
}
