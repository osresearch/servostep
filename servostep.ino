/** \file
 * Closed loop servo with a stepper motor and quadrature encoder.
 *
 */
#include "QuadDecode.h"


#define K_eps 10	// how close is "good enough"


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

	// run the control loop at 1 KHz
	static unsigned last_now;
	const unsigned now = millis();
	if (now == last_now)
		return;
	last_now = now;

	static unsigned update_rate;
	static int16_t old_count;
	int16_t count = QuadDecode.getCounter1();
	int16_t delta = count - old_count;
	old_count = count;

	if ((update_rate++ % 16) == 0)
		Serial.printf( "%+6d %+6d %+6d\r\n", count, delta, target);

	int error = target - count;

	if (error < 0)
	{
		digitalWriteFast(DIR_PIN, 0);
		error = -error;
	} else {
		digitalWriteFast(DIR_PIN, 1);
	}

	if (error < K_eps)
	{
		// do nothing
	} else
	{
		// step
		digitalWriteFast(STEP_PIN, update_rate & 1);
	}
}
