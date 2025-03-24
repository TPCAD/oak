#ifndef OAK_INTERRUPT_H
#define OAK_INTERRUPT_H

#include <oak/types.h>

bool diable_interrupt();
bool get_interrupt_state();
void set_interrupt_state(bool state);

#endif // !OAK_INTERRUPT_H
