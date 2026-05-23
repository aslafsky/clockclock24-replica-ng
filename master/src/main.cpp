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
 * Sets clock time using arrow animation
*/
void set_arrow();

/**
 * Sets clock time using ripple animation
*/
void set_ripple();

/**
 * Sets clock time using globe animation
*/
void set_globe();

/**
 * Sets clock time using bubble animation
*/
void set_bubble();

/**
 * Sets clock time using gear animation
*/
void set_gear();

/**
 * Sets clock time using scatter animation
*/
void set_scatter();

/**
 * Sets clock time using cycle animation (round-robin through every other
 * animation except LAZY).
*/
void set_cycle();

/**
 * Dispatches to the set_<mode>() function for the given clock mode. Used by
 * both set_time() and set_cycle(). When adding a new animation mode, add a
 * case here and the cycle mode picks it up automatically.
*/
void dispatch_animation(int mode);

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
    dispatch_animation(get_clock_mode());
  }
}

void dispatch_animation(int mode)
{
  switch(mode)
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
    case ARROW:
      set_arrow();
      break;
    case RIPPLE:
      set_ripple();
      break;
    case GLOBE:
      set_globe();
      break;
    case BUBBLE:
      set_bubble();
      break;
    case GEAR:
      set_gear();
      break;
    case SCATTER:
      set_scatter();
      break;
    case CYCLE:
      set_cycle();
      break;
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

void set_arrow()
{
  // Phase 1: move all 24 clocks to d_joint using MIN_DISTANCE, then hold.
  set_speed(600 * get_speed_multiplier());
  set_acceleration(150 * get_speed_multiplier());
  set_direction(MIN_DISTANCE);
  set_clock(d_joint);
  _delay(5000 + (9000 - 4000) / sqrt(get_speed_multiplier()));

  // Phase 2: staggered propeller spin into the target time.
  set_speed(450 * get_speed_multiplier());
  set_acceleration(150 * get_speed_multiplier());
  set_direction(CLOCKWISE3);

  t_full_clock target = get_clock_state_from_time(last_hour, last_minute);

  // Staggered wavefront perpendicular to the line through the centres of the
  // top-left (row 0, col 0) and bottom-right (row 2, col 7) sub-clocks. That
  // line has direction (7, 2) in (col, row) space, so the perpendicular is
  // (2, -7) and a clock's projection onto it is proj = 2*col - 7*row.
  // proj ranges from -14 at the bottom-left clock (row 2, col 0) to +14 at the
  // top-right clock (row 0, col 7). Clocks sharing a projection spin together;
  // successive non-empty groups are staggered 200ms apart.
  const int MIN_PROJ = -14; // 2*0 - 7*2
  const int MAX_PROJ = 14;  // 2*7 - 7*0
  for (int proj = MIN_PROJ; proj <= MAX_PROJ; proj++)
  {
    bool group_has_clock = false;
    for (int hd = 0; hd < 8; hd++)
    {
      t_half_digitl lite = target.digit[hd / 2].halfs[hd % 2];
      for (int p = 0; p < 3; p++)
      {
        int col = hd;
        int row = p;
        // Each clock spins h-hand CW and m-hand CCW with 2 extra full
        // rotations (CLOCKWISE3 / COUNTERCLOCKWISE3) into the target.
        if (2 * col - 7 * row == proj)
        {
          set_single_clock_full(hd, p, lite, CLOCKWISE3, COUNTERCLOCKWISE3);
          group_has_clock = true;
        }
      }
    }
    if (group_has_clock && proj < MAX_PROJ)
      delay(25);
  }
}

void set_ripple()
{
  set_speed(800 * get_speed_multiplier());
  set_acceleration(150 * get_speed_multiplier());
  set_direction(MIN_DISTANCE);
  set_clock(d_WAVE);
  _delay(5000 +(9000 - 4000) / sqrt(get_speed_multiplier()));
  set_speed(600 * get_speed_multiplier());
  set_acceleration(500 * get_speed_multiplier());
  set_direction(CLOCKWISE3);


  t_full_clock target = get_clock_state_from_time(last_hour, last_minute);

  const int MAX_DIST = 4; // floor(3.5 + 1.0) = 4
  for (int d = 0; d <= MAX_DIST; d++)
  {
    for (int hd = 0; hd < 8; hd++)
    {
      // Mirror rotation direction for the right half (half-digits 4-7)
      int mode_h = (hd < 4) ? CLOCKWISE3 : COUNTERCLOCKWISE3;
      int mode_m = (hd < 4) ? COUNTERCLOCKWISE3 : CLOCKWISE3;
      t_half_digitl lite = target.digit[hd / 2].halfs[hd % 2];
      for (int p = 0; p < 3; p++)
      {
        float col_dist = fabsf((float)hd - 3.5f);
        float row_dist = fabsf((float)p - 1.0f);
        int dist = (int)(col_dist + row_dist); // floor via truncation
        if (dist == d)
          set_single_clock_full(hd, p, lite, mode_h, mode_m);
      }
    }
    if (d < MAX_DIST)
      delay(400);
  }
}

void set_globe()
{
  set_speed(800 * get_speed_multiplier());
  set_acceleration(150 * get_speed_multiplier());
  set_direction(MIN_DISTANCE);
  set_clock(d_globe);
  _delay(4000 +(9000 - 4000) / sqrt(get_speed_multiplier()));
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
      // Checkerboard spin: (col + row) even -> hour CW / minute CCW;
      // (col + row) odd -> hour CCW / minute CW. Even columns (0,2,4,6)
      // have both hands' rotation direction inverted, and columns 0 and 7
      // are inverted as well.
      bool hour_cw = ((i + j) % 2 == 0);
      if (i % 2 == 0)
        hour_cw = !hour_cw;
      if (i == 0 || i == 7)
        hour_cw = !hour_cw;
      if (hour_cw)
      {
        hd.clocks[j].mode_h = CLOCKWISE3;
        hd.clocks[j].mode_m = COUNTERCLOCKWISE3;
      }
      else
      {
        hd.clocks[j].mode_h = COUNTERCLOCKWISE3;
        hd.clocks[j].mode_m = CLOCKWISE3;
      }
    }
    set_half_digit_full(i, hd);
  }
}

