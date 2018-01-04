#include "main_controller.h"
#include "main_window.h"
#include <file_util.h>

#include <cassert>
#include <cmath>
#include <iostream>  // tmphax

// This is how often we fetch the variables from the device.
static const uint32_t UPDATE_INTERVAL_MS = 50;

// Only update the device list once per second to save CPU time.
static const uint32_t UPDATE_DEVICE_LIST_DIVIDER = 20;

void main_controller::set_window(main_window * window)
{
  this->window = window;
}

void main_controller::start()
{
  assert(!connected());

  // Start the update timer so that update() will be called regularly.
  window->set_update_timer_interval(UPDATE_INTERVAL_MS);
  window->start_update_timer();

  handle_model_changed();
}

// Returns the device that matches the specified OS ID from the list, or a null
// device if none match.
static jrk::device device_with_os_id(
  std::vector<jrk::device> const & device_list,
  std::string const & id)
{
  for (jrk::device const & candidate : device_list)
  {
    if (candidate.get_os_id() == id)
    {
      return candidate;
    }
  }
  return jrk::device(); // null device
}

void main_controller::connect_device_with_os_id(const std::string & id)
{
  if (disconnect_device())
  {
    if (id.size() != 0)
    {
      connect_device(device_with_os_id(device_list, id));
    }
  }
  else
  {
    // User canceled disconnect when prompted about settings that have not been
    // applied. Reset the 'Connected to' dropdown.
    handle_model_changed();
  }
}

bool main_controller::disconnect_device()
{
  if (!connected()) { return true; }

  if (settings_modified)
  {
    std::string question =
      "The settings you changed have not been applied to the device.  "
      "If you disconnect from the device now, those changes will be lost.  "
      "Are you sure you want to disconnect?";
    if (!window->confirm(question))
    {
      return false;
    }
  }

  really_disconnect();
  disconnected_by_user = true;
  connection_error = false;
  handle_model_changed();
  return true;
}

void main_controller::connect_device(jrk::device const & device)
{
  assert(device);

  try
  {
    // Close the old handle in case one is already open.
    device_handle.close();

    connection_error = false;
    disconnected_by_user = false;

    // Open a handle to the specified device.
    device_handle = jrk::handle(device);
  }
  catch (std::exception const & e)
  {
    set_connection_error("Failed to connect to device.");
    show_exception(e, "There was an error connecting to the device.");
    handle_model_changed();
    return;
  }

  try
  {
    settings = device_handle.get_settings();
    handle_settings_loaded();
  }
  catch (std::exception const & e)
  {
    show_exception(e, "There was an error loading settings from the device.");
  }

  // TODO: call window->set_motor_asymmetric() ?

  handle_model_changed();
}

void main_controller::disconnect_device_by_error(std::string const & error_message)
{
  really_disconnect();
  disconnected_by_user = false;
  set_connection_error(error_message);
}

void main_controller::really_disconnect()
{
  device_handle.close();
  settings_modified = false;
}

void main_controller::set_connection_error(std::string const & error_message)
{
  connection_error = true;
  connection_error_message = error_message;
}

void main_controller::reload_settings(bool ask)
{
  if (!connected()) { return; }

  std::string question = "Are you sure you want to reload settings from the "
    "device and discard your recent changes?";
  if (ask && !window->confirm(question))
  {
    return;
  }

  try
  {
    settings = device_handle.get_settings();
    handle_settings_loaded();
  }
  catch (std::exception const & e)
  {
    settings_modified = true;
    show_exception(e, "There was an error loading the settings from the device.");
  }
  handle_settings_changed();
}

void main_controller::restore_default_settings()
{
  if (!connected()) { return; }

  std::string question = "This will reset all of your device's settings "
    "back to their default values.  "
    "You will lose your custom settings.  "
    "Are you sure you want to continue?";
  if (!window->confirm(question))
  {
    return;
  }

  bool restore_success = false;
  try
  {
    device_handle.restore_defaults();
    restore_success = true;
  }
  catch (std::exception const & e)
  {
    show_exception(e);
  }

  // This takes care of reloading the settings and telling the view to update.
  reload_settings(false);

  if (restore_success)
  {
    window->show_info_message(
      "Your device's settings have been reset to their default values.");
  }
}

