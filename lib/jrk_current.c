#include "jrk_internal.h"

// These tables are autogenerated by ruby/vilim_table.rb.

static const uint16_t jrk_umc04a_30v_vilim_table[32] =
{
  0,
  809,
  1528,
  2178,
  2776,
  3335,
  3865,
  4374,
  4868,
  5352,
  5833,
  6314,
  6799,
  7292,
  7797,
  8319,
  8861,
  9428,
  10025,
  10658,
  11331,
  12054,
  12833,
  13679,
  14604,
  15623,
  16752,
  18014,
  19437,
  21058,
  22923,
  25097,
};

static const uint16_t jrk_umc04a_40v_vilim_table[32] =
{
  0,
  1353,
  2592,
  3742,
  4821,
  5845,
  6826,
  7776,
  8703,
  9615,
  10519,
  11423,
  12333,
  13254,
  14192,
  15154,
  16146,
  17175,
  18247,
  19371,
  20555,
  21809,
  23143,
  24572,
  26109,
  27772,
  29583,
  31566,
  33752,
  36179,
  38894,
  41958,
};

static const uint16_t jrk_umc04a_30v_recommended_codes[] =
{
  95,  // VILIM = 1.569 V, Current limit = 75.93 A
  94,  // VILIM = 1.433 V, Current limit = 69.14 A
  93,  // VILIM = 1.316 V, Current limit = 63.31 A
  92,  // VILIM = 1.215 V, Current limit = 58.24 A
  91,  // VILIM = 1.126 V, Current limit = 53.79 A
  90,  // VILIM = 1.047 V, Current limit = 49.85 A
  89,  // VILIM = 0.976 V, Current limit = 46.32 A
  88,  // VILIM = 0.913 V, Current limit = 43.14 A
  87,  // VILIM = 0.855 V, Current limit = 40.25 A
  63,  // VILIM = 0.784 V, Current limit = 36.71 A
  62,  // VILIM = 0.716 V, Current limit = 33.32 A
  61,  // VILIM = 0.658 V, Current limit = 30.40 A
  60,  // VILIM = 0.607 V, Current limit = 27.87 A
  59,  // VILIM = 0.563 V, Current limit = 25.65 A
  58,  // VILIM = 0.524 V, Current limit = 23.68 A
  57,  // VILIM = 0.488 V, Current limit = 21.91 A
  56,  // VILIM = 0.456 V, Current limit = 20.32 A
  55,  // VILIM = 0.427 V, Current limit = 18.87 A
  31,  // VILIM = 0.392 V, Current limit = 17.11 A
  30,  // VILIM = 0.358 V, Current limit = 15.41 A
  29,  // VILIM = 0.329 V, Current limit = 13.95 A
  28,  // VILIM = 0.304 V, Current limit = 12.69 A
  27,  // VILIM = 0.281 V, Current limit = 11.57 A
  26,  // VILIM = 0.262 V, Current limit = 10.59 A
  25,  // VILIM = 0.244 V, Current limit =  9.71 A
  24,  // VILIM = 0.228 V, Current limit =  8.91 A
  23,  // VILIM = 0.214 V, Current limit =  8.19 A
  22,  // VILIM = 0.201 V, Current limit =  7.53 A
  21,  // VILIM = 0.188 V, Current limit =  6.92 A
  20,  // VILIM = 0.177 V, Current limit =  6.35 A
  19,  // VILIM = 0.167 V, Current limit =  5.83 A
  18,  // VILIM = 0.157 V, Current limit =  5.33 A
  17,  // VILIM = 0.147 V, Current limit =  4.87 A
  16,  // VILIM = 0.138 V, Current limit =  4.42 A
  15,  // VILIM = 0.130 V, Current limit =  4.00 A
  14,  // VILIM = 0.122 V, Current limit =  3.59 A
  13,  // VILIM = 0.114 V, Current limit =  3.20 A
  12,  // VILIM = 0.106 V, Current limit =  2.81 A
  11,  // VILIM = 0.099 V, Current limit =  2.43 A
  10,  // VILIM = 0.091 V, Current limit =  2.06 A
  9,   // VILIM = 0.084 V, Current limit =  1.68 A
  8,   // VILIM = 0.076 V, Current limit =  1.30 A
  7,   // VILIM = 0.068 V, Current limit =  0.92 A
  6,   // VILIM = 0.060 V, Current limit =  0.52 A
  5,   // VILIM = 0.052 V, Current limit =  0.11 A
  0,   // VILIM = 0.000 V, Current limit =  0.00 A
};

