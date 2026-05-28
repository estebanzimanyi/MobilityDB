
#include <stdio.h>
#include <stddef.h>
struct t_s { char _pad; __int128 t; };
int main(void) { printf("%d", (int) offsetof(struct t_s, t)); return 0; }
