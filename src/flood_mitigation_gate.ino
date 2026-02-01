/*
 * IoT Flood Mitigation Gate System
 * Major Project - PDEU ICT (10/10 SGPA)
 * Safety-Critical embedded system for automated flood response
 * 
 * Hardware: Arduino Uno/Nano, Dual HC-SR04, Dual IR sensors, MPU6050 Gyroscope, Dual Servos
 * Safety Feature: Bridge stays OPEN if humans detected during flood (life-safety priority)
 */

#include <Servo.h>
#include <Wire.h>
#include <MPU6050.h>

// ===== PIN CONFIGURATION =====
// Entry/Exit IR Sensors (Active LOW)
#define IR_SENSOR_ENTRY 6      // Entry side IR
#define IR_SENSOR_EXIT 7       // Exit side IR

// Ultrasonic Sensors
#define TRIG_BOTTOM 3          // Bottom sensor (flood detection)
#define ECHO_BOTTOM 5          // Under bridge
#define TRIG_TOP 4             // Top sensor (human presence on bridge)
#define ECHO_TOP 2             // Above bridge

// Servo Motors (Dual mechanism for redundancy)
#define SERVO_LEFT_PIN 9
#define SERVO_RIGHT_PIN 10

// ===== SAFETY PARAMETERS =====
#define FLOOD_THRESHOLD_CLOSE 10    // cm - Close bridge if water < 10cm from sensor
#define FLOOD_THRESHOLD_OPEN 13     // cm - Open bridge if water recedes > 13cm
#define HYSTERESIS_BUFFER 3         // cm - Prevent oscillation
#define FLOW_THRESHOLD 15           // degrees/s - Gyroscope turbulence detection

#define POSITION_BRIDGE_OPEN_LEFT 0
#define POSITION_BRIDGE_OPEN_RIGHT 180
#define POSITION_BRIDGE_CLOSED 90

// ===== SYSTEM STATE =====
typedef enum {
  STATE_NORMAL,           // No flood, bridge open
  STATE_FLOOD_ALERT,      // Flood detected, monitoring
  STATE_FLOOD_ACTIVE,     // Flood confirmed, bridge closing
  STATE_EMERGENCY_OPEN    // Flood + Human present = Safety lock
} SystemState;

SystemState currentState = STATE_NORMAL;
bool humanOnBridge = false;
bool bridgeIsOpen = true;
unsigned long stateEntryTime = 0;

MPU6050 mpu;
Servo servoLeft;
Servo servoRight;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // Sensor initialization
  pinMode(IR_SENSOR_ENTRY, INPUT_PULLUP);
  pinMode(IR_SENSOR_EXIT, INPUT_PULLUP);
  pinMode(TRIG_BOTTOM, OUTPUT);
  pinMode(ECHO_BOTTOM, INPUT);
  pinMode(TRIG_TOP, OUTPUT);
  pinMode(ECHO_TOP, INPUT);
  
  // Servo initialization
  servoLeft.attach(SERVO_LEFT_PIN);
  servoRight.attach(SERVO_RIGHT_PIN);
  
  // MPU6050 initialization
  while(!mpu.begin(MPU6050_SCALE_2000DPS, MPU6050_RANGE_2G)) {
    Serial.println("MPU6050 initialization failed! Retrying...");
    delay(500);
  }
  
  Serial.println("Flood Mitigation Gate Initialized");
  Serial.println("Safety Protocol: Human presence = Bridge stays OPEN");
  
  openBridge(); // Start in safe position
  delay(1000);
}

void loop() {
  // Sensor data acquisition
  float waterLevel = getDistance(TRIG_BOTTOM, ECHO_BOTTOM);
  float heightAboveBridge = getDistance(TRIG_TOP, ECHO_TOP);
  Vector normGyro = mpu.readNormalizeGyro();
  float flowRate = abs(normGyro.XAxis); // Turbulence indicator
  
  bool entryDetected = !digitalRead(IR_SENSOR_ENTRY);  // Active LOW
  bool exitDetected = !digitalRead(IR_SENSOR_EXIT);    // Active LOW
  
  // Human presence tracking (directional logic)
  updateHumanPresence(entryDetected, exitDetected);
  
  // Safety-critical decision logic
  processSafetyLogic(waterLevel, heightAboveBridge, flowRate);
  
  // Serial telemetry for monitoring
  logTelemetry(waterLevel, heightAboveBridge, flowRate, entryDetected, exitDetected);
  
  delay(200); // 5Hz update rate (safety-critical systems need responsiveness)
}

