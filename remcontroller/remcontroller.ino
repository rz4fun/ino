#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <Servo.h>

#define DEBUG;

YunServer server(5678);
Servo steer_servo_;
Servo esc_;

int invalid_command_count_;

static char COMMAND_ENGINE_OFF = '0';
static char COMMAND_DRIVE = 'D';
static char COMMAND_STEER = 'S';

static int STEER_PIN = 3;
static int SPEED_PIN = 5;
static int LEFT_TURN_SIGNAL_PIN = 12;
static int RIGHT_TURN_SIGNAL_PIN = 13;
static int BLINK_FREQUENCY = 1;  // 1 up and 1 down per second:

static int STEER_LEFT_STOPPING_ANGLE = 55; //50;
static int STEER_RIGHT_STOPPING_ANGLE = 130; //125;
static int STEER_CENTER = 90;
static int SPEED_ZERO = 90;

// if the number of consecutive invalid commands exceeds this number,
// connection will shutdown automatically.
static int AUTO_SHUTDOWN_THRESHOLD = 15;

inline boolean InitServos() {
  steer_servo_.attach(STEER_PIN);
  esc_.attach(SPEED_PIN);
  if (!steer_servo_.attached()) {
    return false;
  }
  if (!esc_.attached()) {
    return false;
  }
  steer_servo_.write(STEER_CENTER);
  esc_.write(SPEED_ZERO);
  return true;
}


inline void InitServer() {
  server.noListenOnLocalhost();
  server.begin();
}


inline void InitTurnSignals() {
  pinMode(LEFT_TURN_SIGNAL_PIN, OUTPUT);
  pinMode(RIGHT_TURN_SIGNAL_PIN, OUTPUT);
  Blink(LEFT_TURN_SIGNAL_PIN);
  Blink(RIGHT_TURN_SIGNAL_PIN);
  Blink(LEFT_TURN_SIGNAL_PIN);
  Blink(RIGHT_TURN_SIGNAL_PIN);
}


void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  Bridge.begin();
  digitalWrite(13, LOW);
  InitTurnSignals();
  InitServer();
}


void Blink(int pin_id) {
  digitalWrite(pin_id, HIGH);
  delay(500);
  digitalWrite(pin_id, LOW);
}


boolean ProcessTextualCommand(YunClient& client) {
  String command = client.readStringUntil('#');
  if (command == "") {
    if (++invalid_command_count_ == AUTO_SHUTDOWN_THRESHOLD) {
      return false;
    }
  } else {
    invalid_command_count_ = 0;
  }
  char instruction = command[0];
  if (instruction == COMMAND_ENGINE_OFF) {
    // turn off
    return false;
  }
  int value = command.substring(1).toInt();
  if (instruction == COMMAND_DRIVE) {
    // set speed
  #ifdef DEBUG
    Serial.print("Speed = ");
    Serial.println(value);
  #endif
    esc_.write(value);
  } else if (instruction == COMMAND_STEER) {
    // set steer
  #ifdef DEBUG
    Serial.print("Steer = ");
    Serial.println(value);
  #endif
    if (value < STEER_CENTER) {
      Blink(LEFT_TURN_SIGNAL_PIN);
    } else if (value > STEER_CENTER) {
      Blink(RIGHT_TURN_SIGNAL_PIN);
    }
    steer_servo_.write(
        value < STEER_CENTER ?
            (value < STEER_LEFT_STOPPING_ANGLE ? STEER_LEFT_STOPPING_ANGLE : value) :
            (value > STEER_RIGHT_STOPPING_ANGLE ? STEER_RIGHT_STOPPING_ANGLE : value));
  }
  delay(25);
  return true;
}


void loop() {
  YunClient client = server.accept();
  if (client) {
    digitalWrite(13, HIGH);
    delay(2000);
    if (!InitServos()) {
      steer_servo_.detach();
      esc_.detach();
      client.stop();
      return;
    }
    digitalWrite(13, LOW);
    invalid_command_count_ = 0;
    while (ProcessTextualCommand(client)) {}
    steer_servo_.detach();
    esc_.detach();
    client.stop();
  }
}

#undef DEBUG

