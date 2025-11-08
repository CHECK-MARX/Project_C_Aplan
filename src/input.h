#ifndef INPUT_H
#define INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "sim.h"

typedef enum
{
    INPUT_EVENT_KEY_DOWN = 0,
    INPUT_EVENT_KEY_UP = 1
} InputEventType;

void input_handle_key(SimState *sim, unsigned int virtual_key, InputEventType type, bool is_repeat);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_H */
