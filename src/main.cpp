#include "mbed.h"
#include "TextLCD.h"
#include "SB1602E.h"
#include "mcp3208.h"
#include "PID.h"
#include <cmath>
#include <algorithm>

/// 手動機・自動機共有部分
Serial pc(USBTX, USBRX);

AnalogIn shield_button(A0);
TextLCD lcd(D8, D9, D4, D5, D6, D7);
DigitalOut backlight(D10);
Ticker shield_input_ticker;
class ShieldInput {
    public:
        static bool Up, Down, Right, Left, Select;
};
bool ShieldInput::Up = false, ShieldInput::Down = false,
  ShieldInput::Right = false, ShieldInput::Left = false,
  ShieldInput::Select = false;
void get_shield_input(void) {
  int value = shield_button.read() * 1000;
  ShieldInput::Right = value < 50;
  ShieldInput::Up = 50 <= value && value < 350;
  ShieldInput::Down = 350 <= value && value < 700;
  ShieldInput::Left = 900 <= value && value < 950;
  ShieldInput::Select = 950 <= value;
}
void initialize_shield(void) {
  shield_input_ticker.attach(&get_shield_input, 0.1f);
  backlight.write(1);
  lcd.cls();
}
Ticker backlight_toggler;
void backlight_toggle(void) {
  backlight.write(!backlight.read());
}

DigitalOut buzzer(PD_2);
DigitalIn button(USER_BUTTON);
void initialize_buzzer(void) {
  buzzer.write(0);
}
void buzz(float time) {
  buzzer.write(1);
  wait(time);
  buzzer.write(0);
}
void repeat_buzz(float time, int n) {
  for (int i = 0; i < n; i++) {
    wait(time);
    buzz(time);
  }
}

I2C i2c(D14, D15); //SDA, SCL

const uint8_t compass_addr = 0x1E << 1;
void compass_write(uint8_t reg, uint8_t data) {
  char command[2] = { reg, data };
  i2c.write(compass_addr, command, 2);
}
uint16_t x, z, y;
void parse_compass_data(void) {
  char data[6], command[2] = { 0x03 };
  i2c.write(compass_addr, command, 1, true); //point to first data register 03
  i2c.read(compass_addr, data, 6);
  x = (data[0] << 8) | data[1];
  z = (data[2] << 8) | data[3];
  y = (data[4] << 8) | data[5];
}
void initialize_compass(void) {
  //                    +-------- 0   Reserved
  //                    |++------ 00  Samples averaged -> 0
  //                    |||+++--- 110 Typical Data Output Rate -> 75Hz
  //                    ||||||++- 00  Measurement Mode -> Default
  compass_write(0x00, 0b00011000);
  compass_write(0x01, 0xa0);
  //                    +-------- 0  High Speed I2C -> Disabled
  //                    |     ++- 00 Operating Mode -> Continuous-Measurement
  compass_write(0x02, 0b00000000);
  //75Hz -> 13.3ms 15ms秒毎にサンプルするのが妥当?
}

/// 手動機・自動機共有部分

// enum State {INITIALIZE, SELECT_MODE, RUN};
// int state = INITIALIZE;


// InterruptIn button(USER_BUTTON);
// void onButtonClick(void) {
//   switch (state) {
//     case SELECT_MODE:
//       mode = (mode + 1) % 3;
//       break;
//   }
// }
// void initButton() {
//   button.rise(&onButtonClick);
// }


SB1602E lcd2(i2c);

PwmOut motor[4][2] = {
  {PwmOut(PA_10), PwmOut(PB_3)}, //右前
  {PwmOut(PA_11), PwmOut(PB_13)}, //右後
  {PwmOut(PC_6), PwmOut(PC_8)}, //左前
  {PwmOut(PB_14), PwmOut(PC_9)} //左後
};
void initialize_motor(void) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; j++) {
      motor[i][j].period_ms(20);
      //adjust and test around
      //40us ~ 2ms
      motor[i][j].write(0.0);
    }
  }
}

const float pid_interval = 0.05;
PID controller(0.2, 0.1, 0.1, pid_interval);
void initialize_pid(void) {
  controller.setInputLimits(-1.0, 1.0);
  controller.setOutputLimits(0.0, 1.0);
  controller.setBias(0.5);
  controller.setMode(AUTO_MODE);
  controller.setSetPoint(0.0);
}

SPI spi(PA_7, PA_6, PA_5); //mosi, miso, sclk
MCP3208 sensor[4] = {
  MCP3208(spi, PA_4),
  MCP3208(spi, PB_0),
  MCP3208(spi, PC_1),
  MCP3208(spi, PC_0)
};
float sensor_value[4][8];
void get_sensor(void) {
   for (int i = 0; i < 4; i++) {
     for (int j = 0; j < 8; j++) {
       sensor_value[i][j] =  std::max(sensor[i].read_input(j) - 0.05, 0.0);
     }
   }
}

volatile bool kill_flag = false;
void check_kill_switch(void) {
  if (false && !kill_flag) { //<- ここに緊急停止条件
    kill_flag = true;
    backlight_toggler.attach(&backlight_toggle, 1.0f);
  }
}
Ticker kill_switch_watcher;
void initialize_kill_switch(void) {
  kill_switch_watcher.attach(&check_kill_switch, 0.1f);
}
void kill(void) {
  lcd.cls();
  lcd.locate(0, 0);
  lcd.printf("killed.");
}

