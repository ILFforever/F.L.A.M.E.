#include <Arduino.h>
#include <EEPROM.h>
#include "Wire.h"
#include "DHTesp.h"
//สำหรับInject ค่าRo1,Ro2,Ro3 เข้าEEPROM
#define MQ_PIN (0)
#define RL_VALUE (5)
#define RO_CLEAN_AIR_FACTOR (9.83)
#define CALIBARAION_SAMPLE_TIMES (20)
#define CALIBRATION_SAMPLE_INTERVAL (50)
#define READ_SAMPLE_INTERVAL (50)
#define READ_SAMPLE_TIMES (5)
#define GAS_LPG (0)
#define GAS_CO (1)
#define GAS_SMOKE (2)
float LPGCurve[3] = {2.3, 0.21, -0.47};
float COCurve[3] = {2.3, 0.72, -0.34};
float SmokeCurve[3] = {2.3, 0.53, -0.44};
int Ro1 = 10;
int Ro2 = 10;
int Ro3 = 10;
float lpg, co, smoke;
bool Read1 = false;
bool Read2 = false;
bool Read3 = false;
int addr = 0;
#define Sen1Pin 32
#define Sen2Pin 35
#define Sen3Pin 34
#define TempPin 14
DHTesp dht;
void Inject();
float MQResistanceCalculation(int raw_adc)
{
  return (((float)RL_VALUE * (1023 - raw_adc) / raw_adc));
}

float MQCalibration(int mq_pin)
{
  int i;
  float val = 0;

  for (i = 0; i < CALIBARAION_SAMPLE_TIMES; i++)
  { // take multiple samples
    val += MQResistanceCalculation(analogRead(mq_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val / CALIBARAION_SAMPLE_TIMES; // calculate the average value

  val = val / RO_CLEAN_AIR_FACTOR; // divided by RO_CLEAN_AIR_FACTOR yields the Ro
  // according to the chart in the datasheet

  return val;
}

int MQGetPercentage(float rs_ro_ratio, float *pcurve)
{
  return (pow(10, (((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0])));
}

int MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
  if (gas_id == GAS_LPG)
  {
    return MQGetPercentage(rs_ro_ratio, LPGCurve);
  }
  else if (gas_id == GAS_CO)
  {
    return MQGetPercentage(rs_ro_ratio, COCurve);
  }
  else if (gas_id == GAS_SMOKE)
  {
    return MQGetPercentage(rs_ro_ratio, SmokeCurve);
  }

  return 0;
}

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(512);
  pinMode(TempPin, INPUT);
  dht.setup(TempPin, DHTesp::DHT22);
  analogReadResolution(10);
  Serial.print("Calibrating...\n");
  pinMode(Sen1Pin, INPUT);
  pinMode(Sen2Pin, INPUT);
  pinMode(Sen3Pin, INPUT);
  pinMode(15, INPUT);
  pinMode(2, OUTPUT);
  float temperature = dht.getTemperature();
  float Humid = dht.getHumidity(); // just leave it here.
  Serial.println("Temp = " + String(temperature));
  // FirstSensor
  Ro1 = MQCalibration(Sen1Pin);
  Ro2 = MQCalibration(Sen2Pin);
  Ro3 = MQCalibration(Sen3Pin);

  //  Calibrating the sensor. Please make sure the sensor is in clean air
  //  when you perform the calibration
  if (digitalRead(15) == LOW)
  {
    Serial.print("Calibration is done...\n");
    Serial.print("Ro1=");
    Serial.print(Ro1);
    Serial.print("kohm");
    Serial.print("\n");
    Serial.print("Ro2=");
    Serial.print(Ro2);
    Serial.print("kohm");
    Serial.print("\n");
    Serial.print("Ro3=");
    Serial.print(Ro3);
    Serial.print("kohm");
    Serial.print("\n");
    digitalWrite(2, HIGH);
    Inject();
  }
  else
  {
    digitalWrite(2, LOW);
  }
}

void loop()
{
  float temperature = dht.getTemperature();
  float Humid = dht.getHumidity(); // just leave it here.
  Serial.println("Temp = " + String(temperature));
  int address = 0;
  float SavedRo1 = (EEPROM.read(1));
  float SavedRo2 = (EEPROM.read(5));
  float SavedRo3 = (EEPROM.read(10));
  Serial.print("Ro1 = ");
  Serial.println(SavedRo1);
  Serial.print("Ro2 = ");
  Serial.println(SavedRo2);
  Serial.print("Ro3 = ");
  Serial.println(SavedRo3);
  Serial.println("SavedSS1 = " + String(analogRead(Sen1Pin)));
  Serial.println("SavedSS2 = " + String(analogRead(Sen2Pin)));
  Serial.println("SavedSS3 = " + String(analogRead(Sen3Pin)));
  delay(2500);
}

void Inject()
{
  Serial.println("Confirm Write");
  EEPROM.put(1, Ro1);
  EEPROM.put(5, Ro2);
  EEPROM.put(10, Ro3);
  if (EEPROM.commit())
  {
    Serial.println("EEPROM successfully committed");
  }
  else
  {
    Serial.println("ERROR! EEPROM commit failed");
  }
}