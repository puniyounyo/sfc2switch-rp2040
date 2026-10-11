#include <Adafruit_TinyUSB.h>

#define SWITCH_VID  0x0F0D
#define SWITCH_PID  0x0092

// ピン定義 (GP0: Latch, GP1: Clock, GP2: Data)
const int PIN_LATCH = 0;
const int PIN_CLOCK = 1;
const int PIN_DATA  = 2;

// SNES生データのビット位置定義
#define SNES_B      0
#define SNES_Y      1
#define SNES_SELECT 2
#define SNES_START  3
#define SNES_UP     4
#define SNES_DOWN   5
#define SNES_LEFT   6
#define SNES_RIGHT  7
#define SNES_A      8
#define SNES_X      9
#define SNES_L      10
#define SNES_R      11

// ★本物のHORIポッ拳コントローラーのHIDレポート記述子（完全版）
uint8_t const desc_hid_report[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x05,        // Usage (Game Pad)
  0xA1, 0x01,        // Collection (Application)
    0x15, 0x00,      //   Logical Minimum (0)
    0x25, 0x01,      //   Logical Maximum (1)
    0x35, 0x00,      //   Physical Minimum (0)
    0x45, 0x01,      //   Physical Maximum (1)
    0x75, 0x01,      //   Report Size (1)
    0x95, 0x0D,      //   Report Count (13) - ボタン13個
    0x05, 0x09,      //   Usage Page (Button)
    0x19, 0x01,      //   Usage Minimum (Button 1)
    0x29, 0x0D,      //   Usage Maximum (Button 13)
    0x81, 0x02,      //   Input (Data,Var,Abs)
    
    // パディング（3ビットで合計16ビット＝2バイトに調整）
    0x75, 0x03,      //   Report Size (3)
    0x95, 0x01,      //   Report Count (1)
    0x81, 0x03,      //   Input (Const,Var,Abs)

    // ハットスイッチ (方向キー)
    0x05, 0x01,      //   Usage Page (Generic Desktop Ctrls)
    0x25, 0x07,      //   Logical Maximum (7)
    0x46, 0x3B, 0x01,//   Physical Maximum (315)
    0x75, 0x04,      //   Report Size (4)
    0x95, 0x01,      //   Report Count (1)
    0x65, 0x14,      //   Unit (System: English Rotation, Length: Centimeter)
    0x09, 0x39,      //   Usage (Hat switch)
    0x81, 0x42,      //   Input (Data,Var,Abs,Null)

    // パディング（4ビット）
    0x75, 0x04,      //   Report Size (4)
    0x95, 0x01,      //   Report Count (1)
    0x81, 0x03,      //   Input (Const,Var,Abs)

    // 軸 (X, Y, Z, Rz : 16ビット×4)
    0x15, 0x00,      //   Logical Minimum (0)
    0x26, 0xFF, 0x0F,//   Logical Maximum (4095)
    0x75, 0x10,      //   Report Size (16)
    0x95, 0x04,      //   Report Count (4)
    0x09, 0x30,      //   Usage (X)
    0x09, 0x31,      //   Usage (Y)
    0x09, 0x32,      //   Usage (Z)
    0x09, 0x35,      //   Usage (Rz)
    0x81, 0x02,      //   Input (Data,Var,Abs)
  0xC0               // End Collection
};

Adafruit_USBD_HID usb_hid;

struct __attribute((packed)) switch_report_t {
  uint16_t buttons; // ボタン(13bit) + パディング(3bit)
  uint8_t hat;      // ハット(4bit) + パディング(4bit)
  uint16_t x;
  uint16_t y;
  uint16_t z;
  uint16_t rz;
};

void setup() {
  Serial.end();

  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_DATA, INPUT_PULLUP);
  
  digitalWrite(PIN_LATCH, LOW);
  digitalWrite(PIN_CLOCK, HIGH);

  USBDevice.setID(SWITCH_VID, SWITCH_PID);
  USBDevice.setManufacturerDescriptor("HORI CO., LTD.");
  USBDevice.setProductDescriptor("POKKEN CONTROLLER");

  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.begin();

  while (!USBDevice.mounted()) {
    delay(1);
  }
  delay(500);
}

uint16_t read_snes() {
  uint16_t state = 0;

  digitalWrite(PIN_LATCH, HIGH);
  delayMicroseconds(12);
  digitalWrite(PIN_LATCH, LOW);
  delayMicroseconds(6);

  for (int i = 0; i < 16; i++) {
    if (digitalRead(PIN_DATA) == LOW) {
      state |= (1 << i);
    }
    digitalWrite(PIN_CLOCK, LOW);
    delayMicroseconds(6);
    digitalWrite(PIN_CLOCK, HIGH);
    delayMicroseconds(6);
  }
  return state;
}

void loop() {
  if (usb_hid.ready()) {
    uint16_t s = read_snes();
    
    uint16_t switch_buttons = 0;
    uint8_t hat = 8; // 8 = ニュートラル

    // ポッ拳コントローラーの公式ボタン割当（1-index）に合わせたマッピング
    // Button 1: Y, Button 2: B, Button 3: A, Button 4: X
    // Button 5: L, Button 6: R, Button 7: ZL, Button 8: ZR
    // Button 9: Select(-), Button 10: Start(+), Button 11: L3, Button 12: R3, Button 13: HOME
    if (s & (1 << SNES_Y))      switch_buttons |= (1 << 0); // Btn 1 (Y)
    if (s & (1 << SNES_B))      switch_buttons |= (1 << 1); // Btn 2 (B)
    if (s & (1 << SNES_A))      switch_buttons |= (1 << 2); // Btn 3 (A)
    if (s & (1 << SNES_X))      switch_buttons |= (1 << 3); // Btn 4 (X)
    if (s & (1 << SNES_L))      switch_buttons |= (1 << 4); // Btn 5 (L)
    if (s & (1 << SNES_R))      switch_buttons |= (1 << 5); // Btn 6 (R)

    bool select_pressed = (s & (1 << SNES_SELECT));
    bool start_pressed  = (s & (1 << SNES_START));

    if (select_pressed && start_pressed) {
      switch_buttons |= (1 << 12); // Btn 13 (HOME)
    } else {
      if (select_pressed) switch_buttons |= (1 << 8); // Btn 9 (Select / -)
      if (start_pressed)  switch_buttons |= (1 << 9); // Btn 10 (Start / +)
    }

    // 十字キー (Hat switch)
    bool up    = (s & (1 << SNES_UP));
    bool down  = (s & (1 << SNES_DOWN));
    bool left  = (s & (1 << SNES_LEFT));
    bool right = (s & (1 << SNES_RIGHT));

    if (up && right)       hat = 1;
    else if (right && down) hat = 3;
    else if (down && left)  hat = 5;
    else if (left && up)    hat = 7;
    else if (up)            hat = 0;
    else if (right)         hat = 2;
    else if (down)          hat = 4;
    else if (left)          hat = 6;
    else                    hat = 8;

    switch_report_t report;
    memset(&report, 0, sizeof(report));
    
    report.buttons = switch_buttons;
    report.hat = hat;
    report.x = 2048;
    report.y = 2048;
    report.z = 2048;
    report.rz = 2048;

    // Report IDなし（ポッ拳の仕様に直結）
    usb_hid.sendReport(0, &report, sizeof(report));
  }
  delay(8);
}
