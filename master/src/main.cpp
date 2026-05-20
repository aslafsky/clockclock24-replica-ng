#include <Arduino.h>
#include <Wire.h>
#include <TimeLib.h>

#include "board_definitions.h"
#include "i2c.h"
#include "clock_state.h"
#include "clock_manager.h"
#include "digit.h"
#include "wifi_utils.h"
#include "web_server.h"
#include "clock_config.h"
#include "ntp.h"
#include "status_led.h"
#include "captive_portal.h"
#include "mqtt_handler.h"
#include "update_handler.h"

int last_hour = -1;
int last_minute = -1;
bool is_stopped = false;
const unsigned long SCHEDULED_RESTART_DELAY_MS = 3000;
static int last_restart_day = -1;

/**
 * Sets clock to the current time
*/
void set_time();

/**
 * Sets clock time using lazy animation
*/
void set_lazy();

/**
 * Sets clock time using fun animation
*/
void set_fun();

/**
 * Sets clock time using waves animation
*/
void set_waves();

/**
 * Sets clock time using propeller animation
*/
void set_propeller();

/**
 * Sets clock time using fluid wave animation
*/
void set_fluid_wave();

/**
 * Sets clock to stop state
*/
void stop();

/**
 * Quickly stops the clock with fixed speed for OTA
*/
void shutdown();

/**
 * Custom delay to update web clients
 * @param value   time in milliseconds
*/
void _delay(int value);

void setup() {
  Serial.begin(115200);
  Serial.printf("\nclockclock24 replica by Vallasc - %s\n", BOARD_TARGET);
  delay(3000);
  
  // Initialize status LED
  led_init();
  
  // Load configuration from EEPROM
  begin_config();

  Wire.begin(I2C_SDA, I2C_SCL);

  if(get_connection_mode() == HOTSPOT)
    wifi_create_AP("ClockClock 24", get_hostname());
  else if( !wifi_connect(get_ssid(), get_password(), get_hostname()) )
  {
    // Fall back to hotspot temporarily without changing stored configuration
    set_active_connection_mode(HOTSPOT);
    wifi_create_AP("ClockClock 24", get_hostname());
  }

  if(get_active_connection_mode() == EXT_CONN)
  {
    // Initialize NTP
    begin_NTP();
    setSyncProvider(get_NTP_time);
    // Sync every 30 minutes
    setSyncInterval(60 * 30);
    
    // Initialize MQTT (only in external connection mode)
    mqtt_init();
  }

  // Starts web server
  server_start();
}

void loop() {
  // Daily restart at configured hour
  if (!update_in_progress() && get_daily_restart_enabled())
  {
    int current_hour = hour();
    int current_day = day();
    if (current_hour == get_daily_restart_hour() && current_day != last_restart_day)
    {
      last_restart_day = current_day;
      Serial.printf("Daily restart at %d:00\n", current_hour);
      schedule_restart(SCHEDULED_RESTART_DELAY_MS);
    }
  }

  if (!update_in_progress())
  {
    if (get_active_connection_mode() == EXT_CONN)
    {
      if (wifi_sta_watchdog_should_restart())
      {
        Serial.println("Scheduling restart from STA watchdog");
        schedule_restart(SCHEDULED_RESTART_DELAY_MS);
      }
    }
    else if (get_active_connection_mode() == HOTSPOT)
    {
      if (wifi_ap_watchdog_should_restart())
      {
        Serial.println("Scheduling restart from AP watchdog");
        schedule_restart(SCHEDULED_RESTART_DELAY_MS);
      }
    }
    else
    {
      led_set_status(LED_ERROR);
    }
  }

  led_update();
  
  if(get_active_connection_mode() == HOTSPOT)
    captive_portal_update();

  if(get_active_connection_mode() == HOTSPOT && is_time_changed_browser())
  {
    t_browser_time browser_time = get_browser_time();
    setTime(browser_time.hour, 
      browser_time.minute, 
      browser_time.second, 
      browser_time.day, 
      browser_time.month,  
      browser_time.year);
  }

  if(get_active_connection_mode() == EXT_CONN && get_timezone() != get_ntp_timezone())
  {
    set_ntp_timezone(get_timezone());
    setSyncProvider(get_NTP_time);
  }

  get_clock_mode() != OFF ? set_time() : stop();

  handle_webclient();
  
  // Handle MQTT
  if(get_active_connection_mode() == EXT_CONN)
    mqtt_handle();
}

