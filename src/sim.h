#ifndef SIM_H
#define SIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    HVAC_AIRFLOW_FACE = 0,
    HVAC_AIRFLOW_BI_LEVEL = 1,
    HVAC_AIRFLOW_FOOT = 2
} HvacAirflowMode;

typedef struct
{
    bool left_enabled;
    bool right_enabled;
    bool hazard_enabled;
    bool headlight_on;
    bool blink_on;
    double blink_elapsed;
} IndicatorState;

typedef struct
{
    bool ac_on;
    bool auto_mode;
    bool recirculation_on;
    bool defrost_on;
    HvacAirflowMode airflow_mode;
    int fan_level;
    double setpoint_c;
    double cabin_temp_c;
    double outside_temp_c;
    double warmup_elapsed_s;
    double rpm_hot_s;
    bool engine_warm;
} HvacState;

typedef struct
{
    double velocity_kmh;
    double throttle_pct;
    double brake_pct;
    double rpm;
    double fuel_pct;
    double runtime_s;
    IndicatorState indicators;
    HvacState hvac;
} SimState;

void sim_init(SimState *state);
void sim_step(SimState *state, double dt);
void sim_toggle_left_signal(SimState *state);
void sim_toggle_right_signal(SimState *state);
void sim_toggle_hazard(SimState *state);
void sim_toggle_headlight(SimState *state);
void sim_adjust_throttle(SimState *state, double delta_pct);
void sim_apply_brake(SimState *state, bool pressed);
void sim_cycle_fan(SimState *state);
void sim_cycle_airflow(SimState *state);
void sim_toggle_ac(SimState *state);
void sim_toggle_recirc(SimState *state);
void sim_toggle_defrost(SimState *state);
void sim_toggle_auto(SimState *state);
void sim_adjust_setpoint(SimState *state, double delta_c);

#ifdef __cplusplus
}
#endif

#endif /* SIM_H */
