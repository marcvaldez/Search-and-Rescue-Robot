#include <Servo.h>

#include <Wire.h> // for i2c communication

#define I2C_BUS 0x09 // arbitrary unused address in I2C

#define PIN_SERVO 4
#define PIN_LASER 6
#define PIN_TRIG 2
#define PIN_ECHO 3

// pins for controlling the relays
#define PIN_L1 9
#define PIN_L2 10
#define PIN_R1 11
#define PIN_R2 13

Servo myservo;

volatile bool laser_state = false;

int msg; // message from i2c bus

float cm;

void setup() {
  Serial.begin(9600);

  pinMode(PIN_L1, OUTPUT);
  pinMode(PIN_L2, OUTPUT);
  pinMode(PIN_R1, OUTPUT);
  pinMode(PIN_R2, OUTPUT);

  pinMode(PIN_LASER, OUTPUT);

  // set initial state
  robot_stop();

  myservo.attach(PIN_SERVO);
  myservo.write(90);

  // start the i2c bus as slave
  Wire.begin(I2C_BUS);
  // attach delegate for receiving messages
  Wire.onReceive(receiveEvent);
}

void loop() {
  if (msg == 1) {
    forward();
  }
  else if (msg == 2) {
    reverse();
  }
  else if (msg == 3) {
    turn_left();
  }
  else if (msg == 4) {
    turn_right();
  }
  else if (msg == 5) {
    robot_stop(); // stop first before firing
    fire_gun();
  }
  else if (msg == 6) {
    toggle_laser();
  }
  else if (msg == 7) {
    nudge_left();
  }
  else if (msg == 8) {
    nudge_right();
  }
  else {
    robot_stop();
  }
  sense_collission();
  delay(100);
}

void receiveEvent(int bytes) {
  msg = Wire.read();    // read one character from the I2C bus
}

void fire_gun() {
    Serial.println("fire!");

    myservo.write(0);
    delay(200);
    myservo.write(90);
    robot_stop();
}

void robot_stop() {
    digitalWrite(PIN_L1, LOW);
    digitalWrite(PIN_L2, LOW);
    digitalWrite(PIN_R1, LOW);
    digitalWrite(PIN_R2, LOW);
    //Serial.println("stop");
    msg = 0;
}

void forward() {
    digitalWrite(PIN_L1, LOW);
    digitalWrite(PIN_L2, HIGH);
    digitalWrite(PIN_R1, LOW);
    digitalWrite(PIN_R2, HIGH);
    //Serial.println("forward");
}

void reverse() {
    digitalWrite(PIN_L1, HIGH);
    digitalWrite(PIN_L2, LOW);
    digitalWrite(PIN_R1, HIGH);
    digitalWrite(PIN_R2, LOW);
    //Serial.println("backward");
}

void turn_left() {
    digitalWrite(PIN_L1, LOW);
    digitalWrite(PIN_L2, HIGH);
    digitalWrite(PIN_R1, HIGH);
    digitalWrite(PIN_R2, LOW);
    //Serial.println("left");
}

void turn_right() {
    digitalWrite(PIN_L1, HIGH);
    digitalWrite(PIN_L2, LOW);
    digitalWrite(PIN_R1, LOW);
    digitalWrite(PIN_R2, HIGH);
    //Serial.println("right");
}

void nudge_left() {
  turn_left();
  delay(100);
  robot_stop();
}

void nudge_right() {
  turn_right();
  delay(100);
  robot_stop();
}

void toggle_laser() {
  if (laser_state) {
    laser_state = false;
    digitalWrite(PIN_LASER, LOW);
  }
  else {
    laser_state = true;
    digitalWrite(PIN_LASER, HIGH);
  }
  robot_stop();
}

void sense_collission() {
  digitalWrite(PIN_TRIG, LOW);       
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  cm = pulseIn(PIN_ECHO, HIGH) / 58.0; //The echo time is converted into cm
  cm = (int(cm * 100.0)) / 100.0;     //Keep two decimal places
  Serial.print("Distance\t=\t");
  Serial.print(cm);
  Serial.print("cm");
  Serial.println();
  if (cm < 15 && cm > 2 && msg != 2) // stop when near obstacle (15 cm), filter outliers, reverse always allowed
    robot_stop();
}
