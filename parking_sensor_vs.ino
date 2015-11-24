#include <stdarg.h>

#define ARDBUFFER 100
int ardprintf(char *str, ...)
{
	int i, count = 0, j = 0, flag = 0;
	char temp[ARDBUFFER + 1];
	for (i = 0; str[i] != '\0'; i++)  if (str[i] == '%')  count++;

	va_list argv;
	va_start(argv, count);
	for (i = 0, j = 0; str[i] != '\0'; i++)
	{
		if (str[i] == '%')
		{
			temp[j] = '\0';
			Serial.print(temp);
			j = 0;
			temp[0] = '\0';

			switch (str[++i])
			{
			case 'd': Serial.print(va_arg(argv, int));
				break;
			case 'l': Serial.print(va_arg(argv, long));
				break;
			case 'f': Serial.print(va_arg(argv, double));
				break;
			case 'c': Serial.print((char)va_arg(argv, int));
				break;
			case 's': Serial.print(va_arg(argv, char *));
				break;
			default:;
			};
		}
		else
		{
			temp[j] = str[i];
			j = (j + 1) % ARDBUFFER;
			if (j == 0)
			{
				temp[ARDBUFFER] = '\0';
				Serial.print(temp);
				temp[0] = '\0';
			}
		}
	};
	Serial.println();
	return count + 1;
}

#define NUMBER_OF_LEDS 6
#define FIRST_RED_LED 4
#define MAX_DISTANCE 124L
#define SLEEP_DISTANCE_TOLERANCE 4 //tolerance for movement while in SLEEP without becoming ACTIVE
#define SECONDS_UNTIL_SLEEP 20

int ledPins[] = { 11, 10, 9, 6, 5, 3 };
long ledInches[] = { 100L, 76L, 52L, 28L, 28L, 22L };
//long ledInches[] = {36L, 30L, 24L, 18L, 12L, 6L};

typedef enum { ACTIVE, SLEEP } State;

State curState;

int trigPin = 12;    //Trig - orange Jumper
int echoPin = 13;    //Echo - white/orange Jumper
int powerPin = 8;    //solid blue
// ground is blue/white
long sleepInches;
long sleepTime;

void setup() {

	/* set initial states */
	curState = ACTIVE;
	sleepInches = -1;
	sleepTime = millis() / 1000;

#ifdef DEBUG
	//Serial Port begin
	Serial.begin(9600);
#endif

	/* define inputs and outputs and turn on the sensor */
	pinMode(trigPin, OUTPUT);
	pinMode(echoPin, INPUT);
	pinMode(powerPin, OUTPUT);
	digitalWrite(powerPin, HIGH);

	/* turn on LEDs */
	for (int i = 0; i < NUMBER_OF_LEDS; i++)
	{
		pinMode(ledPins[i], OUTPUT);
	}
}

void loop()
{
	// The sensor is triggered by a HIGH pulse of 10 or more microseconds.
	// Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
	digitalWrite(trigPin, LOW);
	delayMicroseconds(10);
	digitalWrite(trigPin, HIGH);
	delayMicroseconds(15);
	digitalWrite(trigPin, LOW);

	// Read the signal from the sensor: a HIGH pulse whose
	// duration is the time (in microseconds) from the sending
	// of the ping to the reception of its echo off of an object.
	pinMode(echoPin, INPUT);
	long duration = pulseIn(echoPin, HIGH);
	long inches = (duration / 2) / 74;

#ifdef DEBUG
	Serial.print(inches);
	Serial.print(" inches");
	Serial.println();
#endif

	/* if the duration was 0 then there was an error.
	so reset the sensor by going low for a quarter of a second and loop again
	*/
	if (duration == 0)
	{
		digitalWrite(powerPin, LOW);
		delay(250);
		digitalWrite(powerPin, HIGH);
		return;
	}

	long seconds = millis() / 1000;

	/* this deals with wraparound of the timer */
	if (sleepTime > seconds)
	{
		sleepTime = seconds;
		curState = ACTIVE;
	}

	if (curState == ACTIVE)
	{
		/* if only little movement in the last SECONDS_UNTIL_SLEEP then go to sleep and turn off all LEDS*/
		if (abs(inches - sleepInches) < SLEEP_DISTANCE_TOLERANCE)
		{
			if (seconds - sleepTime > SECONDS_UNTIL_SLEEP)
			{
				curState = SLEEP;
				for (int i = 0; i < NUMBER_OF_LEDS; i++)
				{
					analogWrite(ledPins[i], 0);
				}
				return;
			}
		}
		else {
			/* if we've moved beyond the SLEEP_DISTANCE_TOLERANCE, then reset the sleep position and the sleep time*/
			sleepInches = inches;
			sleepTime = seconds;
		}

		// color the leds
		for (int i = 0; i < NUMBER_OF_LEDS; i++)
		{
			long earlierStep = i == 0 ? MAX_DISTANCE : ledInches[i - 1];

#ifdef DEBUG
			ardprintf("led: %d earlierSteps: %l ledInches: %l width: %l\n", i, earlierStep, ledInches[i],
				earlierStep - ledInches[i]);
#endif
			/* calculate the intensity based on where the reading is within the this led's inches and the previous led's inches*/
			float intensity = (float)(earlierStep - inches) / (float)(earlierStep - ledInches[i]);
			
			/* if we're outside of span of this led's and the previous one, then set the value to full (255) or off (0)*/
			int byteIntensity = max(min(intensity * 255, 255), 0);

			/* once we get to within the range of a red led, then turn it full on.  No fading for these*/
			if (i >= FIRST_RED_LED && inches < ledInches[i])
			{
				byteIntensity = 255;
			}
#ifdef DEBUG       
			ardprintf("float intensity %f", intensity);
			ardprintf("byte intensity %d", byteIntensity);
#endif
			analogWrite(ledPins[i], byteIntensity);
			/* if we're active, wait a 20th of a second and loop again */
			delay(50);
		}
	}
	else /* then curState == SLEEP */
	{
		/* wake up if the object has moved */
		if (abs(inches - sleepInches) > SLEEP_DISTANCE_TOLERANCE)
		{
			curState = ACTIVE;
			sleepInches = inches;
			sleepTime = seconds;
			return;
		}
		/* if in sleep mode, then wait half a second and then restart the loop*/
		delay(500);
	}
}