void set_time()
{
  int day_week = (weekday() + 5) % 7;
  if(get_sleep_time(day_week, hour()))
    stop();
  else if(hour() != last_hour || minute() != last_minute)
  {
    is_stopped = false;
    last_hour = hour();
    last_minute = minute();
    switch(get_clock_mode())
    {
      case LAZY:
        set_lazy();
        break;
      case FUN:
        set_fun();
        break;
      case WAVES:
        set_waves();
        break;
      case PROPELLER:
        set_propeller();
        break;
      case FLUID_WAVE:
        set_fluid_wave();
        break;
    }
  }
}

void set_lazy()
{
  set_speed(200 * get_speed_multiplier());
  set_acceleration(100 * get_speed_multiplier());
  set_direction(MIN_DISTANCE);
  set_clock_time(last_hour, last_minute);
}

void set_fun()
{
  set_speed(400 * get_speed_multiplier());
  set_acceleration(150 * get_speed_multiplier());
  set_direction(CLOCKWISE2);
  set_clock_time(last_hour, last_minute);
}

void set_waves()
{
  set_speed(800 * get_speed_multiplier());
  set_acceleration(150 * get_speed_multiplier());
  set_direction(MIN_DISTANCE);
  set_clock(d_IIII);
  _delay(4000 +(9000 - 4000) / sqrt(get_speed_multiplier()));
  set_speed(400 * get_speed_multiplier());
  set_acceleration(100 * get_speed_multiplier());
  set_direction(CLOCKWISE2);
  t_full_clock clock = get_clock_state_from_time(last_hour, last_minute);
  for (int i = 0; i <8; i++)
  {
    set_half_digit(i, clock.digit[i/2].halfs[i%2]);
    delay(200 + (400 - 200) / sqrt(get_speed_multiplier()));
  }
}

void set_propeller()
{
  set_speed(600 * get_speed_multiplier());
  set_acceleration(150 * get_speed_multiplier());
  // Use CLOCKWISE3 to populate speed/accel fields via get_full_half_digit
  set_direction(CLOCKWISE3);
  t_full_clock clock = get_clock_state_from_time(last_hour, last_minute);
  for (int i = 0; i < 8; i++)
  {
    t_half_digit hd = get_full_half_digit(clock.digit[i/2].halfs[i%2]);
    for (int j = 0; j < 3; j++)
    {
      hd.clocks[j].mode_h = CLOCKWISE3;
      hd.clocks[j].mode_m = COUNTERCLOCKWISE3;
    }
    set_half_digit_full(i, hd);
  }
}

// ─── Fluid-wave helper ────────────────────────────────────────────────────
// Build a fully-populated t_half_digit with checkerboard per-hand directions.
// Within each clock: mode_h and mode_m are always opposite, so hands
// counter-rotate against each other (propeller effect per clock face).
// The checkerboard flips which hand leads CW vs CCW between adjacent clocks,
// producing the meshing-gear visual in Phase 3.
//   dir_even : mode for the hour   hand when (hd_idx + row) is even
//   dir_odd  : mode for the hour   hand when (hd_idx + row) is odd
// The minute hand always receives the other direction.
static t_half_digit build_fluid_hd(int hd_idx, t_half_digitl lite,
                                   int dir_even, int dir_odd)
{
  t_half_digit hd = get_full_half_digit(lite);
  for (int r = 0; r < 3; r++)
  {
    bool even = ((hd_idx + r) % 2 == 0);
    hd.clocks[r].mode_h = even ? dir_even : dir_odd;
    hd.clocks[r].mode_m = even ? dir_odd  : dir_even;
  }
  return hd;
}

