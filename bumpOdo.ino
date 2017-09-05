#include "motordriver_4wd.h"
#include "seeed_pwm.h"
#include <ChainableLED.h>
#include <math.h>
#define NUM_LEDS  1

const int buttonPin = SCL;
const int pingPin = 11;
bool moveForward = true;
int buttonState = 0;
int avg = 0;
long dist[5];
long fdist = 100L;
long minDist = 15L;
bool phaseOne = true;
bool phaseTwo = false;
ChainableLED leds(0, 1, NUM_LEDS);

//PhaseTwo
double x, y, theta;
double dx = 0, dy = 0;
const double rw = 4.;
const double d = 17.;
const double trot = 72.;
const double PIE = 3.14159265;
const double PIE_2 = PIE/2.;
const double PIE2 = PIE*2.;
const double METER = 100.;
long left_encoder_count = 0, right_encoder_count = 0;
int left_dirn = 1, right_dirn = 1;

void setup() 
{
  // put your setup code here, to run once:
  pinMode(buttonPin, INPUT);
  MOTOR.init();
  leds.init();
  attachInterrupt(0, LeftEncoder, CHANGE);
  attachInterrupt(1, RightEncoder, CHANGE);

  // initial values x,y,theta. Assume starting movement along Y-axis
  x = y = 0;
  theta = PIE_2;

  for(int i = 0; i < 5; i++)
    dist[i] = 100L;
  forward();
  //Serial.begin(9600);
}

void loop() 
{
  // put your main code here, to run repeatedly:
  buttonState = digitalRead(buttonPin);
  if (phaseOne)
  { 
    if (buttonState)
    {
      backward();
      delay(290);
      noMove();
      delay(1025);
      int index = 0;
      delay(1000);
      chacha();
      for(int i = 0; i < 5; i++)
      {
        dist[i] = rangeFind();
        if (dist[i] > 0) index = i;
        //avg += dist[i];
      }
      while(dist[index] == 0)
      {
        chacha();
        for(int i = 0; i < 5; i++)
        {
          dist[i] = rangeFind();
          if (dist[i] > 0) index = i;
        }
        delay(250);
      }
      if(dist[index] > minDist)
      {
        turnRight();
      }
      else
      {
        turnLeft();
      }
      noMove();
      delay(1500);
    }
    else if (!buttonState && !moveForward)
    {
      forward();
      leds.setColorRGB(0, 0, 150, 0);
    }
    if (y >= METER)
    {
      noMove();
      phaseOne = false;
      phaseTwo = true;
      x = y = 0;
      theta = PIE_2;
      right_encoder_count = left_encoder_count = 0;
      leds.setColorRGB(0,255,0,0);
      delay(1000);
      
      //delay(50000);
    }
  }
  else if (phaseTwo)
  {
    //THIS BE A TEST. CHANGE IT LATAH 4/14/16
    if(buttonState)
    {
       backward();
       delay(290);
       noMove();
       delay(1500);
       //right
       turnRight();
       noMove();
       delay(1500);
       forward();
       leds.setColorRGB(0, 255, 0, 0);       
    }
    else if (!buttonState && !moveForward) 
    {
      forward();
      leds.setColorRGB(0, 255, 0, 0);
    }
    calcX();
    calcY();
    if(y >= METER && !(x >= METER))
    {
      backward();
      delay(300);
      turnRight();
      forward();
    }
    else if(!(y >= METER) && x >= METER)
    {
      backward();
      delay(300);
      turnLeft();
      forward();
    }
    else if(y >= METER && x >= METER)
    {
      backward();
      delay(250);
      noMove();
      phaseTwo = false;

    }
  }
  right_encoder_count = left_encoder_count = 0;
}

void forward()
{
    leds.setColorRGB(0, 0, 150, 0);
    left_dirn = right_dirn = 1;
    MOTOR.setSpeedDir1(17, DIRF); //Set motor 1 and motor 2 direction:DIRF, Speed:80 (range:0-100).
    MOTOR.setSpeedDir2(15, DIRR); //Set motor 1 and motor 2 direction:DIRR, Speed:80 (range:0-100).
    moveForward = true;
}

void chacha()
{
    leds.setColorRGB(0, 255, 105, 180);
    MOTOR.setSpeedDir1(17, DIRF); //Set motor 1 and motor 2 direction:DIRF, Speed:80 (range:0-100).
    MOTOR.setSpeedDir2(15, DIRF); //Set motor 1 and motor 2 direction:DIRR, Speed:80 (range:0-100).
    delay(250);
    MOTOR.setSpeedDir1(17, DIRR); //Set motor 1 and motor 2 direction:DIRF, Speed:80 (range:0-100).
    MOTOR.setSpeedDir2(15, DIRR); //Set motor 1 and motor 2 direction:DIRR, Speed:80 (range:0-100).
    delay(250);
    noMove();
}
void backward()
{
    leds.setColorRGB(0, 150, 0, 0);
    left_dirn = right_dirn = -1;
    theta = -PIE_2;
    MOTOR.setSpeedDir1(15, DIRR);
    MOTOR.setSpeedDir2(17, DIRF);
    moveForward = false;
}

void turnRight()
{
    leds.setColorRGB(0, 0, 0, 150);    
    MOTOR.setSpeedDir1(27, DIRF);
    MOTOR.setSpeedDir2(25, DIRF);
    theta -= PIE_2;
    if(theta <= -PIE2)
    {
      theta += PIE2;
    }
    moveForward = false;
    delay(670);
}

void turnLeft()
{
    leds.setColorRGB(0, 150, 150, 0);
    MOTOR.setSpeedDir1(27, DIRR);
    MOTOR.setSpeedDir2(25, DIRR);
    theta += PIE_2;
    if(theta >= PIE2)
    {
      theta -= PIE2;
    }
    moveForward = false;
    delay(740);
}

void noMove()
{
    MOTOR.setStop1();
    MOTOR.setStop2();
    moveForward = false;
}

long rangeFind() {
  long duration, inches, cm;
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);

  // The same pin is used to read the signal from the PING))): a HIGH
  // pulse whose duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(pingPin, INPUT);
  duration = pulseIn(pingPin, HIGH);

  // convert the time into a distance
  inches = microsecondsToInches(duration);
  cm = microsecondsToCentimeters(duration);
  if (cm > 100)
  {
    //cm = 0;
  }
  ////Serial.println(cm);
  return cm;
}
long microsecondsToInches(long microseconds) {
  // According to Parallax's datasheet for the PING))), there are
  // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
  // second).  This gives the distance travelled by the ping, outbound
  // and return, so we divide by 2 to get the distance of the obstacle.
  // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
  return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds) {
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}



// cosine and sine should always be 1 or 0

// NOTICE: Do we need left and right encoder counts? Might have to worry about turns changing the counts. Perhaps 0 them out after a turn or something like that before calculating values x,y
void calcX()
{
  dx = PIE * rw * cos(theta) * ((double)(left_encoder_count + right_encoder_count) / trot);
  x += dx;
}

void calcY()
{
  dy = PIE * rw * sin(theta) * ((double)(left_encoder_count + right_encoder_count) / trot);
  y += dy;  
}

void LeftEncoder()
{
  left_encoder_count = left_encoder_count + left_dirn;
}

void RightEncoder()
{
  right_encoder_count = right_encoder_count + right_dirn;
}
