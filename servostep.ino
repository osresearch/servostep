/** \file
 * Closed loop servo with a stepper motor and quadrature encoder.
 *
 */
#include "QuadDecode.h"

static int16_t Count1;
static int16_t Count2;

void setup()
{
	Serial.begin(115200);  
}

void loop()
{
	Serial.printf( "%+6d %+6d\r\n",
		QuadDecode.getCounter1(),
		QuadDecode.getCounter2()
	);
}
