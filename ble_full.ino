/*
 * ------------------------------------------------------------------
 * NICLA SENSE ME - MONITOR V9.4
 * ------------------------------------------------------------------
 */

#include "Nicla_System.h"
#include "Arduino_BHY2.h"
#include <ArduinoBLE.h>
#include <math.h>
#include <Wire.h> 

// ------------------------------------------------------------------
// UUID DEFINITIONS
// ------------------------------------------------------------------
// Base UUID for the Nicla Service.
// Note: 128-bit UUIDs are used to avoid collision with standard services.
#define BLE_SERVICE_UUID     "19B10000-0000-0000-0000-000000000000"

// Custom Characteristics UUIDs
// These are sequential from the base UUID for organization.
#define UUID_HEADING         "19B10001-0000-0000-0000-000000000000"
#define UUID_PITCH           "19B10002-0000-0000-0000-000000000000"
#define UUID_ROLL            "19B10003-0000-0000-0000-000000000000"
#define UUID_VEC_GRAVITY     "19B10004-0000-0000-0000-000000000000" 
#define UUID_VEC_ACCEL       "19B10013-0000-0000-0000-000000000000" 
#define UUID_TEMP            "19B10007-0000-0000-0000-000000000000"
#define UUID_HUM             "19B10008-0000-0000-0000-000000000000"
#define UUID_PRESS           "19B10009-0000-0000-0000-000000000000"
#define UUID_GAS             "19B10010-0000-0000-0000-000000000000"
#define UUID_BAT_STATE       "19B10011-0000-0000-0000-000000000000"
#define UUID_ALT             "19B10012-0000-0000-0000-000000000000"
#define UUID_RESET           "19B10014-0000-0000-0000-000000000000"

BLEService niclaService(BLE_SERVICE_UUID);

BLECharacteristic      chOrientation(UUID_HEADING, BLERead | BLENotify, 12); // Combined H, P, R
BLEFloatCharacteristic chTemp(UUID_TEMP, BLERead | BLENotify);
BLEFloatCharacteristic chHum(UUID_HUM, BLERead | BLENotify);
BLEFloatCharacteristic chPress(UUID_PRESS, BLERead | BLENotify);
BLEFloatCharacteristic chAlt(UUID_ALT, BLERead | BLENotify);
BLEIntCharacteristic   chGas(UUID_GAS, BLERead | BLENotify);
BLEByteCharacteristic  chBatState(UUID_BAT_STATE, BLERead | BLENotify);
BLEByteCharacteristic  chReset(UUID_RESET, BLEWrite | BLEWriteWithoutResponse);

BLECharacteristic chGravVector(UUID_VEC_GRAVITY, BLERead | BLENotify, 12);
BLECharacteristic chAccVector(UUID_VEC_ACCEL, BLERead | BLENotify, 12);

// ------------------------------------------------------------------
// SENSOR SETUP
// ------------------------------------------------------------------
// Defining the BHY2 sensors (Nicla Sense ME uses Bosch BHY2 hub).
// PMIC Address for accessing battery registers directly via I2C.
// ------------------------------------------------------------------
#define PMIC_ADDR       0x6A 

SensorOrientation ori(SENSOR_ID_ORI); 
SensorXYZ accel(SENSOR_ID_ACC);      
// CHANGE: Using SENSOR_ID_GRA instead of 11 for compatibility
SensorXYZ gravity(SENSOR_ID_GRA);       

Sensor temp(SENSOR_ID_TEMP);
Sensor hum(SENSOR_ID_HUM);
Sensor pressure(SENSOR_ID_BARO);
Sensor gas(SENSOR_ID_GAS);

unsigned long previousMillisPrint = 0;  
const long intervalPrint = 50;  // 50ms = 20 updates/sec for smoother gameplay
unsigned long ledTimer = 0;             
byte batteryCode = 0; 