void set_bubble()
{
  set_speed(800 * get_speed_multiplier());
  set_acceleration(150 * get_speed_multiplier());
  set_direction(MIN_DISTANCE);
  set_clock(d_bubble);
  _delay(4000 +(9000 - 4000) / sqrt(get_speed_multiplier()));
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
      // Checkerboard spin: (col + row) even -> hour CW / minute CCW;
      // (col + row) odd -> hour CCW / minute CW. Even columns (0,2,4,6)
      // have both hands' rotation direction inverted.
      bool hour_cw = ((i + j) % 2 == 0);
      if (i % 2 == 0)
        hour_cw = !hour_cw;
      if (hour_cw)
      {
        hd.clocks[j].mode_h = CLOCKWISE3;
        hd.clocks[j].mode_m = COUNTERCLOCKWISE3;
      }
      else
      {
        hd.clocks[j].mode_h = COUNTERCLOCKWISE3;
        hd.clocks[j].mode_m = CLOCKWISE3;
      }
    }
    set_half_digit_full(i, hd);
  }
}

void set_gear()
{
  // Phase 1: move all 24 clocks to d_CENT using MIN_DISTANCE, then hold.
  set_speed(600 * get_speed_multiplier());
  set_acceleration(150 * get_speed_multiplier());
  set_direction(MIN_DISTANCE);
  set_clock(d_CENT);
  _delay(5000 + (9000 - 4000) / sqrt(get_speed_multiplier()));

  // Phase 2: staggered propeller spin into the target time.
  set_speed(600 * get_speed_multiplier());
  set_acceleration(150 * get_speed_multiplier());
  set_direction(CLOCKWISE3);

  t_full_clock target = get_clock_state_from_time(last_hour, last_minute);

  // Stagger outwards from the centre in a square (Chebyshev) radius.
  // group = max(col_dist, row_dist) where col_dist = min(|hd-3|,|hd-4|) and
  // row_dist = |p-1|. Group 0 is the centre pair (row 1, cols 3-4), group 3
  // is the outermost columns 0 and 7. Successive groups are staggered 100ms.
  const int MAX_GROUP = 3;
  for (int g = 0; g <= MAX_GROUP; g++)
  {
    for (int hd = 0; hd < 8; hd++)
    {
      // All sub-clocks spin clockwise; both hands of each sub-clock rotate
      // the same direction at the same speed (speed is taken from the
      // shared globals).
      int mode = CLOCKWISE3;
      int mode_h = mode;
      int mode_m = mode;
      t_half_digitl lite = target.digit[hd / 2].halfs[hd % 2];
      for (int p = 0; p < 3; p++)
      {
        int col_dist = min(abs(hd - 3), abs(hd - 4));
        int row_dist = abs(p - 1);
        int group = max(col_dist, row_dist);
        if (group == g)
          set_single_clock_full(hd, p, lite, mode_h, mode_m);
      }
    }
    if (g < MAX_GROUP)
      delay(500);
  }
}

void set_scatter()
{
  // All hands rotate counter-clockwise. The hour hand makes 2 full rotations
  // at speed 400; the minute hand makes 4 full rotations at speed 800, so
  // both hands spin for the same duration and land together.
  set_speed(400 * get_speed_multiplier());
  set_acceleration(150);
  set_direction(COUNTERCLOCKWISE3);

  t_full_clock clock = get_clock_state_from_time(last_hour, last_minute);

  // Left-to-right stagger: column 0 starts first, column 7 last, 100ms apart.
  for (int i = 0; i < 8; i++)
  {
    t_half_digit hd = get_full_half_digit(clock.digit[i/2].halfs[i%2]);
    for (int j = 0; j < 3; j++)
    {
      // Hour hand: 2 full rotations counter-clockwise.
      hd.clocks[j].mode_h = COUNTERCLOCKWISE3;
      // Minute hand: 4 full rotations counter-clockwise at speed 800.
      hd.clocks[j].mode_m = COUNTERCLOCKWISE5;
      hd.clocks[j].speed_m = 800 * get_speed_multiplier();
    }
    set_half_digit_full(i, hd);
    if (i < 7)
      delay(400);
  }
}

void set_cycle()
{
  // Round-robin through every animation from FUN through the mode just
  // before CYCLE (LAZY is excluded). Picking up new animations added later
  // is automatic: any new mode inserted after FUN and before CYCLE grows
  // the cycle range, and dispatch_animation() handles routing.
  static int cycle_index = 0;
  int span = CYCLE - FUN;
  int mode = FUN + (cycle_index % span);
  cycle_index++;
  dispatch_animation(mode);
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
