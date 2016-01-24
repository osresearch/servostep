/** \file
 * Closed loop servo with a stepper motor and quadrature encoder.
 *
 */
#include "QuadDecode.h"


#define K_eps 10	// how close is "good enough"
#define MIN_SPEED	1000
#define MAX_SPEED	12000
#define SPEED_RAMP	5


#define STEP_PIN 10
#define DIR_PIN 13

void setup()
{
	Serial.begin(115200);  
	pinMode(STEP_PIN, OUTPUT);
	pinMode(DIR_PIN, OUTPUT);
}

static int16_t target;

void
servo_cmd(
	int cmd
)
{
	if (cmd < -65536 || cmd >= +65536)
	{
		Serial.println("range");
		return;
	}

	target = cmd;
}

void
read_command()
{
	static int flush = 0;
	static int new_cmd = 0;
	static int sign = 1;

	if (!Serial.available())
		return;

	const int c = Serial.read();
	if (c == -1)
		return;

	if (c == '-')
	{
		sign = -1;
		return;
	}
	if (c == '+')
	{
		sign = +1;
		return;
	}

	if ('0' <= c && c <= '9')
	{
		new_cmd = 10 * new_cmd + c - '0';
		return;
	}
		
	if (c == '\n')
	{
		if (!flush)
			servo_cmd(sign * new_cmd);
		flush = 0;
		new_cmd = 0;
		sign = 1;
		return;
	}

	if (c == '\r')
		return;

	Serial.print('?');
	new_cmd = 0;
	flush = 1;
	sign = 1;
	return;
}

void loop()
{
	// check for a command
	read_command();

	// run the control loop at a constant speed
	static unsigned last_now;
	const unsigned now = micros();
	if (now - last_now < 200)
		return;
	last_now = now;

	static unsigned update_rate;
	static int16_t old_count;
	int16_t count = QuadDecode.getCounter1();

	static int current_dir;
	static unsigned current_speed;

	if ((update_rate++ % 16) == 0)
	{
		int16_t delta = count - old_count;
		old_count = count;
		Serial.printf( "%d %+6d %+6d => %+6d %+6d\r\n", now, count, delta, target, current_speed);
	}

	int error = target - count;
	int dir = error > 0;

	if (-K_eps < error && error < K_eps)
	{
		// stop the stepper
		analogWrite(STEP_PIN, 0);
		current_speed = 0;
		return;
	}

	if (current_dir != dir && current_speed > MIN_SPEED)
	{
		// we were going the other way, bring the speed back to 0
		current_speed /= 2;
	} else {
		// ramp the speed up to the max
		current_dir = dir;
		if (current_speed < MIN_SPEED)
			current_speed = MIN_SPEED;
		if (current_speed < MAX_SPEED)
			current_speed += SPEED_RAMP;
		//current_speed = (current_speed * 255 + MAX_SPEED) / 256;
	}

	digitalWriteFast(DIR_PIN, current_dir);
	analogWriteFrequency(STEP_PIN, current_speed);
	analogWrite(STEP_PIN, 128); // 50% duty cycle
}
