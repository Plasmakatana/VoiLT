#ifndef SHUTDOWN_H
#define SHUTDOWN_H

#include <signal.h>

extern volatile sig_atomic_t shutdown_requested;

#endif  