static const uint16_t jrk_umc04a_40v_recommended_codes[] =
{
  95,  // VILIM = 2.622 V, Current limit = 64.31 A
  94,  // VILIM = 2.431 V, Current limit = 59.52 A
  93,  // VILIM = 2.261 V, Current limit = 55.28 A
  92,  // VILIM = 2.110 V, Current limit = 51.49 A
  91,  // VILIM = 1.973 V, Current limit = 48.07 A
  90,  // VILIM = 1.849 V, Current limit = 44.97 A
  89,  // VILIM = 1.736 V, Current limit = 42.15 A
  88,  // VILIM = 1.632 V, Current limit = 39.55 A
  87,  // VILIM = 1.536 V, Current limit = 37.14 A
  86,  // VILIM = 1.446 V, Current limit = 34.91 A
  85,  // VILIM = 1.363 V, Current limit = 32.83 A
  63,  // VILIM = 1.311 V, Current limit = 31.53 A
  62,  // VILIM = 1.215 V, Current limit = 29.14 A
  61,  // VILIM = 1.131 V, Current limit = 27.01 A
  60,  // VILIM = 1.055 V, Current limit = 25.12 A
  59,  // VILIM = 0.986 V, Current limit = 23.41 A
  58,  // VILIM = 0.924 V, Current limit = 21.86 A
  57,  // VILIM = 0.868 V, Current limit = 20.45 A
  56,  // VILIM = 0.816 V, Current limit = 19.15 A
  55,  // VILIM = 0.768 V, Current limit = 17.95 A
  54,  // VILIM = 0.723 V, Current limit = 16.83 A
  31,  // VILIM = 0.656 V, Current limit = 15.14 A
  30,  // VILIM = 0.608 V, Current limit = 13.94 A
  29,  // VILIM = 0.565 V, Current limit = 12.88 A
  28,  // VILIM = 0.527 V, Current limit = 11.93 A
  27,  // VILIM = 0.493 V, Current limit = 11.08 A
  26,  // VILIM = 0.462 V, Current limit = 10.31 A
  25,  // VILIM = 0.434 V, Current limit =  9.60 A
  24,  // VILIM = 0.408 V, Current limit =  8.95 A
  23,  // VILIM = 0.384 V, Current limit =  8.35 A
  22,  // VILIM = 0.362 V, Current limit =  7.79 A
  21,  // VILIM = 0.341 V, Current limit =  7.27 A
  20,  // VILIM = 0.321 V, Current limit =  6.78 A
  19,  // VILIM = 0.303 V, Current limit =  6.32 A
  18,  // VILIM = 0.285 V, Current limit =  5.88 A
  17,  // VILIM = 0.268 V, Current limit =  5.46 A
  16,  // VILIM = 0.252 V, Current limit =  5.06 A
  15,  // VILIM = 0.237 V, Current limit =  4.67 A
  14,  // VILIM = 0.222 V, Current limit =  4.29 A
  13,  // VILIM = 0.207 V, Current limit =  3.93 A
  12,  // VILIM = 0.193 V, Current limit =  3.57 A
  11,  // VILIM = 0.178 V, Current limit =  3.21 A
  10,  // VILIM = 0.164 V, Current limit =  2.86 A
  9,   // VILIM = 0.150 V, Current limit =  2.51 A
  8,   // VILIM = 0.136 V, Current limit =  2.15 A
  7,   // VILIM = 0.122 V, Current limit =  1.79 A
  6,   // VILIM = 0.107 V, Current limit =  1.42 A
  5,   // VILIM = 0.091 V, Current limit =  1.03 A
  4,   // VILIM = 0.075 V, Current limit =  0.63 A
  3,   // VILIM = 0.058 V, Current limit =  0.21 A
  0,   // VILIM = 0.000 V, Current limit =  0.00 A
};

// Given a jrk product code and 5-bit DAC level, this returns the expected
// voltage we will see on the pin of the motor driver that sets the current
// limit.  The units are set such that 0x10000 is the DAC reference, and 0 is
// GND.  Autogenerated from ruby/current_table.rb.
static uint16_t jrk_get_vilim(uint32_t product, uint8_t dac_level)
{
  const uint16_t * table;
  if (product == JRK_PRODUCT_UMC04A_40V)
  {
    table = jrk_umc04a_40v_vilim_table;
  }
  else
  {
    table = jrk_umc04a_30v_vilim_table;
  }
  return table[dac_level & 0x1F];
}

static uint16_t * jrk_get_recommended_codes(uint32_t product)
{
  if (product == JRK_PRODUCT_UMC04A_40V)
  {
    return jrk_umc04a_40v_recommended_codes;
  }
  else
  {
    return jrk_umc04a_30v_recommended_codes;
  }
}

// Gets the rsense resistor value for the jrk, in units of milliohms.
static uint8_t jrk_get_rsense_mohm(uint32_t product)
{
  if (product == JRK_PRODUCT_UMC04A_40V)
  {
    return 2;
  }
  else
  {
    return 1;
  }
}

