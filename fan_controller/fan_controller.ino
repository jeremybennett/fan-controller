/* Fan control.

   Takes analog input from NZXT Sentry 3 Fan Controller and converts to PWM
   for Turnigy Brushed ESC fan control. Feeds back estimated RPM as FM

   Contributor: Peter Bennett <thelargeostrich@gmail.com>
   Contributor: Jeremy Bennett <jeremy@jeremybennett.com>

   SPDX-License-Identifier: GPL-3.0-or-later */

/* The fan is being controlled from a standard PC fan controller card. This
   generates and output in the range 0-5V, which would normally we used to
   power a simple analogue fan (where lower voltage goes slower).

   However, our fan is driven by an external Turnigy Brushed ESC.  This takes
   as an input a 50Hz PWD in servo format.  In this case a write cycle of
   1000us corresponds to zero RPM and 2000us corresponds to maximum RPM
   (estimated 2500 RPM for this particular fan).

   We need to send a signal back to the PC fan controller card, to report on
   the RPM being used.  This a frequency modulated (FM) signal - expecting two
   pulses per rotation of the fan (known as "tone").  However the lowest
   frequency that can be sent is 31Hz (corresponding to 31 / 2 * 60 = 930
   RPM).

   The PC keeps a rolling average of the frequency data it is sent.  Thus to
   report 465 RPM we can alternate reporting zero and 930 RPM. */

#include <Servo.h>

/* Pins */
const int FAN_CONTROLLER_PIN = A0;	/* Input from PC */
const int RPM_TONE_PIN = 10;		/* Output to PC */
const int SERVO_PIN = 9;		/* Output to Turnigy ESC */

const int MAX_RPM = 2500;                  /* Estimate of max RPM */
const int MIN_RPM_FOR_TONE = 31 / 2 * 60;  /* Lowest tone possible is 31Hz */

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
}

void loop() {

  /* If RPM is below the valued we can report, we accumulate the difference to
     send next time. In this way the rolling average will the actual RPM we
     are reading. */
  static int rpmError = 0;

  /* The input from the PC */
  float fanControllerOutput = (float)analogRead(FAN_CONTROLLER_PIN) / 1023.0;
  /* Set the fan ESC speed. 1000 is min, 2000 is max */
  brushedESC.writeMicroseconds(1000 + (int)(fanControllerOutput * 1000.0));
  /* Estimated RPM value is proportional to the cube root of the fan
     controller signal */
  int rpmEst = (int) ((float) MAX_RPM * pow (fanControllerOutput, 1.0 / 3.0));

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

  /* Don't need to thrash this. 10 times a second is enough. */
  delay(100);

}	/* loop () */