void main_controller::upgrade_firmware()
{
  if (connected())
  {
    std::string question =
      "This action will restart the device in bootloader mode, which "
      "is used for firmware upgrades.  The device will disconnect "
      "and reappear to your system as a new device.\n\n"
      "Are you sure you want to proceed?";
    if (!window->confirm(question))
    {
      return;
    }

    really_disconnect();
    disconnected_by_user = true;
    connection_error = false;
    handle_model_changed();
  }

}

// Returns true if the device list includes the specified device.
static bool device_list_includes(
  std::vector<jrk::device> const & device_list,
  jrk::device const & device)
{
  return device_with_os_id(device_list, device.get_os_id());
}

void main_controller::update()
{
  // This is called regularly by the view when it is time to check for
  // updates to the state of USB devices.  This runs on the same thread as
  // everything else, so we should be careful not to do anything too slow
  // here.  If the user tries to use the UI at all while this function is
  // running, the UI cannot respond until this function returns.

  bool successfully_updated_list = false;
  if (--update_device_list_counter == 0)
  {
    update_device_list_counter = UPDATE_DEVICE_LIST_DIVIDER;

    successfully_updated_list = update_device_list();
    if (successfully_updated_list && device_list_changed)
    {
      window->set_device_list_contents(device_list);
      if (connected())
      {
        window->set_device_list_selected(device_handle.get_device());
      }
      else
      {
        window->set_device_list_selected(jrk::device()); // show "Not connected"
      }
    }
  }

  if (connected())
  {
    // First, see if the device we are connected to is still available.
    // Note: It would be nice if the libusbp::generic_handle class had a
    // function that tests if the actual handle we are using is still valid.
    // This would be better for tricky cases like if someone unplugs and
    // plugs the same device in very fast.
    bool device_still_present = device_list_includes(
      device_list, device_handle.get_device());

    if (device_still_present)
    {
      // Reload the variables from the device.
      try
      {
        // Reset command timeout AFTER reloading the variables so we can
        // indicate an active error if the command timeout interval is shorter
        // than the interval between calls to update().
        reload_variables();
      }
      catch (std::exception const & e)
      {
        // Ignore the exception.  The model provides other ways to tell that
        // the variable update failed, and the exact message is probably
        // not that useful since it is probably just a generic problem with
        // the USB connection.
      }
      handle_variables_changed();
    }
    else
    {
      // The device is gone.
      disconnect_device_by_error("The connection to the device was lost.");
      handle_model_changed();
    }
  }
  else
  {
    // We are not connected, so consider auto-connecting to a device.

    if (connection_error)
    {
      // There is an error related to a previous connection or connection
      // attempt, so don't automatically reconnect.  That would be
      // confusing, because the user might be looking away and not notice
      // that the connection was lost and then regained, or they could be
      // trying to read the error message.
    }
    else if (disconnected_by_user)
    {
      // The user explicitly disconnected the last connection, so don't
      // automatically reconnect.
    }
    else if (successfully_updated_list && (device_list.size() == 1))
    {
      // Automatically connect if there is only one device and we were not
      // recently disconnected from a device.
      connect_device(device_list.at(0));
    }
  }
}

bool main_controller::exit()
{
  if (connected() && settings_modified)
  {
    std::string question =
      "The settings you changed have not been applied to the device.  "
      "If you exit now, those changes will be lost.  "
      "Are you sure you want to exit?";
    return window->confirm(question);
  }
  else
  {
    return true;
  }
}

static bool device_lists_different(std::vector<jrk::device> const & list1,
  std::vector<jrk::device> const & list2)
{
  if (list1.size() != list2.size())
  {
    return true;
  }
  else
  {
    for (int i = 0; i < list1.size(); i++)
    {
      if (list1.at(i).get_os_id() != list2.at(i).get_os_id())
      {
        return true;
      }
    }
    return false;
  }
}

bool main_controller::update_device_list()
{
  try
  {
    std::vector<jrk::device> new_device_list = jrk::list_connected_devices();
    if (device_lists_different(device_list, new_device_list))
    {
      device_list_changed = true;
    }
    else
    {
      device_list_changed = false;
    }
    device_list = new_device_list;
    return true;
  }
  catch (std::exception const & e)
  {
    set_connection_error("Failed to get the list of devices.");
    show_exception(e, "There was an error getting the list of devices.");
    return false;
  }
}

