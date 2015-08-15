#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <Servo.h>

#define DEBUG

YunServer server_(5678);
YunClient client_;
Servo steer_servo_;
Servo esc_;

int invalid_command_count_;

#define SECURITY_TOKEN "308ac3d3d02a3e6c0efe8e1a3f17df3d"
static uint8_t confirmation_ = 0x08;

#define COMMAND_ENGINE_OFF '0'
#define COMMAND_DRIVE 'D'
#define COMMAND_STEER 'S'
#define COMMAND_LIGHT 'L'
#define COMMAND_HAZARD 'H'

#define STEER_PIN 3
#define SPEED_PIN 5
#define LEFT_TURN_SIGNAL_PIN 7
#define RIGHT_TURN_SIGNAL_PIN 8
#define HEADLIGHT_SIGNAL_PIN 12

#define STEER_LEFT_STOPPING_ANGLE 55 //50;
#define STEER_RIGHT_STOPPING_ANGLE 130 //125;
#define STEER_CENTER 90
#define SPEED_ZERO 90

#define LIGHT_ON 1
#define LIGHT_OFF 0

// if the number of consecutive invalid commands exceeds this number,
// connection will shutdown automatically.
static int AUTO_SHUTDOWN_THRESHOLD = 15;

#define STREAM_TIMEOUT 40  // client read timeout milliseconds

unsigned long current_timestamp_;
unsigned long previous_timestamp_;
// LED/pin state
static int left_turn_signal_pin_state_;
static int right_turn_signal_pin_state_;
// whether the signal should be "turned" on
static boolean left_turn_signal_on_;
static boolean right_turn_signal_on_;
static boolean hazard_blinker_on_;
#define TURN_SIGNAL_LENGTH 400  // blink once per 400 millisec.


inline boolean InitServos() {
  steer_servo_.attach(STEER_PIN);
  if (!steer_servo_.attached()) {
    return false;
  }
  steer_servo_.write(STEER_CENTER);
  esc_.attach(SPEED_PIN);
  if (!esc_.attached()) {
    return false;
  }
  esc_.write(SPEED_ZERO);
  return true;
}


inline void InitTurnSignals() {
  pinMode(LEFT_TURN_SIGNAL_PIN, OUTPUT);
  pinMode(RIGHT_TURN_SIGNAL_PIN, OUTPUT);
  left_turn_signal_pin_state_ = LOW;
  left_turn_signal_on_ = false;
  right_turn_signal_pin_state_ = LOW;
  right_turn_signal_on_ = false;
  digitalWrite(LEFT_TURN_SIGNAL_PIN, LOW);
  digitalWrite(RIGHT_TURN_SIGNAL_PIN, LOW);
  hazard_blinker_on_ = false;
}


inline void InitMainLights() {
  pinMode(HEADLIGHT_SIGNAL_PIN, OUTPUT);
  digitalWrite(HEADLIGHT_SIGNAL_PIN, LOW);
}


/**
 * Verify the connection
 */
inline int Handshake() {
  String const& handshake_message = client_.readStringUntil('#');
  if (handshake_message == SECURITY_TOKEN) {
#ifdef DEBUG
    Serial.print("security token received: ");
    Serial.println(handshake_message);
#endif
    server_.write(confirmation_);
    return 0;
  }
  return 1;
}


inline void InitServer() {
  server_.noListenOnLocalhost();
  server_.begin();
  InitTurnSignals();
  InitMainLights();
  Serial.println("Starting server");
  while (true) {
    client_ = server_.accept();
    if (client_) {
      client_.setTimeout(STREAM_TIMEOUT);
      if (Handshake()) {
        client_.stop();
        continue;
      } 
      if (!InitServos()) {
        steer_servo_.detach();
        esc_.detach();
        client_.stop();
        // serious error, terminate
        return;
      }
      digitalWrite(13, HIGH);
      delay(500);
      digitalWrite(13, LOW);
      previous_timestamp_ = current_timestamp_ = millis();
      invalid_command_count_ = 0;
      return;
    }
  }
}


void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  Bridge.begin();
  digitalWrite(13, LOW);
  InitServer();
}


boolean ProcessTextualCommand(YunClient& client) {
  String const& command = client.readStringUntil('#');
  if (command == "") {
    if (++invalid_command_count_ == AUTO_SHUTDOWN_THRESHOLD) {
      //return false;
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
      right_turn_signal_on_ = false;
      digitalWrite(RIGHT_TURN_SIGNAL_PIN, LOW);
      left_turn_signal_on_ = true;
    } else if (value > STEER_CENTER) {
      left_turn_signal_on_ = false;
      digitalWrite(LEFT_TURN_SIGNAL_PIN, LOW);
      right_turn_signal_on_ = true;
    } else {
      left_turn_signal_on_ = false;
      digitalWrite(LEFT_TURN_SIGNAL_PIN, LOW);
      right_turn_signal_on_ = false;
      digitalWrite(RIGHT_TURN_SIGNAL_PIN, LOW);
    }
    steer_servo_.write(
        value < STEER_CENTER ?
            (value < STEER_LEFT_STOPPING_ANGLE ? STEER_LEFT_STOPPING_ANGLE : value) :
            (value > STEER_RIGHT_STOPPING_ANGLE ? STEER_RIGHT_STOPPING_ANGLE : value));
  } else if (instruction == COMMAND_LIGHT) {
#ifdef DEBUG
    Serial.print("Light = ");
    Serial.println(value);
#endif
    value == LIGHT_OFF ?
        digitalWrite(HEADLIGHT_SIGNAL_PIN, LIGHT_OFF) : digitalWrite(HEADLIGHT_SIGNAL_PIN, LIGHT_ON);
  } else if (instruction == COMMAND_HAZARD) {
    hazard_blinker_on_ = (value == 1); 
  }
  return true;
}


void loop() {
  current_timestamp_ = millis();
  if (current_timestamp_ - previous_timestamp_ > TURN_SIGNAL_LENGTH) {
    previous_timestamp_ = current_timestamp_;
    if (left_turn_signal_on_ || hazard_blinker_on_) {
      left_turn_signal_pin_state_ == LOW ?
          left_turn_signal_pin_state_ = HIGH : left_turn_signal_pin_state_ = LOW;
      digitalWrite(LEFT_TURN_SIGNAL_PIN, left_turn_signal_pin_state_);
    }
    if (right_turn_signal_on_ || hazard_blinker_on_) {
      right_turn_signal_pin_state_ == LOW ?
          right_turn_signal_pin_state_ = HIGH : right_turn_signal_pin_state_ = LOW;
      digitalWrite(RIGHT_TURN_SIGNAL_PIN, right_turn_signal_pin_state_);
    }
  }
  if (client_) {
    if (!ProcessTextualCommand(client_)) {
      steer_servo_.detach();
      esc_.detach();
      client_.stop();
      Serial.println("Terminate control.");
      InitServer();
      return;
    }
  }
  delay(5);
}

#undef DEBUG