// ===== SAFETY ENGINEERING FUNCTIONS =====

void processSafetyLogic(float waterLevel, float heightAboveBridge, float flowTurbulence) {
  // Hysteresis implementation to prevent rapid switching
  bool floodCondition = (waterLevel < FLOOD_THRESHOLD_CLOSE) || (flowTurbulence > FLOW_THRESHOLD);
  bool clearCondition = (waterLevel > FLOOD_THRESHOLD_OPEN) && (flowTurbulence < FLOW_THRESHOLD * 0.5);
  
  switch(currentState) {
    case STATE_NORMAL:
      if(floodCondition) {
        Serial.println("ALERT: Flood condition detected");
        currentState = STATE_FLOOD_ALERT;
        stateEntryTime = millis();
      }
      break;
      
    case STATE_FLOOD_ALERT:
      // Debounce period: Confirm flood for 3 seconds before acting
      if(millis() - stateEntryTime > 3000) {
        if(humanOnBridge) {
          Serial.println("EMERGENCY: Flood detected + Human on bridge!");
          Serial.println("SAFETY LOCK: Bridge remains OPEN");
          currentState = STATE_EMERGENCY_OPEN;
          if(!bridgeIsOpen) openBridge();
        } else {
          Serial.println("FLOOD CONFIRMED: Closing bridge");
          currentState = STATE_FLOOD_ACTIVE;
          closeBridge();
        }
      }
      // False alarm check
      if(clearCondition) {
        currentState = STATE_NORMAL;
        Serial.println("False alarm - Return to normal");
      }
      break;
      
    case STATE_FLOOD_ACTIVE:
      if(humanOnBridge) {
        Serial.println("Human detected during flood - EMERGENCY OPEN!");
        openBridge();
        currentState = STATE_EMERGENCY_OPEN;
      } else if(clearCondition) {
        Serial.println("Flood receded - Opening bridge");
        openBridge();
        currentState = STATE_NORMAL;
      }
      break;
      
    case STATE_EMERGENCY_OPEN:
      // Safety lock: Bridge stays open until flood clears AND human leaves
      if(clearCondition && !humanOnBridge) {
        Serial.println("All clear - Returning to normal operation");
        currentState = STATE_NORMAL;
      }
      break;
  }
  
  bridgeIsOpen = (currentState != STATE_FLOOD_ACTIVE);
}

void updateHumanPresence(bool entry, bool exit) {
  static unsigned long lastEntryTime = 0;
  static unsigned long lastExitTime = 0;
  
  // Debounce and directional logic
  if(entry && (millis() - lastEntryTime > 2000)) {
    humanOnBridge = true;
    lastEntryTime = millis();
    Serial.println("Event: Human entered bridge");
  }
  
  if(exit && (millis() - lastExitTime > 2000)) {
    // Simple logic: if exit detected, assume bridge clearing
    // Real system would use ultrasonic confirmation
    delay(5000); // Wait for person to fully clear
    humanOnBridge = false;
    lastExitTime = millis();
    Serial.println("Event: Human exited bridge");
  }
  
  // Double-check with top ultrasonic (if height < 100cm = person present)
  // This is secondary confirmation
  float topDistance = getDistance(TRIG_TOP, ECHO_TOP);
  if(topDistance < 100) {
    humanOnBridge = true;
  }
}

void openBridge() {
  Serial.println("ACTION: Opening bridge gates");
  servoLeft.write(POSITION_BRIDGE_OPEN_LEFT);
  servoRight.write(POSITION_BRIDGE_OPEN_RIGHT);
  bridgeIsOpen = true;
}

void closeBridge() {
  Serial.println("ACTION: Closing bridge gates");
  servoLeft.write(POSITION_BRIDGE_CLOSED);
  servoRight.write(POSITION_BRIDGE_CLOSED);
  bridgeIsOpen = false;
}

// ===== UTILITY FUNCTIONS =====

float getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout
  
  if(duration == 0) {
    Serial.println("Warning: Sensor timeout");
    return 999.0; // Return max distance on failure
  }
  
  return (duration * 0.034) / 2; // Convert to cm
}

void logTelemetry(float waterLevel, float heightAbove, float flowRate, bool entry, bool exit) {
  Serial.print("WaterLevel:"); Serial.print(waterLevel);
  Serial.print(",HeightAbove:"); Serial.print(heightAbove);
  Serial.print(",FlowRate:"); Serial.print(flowRate);
  Serial.print(",HumanPresent:"); Serial.print(humanOnBridge ? 1 : 0);
  Serial.print(",State:"); Serial.println(currentState);
}