bool main_controller::do_motor_direction_detect()
{
  int scaled_feedback_stop, scaled_feedback_forward;
  bool inverted = false;

  // require that the feedback is close to its midpoint before proceeding
  while(true)
  {
    scaled_feedback_stop = variables.get_scaled_feedback();

    if (scaled_feedback_stop >= 1024 && scaled_feedback_stop <= 2047 + 1024)
        break;

    if (!window->confirm("Center the output, then click OK."))
        return false;
  }

  int diff;
  int factor = 1;
  if (scaled_feedback_stop >= 2048)
    factor = -1; // go downward instead of upward so that we have more range

  // step up +300, trying to move the motor
  for (diff = 32; diff <= 4095; diff += 32)
  {
    uint16_t target = (uint16_t)(2048 + diff * factor);
    if (target > 4095)
      target = 4095; // since 2048 + 2048 = 4096.
    device_handle.set_target(target);
    scaled_feedback_forward = variables.get_scaled_feedback();
    if (scaled_feedback_forward > scaled_feedback_stop + 100)
    {
      if (factor == -1)
          inverted = true; // because we were trying to move downward
      break;
    }
    if (scaled_feedback_forward < scaled_feedback_stop - 100)
    {
      if (factor == 1)
          inverted = true; // because we were trying to move upward
      break;
    }
  }

  int duty_cycle = variables.get_duty_cycle();;

  device_handle.stop_motor();

  if (diff > 4095) // we didn't break out of the loop
  {
    // window->show_error_message("Error detecting motor direction:  Driving the motor with a duty cyle of "
    //     + duty_cycle + " did not change the measured feedback. "
    //     + " Check your motor and feedback connections.");
    return false;
  }

  // now we can set the checkbox
  bool previously_checked = settings.get_motor_invert();
  if(settings.get_motor_invert())
  {
    settings.set_motor_invert(!inverted); // since it responds backwards when it is already inverted
  }
  else
  {
    settings.set_motor_invert(inverted);
  }

  // display a nice message
  if (previously_checked && !settings.get_motor_invert())
  {
    window->show_error_message("The motor is NOT inverted.  Please click the apply button to apply this setting to the jrk.");
    return true;
  }
  else if (!previously_checked && settings.get_motor_invert())
  {
    window->show_info_message("The motor is inverted.  Please click the apply button to apply this setting to the jrk.");
    return true;
  }

  return false;
}

void main_controller::show_exception(std::exception const & e,
    std::string const & context)
{
    std::string message;
    if (context.size() > 0)
    {
      message += context;
      message += "  ";
    }
    message += e.what();
    window->show_error_message(message);
}

void main_controller::handle_model_changed()
{
  handle_device_changed();
  handle_variables_changed();
  handle_settings_changed();
}

void main_controller::handle_device_changed()
{
  if (connected())
  {
    const jrk::device & device = device_handle.get_device();

    window->set_device_list_selected(device);

    // TODO:
    // window->set_device_name(jrk_look_up_product_name_ui(device.get_product()), true);
    // window->set_serial_number(device.get_serial_number());
    window->set_firmware_version(device_handle.get_firmware_version_string());
    window->set_device_reset(
      jrk_look_up_device_reset_name_ui(variables.get_device_reset()));
    // window->set_connection_status("", false);

    // window->reset_error_counts();

  }
  else
  {
    window->set_device_list_selected(jrk::device()); // show "Not connected"

    std::string na = "N/A";
    //TODO:
    // window->set_device_name(na, false);
    // window->set_serial_number(na);
    window->set_firmware_version(na);

    // if (connection_error)
    // {
    //   window->set_connection_status(connection_error_message, true);
    // }
    // else
    // {
    //   window->set_connection_status("", false);
    // }
  }

  window->set_disconnect_enabled(connected());
  window->set_open_save_settings_enabled(connected());
  window->set_reload_settings_enabled(connected());
  window->set_restore_defaults_enabled(connected());
  window->set_stop_motor_enabled(connected());
  window->set_run_motor_enabled(connected());
  window->set_tab_pages_enabled(connected());
}

void main_controller::handle_variables_changed()
{
  window->set_up_time(variables.get_up_time());
  window->set_duty_cycle(variables.get_duty_cycle());
  window->set_scaled_current_mv(
    variables.get_scaled_current_mv(window->get_current_offset_mv()));
  window->set_raw_current_mv(variables.get_raw_current_mv());
  window->set_current_chopping_log(variables.get_current_chopping_log());
  window->set_vin_voltage(variables.get_vin_voltage());
  window->set_error_flags_halting(variables.get_error_flags_halting());
  // TODO: window->increment_errors_occurred(variables.get_errors_occurred());
}