void test_move(void) {
  while (true) {
    motor[0][0].write(1.0);
    motor[1][1].write(1.0);
    motor[2][1].write(1.0);
    motor[3][0].write(1.0);
    wait(1);
    repeat_buzz(0.05, 1);
    motor[0][0].write(0.0);
    motor[1][1].write(0.0);
    motor[2][1].write(0.0);
    motor[3][0].write(0.0);
    wait(3);
    motor[0][1].write(1.0);
    motor[1][0].write(1.0);
    motor[2][0].write(1.0);
    motor[3][1].write(1.0);
    wait(1);
    motor[0][1].write(0.0);
    motor[1][0].write(0.0);
    motor[2][0].write(0.0);
    motor[3][1].write(0.0);
    wait(3);
  }
}
void test_sensor(void) {
  while (true) {
    lcd2.printf(0, "Ch0");    // line# (0 or 1), string
    lcd2.printf(1, "%f", sensor[0].read_input(0));
    wait(1);
    lcd2.clear();
  }
}
void test_pid(void) {
  while (true) {
    get_sensor();
    // float pv = (1.0 - (sensor_value[0][0] * 0.35 + sensor_value[0][1] * 0.25 + sensor_value[0][2] * 0.15 + sensor_value[0][3] * 0.05)) * -1 + (1.0 - (sensor_value[0][7] * 0.35 + sensor_value[0][6] * 0.25 + sensor_value[0][5] * 0.15 + sensor_value[0][4] * 0.05));
    float pv = (1.0 - (sensor_value[0][0] * 70 + sensor_value[0][1] * 10 + sensor_value[0][2] * 6 + sensor_value[0][3] * 2)) * -1 + (1.0 - (sensor_value[0][7] * 70 + sensor_value[0][6] * 10 + sensor_value[0][5] * 6 + sensor_value[0][4] * 2));
    //for(int i = 0; i < 4; i++) {
    //  float data = 1.0 - sensor[i].read();
    //  pc.printf("%f ", data);
    //  pv += data * weight[i];
    //}
    controller.setProcessValue(pv);
    float co = controller.compute();
    lcd.locate(0,0);
    pc.printf("pv:  %f, co: %f : %f\n\r", pv, co, 1.0-co);
    lcd.printf("pv: %.3f", pv);
    lcd.locate(0,1);
    lcd.printf("co: %.3f, %.3f", co, 1.0-co);
    lcd2.printf(0, "Ch0\r");    // line# (0 or 1), string
    lcd2.printf(1, "%f", sensor_value[0][0]);
    // motor[1][0].write(co/**0.5*/);
    // motor[0][0].write((1.0-co)/**0.5*/);
    motor[0][0].write(co);
    motor[1][1].write(co);
    motor[2][1].write(1.0 - co);
    motor[3][0].write(1.0 - co);
    wait(pid_interval);
    lcd.cls();
  }
}
void test_compass(void) {
  while (true) {
    parse_compass_data();
    lcd.cls();
    lcd.locate(0, 0);
    lcd.printf("x: %04X y: %04X", x, y);
    lcd.locate(0, 1);
    lcd.printf("z: %04X", z);
    wait(1);
  }
}
void mode_select(void) {
  enum Mode { TEST_MOVE = 0, TEST_SENSOR, TEST_PID, TEST_COMPASS };
  const int messages_size = 4;
  const char *messages[messages_size] = {
    "TEST_MOVE", "TEST_SENSOR", "TEST_PID", "TEST_COMPASS"
  };
  Mode mode = TEST_SENSOR;
  do {
    lcd.cls();
    lcd.locate(0, 0);
    lcd.printf("MODE: ");
    lcd.printf(messages[(int)mode]);
    wait(0.1);
    if (ShieldInput::Up) {
      mode = (Mode) std::min((int) mode + 1, messages_size - 1);
    } else if (ShieldInput::Down) {
      mode = (Mode) std::max((int) mode - 1, 0);
    }
  } while (!ShieldInput::Select);
  repeat_buzz(0.05, (int) mode + 1);
  wait(1.0);
  switch (mode) {
    case TEST_MOVE:
      test_move();
      break;
    case TEST_SENSOR:
      test_sensor();
      break;
    case TEST_PID:
      test_pid();
      break;
    case TEST_COMPASS:
      test_compass();
      break;
    default:
      //This should not happen!
      lcd.cls();
      lcd.locate(0, 0);
      lcd.printf("!!!NO MODE!!!");
      wait(10.0);
      kill();
      return;
  }
}
void initialize_io(void) {
  pc.baud(19200);
  initialize_buzzer();
  initialize_shield();
  initialize_motor();
  initialize_compass();
  initialize_pid();


  lcd.cls();
  wait(0.5);
  lcd.locate(0, 0);
  repeat_buzz(0.05, 2);
  lcd.printf("Initialized.");
  lcd2.printf(0, "Initialized.");
  pc.printf("Initialized.\n\r");
  wait(1.0);
  lcd.cls();
  lcd2.clear();
}
int main() {
  initialize_io();
  mode_select();
  while(true);
}
