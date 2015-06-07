#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>

YunServer server(5678);
int invalid_command_count_(0);

void setup() {
  Serial.begin(9600);
  pinMode(13,OUTPUT);
  digitalWrite(13, HIGH);
  Bridge.begin();
  digitalWrite(13, LOW);
  server.noListenOnLocalhost();
  server.begin();
}

void loop() {
  YunClient client = server.accept();
  if (client) {
    digitalWrite(13, HIGH);
    delay(2000);
    digitalWrite(13, LOW);
    invalid_command_count_ = 0;
    while (ProcessTextualCommand(client)) {}
    //while (ProcessBinCommand(client)) {}
    client.stop();
  }
}

boolean ProcessTextualCommand(YunClient client) {
  String command = client.readStringUntil('#');
  if (command == "") {
    if (++invalid_command_count_ == 15) {
      return false;
    }
  } else {
    invalid_command_count_ = 0;
  }
  //Serial.println(command);
  char instruction = command[0];
  if (instruction == '0') {
    // turn off
    return false;
  }
  int value = command.substring(1).toInt();
  if (instruction == 'F') {
    // set speed
    Serial.print("Speed = ");
    Serial.println(value);
  } else if (instruction == 'B') {
    // set speed
    Serial.print("Speed = ");
    Serial.println(-value);
  } else if (instruction == 'L') {
    // set steer
    Serial.print("Steer = Left:");
    Serial.println(value);
  } else if (instruction == 'R') {
    // set steer
    Serial.print("Steer = Right:");
    Serial.println(value);
  }
  delay(50);
  return true;
}

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
