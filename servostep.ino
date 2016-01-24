/** \file
 * Closed loop servo with a stepper motor and quadrature encoder.
 *
 */
#include "QuadDecode.h"

static int16_t old_count;

#define STEP_PIN 10
#define DIR_PIN 13

void setup()
{
	Serial.begin(115200);  
	pinMode(STEP_PIN, OUTPUT);
	pinMode(DIR_PIN, OUTPUT);
}

void loop()
{
	int16_t count = QuadDecode.getCounter1();
	int16_t delta = count - old_count;

	Serial.printf( "%+6d %+6d\r\n", count, delta);
	old_count = count;

	if (delta == 0)
		return;

	int sign = 1;
	if (delta < 0)
	{
		sign = 0;
		delta = -delta;
	}
	
	digitalWriteFast(DIR_PIN, sign);
	delayMicroseconds(100);
	//for(int i = 0 ; i < delta ; i++)
	{
		digitalWriteFast(STEP_PIN, 1);
		delayMicroseconds(10000);
		digitalWriteFast(STEP_PIN, 0);
		delayMicroseconds(10000);
	}
}