void main_controller::handle_settings_changed()
{
  // [all-settings]


  window->set_input_mode(settings.get_input_mode());
  window->set_input_analog_samples_exponent(settings.get_input_analog_samples_exponent());
  window->set_input_detect_disconnect(settings.get_input_detect_disconnect());
  window->set_input_serial_mode(settings.get_serial_mode());
  window->set_input_baud_rate(settings.get_serial_baud_rate());
  window->set_input_enable_crc(settings.get_serial_enable_crc());
  window->set_input_device_number(settings.get_serial_device_number());
  window->set_input_enable_device_number(settings.get_serial_enable_14bit_device_number());
  window->set_input_serial_timeout(settings.get_serial_timeout());
  window->set_input_compact_protocol(settings.get_serial_disable_compact_protocol());
  window->set_input_never_sleep(settings.get_never_sleep());
  window->set_input_invert(settings.get_input_invert());
  window->set_input_absolute_minimum(settings.get_input_absolute_minimum());
  window->set_input_absolute_maximum(settings.get_input_absolute_maximum());
  window->set_input_minimum(settings.get_input_minimum());
  window->set_input_maximum(settings.get_input_maximum());
  window->set_input_neutral_minimum(settings.get_input_neutral_minimum());
  window->set_input_neutral_maximum(settings.get_input_neutral_maximum());
  window->set_input_output_minimum(settings.get_output_minimum());
  window->set_input_output_neutral(settings.get_output_neutral());
  window->set_input_output_maximum(settings.get_output_maximum());
  window->set_input_scaling_degree(settings.get_input_scaling_degree());
  window->set_input_scaling_order_warning_label();
  window->set_feedback_mode(settings.get_feedback_mode());
  window->set_feedback_invert(settings.get_feedback_invert());
  window->set_feedback_absolute_minimum(settings.get_feedback_absolute_minimum());
  window->set_feedback_absolute_maximum(settings.get_feedback_absolute_maximum());
  window->set_feedback_maximum(settings.get_feedback_maximum());
  window->set_feedback_minimum(settings.get_feedback_minimum());
  window->set_feedback_scaling_order_warning_label();
  window->set_feedback_mode(settings.get_feedback_mode());
  window->set_feedback_analog_samples_exponent(settings.get_feedback_analog_samples_exponent());

  window->set_pid_period(settings.get_pid_period());
  window->set_pid_integral_limit(settings.get_pid_integral_limit());
  window->set_pid_reset_integral(settings.get_pid_reset_integral());
  window->set_feedback_dead_zone(settings.get_feedback_dead_zone());

  window->set_motor_pwm_frequency(settings.get_motor_pwm_frequency());
  window->set_motor_invert(settings.get_motor_invert());

  window->set_motor_max_duty_cycle_out_of_range(settings.get_motor_max_duty_cycle_while_feedback_out_of_range());
  window->set_motor_coast_when_off(settings.get_motor_coast_when_off());
  window->set_motor_invert(settings.get_motor_invert());

  window->set_motor_asymmetric(motor_asymmetric);

  window->set_motor_max_duty_cycle_reverse(settings.get_motor_max_duty_cycle_reverse());
  window->set_motor_max_acceleration_reverse(settings.get_motor_max_acceleration_reverse());
  window->set_motor_max_deceleration_reverse(settings.get_motor_max_deceleration_reverse());
  window->set_motor_brake_duration_reverse(settings.get_motor_brake_duration_reverse());
  window->set_motor_max_current_reverse(settings.get_motor_max_current_reverse());
  window->set_motor_current_calibration_reverse(settings.get_motor_current_calibration_reverse());

  window->set_motor_max_duty_cycle_forward(settings.get_motor_max_duty_cycle_forward());
  window->set_motor_max_acceleration_forward(settings.get_motor_max_acceleration_forward());
  window->set_motor_max_deceleration_forward(settings.get_motor_max_deceleration_forward());
  window->set_motor_brake_duration_forward(settings.get_motor_brake_duration_forward());
  window->set_motor_max_current_forward(settings.get_motor_max_current_forward());
  window->set_motor_current_calibration_forward(settings.get_motor_current_calibration_forward());



  window->set_apply_settings_enabled(connected() && settings_modified);
}

void main_controller::handle_settings_loaded()
{
  recalculate_motor_asymmetric();

  for (int i = 0; i < 3; ++i)
  {
    recalculate_pid_coefficients(i);
  }

  cached_settings = settings;

  settings_modified = false;
}