// ─── FLUID_WAVE ───────────────────────────────────────────────────────────
// Five-phase choreography (~37 s total, scaled by speed_multiplier).
//
// Column stagger: 250 ms between consecutive column sends creates the
// travelling-wave effect.  Column 0 always leads column 7 in Phases 1–2;
// the wave reverses to right-to-left in Phase 3.
//
// Hand-direction rule (checkerboard):
//   hour hand  CW  when (col + row) even, CCW when odd.
//   minute hand always opposite → counter-rotating pair on every clock face.
//
// Phase 4 uses MIN_DISTANCE + lower acceleration for ease-out convergence
// with no overshoot on the final digit positions.
void set_fluid_wave()
{
  const int mult    = get_speed_multiplier();
  const int STAGGER = 250;  // ms column-to-column delay

  // Read the new time positions now — Phase 4 converges to these.
  t_full_clock target = get_clock_state_from_time(last_hour, last_minute);

  // ─── Phase 1: Dissolution (~5 s) ──────────────────────────────────────
  // All hands fan outward to POSE_WAVE_OPEN in a left-to-right wave.
  set_speed(400 * mult);
  set_acceleration(100 * mult);
  for (int col = 0; col < 8; col++)
  {
    set_half_digit_full(col,
      build_fluid_hd(col, POSE_WAVE_OPEN, CLOCKWISE, COUNTERCLOCKWISE));
    delay(STAGGER);
  }
  _delay(4000 / sqrt(mult));

  // ─── Phase 2a: Sweeping wave L→R — bracket / open shapes (~5 s) ───────
  // Outer columns stay wide; inner columns compress to bracket / flat poses.
  set_speed(600 * mult);
  set_acceleration(120 * mult);
  for (int col = 0; col < 8; col++)
  {
    t_half_digitl pose;
    if      (col == 0 || col == 7) pose = POSE_WAVE_OPEN;
    else if (col == 1 || col == 6) pose = POSE_WAVE_BKT_LEFT;
    else if (col == 2 || col == 5) pose = POSE_WAVE_BKT_RIGHT;
    else                           pose = POSE_WAVE_FLAT;
    set_half_digit_full(col,
      build_fluid_hd(col, pose, CLOCKWISE, COUNTERCLOCKWISE));
    delay(STAGGER);
  }
  _delay(4000 / sqrt(mult));

  // ─── Phase 2b: Alternating vertical / flat, L→R (~4 s) ────────────────
  for (int col = 0; col < 8; col++)
  {
    t_half_digitl pose = (col % 2 == 0) ? POSE_WAVE_VERTICAL : POSE_WAVE_FLAT;
    set_half_digit_full(col,
      build_fluid_hd(col, pose, CLOCKWISE, COUNTERCLOCKWISE));
    delay(STAGGER);
  }
  _delay(3500 / sqrt(mult));

  // ─── Phase 3: Counter-rotation / breathing — R→L wave (~7 s) ──────────
  // POSE_WAVE_BREATH differentiates rows: top=^, middle=─, bottom=v.
  // Wave travels right-to-left; checkerboard direction flips so adjacent
  // columns rotate opposite ways — the "meshing gear" effect.
  set_speed(500 * mult);
  set_acceleration(100 * mult);
  for (int col = 7; col >= 0; col--)
  {
    set_half_digit_full(col,
      build_fluid_hd(col, POSE_WAVE_BREATH, COUNTERCLOCKWISE, CLOCKWISE));
    delay(STAGGER);
  }
  _delay(5000 / sqrt(mult));

  // ─── Phase 4: Convergence — outer columns first, ease-out (~12 s) ─────
  // MIN_DISTANCE guarantees the shortest-path, overshoot-free landing.
  // Low acceleration lets the motors decelerate smoothly into position.
  // Column order: 0,7 → 1,6 → 2,5 → 3,4 (outside in, mirrored).
  set_speed(350 * mult);
  set_acceleration(60 * mult);
  set_direction(MIN_DISTANCE);
  {
    const int order[8] = {0, 7, 1, 6, 2, 5, 3, 4};
    for (int i = 0; i < 8; i++)
    {
      int col = order[i];
      set_half_digit(col, target.digit[col / 2].halfs[col % 2]);
      delay(300);
    }
  }
  _delay(10000 / sqrt(mult));

  // ─── Phase 5: Arrival ──────────────────────────────────────────────────
  // Motors are at their target digit positions. New time is displayed.
}

void stop()
{
  if(!is_stopped)
  {
    is_stopped = true;
    last_hour = -1;
    last_minute = -1;
    set_direction(MIN_DISTANCE);
    set_speed(200 * get_speed_multiplier());
    set_acceleration(100 * get_speed_multiplier());
    set_clock(d_stop);
  }
}

void shutdown()
{
  set_clock_mode_temp(OFF);
  if(!is_stopped)
  {
    is_stopped = true;
    last_hour = -1;
    last_minute = -1;
    set_direction(MIN_DISTANCE);
    set_speed(200 * 100);
    set_acceleration(100 * 100);
    set_clock(d_stop);
  }
}

void _delay(int value)
{
  for (int i = 0; i <value/100; i++)
  {
    handle_webclient();
    delay(value/100);
  }
}