/** \file
 * Closed loop servo with a stepper motor and quadrature encoder.
 *
 */
#include "QuadDecode.h"

#define K_eps 10	// how close is "good enough"
#define MAX_SPEED	20000
#define SPEED_RAMP	100


#define STEP_PIN 10
#define DIR_PIN 13

void setup()
{
	Serial.begin(115200);  
	pinMode(STEP_PIN, OUTPUT);
	pinMode(DIR_PIN, OUTPUT);

	// 50% duty cycle
	analogWriteFrequency(STEP_PIN, 1);
	analogWrite(STEP_PIN, 128);
}

static int16_t target;

void
servo_cmd(
	int cmd
)
{
	if (cmd < -65536 || cmd > +65535)
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

	// run the control loop at 1 KHz
	static unsigned last_now;
	const unsigned now = micros();
	if (now - last_now < 500)
		return;
	last_now = now;

	static unsigned update_rate;
	static int16_t old_count;
	const int16_t count = QuadDecode.getCounter1();
	const int delta = count - old_count;
	old_count = count;

	static int current_dir;
	static unsigned current_speed;

	if ((update_rate++ % 16) == 0)
	{
		static int16_t print_count;
		int16_t print_delta = count - print_count;
		print_count = count;
		Serial.printf( "%d %+6d %+6d => %+6d %+6d\r\n", now, count, print_delta, target, current_speed);
	}

#if 0
	static int old_target;
	//if (target != old_target)
	{
		if (target != 0)
		{
		//analogWriteFrequency(STEP_PIN, target);
		//analogWrite(STEP_PIN, 128);
		tone(STEP_PIN, target);
		}
		old_target = target;
	}

#else
	int error = target - count;
	int dir = error > 0;

	if (-K_eps < error && error < K_eps)
	{
		// stop the stepper by doing to a 0% duty cycle
cli();
		analogWrite(STEP_PIN, 0);
sei();
		current_speed = -1;
		return;
	}

	if (current_speed == -1)
	{
		// we are resuming motion, restore the duty cycle
cli();
		analogWrite(STEP_PIN, 128);
sei();
		current_speed = 0;
	}

	if (current_dir != dir && current_speed > 0)
	{
		// we were going the other way, bring the speed back to 0
		current_speed /= 2;
	} else {
		// ramp the speed up to the max
		current_dir = dir;
		if (current_speed < MAX_SPEED)
			current_speed += SPEED_RAMP;
/*
		current_speed = (current_speed * 255 + MAX_SPEED) / 256;
*/
	}

	// limit the speed to be proportional to the current change
	// to avoid missing steps
	const int delta_mag = abs(delta);
	if (current_speed > (delta_mag+1) * 2000)
		current_speed = (delta_mag+1) * 2000;

	digitalWriteFast(DIR_PIN, current_dir);

	cli();
	// if interrupts are not disabled when these values are changed
	// it seems that corruption of the registers can result.
	analogWriteFrequency(STEP_PIN, current_speed);
	analogWrite(STEP_PIN, 128);
	sei();
#endif
}
