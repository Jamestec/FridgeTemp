#define BATTERY_PIN 35 // This is with a voltage divider

float getBatteryVoltage(int analogVal) {
//  float real = map(analogVal, 0, 4095, 0, 360) * 0.02;
  if (!analogVal) {
    analogVal = analogRead(BATTERY_PIN);
  }
  return analogVal / 568.8;
}
