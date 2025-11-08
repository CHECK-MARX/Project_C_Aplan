#include "input.h"

#include <windows.h>

static bool is_toggle_press(bool is_down, bool is_repeat)
{
    return (is_down && !is_repeat);
}

void input_handle_key(SimState *sim, unsigned int virtual_key, InputEventType type, bool is_repeat)
{
    if (sim == NULL)
    {
        return;
    }

    const bool is_down = (type == INPUT_EVENT_KEY_DOWN);

    switch (virtual_key)
    {
        case VK_LEFT:
            if (is_toggle_press(is_down, is_repeat))
            {
                sim_toggle_left_signal(sim);
            }
            break;
        case VK_RIGHT:
            if (is_toggle_press(is_down, is_repeat))
            {
                sim_toggle_right_signal(sim);
            }
            break;
        case 'H':
            if (is_toggle_press(is_down, is_repeat))
            {
                sim_toggle_hazard(sim);
            }
            break;
        case 'L':
            if (is_toggle_press(is_down, is_repeat))
            {
                sim_toggle_headlight(sim);
            }
            break;
        case VK_UP:
            if (is_down)
            {
                sim_adjust_throttle(sim, 5.0);
            }
            break;
        case VK_DOWN:
            if (is_down)
            {
                sim_adjust_throttle(sim, -5.0);
            }
            break;
        case VK_SPACE:
            sim_apply_brake(sim, is_down);
            break;
        case 'A':
            if (is_toggle_press(is_down, is_repeat))
            {
                sim_toggle_ac(sim);
            }
            break;
        case 'F':
            if (is_toggle_press(is_down, is_repeat))
            {
                sim_cycle_fan(sim);
            }
            break;
        case 'R':
            if (is_toggle_press(is_down, is_repeat))
            {
                sim_toggle_recirc(sim);
            }
            break;
        case 'D':
            if (is_toggle_press(is_down, is_repeat))
            {
                sim_toggle_defrost(sim);
            }
            break;
        case 'O':
            if (is_toggle_press(is_down, is_repeat))
            {
                sim_toggle_auto(sim);
            }
            break;
        case 'M':
            if (is_toggle_press(is_down, is_repeat))
            {
                sim_cycle_airflow(sim);
            }
            break;
        case VK_ADD:
        case VK_OEM_PLUS:
            if (is_toggle_press(is_down, is_repeat))
            {
                sim_adjust_setpoint(sim, 0.5);
            }
            break;
        case VK_SUBTRACT:
        case VK_OEM_MINUS:
            if (is_toggle_press(is_down, is_repeat))
            {
                sim_adjust_setpoint(sim, -0.5);
            }
            break;
        default:
            break;
    }
}