/*
 * Function: actualizarEstadoBateria
 * ------------------------------------------------------------------
 * Reads the PMIC (Power Management IC) registers via I2C to determine:
 * 1. Charging Status (Charging, Full, or discharging).
 * 2. Battery Health/Level (Critical or Normal).
 * 
 * Logic:
 * - Writes to register 0x00 and reads status.
 * - Writes to register 0x09 to check for faults/low bat.
 * - Updates global variable `batteryCode` used by LED logic.
 */
void updateBatteryStatus() {
  Wire1.beginTransmission(PMIC_ADDR); Wire1.write(0x00); Wire1.endTransmission();
  Wire1.requestFrom(PMIC_ADDR, 1);
  byte statusReg = (Wire1.available()) ? Wire1.read() : 0xFF;
  Wire1.beginTransmission(PMIC_ADDR); Wire1.write(0x09); Wire1.endTransmission();
  Wire1.requestFrom(PMIC_ADDR, 1);
  byte faultReg = (Wire1.available()) ? Wire1.read() : 0x00;
  
  byte stat = (statusReg >> 6) & 0x03;   
  bool bat_low = (faultReg >> 5) & 0x01; 

  // Priority: 1=Charging, 2=Full, 3=Low, 0=Normal
  if (stat == 0b01) batteryCode = 1;      // Charging
  else if (stat == 0b10) batteryCode = 2; // Full
  else if (bat_low) batteryCode = 3;      // Critical
  else batteryCode = 0;                   // Normal use
}

/*
 * Function: manageLEDs
 * ------------------------------------------------------------------
 * Controls the RGB LED based on system state priority:
 * 1. Fully Charged (Solid Green)   -> Highest Priority
 * 2. Charging (Blinking Green)     -> High Priority
 * 3. Low Battery (Rapid Red Blink) -> Medium Priority
 * 4. BLE Connection (Blue logic)   -> Normal Priority
 * 
 * BLE Logic:
 * - Connected: Blue Double Blink (Flash-Flash-Pause)
 * - Waiting: Blue Single Blink (Flash-Pause)
 */
void manageLEDs() {
  unsigned long currentMillis = millis();
  int r = 0, g = 0, b = 0;
  long totalPeriod = 1000;
  long onTime = 0; 
  bool doubleBlink = false;

  // 1. ABSOLUTE Priority: Fully Charged (Solid Green)
  if (batteryCode == 2) { 
    nicla::leds.setColor(0, 50, 0); 
    return; 
  } 

  // 2. HIGH Priority: Charging (Slow Green Blink)
  if (batteryCode == 1) { 
    r = 0; g = 50; b = 0; 
    totalPeriod = 2000; onTime = 1000; 
  } 
  // 3. MEDIUM Priority: Battery Critical (Fast Red Blink)
  else if (batteryCode == 3) { 
    r = 255; g = 0; b = 0; 
    totalPeriod = 500; onTime = 250; 
  } 
  // 4. Normal State: Connected or Advertising
  else { 
    BLEDevice central = BLE.central();
    if (central && central.connected()) { 
      // CONNECTED: Blue (R=0, G=0, B=255) with Double Blink
      r = 0; g = 0; b = 255; 
      totalPeriod = 2000; // Ciclo de 2 segundos
      doubleBlink = true;
    } 
    else { 
      // ADVERTISING: Single Blue Blink
      r = 0; g = 0; b = 255; 
      totalPeriod = 3000; onTime = 150; 
    }
  }

  // Blink logic
  unsigned long timeInCycle = currentMillis - ledTimer;
  if (timeInCycle > totalPeriod) { ledTimer = currentMillis; timeInCycle = 0; }
  
  if (doubleBlink) {
      // Pattern: ON (100ms) - OFF (100ms) - ON (100ms) - OFF (Rest)
      if (timeInCycle < 100) nicla::leds.setColor(r, g, b);
      else if (timeInCycle < 200) nicla::leds.setColor(0, 0, 0);
      else if (timeInCycle < 300) nicla::leds.setColor(r, g, b);
      else nicla::leds.setColor(0, 0, 0);
  } else {
      // Simple pattern
      if (timeInCycle < onTime) nicla::leds.setColor(r, g, b);
      else nicla::leds.setColor(0, 0, 0);
  }
}

