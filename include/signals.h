#ifndef SIGNALS_H
#define SIGNALS_H

void init_signals(void);
int was_sigint_received(void);
void reset_sigint_flag(void);

#endif