// Calculates the measured current in milliamps for a umc04a board.
//
// This is the same as the calculation that is done in the firmware to
// produce 'current' variable (see jrk_variables_get_current()).
//
// raw_current: From jrk_variables_get_raw_current().
// current_limit_code: The hardware current limiting configuration, from
//   from jrk_variables_get_max_current().
// rsense: Sense resistor resistance, in units of mohms.
// current_offset_calibration: from jrk_settings_get_current_offset_calibration()
// current_scale_calibration: from jrk_settings_get_current_scale_calibration()
static uint16_t jrk_calculate_measured_current_ma_umc04a(
  uint16_t raw_current,
  uint16_t current_limit_code,
  int16_t duty_cycle,
  uint8_t rsense,
  int16_t current_offset_calibration,
  int16_t current_scale_calibration
  )
{
  if (duty_cycle == 0)
  {
    return 0;
  }

  uint16_t offset = 800 + current_offset_calibration;
  uint16_t scale = 1875 + current_scale_calibration;

  uint8_t dac_ref = current_limit_code >> 5 & 3;

  // Convert the reading on the current sense line to units of mV/16.
  uint16_t current = raw_current >> ((2 - dac_ref) & 3);

  // Subtract the 50mV offset voltage, without making the reading negative.
  if (offset > current)
  {
    current = 0;
  }
  else
  {
    current -= offset;
  }

  uint16_t duty_cycle_unsigned;
  if (duty_cycle < 0)
  {
    duty_cycle_unsigned = -duty_cycle;
  }
  else
  {
    duty_cycle_unsigned = duty_cycle;
  }

  // Divide by the duty cycle and apply scaling factors to get the current in
  // milliamps.  The product will be at most 0xFFFF*(2*1875) = 0x0EA5F15A.
  uint32_t current32 = current * scale / (duty_cycle_unsigned * rsense);

  if (current32 > 0xFFFF)
  {
    return 0xFFFF;
  }
  else
  {
    return current32;
  }
}

uint32_t jrk_current_limit_code_to_ma(const jrk_settings * settings, uint16_t code)
{
  uint32_t product = jrk_settings_get_product(settings);
  if (!settings || !product) { return 0; }

  if (product == JRK_PRODUCT_UMC04A_30V || product == JRK_PRODUCT_UMC04A_40V)
  {
    // umc04a jrks ignore the top 8 bits and treat codes as zero if the top 3
    // bits are invalid.
    code &= 0xFF;
    if (code > 95) { code = 0; }

    return jrk_calculate_measured_current_ma_umc04a(
      jrk_get_vilim(product, code & 0x1F),
      code,
      600,
      jrk_get_rsense_mohm(product),
      jrk_settings_get_current_offset_calibration(settings),
      jrk_settings_get_current_scale_calibration(settings)
    );
  }

  return 0;
}

uint16_t jrk_current_limit_ma_to_code(const jrk_settings * settings, uint32_t ma)
{
  uint32_t product = jrk_settings_get_product(settings);
  if (product == 0) { return 0; }

  for (uint16_t * c = jrk_get_recommended_codes(product); *c; c++)
  {
    if (jrk_current_limit_code_to_ma(settings, *c) <= ma)
    {
      return *c;
    }
  }
  return 0;
}

uint32_t jrk_calculate_measured_current_ma(
  const jrk_settings * settings, const jrk_variables * vars)
{
  uint32_t product = jrk_settings_get_product(settings);
  if (!product || !vars) { return 0; }

  if (product == JRK_PRODUCT_UMC04A_30V || product == JRK_PRODUCT_UMC04A_40V)
  {
    uint16_t firmware_calculated_current = jrk_variables_get_current(vars);

    // Enable this code if you want to compare the firmware currrent calculation
    // to the software one and detect mismatches.
    if (0)
    {
      uint16_t software_calculated_current =
        jrk_calculate_measured_current_ma_umc04a(
          jrk_variables_get_raw_current(vars),
          jrk_variables_get_current_limit_code(vars),
          jrk_variables_get_last_duty_cycle(vars),
          jrk_get_rsense_mohm(product),
          jrk_settings_get_current_offset_calibration(settings),
          jrk_settings_get_current_scale_calibration(settings)
          );

      if (firmware_calculated_current != software_calculated_current)
      {
        // The only reason this should happen is if there is a bug in the firmware
        // or software calculation.
        fprintf(stderr, "warning: current calculation mismatch: %u != %u\n",
          firmware_calculated_current, software_calculated_current);
        fflush(stderr);
      }
    }

    return firmware_calculated_current;
  }

  return 0;
}

uint32_t jrk_calculate_raw_current_mv64(
  const jrk_settings * settings, const jrk_variables * vars)
{
  uint32_t product = jrk_settings_get_product(settings);
  if (!product || !vars) { return 0; }

  if (product == JRK_PRODUCT_UMC04A_30V || product == JRK_PRODUCT_UMC04A_40V)
  {
    uint16_t current = jrk_variables_get_raw_current(vars);
    uint8_t dac_ref = jrk_variables_get_current_limit_code(vars) >> 5 & 3;
    return current << dac_ref;
  }

  return 0;
}