void main_controller::recalculate_pid_coefficients(int index)
{
  if (!connected()) { return; }
  double multiplier;
  uint16_t exponent;
  switch (index)
  {
    case 0:
      multiplier = settings.get_proportional_multiplier();
      exponent = settings.get_proportional_exponent();
      window->set_pid_multiplier(index, multiplier);
      window->set_pid_exponent(index, exponent);
      for (int i = 0; i < exponent; ++i)
      {
        multiplier /= 2;
      }
      window->set_pid_constant(index, multiplier);
      break;
    case 1:
      multiplier = settings.get_integral_multiplier();
      exponent = settings.get_integral_exponent();
      window->set_pid_multiplier(index, multiplier);
      window->set_pid_exponent(index, exponent);
      for (int i = 0; i < exponent; ++i)
      {
        multiplier /= 2;
      }
      window->set_pid_constant(index, multiplier);
      break;
    case 2:
      multiplier = settings.get_derivative_multiplier();
      exponent = settings.get_derivative_exponent();
      window->set_pid_multiplier(index, multiplier);
      window->set_pid_exponent(index, exponent);
      for (int i = 0; i < exponent; ++i)
      {
        multiplier /= 2;
      }
      window->set_pid_constant(index, multiplier);
      break;
    default:
      return;
  }

  // settings_modified = true;
  // handle_settings_changed();
}

void main_controller::recalculate_motor_asymmetric()
{
  motor_asymmetric =
    (settings.get_motor_max_duty_cycle_forward() !=
      settings.get_motor_max_duty_cycle_reverse()) ||
    (settings.get_motor_max_acceleration_forward() !=
      settings.get_motor_max_acceleration_reverse()) ||
    (settings.get_motor_max_deceleration_forward() !=
      settings.get_motor_max_deceleration_reverse()) ||
    (settings.get_motor_brake_duration_forward() !=
      settings.get_motor_brake_duration_reverse()) ||
    (settings.get_motor_max_current_forward() !=
      settings.get_motor_max_current_reverse()) ||
    (settings.get_motor_current_calibration_forward() !=
      settings.get_motor_current_calibration_reverse());
}

