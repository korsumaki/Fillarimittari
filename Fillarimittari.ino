// Fillarimittari

/* TODO
 * - average speed
 * - stop time and zero speed, when long time without pulse
 */ 

#define FILLARIMITTARI_VERSION "v0.1"

#include "unit_test.h"

#include <Wire.h>
#include <LOLIN_I2C_BUTTON.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET -1
Adafruit_SSD1306 display(OLED_RESET);
I2C_BUTTON button(DEFAULT_I2C_BUTTON_ADDRESS);

#define SENSOR_PIN D3


const float diameter_in_meters = 0.70;
const float perimeter_in_meters = diameter_in_meters * PI;

float perimeter = 1.25;

unsigned int pulse_count = 0;
unsigned long last_pulse_time_ms = 0;
unsigned long time_ms_delta = 0;

float distance = 0;
float speed = 0;
float acceleration = 0;


void setup()
{
	Serial.begin(115200);
	pinMode(SENSOR_PIN, INPUT);

	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

	unit_test();

	clear_counters();
	set_perimeter(perimeter_in_meters);
	draw_ui();
}

void loop()
{
	// NOTE: Reading button state will take about 50 ms time.
	// It affects to accuracy of pulse timings, when pulse starts during button state reading.
	// Error is max 12.6% at 20 km/h, and 25.2% at 40 km/h.
	// Solution:
	// - Read it right after pulse is triggered (=there are plenty of time to next pulse)
	// - or when there is long enough time from last pulse (=we are stopped or low speed -> error is small)
	bool can_check_buttons = false;

	if (sensor_triggered(SENSOR_PIN))
	{
		set_pulse(millis());
		draw_ui();
		can_check_buttons = true;
	}

	if (millis() - last_pulse_time_ms > 2000)
	{
		can_check_buttons = true;
	}

	if (can_check_buttons)
	{
		// Now it is safe to check buttons
		if (button.get() == 0)
		{
			if (button.BUTTON_A) // Switch to next screen
			{
				next_ui();
				draw_ui();
			}
			if (button.BUTTON_B) // Reset counters
			{
				clear_counters();
				draw_ui();
			}
		}
	}
}

typedef enum {
	screen_all_e,
	screen_distance_e,
	screen_speed_e,
	screen_acc_e,
	//screen_acc_change_e,
	screen_version_e,
	screen_last_e,
} ui_screen_t;

int current_ui = screen_all_e;

void next_ui()
{
	current_ui++;
	if (current_ui >= screen_last_e)
	{
		current_ui = screen_all_e;
	}
}


void draw_screen_all()
{
	display.setTextSize(1);
	display.print("Dst ");
	display.println( get_distance_in_km() );
	display.print("Spd ");
	display.println( get_speed_in_kmh() );
	display.print("Acc ");
	display.println(get_acceleration() );
}

void draw_screen_distance()
{
	display.setTextSize(1);
	display.println("Dist km");
	display.setTextSize(2);
	display.println( get_distance_in_km() );
	display.setTextSize(1);
	display.print("Spd ");
	display.println( get_speed_in_kmh() );
	display.print("Acc ");
	display.println(get_acceleration() );
}

void draw_screen_speed()
{
	display.setTextSize(1);
	display.print("Dst: ");
	display.println( get_distance_in_km() );
	display.println("Speed km/h");
	display.setTextSize(2);
	display.println( get_speed_in_kmh() );
	display.setTextSize(1);
	display.print("Acc ");
	display.println(get_acceleration() );
}

void draw_screen_acc()
{
	display.setTextSize(1);
	display.print("Dst ");
	display.println( get_distance_in_km() );
	display.print("Spd ");
	display.println( get_speed_in_kmh() );
	display.println("Accel m/s2");
	display.setTextSize(2);
	display.println(get_acceleration() );
	display.setTextSize(1);
}

void draw_screen_version()
{
	display.setTextSize(1);
	display.println("Version");
	display.setTextSize(2);
	display.println( FILLARIMITTARI_VERSION );
	display.setTextSize(1);
	display.println( __DATE__ );
	display.println( __TIME__ );
}