void setup() {
  Serial.begin(115200);
  nicla::begin(); nicla::leds.begin(); BHY2.begin();

  // Startup Flash: Red -> Green -> Blue -> Off
  nicla::leds.setColor(255, 0, 0); delay(200);
  nicla::leds.setColor(0, 255, 0); delay(200);
  nicla::leds.setColor(0, 0, 255); delay(200);
  nicla::leds.setColor(0, 0, 0);

  ori.begin(); accel.begin(); 
  gravity.begin(); // Start sensor with official ID 
  
  temp.begin(); hum.begin(); pressure.begin(); gas.begin();

  if (!BLE.begin()) while (1);

  BLE.setLocalName("Nicla_V9.4"); 
  BLE.setAdvertisedService(niclaService);

  niclaService.addCharacteristic(chOrientation);
  niclaService.addCharacteristic(chGravVector);
  niclaService.addCharacteristic(chAccVector);  
  niclaService.addCharacteristic(chTemp);
  niclaService.addCharacteristic(chHum);
  niclaService.addCharacteristic(chPress);
  niclaService.addCharacteristic(chAlt);
  niclaService.addCharacteristic(chGas);
  niclaService.addCharacteristic(chBatState);
  niclaService.addCharacteristic(chReset);

  BLE.addService(niclaService);
  BLE.advertise();
  Serial.println("--- NICLA LISTA V9.4 ---");
}

/*
 * Function: loop
 * ------------------------------------------------------------------
 * Main execution loop:
 * 1. Updates battery status and LED indicators (always active for UI feedback).
 * 2. Polls BLE radio for events.
 * 3. If a client is CONNECTED:
 *    - Updates sensor fusion algorithm (BHY2.update).
 *    - Reads sensor values (Heading, Pitch, Roll, Gravity, Accel, Env).
 *    - Performs the GRAVITY FIX: If virtual gravity sensor is asleep (0,0,0),
 *      it forces accelerometer data as gravity vector (valid when static).
 *    - Updates BLE characteristics.
 * 4. If DISCONNECTED:
 *    - Sleeps briefly to save power.
 */
void loop() {
  // Always manage battery and LEDs for immediate visual feedback
  updateBatteryStatus();
  manageLEDs();
  BLE.poll();

  BLEDevice central = BLE.central();
  if (central && central.connected()) {
      // ONLY update sensors if someone is connected
      BHY2.update();

      if (millis() - previousMillisPrint >= intervalPrint) {
        previousMillisPrint = millis(); 

        float h = ori.heading(); float p = ori.pitch(); float r = ori.roll();
        float t = temp.value(); float hm = hum.value(); float pr = pressure.value();
        uint32_t g = gas.value();
        
        // Altitude Calculation
        float alt = (pr > 0) ? 44330.0 * (1.0 - pow(pr / 1013.25, 0.1903)) : 0;

        float gx = gravity.x(); float gy = gravity.y(); float gz = gravity.z();
        
        // Safety fallback: If gravity is 0,0,0 (sensor off), use accel
        if (gx == 0 && gy == 0 && gz == 0) {
           gx = accel.x(); gy = accel.y(); gz = accel.z();
        }

        float gravVector[3] = { gx, gy, gz };
        float accelVector[3]    = { (float)accel.x(), (float)accel.y(), (float)accel.z() };

        float orientationVector[3] = { h, p, r };
        chOrientation.writeValue((byte*)orientationVector, 12);
        
        chGravVector.writeValue((byte*)gravVector, 12);
        chAccVector.writeValue((byte*)accelVector, 12);
        chTemp.writeValue(t);
        chHum.writeValue(hm);
        chPress.writeValue(pr);
        chAlt.writeValue(alt);
        chGas.writeValue(g);
        chBatState.writeValue(batteryCode);
      }

      // Check for Reset Command
      if (chReset.written()) {
        if (chReset.value() == 1) {
           NVIC_SystemReset();
        }
      }
  } else {
      // If no client, wait a bit to save power and not saturate loop
      delay(100);
  }
}