/* Fan control.

   Takes analog input from NZXT Sentry 3 Fan Controller and converts to PWM
   for Turnigy Brushed ESC fan control. Feeds back estimated RPM as FM

   Contributor: Peter Bennett <thelargeostrich@gmail.com>
   Contributor: Jeremy Bennett <jeremy@jeremybennett.com>

   SPDX-License-Identifier: GPL-3.0-or-later */

/* The fan is being controlled from a standard PC fan controller card. This
   generates and output in the range 0-12V, which would normally we used to
   power a simple analogue fan (where lower voltage goes slower).

   The controller relies on feedback to determine if it has sent the correct
   voltage. The voltage never goes below 6.5V, which is used for 40%
   fan. Because we are using TTL, we need to scale this, so divide by 3 to
   give a range up to 4V.

   However, our fan is driven by an external Turnigy Brushed ESC.  This takes
   as an input a 50Hz PWD in servo format.  In this case a write cycle of
   1000us corresponds to zero RPM and 2000us corresponds to maximum RPM
   (estimated 2500 RPM for this particular fan).

   We need to send a signal back to the PC fan controller card, to report on
   the RPM being used.  This a frequency modulated (FM) signal - expecting two
   pulses per rotation of the fan (known as "tone").  However the lowest
   frequency that can be sent is 31Hz (corresponding to 31 / 2 * 60 = 930
   RPM).

   For now, in the absence of an RPM counter on the fan, we need to generate an
   appropriate RPM. We shall do this by mapping voltage to frequency and
   report the frequency back that is expected.

   The PC keeps a rolling average of the frequency data it is sent.  Thus to
   report 465 RPM we can alternate reporting zero and 930 RPM. */

#include <Servo.h>

/* Pins */
const int FAN_CONTROLLER_PIN = A0;	/* Input from PC */
const int RPM_TONE_PIN = 10;		/* Output to PC */
const int SERVO_PIN = 9;		/* Output to Turnigy ESC */

const int MAX_RPM = 2500;                  /* Estimate of max RPM */
const int MIN_RPM_FOR_TONE = 31 / 2 * 60;  /* Lowest tone possible is 31Hz */

/* Parameters for mapping analog input to voltages */
const float MAX_V = 12.0;
const float MAX_ANALOG_VAL = 833;	/* Corresponds to 12V */

/* Parameters for linear mapping of speed voltages */
const float CONV_M = 0.6 / 5.5;
const float CONV_C = 1.0 - (0.6 * 12.0 / 5.5);

/* The ESC controll structure */
Servo brushedESC;


void setup() {
  /* Set AREF to internal 5V */
  analogReference(DEFAULT);
  /* Set ESC as Servo output */
  brushedESC.attach(SERVO_PIN);
  /* 2000 is full, this is effectively zero */
  brushedESC.writeMicroseconds(1000);
  /* Tone is an output pin */
  pinMode(RPM_TONE_PIN, OUTPUT);

  /* Allow logging to serial output */
  Serial.begin (9600);
}

/* Convert an analog input representing a scaled voltage to a percentage speed
   requested.

   1023 is the top analog input, corresponding to 5V. We have divided our 12V
   signal by 3, so the maximum we ever see is 4V.  The lowest input we can get
   is 6.5V / 3, corresponding to 40% speed, unless we get zero. We interpolate
   accordingly. The formula is:

     pc_speed = 0.10909 * orig_v + 1.30909 */

float fc2pc (int  analog_val)
{
  float orig_v = ((float) analog_val) / ((float) MAX_ANALOG_VAL) * MAX_V;
  float pc = CONV_M * orig_v + CONV_C;

  return (pc < 0.0) ? 0.0 : pc;
}

void loop() {

  /* If RPM is below the valued we can report, we accumulate the difference to
     send next time. In this way the rolling average will the actual RPM we
     are reading. */
  static int rpmError = 0;
  static float roll_av_pc = 0.0;
  static float roll_av_rpm = 0.0;

  /* The input from the PC. This is scaled to 4V, but 1023 corresponds to 4V,
     so we scale accordingly. */
  float speedPercent = fc2pc (analogRead(FAN_CONTROLLER_PIN));
  /* Set the fan ESC speed. 1000 is min, 2000 is max */
  brushedESC.writeMicroseconds(1000 + (int)(speedPercent * 1000.0));
  int rpmEst = (int) ((float) MAX_RPM * speedPercent);

  roll_av_pc = roll_av_pc * 0.9 + speedPercent * 0.1;
  roll_av_rpm = roll_av_rpm * 0.9 + rpmEst * 0.1;

  /* Logging to serial */
  Serial.print ("Percent speed: ");
  Serial.print ((int) (roll_av_pc * 100.0));
  Serial.print ("%  RPM: ");
  Serial.println ((int) roll_av_rpm);

  if ((rpmEst + rpmError) >= MIN_RPM_FOR_TONE)
    {
      /* RPM is high enough, so just report it straight back to the PC as a
	 tone in Hz, adding in any accumulated error, which can then be
	 discarded.  Remember two pulses per
	 revolution. */
      tone (RPM_TONE_PIN, (rpmEst + rpmError) * 2 / 60);
      rpmError = 0;
    }
  else
    {
      /* Not high enough yet, just accumulate the error. */
      rpmError = rpmError + rpmEst;
      noTone (RPM_TONE_PIN);
    }

  /* Don't need to thrash this. 5 times a second is enough. */
  delay(200);

}	/* loop () */