void main_controller::handle_input_mode_input(uint8_t input_mode)
{
  if (!connected()) { return; }
  settings.set_input_mode(input_mode);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_analog_samples_input(uint8_t input_analog_samples)
{
  if (!connected()) { return; }
  settings.set_input_analog_samples_exponent(input_analog_samples);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_detect_disconnect_input(bool detect_disconnect)
{
  if (!connected()) { return; }
  settings.set_input_detect_disconnect(detect_disconnect);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_serial_mode_input(uint8_t value)
{
  if (!connected()) { return; }
  settings.set_serial_mode(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_uart_fixed_baud_input(uint32_t value)
{
  if (!connected()) { return; }
  settings.set_serial_baud_rate(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_enable_crc_input(bool value)
{
  if (!connected()) { return; }
  settings.set_serial_enable_crc(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_device_input(uint16_t value)
{
  if (!connected()) { return; }
  settings.set_serial_device_number(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_device_number_input(bool value)
{
  if (!connected()) { return; }
  settings.set_serial_enable_14bit_device_number(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_timeout_input(uint16_t value)
{
  if (!connected()) { return; }
  settings.set_serial_timeout(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_disable_compact_protocol_input(bool value)
{
  if (!connected()) { return; }
  settings.set_serial_disable_compact_protocol(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_never_sleep_input(bool value)
{
  if (!connected()) { return; }
  settings.set_never_sleep(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_invert_input(bool input_invert)
{
  if (!connected()) { return; }
  settings.set_input_invert(input_invert);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_absolute_minimum_input(uint16_t input_absolute_minimum)
{
  if (!connected()) { return; }
  settings.set_input_absolute_minimum(input_absolute_minimum);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_absolute_maximum_input(uint16_t input_absolute_maximum)
{
  if (!connected()) { return; }
  settings.set_input_absolute_maximum(input_absolute_maximum);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_minimum_input(uint16_t input_minimum)
{
  if (!connected()) { return; }
  settings.set_input_minimum(input_minimum);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_maximum_input(uint16_t input_maximum)
{
  if (!connected()) { return; }
  settings.set_input_maximum(input_maximum);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_neutral_minimum_input(uint16_t input_neutral_minimum)
{
  if (!connected()) { return; }
  settings.set_input_neutral_minimum(input_neutral_minimum);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_neutral_maximum_input(uint16_t input_neutral_maximum)
{
  if (!connected()) { return; }
  settings.set_input_neutral_maximum(input_neutral_maximum);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_output_minimum_input(uint16_t output_minimum)
{
  if (!connected()) { return; }
  settings.set_output_minimum(output_minimum);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_output_neutral_input(uint16_t output_neutral)
{
  if (!connected()) { return; }
  settings.set_output_neutral(output_neutral);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_output_maximum_input(uint16_t output_maximum)
{
  if (!connected()) { return; }
  settings.set_output_maximum(output_maximum);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_input_scaling_degree_input(uint8_t input_scaling_degree)
{
  if (!connected()) { return; }
  settings.set_input_scaling_degree(input_scaling_degree);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_feedback_mode_input(uint8_t feedback_mode)
{
  if (!connected()) { return; }
  settings.set_feedback_mode(feedback_mode);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_feedback_invert_input(bool invert_feedback)
{
  if (!connected()) { return; }
  settings.set_feedback_invert(invert_feedback);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_feedback_absolute_minimum_input(uint16_t value)
{
  if (!connected()) { return; }
  settings.set_feedback_absolute_minimum(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_feedback_absolute_maximum_input(uint16_t value)
{
  if (!connected()) { return; }
  settings.set_feedback_absolute_maximum(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_feedback_maximum_input(uint16_t value)
{
  if (!connected()) { return; }
  settings.set_feedback_maximum(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_feedback_minimum_input(uint16_t value)
{
  if (!connected()) { return; }
  settings.set_feedback_minimum(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_feedback_analog_samples_input(uint8_t feedback_analog_samples)
{
  if (!connected()) { return; }
  settings.set_feedback_analog_samples_exponent(feedback_analog_samples);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_feedback_detect_disconnect_input(bool detect_disconnect)
{
  if (!connected()) { return; }
  settings.set_feedback_detect_disconnect(detect_disconnect);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_pid_constant_control_multiplier(int index, uint16_t multiplier)
{
  if (!connected()) { return; }
  double x = multiplier;
  uint16_t exponent;

  switch (index)
  {
    case 0:
      settings.set_proportional_multiplier(multiplier);
      break;
    case 1:
      settings.set_integral_multiplier(multiplier);
      break;
    case 2:
      settings.set_derivative_multiplier(multiplier);
      break;
    default:
      return;
  }

  for (int i = 0; i < 3; ++i)
  {
    recalculate_pid_coefficients(i);
  }

  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_pid_constant_control_exponent(int index, uint16_t exponent)
{
  if (!connected()) { return; }
  double x;

  switch (index)
  {
    case 0:
      settings.set_proportional_exponent(exponent);
      break;
    case 1:
      settings.set_integral_exponent(exponent);
      break;
    case 2:
      settings.set_derivative_exponent(exponent);
      break;
    default:
      return;
  }

  for (int i = 0; i < 3; ++i)
  {
    recalculate_pid_coefficients(i);
  }

  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_pid_constant_control_constant(int index, double constant)
{
  if (!connected()) { return; }
  uint16_t largest_divisor = 1;
  int i;

  for (i = 0; i < 18; i++)
  {
    largest_divisor *= 2;
    if ((largest_divisor * constant) > 1023)
    {
      largest_divisor /= 2;
      break;
    }
  }

  uint16_t multiplier = largest_divisor * constant;
  int exponent = i;

  while (multiplier % 2 == 0 && exponent != 0)
  {
    multiplier /= 2;
    exponent -= 1;
  }

  switch (index)
  {
    case 0:
      settings.set_proportional_multiplier(multiplier);
      settings.set_proportional_exponent(exponent);
      break;
    case 1:
      settings.set_integral_multiplier(multiplier);
      settings.set_integral_exponent(exponent);
      break;
    case 2:
      settings.set_derivative_multiplier(multiplier);
      settings.set_derivative_exponent(exponent);
    default:
      return;
  }

  for (int i = 0; i < 3; ++i)
  {
    recalculate_pid_coefficients(i);
  }

  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_pid_period_input(uint16_t value)
{
  if (!connected()) { return; }
  settings.set_pid_period(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_pid_integral_limit_input(uint16_t value)
{
  if (!connected()) { return; }
  settings.set_pid_integral_limit(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_pid_reset_integral_input(bool checked)
{
  if (!connected()) { return; }
  settings.set_pid_reset_integral(checked);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_pid_feedback_dead_zone_input(uint8_t value)
{
  if (!connected()) { return; }
  settings.set_feedback_dead_zone(value);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_pwm_frequency_input(uint8_t motor_pwm_frequency)
{
  if (!connected()) { return; }
  settings.set_motor_pwm_frequency(motor_pwm_frequency);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_invert_input(bool invert_motor)
{
  if (!connected()) { return; }
  settings.set_motor_invert(invert_motor);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_detect_direction_button_clicked()
{
  if (!connected()) { return; }

  if((variables.get_error_flags_halting() &
    ~(1<<(uint16_t)JRK_ERROR_AWAITING_COMMAND | 1<<(uint16_t)JRK_ERROR_INPUT_INVALID)) != 0)
  {
      // there is some error stopping the motor other than just waiting for a command
      window->show_error_message("An error is stopping the motor.  You must fix this before detecting the direction.");
      return;
  }

  if (settings.get_feedback_mode() != JRK_FEEDBACK_MODE_ANALOG)
  {
      window->show_error_message("Feedback mode must be Analog to detect the motor direction.");
      return;
  }

  device_handle.stop_motor();

  settings.set_input_mode(JRK_INPUT_MODE_SERIAL);
  settings.set_pid_period(1);
  settings.set_proportional_exponent(3);
  settings.set_proportional_multiplier(8);
  settings.set_derivative_multiplier(0);
  settings.set_integral_multiplier(0);
  device_handle.reinitialize();

  bool updated = do_motor_direction_detect();

  settings.set_input_mode(settings.get_input_mode());
  settings.set_pid_period(settings.get_pid_period());
  settings.set_proportional_exponent(settings.get_proportional_exponent());
  settings.set_proportional_multiplier(settings.get_proportional_multiplier());
  settings.set_derivative_multiplier(settings.get_derivative_multiplier());
  settings.set_integral_multiplier(settings.get_integral_multiplier());
  device_handle.reinitialize();

  device_handle.stop_motor();

  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_asymmetric_input(bool checked)
{
  if (!connected()) { return; }
  motor_asymmetric = checked;

  if (!motor_asymmetric)
  {
    settings.set_motor_max_duty_cycle_reverse(settings.get_motor_max_duty_cycle_forward());
    settings.set_motor_max_acceleration_reverse(settings.get_motor_max_acceleration_forward());
    settings.set_motor_max_deceleration_reverse(settings.get_motor_max_deceleration_forward());
    settings.set_motor_brake_duration_reverse(settings.get_motor_brake_duration_forward());
    settings.set_motor_max_current_reverse(settings.get_motor_max_current_forward());
    settings.set_motor_current_calibration_reverse(settings.get_motor_current_calibration_forward());
  }

  // TODO: implement logic for when asymmetric checkbox changes

  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_max_duty_cycle_forward_input(uint16_t duty_cycle)
{
  if (!connected()) { return; }
  settings.set_motor_max_duty_cycle_forward(duty_cycle);
  if (!motor_asymmetric)
    settings.set_motor_max_duty_cycle_reverse(duty_cycle);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_max_duty_cycle_reverse_input(uint16_t duty_cycle)
{
  if (!connected()) { return; }
  settings.set_motor_max_duty_cycle_reverse(duty_cycle);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_max_acceleration_forward_input(uint16_t acceleration)
{
  if (!connected()) { return; }
  settings.set_motor_max_acceleration_forward(acceleration);
  if (!motor_asymmetric)
    settings.set_motor_max_acceleration_reverse(acceleration);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_max_acceleration_reverse_input(uint16_t acceleration)
{
  if (!connected()) { return; }
  settings.set_motor_max_acceleration_reverse(acceleration);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_max_deceleration_forward_input(uint16_t deceleration)
{
  if (!connected()) { return; }
  settings.set_motor_max_deceleration_forward(deceleration);
  if (!motor_asymmetric)
    settings.set_motor_max_deceleration_reverse(deceleration);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_max_deceleration_reverse_input(uint16_t deceleration)
{
  if (!connected()) { return; }
  settings.set_motor_max_deceleration_reverse(deceleration);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_brake_duration_forward_input(uint32_t deceleration)
{
  if (!connected()) { return; }
  settings.set_motor_brake_duration_forward(deceleration);
  if (!motor_asymmetric)
    settings.set_motor_brake_duration_reverse(deceleration);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_brake_duration_reverse_input(uint32_t deceleration)
{
  if (!connected()) { return; }
  settings.set_motor_brake_duration_reverse(deceleration);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_max_current_forward_input(uint16_t current)
{
  if (!connected()) { return; }
  settings.set_motor_max_current_forward(current);
  if (!motor_asymmetric)
    settings.set_motor_max_current_reverse(current);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_max_current_reverse_input(uint16_t current)
{
  if (!connected()) { return; }
  settings.set_motor_max_current_reverse(current);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_current_calibration_forward_input(uint16_t current)
{
  if (!connected()) { return; }
  settings.set_motor_current_calibration_forward(current);
  if (!motor_asymmetric)
    settings.set_motor_current_calibration_reverse(current);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_current_calibration_reverse_input(uint16_t current)
{
  if (!connected()) { return; }
  settings.set_motor_current_calibration_reverse(current);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_motor_out_range_input(uint16_t value)
{
  {
  if (!connected()) { return; }
  settings.set_motor_max_duty_cycle_while_feedback_out_of_range(value);
  settings_modified = true;
  handle_settings_changed();
}
}

void main_controller::handle_motor_coast_when_off_input(bool motor_coast)
{
  if (!connected()) { return; }
  settings.set_motor_coast_when_off(motor_coast);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_never_sleep_input(bool never_sleep)
{
  if (!connected()) { return; }
  jrk_settings_set_never_sleep(settings.get_pointer(), never_sleep);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_vin_calibration_input(int16_t vin_calibration)
{
  if (!connected()) { return; }
  jrk_settings_set_vin_calibration(settings.get_pointer(), vin_calibration);
  settings_modified = true;
  handle_settings_changed();
}

void main_controller::handle_upload_complete()
{
  // After a firmware upgrade is complete, allow the GUI to reconnect to the
  // device automatically.
  disconnected_by_user = false;
}

void main_controller::apply_settings()
{
  if (!connected()) { return; }

  try
  {
    assert(connected());

    jrk::settings fixed_settings = settings;
    std::string warnings;
    fixed_settings.fix(&warnings);
    if (warnings.empty() ||
      window->confirm(warnings.append("\nAccept these changes and apply settings?")))
    {
      settings = fixed_settings;
      device_handle.set_settings(settings);
      device_handle.reinitialize();
      cached_settings = settings;
      settings_modified = false;  // this must be last in case exceptions are thrown
    }
  }
  catch (const std::exception & e)
  {
    show_exception(e);
  }
  handle_settings_changed();
}

void main_controller::stop_motor()
{
  if (!connected()) { return; }

  try
  {
    device_handle.stop_motor();
  }
  catch (const std::exception & e)
  {
    show_exception(e);
  }
}

void main_controller::run_motor()
{
  if (!connected()) { return; }

  try
  {
    device_handle.run_motor();
  }
  catch (const std::exception & e)
  {
    show_exception(e);
  }
}

void main_controller::set_target(uint16_t target)
{
  if (!connected()) { return; }

  try
  {
    device_handle.set_target(target);
  }
  catch (const std::exception & e)
  {
    show_exception(e);
  }
}

void main_controller::open_settings_from_file(std::string filename)
{
  if (!connected()) { return; }

  try
  {
    assert(connected());

    std::string settings_string = read_string_from_file(filename);
    jrk::settings fixed_settings = jrk::settings::read_from_string(settings_string);
    std::string warnings;
    fixed_settings.fix(&warnings);
    if (warnings.empty() ||
      window->confirm(warnings.append("\nAccept these changes and load settings?")))
    {
      settings = fixed_settings;
      settings_modified = true;
    }
  }
  catch (std::exception const & e)
  {
    show_exception(e);
  }

  handle_settings_changed();
}

void main_controller::save_settings_to_file(std::string filename)
{
  if (!connected()) { return; }

  try
  {
    assert(connected());

    jrk::settings fixed_settings = settings;
    std::string warnings;
    fixed_settings.fix(&warnings);
    if (!warnings.empty())
    {
      if (window->confirm(warnings.append("\nAccept these changes and save settings?")))
      {
        settings = fixed_settings;
        settings_modified = true;
      }
      else
      {
        return;
      }
    }
    std::string settings_string = settings.to_string();
    write_string_to_file(filename, settings_string);
  }
  catch (std::exception const & e)
  {
    show_exception(e);
  }

  handle_settings_changed();
}

void main_controller::reload_variables()
{
  assert(connected());

  try
  {
    variables = device_handle.get_variables(true);
    variables_update_failed = false;
  }
  catch (...)
  {
    variables_update_failed = true;
    throw;
  }
}
