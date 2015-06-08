#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <Servo.h>

#define DEBUG;

YunServer server(5678);
Servo steer_servo_;

int invalid_command_count_;

static char ENGINE_OFF = '0';
static char DRIVE_FORWARD = 'F';
static char DRIVE_BACKWARD = 'B';
static char STEER_LEFT = 'L';
static char STEER_RIGHT = 'R';

static int STEER_PIN = 3;
static int SPEED_PIN = 5;

static int STEER_LEFT_STOPPING_ANGLE = 50;
static int STEER_RIGHT_STOPPING_ANGLE = 125;

// if the number of consecutive invalid commands exceeds this number,
// connection will shutdown automatically.
static int AUTO_SHUTDOWN_THRESHOLD = 15;

inline boolean InitServo() {
  steer_servo_.attach(STEER_PIN);
  if (steer_servo_.attached()) {
    steer_servo_.write(90);
    return true;
  }
  return false;
}


inline void InitServer() {
  server.noListenOnLocalhost();
  server.begin();
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
  String command = client.readStringUntil('#');
  if (command == "") {
    if (++invalid_command_count_ == AUTO_SHUTDOWN_THRESHOLD) {
      return false;
    }
  } else {
    invalid_command_count_ = 0;
  }
  char instruction = command[0];
  if (instruction == ENGINE_OFF) {
    // turn off
    return false;
  }
  int value = command.substring(1).toInt();
  if (instruction == DRIVE_FORWARD) {
    // set speed
  #ifdef DEBUG
    Serial.print("Speed = ");
  #endif
    Serial.println(value);
  } else if (instruction == DRIVE_BACKWARD) {
    // set speed
  #ifdef DEBUG
    Serial.print("Speed = ");
  #endif
    Serial.println(-value);
  } else if (instruction == STEER_LEFT) {
    // set steer
  #ifdef DEBUG
    Serial.print("Steer = Left:");
    Serial.println(value);
  #endif
    steer_servo_.write(
        value < STEER_LEFT_STOPPING_ANGLE ? STEER_LEFT_STOPPING_ANGLE : value);
  } else if (instruction == STEER_RIGHT) {
    // set steer
  #ifdef DEBUG
    Serial.print("Steer = Right:");
    Serial.println(value);
  #endif
    steer_servo_.write(
        value > STEER_RIGHT_STOPPING_ANGLE ? STEER_RIGHT_STOPPING_ANGLE : value);
  }
  delay(50);
  return true;
}

#if 0
uint8_t buf[8];

boolean ProcessBinCommand(YunClient client) {
  int bytes_read = client.read(buf, 8);
  Serial.print("bytes read -- ");
  Serial.print(bytes_read);
  Serial.print("\n");
  if (bytes_read != 8) {
    if (++invalid_command_count_ == 15) {
      return false;
    }
  } else {
    invalid_command_count_ = 0;
  }
  int category = *(int*)(&buf[0]);
  int value = *(int*)(&buf[4]);
  if (category == 1) {
    Serial.println("SPEED -- " + value);
  } else if (category == 2) {
    Serial.println("STEER -- " + value);
  } else if (category == -1) {
    Serial.println("OFF!");
    return false;
  }
  delay(200);
  return true;
}
#endif


void loop() {
  YunClient client = server.accept();
  if (client) {
    digitalWrite(13, HIGH);
    delay(2000);
    if (!InitServo()) {
      steer_servo_.detach();
      client.stop();
      return;
    }
    digitalWrite(13, LOW);
    invalid_command_count_ = 0;
    while (ProcessTextualCommand(client)) {}
    steer_servo_.detach();
    client.stop();
  }
}

#undef DEBUG

