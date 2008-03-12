// Windows doesn't have the POSIX random()/srandom() interface: it
// only has the obsolete rand()/srand().  These don't provide very
// good results.  Replacing them with something better is easy, so we
// do.
//
// The code is based on the Lehmer RNG implementation rngs.c by Steve
// Park & Dave Geyer, which appears to be unencumbered.

#include "defs.h"

#define MODULUS    2147483647
#define MULTIPLIER 48271
#define CHECK      399268537

static long seed;

void init_rnd()
{
    seed = time(NULL) % MODULUS;
    get_rnd(0, 0);
    get_rnd(0, 0);
    get_rnd(0, 0);
}

int get_rnd(int lower, int upper)
{
    const long Q = MODULUS / MULTIPLIER;
    const long R = MODULUS % MULTIPLIER;
    long t = MULTIPLIER * (seed % Q) - R * (seed / Q);
    if (t > 0) 
	seed = t;
    else 
	seed = t + MODULUS;

    double val = (double) seed / MODULUS;
    return lower + int(double(upper - lower + 1) * val);
}