void draw_ui()
{
	display.clearDisplay();
	display.setTextWrap(false);

	// Draw horizontal line to indicate current screen
	int max_items = screen_last_e;
	int width = display.width();
	int len = width/max_items;		// Length of one line
	// ...to last line
	display.drawFastHLine(current_ui*len, display.height()-1, len, WHITE),

	// Pulse indicator to second last line
	display.drawPixel(pulse_count % width, display.height()-2, WHITE);
	

	display.setCursor(0, 0);
    display.setTextColor(WHITE);

	switch (current_ui)
	{
		case screen_all_e:
			draw_screen_all();
			break;
		case screen_distance_e:
			draw_screen_distance();
			break;
		case screen_speed_e:
			draw_screen_speed();
			break;
		case screen_acc_e:
			draw_screen_acc();
			break;
		//case screen_acc_change_e:
		//	draw_screen_acc_change();
		//	break;
		case screen_version_e:
			draw_screen_version();
			break;
	}
	display.println(time_ms_delta);
	display.display();
}

bool sensor_triggered(int pin)
{
	// TODO use debounce, both directions?
	static int is_sensor_triggered = false;
	int sensor_pin_state = digitalRead(pin);

	if (is_sensor_triggered == false && sensor_pin_state == LOW)
	{
		// Change HIGH->LOW
		is_sensor_triggered = true;
		return true;
	}

	if (sensor_pin_state == HIGH)
	{
		is_sensor_triggered = false;
	}

	return false;
}


void calc_numbers()
{
	// distance
	float distance_prev = distance;
	distance = pulse_count * perimeter;
	float distance_delta = distance - distance_prev;

	// speed
	float speed_prev = speed;
	speed = distance_delta*1000 / time_ms_delta;
	float speed_delta = speed - speed_prev;

	// acceleration
	//float acceleration_prev = acceleration;
	acceleration = speed_delta*1000 / time_ms_delta;
	//float acceleration_delta = acceleration - acceleration_prev;

	// acceleration ^2
}

// One pulse is detected, when time is "time_ms"
void set_pulse(int time_ms)
{
	pulse_count++;

	time_ms_delta = time_ms - last_pulse_time_ms;
	last_pulse_time_ms = time_ms;

	calc_numbers();
}

void clear_counters()
{
	pulse_count = 0;
	distance = 0;
	speed = 0;
	acceleration = 0;
	last_pulse_time_ms = 0;
}

float get_distance()
{
	return distance;
}

float get_distance_in_km()
{
	return distance/1000;
}

float get_speed()
{
	return speed;
}

float get_speed_in_kmh()
{
	return speed * 3.6;
}

float get_acceleration()
{
	return acceleration;
}

void set_perimeter(float p)
{
	perimeter = p;
}


void unit_test()
{
	display.clearDisplay();
	display.setTextSize(1);
	display.setCursor(0, 0);
	display.setTextColor(WHITE);

	// ==========================================
	// Unit test code begin
	// ==========================================
	UT_BEGIN();
	UT_EXPECT(true, true); // Just dummy test for macro

	clear_counters();
	set_perimeter(2.5);
	UT_EXPECT(0, get_distance());

	set_pulse(1000);
	UT_EXPECT(2.5, get_distance());

	set_pulse(2000);
	UT_EXPECT(5.0, get_distance());

	clear_counters();
	set_perimeter(1.0);
	UT_EXPECT(0, get_distance());

	set_pulse(3000);
	UT_EXPECT(1, get_distance());

	clear_counters();
	UT_EXPECT(0, get_distance());
	UT_EXPECT(0, get_speed());
	UT_EXPECT(0, get_speed_in_kmh());
	UT_EXPECT(0, get_acceleration());

	set_pulse(4000);
	UT_EXPECT(1, get_distance());
	UT_EXPECT(0.25, get_speed());
	UT_EXPECT_FLOAT(0.25 * 3.6, get_speed_in_kmh());

	UT_EXPECT(0.25/4, get_acceleration());

	set_pulse(5000);
	UT_EXPECT(2, get_distance());
	UT_EXPECT(1, get_speed());
	UT_EXPECT_FLOAT(3.6, get_speed_in_kmh());

	set_pulse(5500);
	UT_EXPECT(3, get_distance());
	UT_EXPECT(2, get_speed());
	UT_EXPECT_FLOAT(7.2, get_speed_in_kmh());

	UT_EXPECT(1/0.5, get_acceleration());

	UT_END();
	// ==========================================
	// Unit test code end
	// ==========================================

	display.println("TEST:");
	if (UT_fails > 0)
	{
		display.println("FAILED");
		display.print(UT_fails);
		display.print(" errors");
		display.display();
		delay(10000);
	}
	else
	{
		display.println("PASSED");
		display.display();
		delay(2000);
	}
}

