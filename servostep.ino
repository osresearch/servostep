/** \file
 * Closed loop servo with a stepper motor and quadrature encoder.
 *
 */
#include "QuadDecode.h"

#define K_eps 10	// how close is "good enough"
#define MAX_SPEED	20000
#define SPEED_RAMP	100

static const float dt = 1.0 / 500; // 500 Hz update rate
static const unsigned dt_in_micros = 1.0e6 * dt;
//static const float max_j = 30000; // mm/s^3
static const float max_j = 30000; // mm/s^3
static const float max_a = 10000; // mm/s^2
static const float max_v = 2400; // mm/s
static const float steps_per_mm = (200 * 16) / 350.0;
static const float steps_per_tick =  (200 * 16 / 4096);
static float a = 0;
static float j = 0;
static float v = 0;


#define STEP_PIN 10
#define DIR_PIN 13

void setup()
{
	Serial.begin(115200);  
	pinMode(STEP_PIN, OUTPUT);
	pinMode(DIR_PIN, OUTPUT);

	// 50% duty cycle
	analogWriteFrequency(STEP_PIN, 0);
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


void velocity_cmd(int frequency)
{
	int dir = 1;

	if (frequency < 0)
	{
		frequency = -frequency;
		dir = 0;
	}

#if 0
	uint32_t prescale;

	for (prescale = 0; prescale < 7; prescale++)
	{
		uint32_t minfreq = (F_BUS >> 16) >> prescale;
		if (frequency > minfreq)
			break;
	}

	uint32_t mod = (((F_BUS >> prescale) + (frequency >> 1)) / frequency) - 1;
	if (mod > 65535)
		mod = 65535;

	FTM0_SC = 0;
	FTM0_MOD = mod;
	FTM0_SC = FTM_SC_CLKS(1) | FTM_SC_PS(prescale);


	// make sure we don't glitch by not resetting FMT0_CNT
	// FMT0_CNT = 0;
	// should check that we haven't hit the overflow case
	// if so our next pulse will be delayed
#endif
	analogWriteFrequency(STEP_PIN, frequency);

	// make sure we are going in the right diretion
	digitalWriteFast(DIR_PIN, dir);

	// and that our duty cycle is right
	analogWrite(STEP_PIN, 128);
}


void
compute_speed(
	int	current_count,
	int	current_speed,
	int	target_count,
	int	target_speed,
	int	end_speed
)
{
	// determine what phase we are in:
	// accelerating towards the target speed,
	// declerating towards the target speed
	// maintaining speed
	// decelerating towards the end speed
}


void loop()
{
	// check for a command
	read_command();

	static unsigned last_now;
	const unsigned now = micros();
	if (now - last_now < dt_in_micros)
		return;
	last_now = now;

	if (target < -max_v)
		target = -max_v;
	else
	if (target > +max_v)
		target = +max_v;

	const float err_v = target - v;
	const float eps = 1; // mm/s
  
	// we want to start decelerating when our velocity is
	// within this change. there is a little bit of slop here
	const float max_v_change = a * a / max_j / 2 - 3;

 	if (fabs(err_v) < eps)
	{
		// hopefully we have hit our target
		// and our acceleration ramped to zero so that
		// this is not discontinuous
		a = j = 0;
	} else
	if (err_v > max_v_change)
	{
		// we need to speed up to reach our desired speed.
		// accelerate towards v
		if (a < max_a)
			j = +max_j;
		else
			j = 0;
	} else
	if (err_v > eps)
	{
		// our error is small, we need to start decelerating
		if (a > -max_a)
			j = -max_j;
		else
			j = 0;
	} else
	if (err_v < -max_v_change)
	{
		// accelerate towards target
		if (a > -max_a)
			j = -max_j;
		else
			j = 0;
	} else
	if (err_v < -eps)
	{
		// start decelerating
		if (a < +max_a)
			j = +max_j;
		else
			j = 0;
	}

	a += j * dt;
	v += a * dt;

	velocity_cmd(v * steps_per_mm);

	Serial.printf("%+8.3f %+8.3f %+8.3f %+8d\r\n",
		v,
		a,
		j,
		QuadDecode.getCounter1()
	);
	
#if 0
	// compute the instantaneous delta in position
	static int16_t old_count;
	const int16_t count = QuadDecode.getCounter1();
	int delta = count - old_count;

	// fixup delta if it wraps around
	if (delta > 32768)
		delta -= 65536;
	else
	if (delta < -32768)
		delta += 65536;

	old_count = count;

	// compute a smoothed velocity
	static float speed;
	speed = speed + delta - speed * (dt / 1.0e6);


	if ((update_rate++ % 16) == 0)
	{
		//Serial.printf( "%d %+6d %+6d => %+6d %+6d\r\n", now, count, speed, target, current_speed);
		Serial.printf( "%d %+6d %+6d %+6d => %+6d\r\n", now, count, delta, (int)(10 * speed), target);
	}

	compute_target(count, speed, target, target_speed, end_speed);

#if 0
	static int old_target;
	if (target != old_target)
	{
		velocity_cmd(target);
		old_target = target;
	}


#else
	static int current_dir;
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
	analogWriteFrequency2(STEP_PIN, current_speed);
	analogWrite(STEP_PIN, 128);
	sei();
#endif
#endif
}
