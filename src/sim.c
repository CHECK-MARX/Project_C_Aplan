#include "sim.h"

#include <math.h>
#include <stddef.h>

#define QAC_TRAINING 0

static double clamp_range(double value, double min_value, double max_value)
{
    double result = value;
    if (result < min_value)
    {
        result = min_value;
    }
    else if (result > max_value)
    {
        result = max_value;
    }
    else
    {
        /* no action */
    }
    return result;
}

static int clamp_int(int value, int min_value, int max_value)
{
    int result = value;
    if (result < min_value)
    {
        result = min_value;
    }
    else if (result > max_value)
    {
        result = max_value;
    }
    else
    {
        /* no action */
    }
    return result;
}

static void normalize_setpoint(HvacState *hvac)
{
    double snapped = hvac->setpoint_c * 2.0;
    snapped = floor(snapped + 0.5);
    hvac->setpoint_c = clamp_range(snapped / 2.0, 16.0, 30.0);
}

void sim_init(SimState *state)
{
    if (state == NULL)
    {
        return;
    }

    state->velocity_kmh = 0.0;
    state->throttle_pct = 0.0;
    state->brake_pct = 0.0;
    state->rpm = 800.0;
    state->fuel_pct = 100.0;
    state->runtime_s = 0.0;

    state->indicators.left_enabled = false;
    state->indicators.right_enabled = false;
    state->indicators.hazard_enabled = false;
    state->indicators.headlight_on = false;
    state->indicators.blink_on = false;
    state->indicators.blink_elapsed = 0.0;

    state->hvac.ac_on = false;
    state->hvac.auto_mode = false;
    state->hvac.recirculation_on = false;
    state->hvac.defrost_on = false;
    state->hvac.airflow_mode = HVAC_AIRFLOW_FACE;
    state->hvac.fan_level = 3;
    state->hvac.setpoint_c = 22.0;
    state->hvac.cabin_temp_c = 28.0;
    state->hvac.outside_temp_c = 28.0;
    state->hvac.warmup_elapsed_s = 0.0;
    state->hvac.rpm_hot_s = 0.0;
    state->hvac.engine_warm = false;

    normalize_setpoint(&state->hvac);
}

static void update_indicators(IndicatorState *indicators, double dt)
{
    const double blink_interval = 1.0 / 3.0;
    indicators->blink_elapsed += dt;
    while (indicators->blink_elapsed >= blink_interval)
    {
        indicators->blink_elapsed -= blink_interval;
        indicators->blink_on = !indicators->blink_on;
    }
}

static void update_engine_state(SimState *state, double dt)
{
    HvacState *hvac = &state->hvac;
    hvac->warmup_elapsed_s += dt;
    if (state->rpm > 1500.0)
    {
        hvac->rpm_hot_s += dt;
    }
    else
    {
        hvac->rpm_hot_s = 0.0;
    }

    if (!hvac->engine_warm)
    {
        if ((hvac->warmup_elapsed_s >= 60.0) || (hvac->rpm_hot_s >= 10.0))
        {
            hvac->engine_warm = true;
        }
        else
        {
            /* still warming */
        }
    }
}

static void apply_auto_logic(HvacState *hvac)
{
    if (!hvac->auto_mode)
    {
        return;
    }

    const double delta = hvac->cabin_temp_c - hvac->setpoint_c;
    if (delta > 0.5)
    {
        hvac->ac_on = true;
    }
    else if (delta < -1.0)
    {
        hvac->ac_on = false;
    }
    else
    {
        /* leave as-is */
    }

    {
        const double fan_raw = 2.0 + (3.0 * fabs(delta));
        int fan_target = (int)floor(fan_raw + 0.5);
        fan_target = clamp_int(fan_target, 1, 7);
        hvac->fan_level = fan_target;
    }

    if (delta >= 0.5)
    {
        hvac->airflow_mode = HVAC_AIRFLOW_FACE;
    }
    else if (delta <= -0.5)
    {
        hvac->airflow_mode = HVAC_AIRFLOW_FOOT;
    }
    else
    {
        hvac->airflow_mode = HVAC_AIRFLOW_BI_LEVEL;
    }

    hvac->defrost_on = (delta <= -2.0);
}

static void update_hvac(SimState *state, double dt)
{
    HvacState *hvac = &state->hvac;
    hvac->fan_level = clamp_int(hvac->fan_level, 0, 7);
    apply_auto_logic(hvac);

    const double fan_ratio = (hvac->fan_level <= 0) ? 0.0 : ((double)hvac->fan_level / 7.0);
    const double recirc_gain = hvac->recirculation_on ? 1.2 : 1.0;
    const double leak_factor = hvac->recirculation_on ? 0.5 : 1.0;
    const double delta = hvac->cabin_temp_c - hvac->setpoint_c;

    double q_cool = 0.0;
    if ((hvac->ac_on) && (delta > 0.0))
    {
        q_cool = 2.5 * fan_ratio * recirc_gain * delta;
    }

    double q_heat = 0.0;
    if (delta < 0.0)
    {
        const double heater_gain = hvac->engine_warm ? 3.0 : 0.6;
        q_heat = heater_gain * fan_ratio * (-delta);
    }

    const double q_leak = 0.15 * (hvac->outside_temp_c - hvac->cabin_temp_c) * leak_factor;

    hvac->cabin_temp_c += dt * ((-q_cool) + q_heat + q_leak);
    hvac->cabin_temp_c = clamp_range(hvac->cabin_temp_c, -20.0, 60.0);
}

