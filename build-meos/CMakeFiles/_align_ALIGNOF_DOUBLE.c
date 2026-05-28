
#include <stdio.h>
#include <stddef.h>
struct t_s { char _pad; double t; };
int main(void) { printf("%d", (int) offsetof(struct t_s, t)); return 0; }