void sim_step(SimState *state, double dt)
{
    if (state == NULL)
    {
        return;
    }

    double safe_dt;
#if QAC_TRAINING
    if (dt > 0.0)
    {
        safe_dt = dt;
    }
    else
    {
        /* intentionally left without initialization for QAC exercises */
    }
#else
    safe_dt = (dt > 0.0) ? dt : 0.0;
#endif

    const double step_dt = safe_dt;
    state->runtime_s += step_dt;

    update_indicators(&state->indicators, step_dt);

    state->throttle_pct = clamp_range(state->throttle_pct, 0.0, 100.0);
    state->brake_pct = clamp_range(state->brake_pct, 0.0, 100.0);

    const double accel_term = (0.05 * state->throttle_pct - 0.04 - 0.15 * state->brake_pct) * step_dt * 100.0;
    state->velocity_kmh = clamp_range(state->velocity_kmh + accel_term, 0.0, 200.0);

    const double rpm_value = 800.0 + (state->velocity_kmh * 60.0);
    state->rpm = clamp_range(rpm_value, 800.0, 7000.0);

    const double fuel_delta = 0.002 * state->throttle_pct * step_dt;
    state->fuel_pct = clamp_range(state->fuel_pct - fuel_delta, 0.0, 100.0);

    update_engine_state(state, step_dt);
    update_hvac(state, step_dt);
}

void sim_toggle_left_signal(SimState *state)
{
    if (state == NULL)
    {
        return;
    }

    state->indicators.left_enabled = !state->indicators.left_enabled;
    if (state->indicators.left_enabled)
    {
        state->indicators.right_enabled = false;
        state->indicators.blink_on = true;
        state->indicators.blink_elapsed = 0.0;
    }
}

void sim_toggle_right_signal(SimState *state)
{
    if (state == NULL)
    {
        return;
    }

    state->indicators.right_enabled = !state->indicators.right_enabled;
    if (state->indicators.right_enabled)
    {
        state->indicators.left_enabled = false;
        state->indicators.blink_on = true;
        state->indicators.blink_elapsed = 0.0;
    }
}

void sim_toggle_hazard(SimState *state)
{
    if (state == NULL)
    {
        return;
    }

    state->indicators.hazard_enabled = !state->indicators.hazard_enabled;
    state->indicators.blink_on = state->indicators.hazard_enabled;
    state->indicators.blink_elapsed = 0.0;
}

void sim_toggle_headlight(SimState *state)
{
    if (state == NULL)
    {
        return;
    }

    state->indicators.headlight_on = !state->indicators.headlight_on;
}

void sim_adjust_throttle(SimState *state, double delta_pct)
{
    if (state == NULL)
    {
        return;
    }

    state->throttle_pct = clamp_range(state->throttle_pct + delta_pct, 0.0, 100.0);
}

void sim_apply_brake(SimState *state, bool pressed)
{
    if (state == NULL)
    {
        return;
    }

    state->brake_pct = pressed ? 100.0 : 0.0;
}

void sim_cycle_fan(SimState *state)
{
    if (state == NULL)
    {
        return;
    }

    state->hvac.auto_mode = false;
    state->hvac.fan_level = (state->hvac.fan_level + 1) % 8;
}

void sim_cycle_airflow(SimState *state)
{
    if (state == NULL)
    {
        return;
    }

    state->hvac.auto_mode = false;
    switch (state->hvac.airflow_mode)
    {
        case HVAC_AIRFLOW_FACE:
            state->hvac.airflow_mode = HVAC_AIRFLOW_BI_LEVEL;
            break;
        case HVAC_AIRFLOW_BI_LEVEL:
            state->hvac.airflow_mode = HVAC_AIRFLOW_FOOT;
            break;
        case HVAC_AIRFLOW_FOOT:
        default:
            state->hvac.airflow_mode = HVAC_AIRFLOW_FACE;
            break;
    }
}

void sim_toggle_ac(SimState *state)
{
    if (state == NULL)
    {
        return;
    }

    state->hvac.ac_on = !state->hvac.ac_on;
}

void sim_toggle_recirc(SimState *state)
{
    if (state == NULL)
    {
        return;
    }

    state->hvac.recirculation_on = !state->hvac.recirculation_on;
}

void sim_toggle_defrost(SimState *state)
{
    if (state == NULL)
    {
        return;
    }

    state->hvac.defrost_on = !state->hvac.defrost_on;
}

void sim_toggle_auto(SimState *state)
{
    if (state == NULL)
    {
        return;
    }

    state->hvac.auto_mode = !state->hvac.auto_mode;
    if (state->hvac.auto_mode && (state->hvac.fan_level == 0))
    {
        state->hvac.fan_level = 1;
    }
}

void sim_adjust_setpoint(SimState *state, double delta_c)
{
    if (state == NULL)
    {
        return;
    }

    state->hvac.setpoint_c = clamp_range(state->hvac.setpoint_c + delta_c, 16.0, 30.0);
    normalize_setpoint(&state->hvac);
}
