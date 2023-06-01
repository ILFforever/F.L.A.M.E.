#include <Arduino.h>
#include <ESP8266SAM.h>
#include "AudioOutputI2SNoDAC.h"
#include "painlessMesh.h"
#include <ArduinoJson.h>
#define _DICT_PACK_STRUCTURES
#define MESH_PREFIX "FireAlarm"
#define MESH_PASSWORD "19283746"
#define MESH_PORT 1010
#include <Dictionary.h>
#include <EEPROM.h>
#include "Wire.h"
#include "DHTesp.h"
#define RXD2 16
#define TXD2 17
#define MESH_PREFIX "FireAlarm"
#define MESH_PASSWORD "19283746"
#define MESH_PORT 1010
#define MQ_PIN (0)
#define RL_VALUE (5)
#define RO_CLEAN_AIR_FACTOR (9.83)
#define CALIBARAION_SAMPLE_TIMES (20)
#define CALIBRATION_SAMPLE_INTERVAL (50)
#define GAS_LPG (0)
#define GAS_CO (1)
#define GAS_SMOKE (2)
Dictionary &d = *(new Dictionary());  // fire dict
Dictionary &s = *(new Dictionary());  // relay dict
Dictionary &p = *(new Dictionary());  // another on fire dict for samsay to cycle through
Dictionary &pa = *(new Dictionary()); // Annoucements
Dictionary &SamDict = *(new Dictionary());
Scheduler userScheduler; // to control your personal task
painlessMesh mesh;
//-------------------------MQ2 Varibles-------------------------------
void ReadSensor();
void CalibrateSensor();
unsigned int CO, CO1, CO2, CO3, SM, SumCo, SumSm, Smoke1, Smoke2, Smoke3;
int detectionTimes = 1;
#define COTreshold 367
#define SMTreshold 500
unsigned int FinalCO;
unsigned int FinalSM;
int STR_A1, STR_A2, STR_A3;
bool debug = true;
#define Sen1Pin 32
#define Sen2Pin 35
#define Sen3Pin 34
#define WFCPin 39
char Room[] = "001"; // change room name
bool StartPA;
int BlkOutPin;
//--------------------------------
#define RoTresh 5
#define RelaySW 15
#define buzpin 13 // pin for buzzer pin5
int TempPin = 14;
// #define TempPin 27 // for Unit 4 (Pin14 is broken)
DHTesp dht;
int OldSM, OldCO, CODif12, CODif13, CODif23, SMDif12, SMDif13, SMDif23;
bool Dif12, Dif13, Dif23;
bool SMError, COError;
int Lasttemp; // Last time temp was ran
//--------------------------------------------------------------------
#define Alarmlv 500 // gas ขั้นต่ำที่ทำให้ระบบเตือน
#define DifTresholdCO 5000
#define DifTresholdSM 5000
int Tick = 0;
int OVRTick = 0;
unsigned int LastTick;
#define RoomTresh 10
#define SecondInterval 10 // seconds of interval for samsay "Still on Fire"
#define BlackoutTresh 3600
int ID = 0;
int ResetTimer;
int BlackOutTimer;
int Buzlevel = 0;
bool Sensorcalib;
bool Onfire = false;
bool SentFire = false;
bool BuzzerON = false;
bool ovr_Status = false; // Overide for buzzer
bool BLKOUT = false;
bool OneOTwo, OneOFour, OneOTen;
int TempCheckTimes = 0;
float Temp, SumTemp, FinalTemp;
#define TempTresh 5.0 // treshold for temp check times
bool SysOn = false;
bool FdictCheck = false;
bool FDFirstRun = true;
//---------------------------------------------------------------------
bool SerialDebug = false; // TOGGLE SERIAL DEBUG
//----------------------------------------------------------------------
const char *Recieved_ID;
String MSG_ID;
String RE_ID;
String Recieved_status;
String Relay_Message;
String RoomStr;
String PAparse, PAData, PAInput, PAStatus;
String PAID, OLDPAID;
int Roomint;
int CurrentRoom = 0;
String json1;
SimpleList<uint32_t> nodes;
//----------For Serial---------------
String msg;
String message = "";
bool messageReady = false;
int UARTSW = 1;
int Req_ID;
bool Doc_Created = false;
bool DocRecieve_Created = false;
//-----------------------------------
DynamicJsonDocument doc(1024);
DynamicJsonDocument doc_Recieved(1024);
DynamicJsonDocument Request(1024);
AudioOutputI2SNoDAC *out = NULL;
//------------------Processes--------------------------------------
void sendMessage();
void Firedetect();
void Database();
void BuzzerAlert102();
void BuzzerAlert104();
void BuzzerAlert1010();
void Beep();
void Relay();
void SensorRead();
void UARTSend();
void SayOut();
void COClean();
void SMClean();
void ClearFdict();
void ClearFdictCheck(); // turn on or off ClearFdict Check
void ESPReset();
void Database2();
void BlackOut();
void TempCheckCode();
void SamSayCode(void *pvParameters);
void Loop2Code(void *pvParameters);
//------------------Key generator------------------
int Single = 1;
int Tens = 1;
String KeyGenerator();
//----------------------------------- For MQ2 --------------------------------
float LPGCurve[3] = {2.3, 0.21, -0.47};
float COCurve[3] = {2.3, 0.72, -0.34};
float SmokeCurve[3] = {2.3, 0.53, -0.44};
float Ro1 = 10;
float Ro2 = 10;
float Ro3 = 10;
//------------------------------------------------
TaskHandle_t Samsay;
TaskHandle_t Loop2;
//---------------------------------------------------------------------------
Task taskSendMessage(TASK_SECOND * 2, TASK_FOREVER, &sendMessage);
Task taskFiredetect(TASK_SECOND * 1, TASK_FOREVER, &Firedetect);
Task taskBuzzer102(TASK_SECOND * 2, TASK_FOREVER, &BuzzerAlert102);
Task taskBuzzer104(TASK_SECOND * 4, TASK_FOREVER, &BuzzerAlert104);
Task taskBuzzer1010(TASK_SECOND * 10, TASK_FOREVER, &BuzzerAlert1010);
Task taskBeep(TASK_SECOND * 0.5, TASK_FOREVER, &Beep);
Task taskSensorRead(TASK_SECOND * 0.5, TASK_FOREVER, &SensorRead);
Task taskUARTSend(TASK_SECOND * 0.1, TASK_FOREVER, &UARTSend);
Task taskClearFdict(TASK_SECOND * 180, TASK_FOREVER, &ClearFdict);
Task taskESPReset(TASK_SECOND * 1, TASK_FOREVER, &ESPReset);
Task taskBlackOut(TASK_SECOND * 1, TASK_FOREVER, &BlackOut);
Task taskClearFdictCheck(TASK_SECOND * 10, TASK_FOREVER, &ClearFdictCheck);

//------------------------------MQ2 Funtions------------------------------------

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
    // Serial.print("Cal pin read = " + String(analogRead(mq_pin)));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val / CALIBARAION_SAMPLE_TIMES; // calculate the average value
  val = val / RO_CLEAN_AIR_FACTOR;      // divided by RO_CLEAN_AIR_FACTOR yields the Ro
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
//-------------------Mesh commands-------------------------
void newConnectionCallback(uint32_t nodeId)
{
  Serial.printf("-- > startHere: New Connection, nodeId = % u\n", nodeId);
}
void changedConnectionCallback()
{
}
void nodeTimeAdjustedCallback(int32_t offset)
{
}
void receivedCallback(uint32_t from, String &Recievedmsg)
{ // Recieved messages
  Relay_Message = Recievedmsg;
  json1 = Recievedmsg.c_str();
  DeserializationError error = deserializeJson(doc_Recieved, json1); // If error
  if (error)
  {
    // Serial.print("deserializeJson() failed: ");
    // Serial.println(error.c_str());
  }
  DocRecieve_Created = true; // First Doc received created
  Recieved_ID = doc_Recieved["Room"];
  Recieved_status = doc_Recieved["status"].as<String>();
  RE_ID = doc_Recieved["MSG_ID"].as<String>();
  if (Recieved_status == "3") // If its a PA get the msg
  {
    PAInput = doc_Recieved["PAmsg"].as<String>();
    PAID = RE_ID;
    if (SerialDebug)
      Serial.println("PAinput = " + String(PAInput));
  }
  if (SerialDebug)
  {
    Serial.print("RoomID = " + String(Recieved_ID));
    Serial.print(" Recieved_status = " + String(Recieved_status));
    Serial.println(" RE_ID = " + String(RE_ID));
  }
  Relay();
}
//--------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); // For sending data to another ESP32
  Serial2.setTimeout(100);
  Serial.println("DHT initiated");
  Serial.println("Version 4.0 --- Now with PA capabilities");
  if (strcmp(Room, "001") == 0) // if unit 1 with pin 33 broken
    BlkOutPin = 39;
  else
    BlkOutPin = 33;
  if (strcmp(Room, "004") == 0) // if unit 4 with pin 14 broken
    TempPin = 27;
  else
    TempPin = 14;
  pinMode(Sen1Pin, INPUT);
  pinMode(Sen2Pin, INPUT);
  pinMode(Sen3Pin, INPUT);
  pinMode(buzpin, OUTPUT);
  pinMode(WFCPin, INPUT);
  pinMode(BlkOutPin, INPUT);
  pinMode(RelaySW, OUTPUT);
  pinMode(TempPin, INPUT);
  dht.setup(TempPin, DHTesp::DHT22);
  analogReadResolution(10);
  EEPROM.begin(512);
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  userScheduler.addTask(taskSendMessage);
  userScheduler.addTask(taskFiredetect);
  userScheduler.addTask(taskBuzzer102);
  userScheduler.addTask(taskBuzzer104);
  userScheduler.addTask(taskBuzzer1010);
  userScheduler.addTask(taskBeep);
  userScheduler.addTask(taskSensorRead);
  userScheduler.addTask(taskUARTSend);
  userScheduler.addTask(taskClearFdictCheck);
  userScheduler.addTask(taskClearFdict);
  userScheduler.addTask(taskESPReset);
  userScheduler.addTask(taskBlackOut);
  taskUARTSend.enable();
  taskClearFdict.enable();
  taskBlackOut.enable();
  taskClearFdictCheck.enable();
  // taskSendMessage.enable(); taskFiredetect.enable(); taskSensorRead.enable(); taskESPReset.enable(); //(Turned on by Tempcheck)
  xTaskCreatePinnedToCore(Loop2Code, "Loop2", 10000, NULL, 1, &Loop2, 1);
  xTaskCreatePinnedToCore(SamSayCode, "Samsay", 10000, NULL, 1, &Samsay, 0);

  out = new AudioOutputI2SNoDAC();
  out->begin();
}
void loop()
{
  vTaskDelete(NULL);
}
void Loop2Code(void *pvParameters)
{
  for (;;)
  {
    mesh.update();                                       // update mesh+ run scheduler
    if ((millis() - Lasttemp > 2500) || (Lasttemp == 0)) // run temp chack every 2.5 sec
    {
      TempCheckCode();
      Lasttemp = millis();
    }
  }
}

void OverrideBuzzer()
{
  if ((d.count()) >= RoomTresh)
  { // Rooms on fire >= 10
    ovr_Status = true;
    if (Buzlevel < 3)
    { // if Buzzlevel lower than highest(3)
      taskBuzzer102.enable();
      taskBuzzer104.disable();
      taskBuzzer1010.disable();
      Buzlevel = 3;
    }
  }
  else
  {
    ovr_Status = false;
    taskBuzzer102.disable();
    taskBuzzer104.disable();
    taskBuzzer1010.disable();
  }
}

void Relay()
{ // Filter massages to relay forward
  bool IDMatch;
  IDMatch = s(RE_ID); // chexk in the database for sent ids
  if (IDMatch == true)
  {
    // Do nothing bc dupicate message
    // Serial.println("ID Match Relay off");
  }
  else
  {
    if (s.count() >= 100) // Numbers of ID saved in "s" Dictionary
    {
      s.remove(s(0)); // remove oldest input data to prevent overflow
      s(RE_ID, "0");  // MSGID for reference with empty value(value not used)
    }
    else
    {
      s(RE_ID, "0");
    }
    Database();
  }
}
void UARTSend()
{
  if (analogRead(WFCPin) >= 1000)
  { // if the wifi contact pin is HIGH
    while (Serial2.available())
    {
      message = Serial2.readString();

      Serial.println(message);
      messageReady = true;
      if (SerialDebug)
      {
        Serial.print("Received message : ");
        Serial.println(message);
      }
    }
    if (messageReady == true)
    {
      DeserializationError error = deserializeJson(Request, message);
      if (error)
      {
        messageReady = false;
        return;
      }
      if (Request["type"] == "request")
      {
        if (Request["Req_ID"] != Req_ID)
        { // new ID
          Req_ID = Request["Req_ID"];
          Serial.println("Received New Request");
          if (UARTSW < 4)
          {
            if (DocRecieve_Created == true)
            {                                       // If Doc of recieved signal was created
              serializeJson(doc_Recieved, Serial2); // Sending data to another ESP32
              serializeJson(doc_Recieved, Serial);  // not important?
              messageReady = false;
            }
            UARTSW += 1;
          }
          else
          {
            if (Doc_Created == true)
            {                              // If Doc of own status was created
              serializeJson(doc, Serial2); // send own data to ESP32 via UART

              // Serial.println("Sending Data : ");
              serializeJson(doc, Serial);

              // Serial.println("");
              messageReady = false;
            }
            UARTSW = 1;
          }
        }
      }
    }
  }
}

void Database()
{
  bool DataMatch;
  DataMatch = d(Recieved_ID); // check in the database for rooms on fire
  if (DataMatch == true)
  { // Found match in database
    if (Recieved_status == "1")
    {

      // Serial.println(String(Recieved_ID) + " Status is still on fire");
      d.remove(Recieved_ID);
      d(Recieved_ID, "1");
    }
    else if (Recieved_status == "0")
    {
      // Serial.println("Fire stopped in room " + String(Recieved_ID));
      SamDict(KeyGenerator(), String(Recieved_ID) + "0");
      d.remove(Recieved_ID);
    }
  }
  else
  {
    if (Recieved_status == "1")
    {
      // Serial.println("New fire reported in room " + String(Recieved_ID));
      d(Recieved_ID, "1");
      SamDict(KeyGenerator(), String(Recieved_ID) + "1");
    }
  }
  if (Recieved_status == "3")
  {
    const char *PAIDchar = PAID.c_str();
    const char *OLDPAIDchar = OLDPAID.c_str();
    if (strcmp(PAIDchar, OLDPAIDchar) == 0)
    {
      if (SerialDebug)
        Serial.println("Duplicate PA detected");
    }
    else if (PAInput.length() == 0)
    {
      if (SerialDebug)
        Serial.println("PA empty");
    }
    else
    {
      OLDPAID = PAID;
      int Size = PAInput.length();
      if (SerialDebug)
      {
        Serial.println("SIZE = " + String(Size));
      }
      if (Size >= 255)
      {
        int num = 0;
        bool DPC;
        String DictParse, OldParse;
        while (true)
        {
          unsigned char Pchar = PAInput[num];
          if (isWhitespace(Pchar)) // word ended
          {
            if (DictParse.length() >= 255) // use old string from last check put it in dict
            {
              pa(KeyGenerator(), OldParse); // upload this to dict
              PAInput.remove(0, OldParse.length());
              if (SerialDebug)
              {
                Serial.println("OLDpalse = " + String(OldParse));
              }
              OldParse = "";
              DictParse = "";
              num = 0;
            }
            else
            {
              OldParse = DictParse;
            }
          }
          DictParse += PAInput[num];
          num++;
          if (DictParse.length() >= PAInput.length()) // last batch
          {
            pa(KeyGenerator(), DictParse); // upload this to dict
            if (SerialDebug)
            {
              Serial.println("Last batch Parse = " + String(DictParse));
            }
            PAInput = "";
            break;
          }
        }
      }
      else
      {
        pa(KeyGenerator(), PAInput); // Add to dict
        PAInput = "";                // clear msg
      }
    }
  }
  Database2();
  OverrideBuzzer();
}
void Database2()
{ // for speak code (
  bool DataMatch2;
  DataMatch2 = p(Recieved_ID);
  if (DataMatch2 == true)
  { // Found match in database
    if (Recieved_status == "0")
    {
      p.remove(Recieved_ID);
    }
  }
  else
  {
    if (Recieved_status == "1")
    {
      p(Recieved_ID, "1");
    }
  }
}
void BlackOut()
{
  if (SerialDebug)
    Serial.println("BLACKOUT PIN = " + String(analogRead(BlkOutPin)));
  if (analogRead(BlkOutPin) <= 500)
  {
    if (BlackOutTimer <= BlackoutTresh)
    {
      // Serial.print("BlackOutTimer = " + String(BlackOutTimer));
      BlackOutTimer++;
    }
    else if (BlackOutTimer > BlackoutTresh)
    {
      ESP8266SAM *sam = new ESP8266SAM;
      if (!BLKOUT)
      {
        digitalWrite(RelaySW, HIGH);
        sam->Say(out, "1 hour Black out reached");
        delay(500);
        sam->Say(out, "turning off sensors");
        digitalWrite(RelaySW, LOW);
        delete sam;
      }
      BLKOUT = true;
      msg = ""; // reset msg string
      taskESPReset.disable();
      taskSendMessage.disable();
      taskFiredetect.disable();
      taskSensorRead.disable();
      taskBuzzer102.disable();
      taskBuzzer104.disable();
      taskBuzzer1010.disable();
      doc["ERROR"] = "2";
      RoomStr = String(Room);
      Roomint = RoomStr.toInt();
      if (ID < 100)
      {
        MSG_ID = String((Roomint * 100) + ID);
        ID = ID + 1;
      }
      else
      {
        ID = 0;
        MSG_ID = String((Roomint * 100) + ID);
        ID = ID + 1;
      }
      doc["MSG_ID"] = MSG_ID;
      serializeJson(doc, msg);
      mesh.sendBroadcast(msg);
      if (SerialDebug)
        Serial.println(msg);
    }
  }
  else if (BLKOUT)
  { // If was blackout but regained power
    ESP8266SAM *sam = new ESP8266SAM;
    digitalWrite(RelaySW, HIGH);
    sam->Say(out, "Regained Power!");
    delay(500);
    sam->Say(out, "Resuming Operations.");
    digitalWrite(RelaySW, LOW);
    delete sam;
    taskESPReset.enable();
    taskSendMessage.enable();
    taskFiredetect.enable();
    taskSensorRead.enable();
    taskBuzzer102.enable();
    taskBuzzer104.enable();
    taskBuzzer1010.enable();
    BLKOUT = false;
    BlackOutTimer = 0;
  }
  else
  {
    BlackOutTimer = 0; // reset blackout timer
  }
}
String KeyGenerator()
{
  String Keygen;
  char SDigit = Single + 'A';
  char FDigit = Tens + 'A';
  if (Single < 25)
  {
    Single += 1;
  }
  else
  {
    if (Tens < 25)
    {
      Single = 0;
      Tens += 1;
    }
    else
    {
      Single = 0;
      Tens = 0;
    }
  }
  Keygen = "";
  Keygen += FDigit;
  Keygen += SDigit;
  return Keygen;
}

void ESPReset()
{
  nodes = mesh.getNodeList();
  if (SerialDebug)
    Serial.printf("Num nodes: %d\n ", nodes.size());

  // Serial.println("Reset Timer = " + String(ResetTimer));
  if (nodes.size() <= 0)
  { // no connected devices
    if (ResetTimer >= 60)
    {
      Serial.println("No Connections Restarting");
      ESP.restart();
    }
    else
    {
      ResetTimer++;
    }
  }
  else
  {
    ResetTimer = 0;
  }
}
void sendMessage()
{
  msg = ""; // reset msg string
  doc["Room"] = Room;
  RoomStr = String(Room);
  Roomint = RoomStr.toInt();
  if (ID < 100)
  {
    MSG_ID = String((Roomint * 100) + ID);
    ID = ID + 1;
  }
  else
  {
    ID = 0;
    MSG_ID = String((Roomint * 100) + ID);
    ID = ID + 1;
  }
  doc["MSG_ID"] = MSG_ID;
  if (s.count() >= 99)
  {
    s.remove(s(0)); // remove oldest input data to prevent overflow
    s(MSG_ID, "0"); // MSGID for reference with empty value(value not used)
  }
  else
  {
    s(MSG_ID, "0"); // Save sent msgid to dictionary
  }
  Doc_Created = true; // Own doc created
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);
}

void SamSayCode(void *pvParameters)
{ //---------------------SamSay----------------------------
  for (;;)
  {
    if (pa.count() > 0)
    {
      if (StartPA != true) // hasn't start parsing add to dict
      {
        PAData = pa[0];
        StartPA = true;
        if (SerialDebug)
        {
          Serial.println("PAdata = " + String(PAData));
        }
      }
      else
      {
        int spaceCT = 0;
        int num = 0;
        while (spaceCT < 6)
        {
          if (num + 1 > PAData.length())
          {
            break;
          }
          unsigned char Pchar = PAData[num];
          if (isWhitespace(Pchar))
          {
            spaceCT++;
          }
          PAparse += PAData[num];
          num++;
        }
        PAData.remove(0, PAparse.length());
        if (SerialDebug)
        {
          Serial.println("PAparse = " + String(PAparse));
          Serial.println("PAdata = " + String(PAData));
        }

        digitalWrite(RelaySW, HIGH);
        ESP8266SAM *sam = new ESP8266SAM;
        sam->SetVoice(sam->SAMVoice::VOICE_ELF);
        const char *PAChar = PAparse.c_str();
        sam->Say(out, PAChar);

        delete sam;
        PAparse = ""; // reset PA

        if (PAData.length() == 0)
        { // PA now empty
          digitalWrite(RelaySW, LOW);
          StartPA = false;
          pa.remove(pa(0)); // delete
          if (SerialDebug)
          {
            Serial.println("PA empty delete dict");
          }
        }
      }
    }
    else if (SamDict.count() > 0)
    {
      ESP8266SAM *sam = new ESP8266SAM;
      String SamData = SamDict[0];
      String SamStatus = String(SamData[SamData.length() - 1]);
      unsigned int Delnumber = SamData.length() - 1;
      String SamRoom = SamData;
      SamRoom.remove(Delnumber, 1);
      if (SerialDebug)
      {
        Serial.print("SamData = " + SamData);
        Serial.print(" : SamStatus = " + SamStatus + "\n");
        Serial.print("SamRoom = " + SamRoom);
      }
      digitalWrite(RelaySW, HIGH);
      if (SamStatus == "1")
      { // Room is on fire
        sam->Say(out, "New fire reported in room ");
        const char *VoiceChar = SamRoom.c_str();
        sam->Say(out, VoiceChar);
      }
      else if (SamStatus == "0")
      {
        sam->Say(out, "Fire Stopped in room ");
        const char *Voice2Char = SamRoom.c_str();
        sam->Say(out, Voice2Char);
      }
      digitalWrite(RelaySW, LOW);
      SamDict.remove(SamDict(0));
      delete sam;
    }
    else if (ovr_Status)
    {
      if (OVRTick < 5)
      {
        OVRTick++;
      }
      else
      {
        digitalWrite(RelaySW, HIGH);
        ESP8266SAM *sam = new ESP8266SAM;
        sam->Say(out, "There are currently ");
        String RoomTreshStr = String(d.count());
        const char *TreshChar = RoomTreshStr.c_str();
        sam->Say(out, TreshChar);
        sam->Say(out, "Rooms on fire");
        digitalWrite(RelaySW, LOW);
        delete sam;
        OVRTick = 0;
      }
    }
    else
    {
      if (LastTick == 0)
      {
        LastTick = millis();
      }
      if (millis() - LastTick > SecondInterval * 1000)
      {
        if (SerialDebug)
        {
          Serial.print("Number of rooms on fire =");
          Serial.println(d.count());
        }
        Tick += 1;
        LastTick = millis();
        if (p.count() > 0)
        { // if there is Room on Fire
          digitalWrite(RelaySW, HIGH);
          ESP8266SAM *sam = new ESP8266SAM;
          sam->Say(out, "Room ");
          if (CurrentRoom + 1 <= d.count())
          {
            SayOut();
            CurrentRoom++;
          }
          else
          {
            CurrentRoom = 0; // cycle send
            SayOut();
            CurrentRoom++;
          }
          digitalWrite(RelaySW, HIGH);
          sam->Say(out, " is still on fire!");
          delay(150);
          digitalWrite(RelaySW, LOW);
        }
      }
    }
    delay(100);
  }
}

void SayOut()
{
  digitalWrite(RelaySW, HIGH);
  ESP8266SAM *sam = new ESP8266SAM;
  String RoomCycle = p(CurrentRoom);
  const char *RoomCycleChar = RoomCycle.c_str();
  sam->Say(out, RoomCycleChar);

  // Serial.print("SamSayOut Room" + String(p(CurrentRoom)) + " is still on fire");
  delay(150);
  digitalWrite(RelaySW, LOW);
  delete sam;
}

void Beep()
{
  if (BuzzerON == false)
  {
    digitalWrite(buzpin, HIGH);
    BuzzerON = true;
  }
  else
  {
    digitalWrite(buzpin, LOW);
    BuzzerON = false;
    taskBeep.disable();
  }
}

void BuzzerAlert102()
{
  if (SerialDebug)
  {
    Serial.println("BEEPING");
  }
  taskBeep.enable();
}
void BuzzerAlert104()
{
  taskBeep.enable();
}
void BuzzerAlert1010()
{
  taskBeep.enable();
}

void Firedetect()
{
  ReadSensor();
  CODif12 = CO1 - CO2;
  CODif13 = CO1 - CO3;
  CODif23 = CO2 - CO3;
  SMDif12 = Smoke1 - Smoke2;
  SMDif13 = Smoke1 - Smoke3;
  SMDif23 = Smoke2 - Smoke3;
  Dif12 = true;
  Dif13 = true;
  Dif23 = true;
  COClean(); // clean up data
  Dif12 = true;
  Dif13 = true;
  Dif23 = true;
  SMClean(); // clean up data
  if (SMError || COError)
  { // if any error
    doc["ERROR"] = "1";
  }
  else
  {
    doc["ERROR"] = "0";
  }
  if (detectionTimes <= 5)
  { // เก็บข้อมูล
    SumCo = SumCo + CO;
    SumSm = SumSm + SM;
    detectionTimes++;
  }
  else
  {
    detectionTimes = 1;
    unsigned int FinalCO = SumCo / 5;
    unsigned int FinalSM = SumSm / 5;
    SumCo = 0;
    SumSm = 0;
    if (SerialDebug)
    {
      Serial.print("FinalCO = ");
      Serial.println(FinalCO);
      Serial.print("FinalSM = ");
      Serial.println(FinalSM);
    }
    if ((FinalCO >= COTreshold) && (FinalSM >= SMTreshold))
    { // both more than treshold
      if (ovr_Status == false)
      {
        if (OneOTwo == false)
        { // want this
          taskBuzzer102.enable();
          // Serial.println("taskBuzzer102.enable()");
          OneOTwo = true;
        }
        if (OneOFour == true)
        {
          taskBuzzer104.disable();
          OneOFour = false;
        }
        if (OneOTen == true)
        {
          taskBuzzer1010.disable();
          OneOTen = false;
        }
        Buzlevel = 3;
      }
      if (SerialDebug)
        Serial.println("Both more than treshold");
      Onfire = true;
    }
    else if ((FinalCO >= COTreshold) && (FinalSM >= SMTreshold / 2))
    { // Co more than treshold, smoke more than half
      if (ovr_Status == false)
      {
        if (OneOTwo == false)
        { // want this
          taskBuzzer102.enable();
          // Serial.println("taskBuzzer102.enable()");
          OneOTwo = true;
        }
        if (OneOFour == true)
        {
          taskBuzzer104.disable();
          OneOFour = false;
        }
        if (OneOTen == true)
        {
          taskBuzzer1010.disable();
          OneOTen = false;
        }
        Buzlevel = 3;
      }
      if (SerialDebug)
        Serial.println("Co more than treshold, smoke more than half");
      Onfire = true;
    }
    else if ((FinalSM >= SMTreshold) && (FinalCO >= COTreshold / 2))
    { // Smoke more than treshold, Co more than half
      if (ovr_Status == false)
      {
        if (OneOTwo == false)
        { // want this
          taskBuzzer102.enable();
          // Serial.println("taskBuzzer102.enable()");
          OneOTwo = true;
        }
        if (OneOFour == true)
        {
          taskBuzzer104.disable();
          OneOFour = false;
        }
        if (OneOTen == true)
        {
          taskBuzzer1010.disable();
          OneOTen = false;
        }
        Buzlevel = 3;
      }
      if (SerialDebug)
        Serial.println("Smoke more than treshold, Co more than half");
      Onfire = true;
    }
    else if ((FinalCO >= COTreshold / 2) && (FinalSM >= SMTreshold / 2))
    { // both more than half
      if (ovr_Status == false)
      {
        if (OneOTwo == true)
        {
          taskBuzzer102.disable();
          OneOTwo = false;
        }
        if (OneOFour == false)
        { // want this
          taskBuzzer104.enable();
          // Serial.println("taskBuzzer104.enable()");
          OneOFour = true;
        }
        if (OneOTen == true)
        {
          taskBuzzer1010.disable();
          OneOTen = false;
        }
        Buzlevel = 2;
      }
      if (SerialDebug)
        Serial.println("both more than half");
      Onfire = false;
    }
    else if ((FinalCO >= COTreshold / 2) && (FinalSM < SMTreshold / 2))
    { // CO more than half Sm less than half
      if (ovr_Status == false)
      {
        if (OneOTwo == true)
        {
          taskBuzzer102.disable();
          OneOTwo = false;
        }
        if (OneOFour == true)
        {
          taskBuzzer104.disable();
          OneOFour = false;
        }
        if (OneOTen == false)
        { // want this
          taskBuzzer1010.enable();
          OneOTen = true;
        }
        Buzlevel = 1;
      }
      if (SerialDebug)
        Serial.println("CO more than half Sm less than half");
      Onfire = false;
    }
    else if ((FinalCO < COTreshold / 2) && (FinalSM >= SMTreshold / 2))
    { // SM more than half Co less than half
      if (ovr_Status == false)
      {
        if (OneOTwo == true)
        {
          taskBuzzer102.disable();
          OneOTwo = false;
        }
        if (OneOFour == true)
        {
          taskBuzzer104.disable();
          OneOFour = false;
        }
        if (OneOTen == false)
        { // want this
          taskBuzzer1010.enable();
          OneOTen = true;
        }
        Buzlevel = 1;
      }
      if (SerialDebug)
      {
        Serial.println("CO more than half Sm less than half");
        Serial.println("SM more than half Co less than half");
      }

      Onfire = false;
    }
    else if ((FinalCO < COTreshold / 2) && (FinalSM < SMTreshold / 2))
    { // Both less than half
      if (ovr_Status == false)
      {
        if (OneOTwo == true)
        {
          taskBuzzer102.disable();
          OneOTwo = false;
        }
        if (OneOFour == true)
        {
          taskBuzzer104.disable();
          OneOFour = false;
        }
        if (OneOTen == true)
        {
          taskBuzzer1010.disable();
          OneOTen = false;
        }
        Buzlevel = 0;
      }
      if (SerialDebug)
        Serial.println("Both less than half");
      Onfire = false;
    }
  }
  if (Onfire == true)
  { // address data to send
    doc["status"] = "1";
  }
  else
  {
    doc["status"] = "0";
  }
}

void SMClean()
{
  if (abs(SMDif12) >= DifTresholdSM)
  {
    Dif12 = false;
  }
  if (abs(SMDif13) >= DifTresholdSM)
  {
    Dif13 = false;
  }
  if (abs(SMDif23) >= DifTresholdSM)
  {
    Dif23 = false;
  }
  if (Dif12 == true && Dif13 == true && Dif23 == true)
  {
    SM = ((Smoke1 + Smoke2 + Smoke3) / 3);
    if (SerialDebug)
      Serial.println("SM All sensor read");
    OldSM = SM;
  }
  else if (Dif12 == true && Dif13 == false && Dif23 == false)
  {
    SM = ((Smoke1 + Smoke2) / 2);
    if (SerialDebug)
      Serial.println("SM Sensor 1 2 Read");
    OldSM = SM;
  }
  else if (Dif12 == false && Dif13 == true && Dif23 == false)
  {
    SM = ((Smoke1 + Smoke3) / 2);
    if (SerialDebug)
      Serial.println("SM Sensor 1 3 Read");
    OldSM = SM;
  }
  else if (Dif12 == false && Dif13 == false && Dif23 == true)
  {
    SM = ((Smoke2 + Smoke3) / 2);
    if (SerialDebug)
      Serial.println("SM Sensor 2 3 Read");
    OldSM = SM;
  }
  else
  { // if (Dif12 == false && Dif13 == false && Dif23 == false) (false 2,1 อัน)
    if (SerialDebug)
      Serial.println("SM Sensor Read Failed");
    SM = OldSM;
    SMError = true;
  }
}
void COClean()
{
  if (abs(CODif12) >= DifTresholdCO)
  {
    Dif12 = false;
  }
  if (abs(CODif13) >= DifTresholdCO)
  {
    Dif13 = false;
  }
  if (abs(CODif23) >= DifTresholdCO)
  {
    Dif23 = false;
  }
  if (Dif12 == true && Dif13 == true && Dif23 == true)
  {
    CO = ((CO1 + CO2 + CO3) / 3);
    if (SerialDebug)
      Serial.println("CO All sensor read");
    OldCO = CO;
  }
  else if (Dif12 == true && Dif13 == false && Dif23 == false)
  {
    CO = ((CO1 + CO2) / 2);
    if (SerialDebug)
      Serial.println("CO Sensor 1 2 Read");
    OldCO = CO;
  }
  else if (Dif12 == false && Dif13 == true && Dif23 == false)
  {
    CO = ((CO1 + CO3) / 2);
    if (SerialDebug)
      Serial.println("CO Sensor 1 3 Read");
    OldCO = CO;
  }
  else if (Dif12 == false && Dif13 == false && Dif23 == true)
  {
    CO = ((CO2 + CO3) / 2);
    if (SerialDebug)
      Serial.println("CO Sensor 2 3 Read");
    OldCO = CO;
  }
  else
  { // if (Dif12 == false && Dif13 == false && Dif23 == false) (false 2,1 อัน)
    if (SerialDebug)
      Serial.println("CO Sensor Read Failed");
    CO = OldCO;
    COError = true;
  }
}
void ClearFdictCheck()
{
  if (d.count() > 0)
  {
    if (!FdictCheck)
    {
      taskClearFdict.enable();
      FdictCheck = true;
      FDFirstRun = true; // tell it to not clear on first run
    }
  }
  else
  {
    if (FdictCheck)
    {
      taskClearFdict.disable();
      FdictCheck = false;
    }
  }
}
void ClearFdict()
{
  if (FDFirstRun)
  {
    FDFirstRun = false;
  }
  else
  {
    String RemoveP = d(0);
    d.remove(d(0));    // delete entry in onfire dict
    p.remove(RemoveP); // delete entry in samsay dict
  }
}

void TempCheckCode()
{
  float temperature = dht.getTemperature();
  float Humid = dht.getHumidity(); // just leave it here.
  if (TempCheckTimes < TempTresh)
  {
    if (SerialDebug)
      Serial.println("Temp = " + String(temperature));
    SumTemp = temperature + SumTemp;

    if (SerialDebug)
      Serial.println("SumTemp = " + String(SumTemp));
    TempCheckTimes++;
  }
  else
  {
    FinalTemp = SumTemp / TempTresh;

    if (SerialDebug)
      Serial.println("FinalTemp = " + String(FinalTemp));
    TempCheckTimes = 0;
    if (FinalTemp >= 40)
    { // Sensor is hot enough
      if (SysOn == false)
      {

        if (SerialDebug)
          Serial.println("Sensor Heatup Complete : System ON ");
        if (!Sensorcalib)
        {
          CalibrateSensor();
          Sensorcalib = true;
        }
        taskSendMessage.enable();
        taskFiredetect.enable();
        taskSensorRead.enable();
        taskESPReset.enable();
        SysOn = true;
      }
    }
    else
    {
      if (SerialDebug)
        Serial.println("Sensor Heatup Incomplete : System OFF ");
      msg = ""; // reset msg string
      doc["Room"] = Room;
      RoomStr = String(Room);
      Roomint = RoomStr.toInt();
      if (ID < 100)
      {
        MSG_ID = String((Roomint * 100) + ID);
        ID = ID + 1;
      }
      else
      {
        ID = 0;
        MSG_ID = String((Roomint * 100) + ID);
        ID = ID + 1;
      }
      doc["MSG_ID"] = MSG_ID;
      if (s.count() >= 99)
      {
        s.remove(s(0)); // remove oldest input data to prevent overflow
        s(MSG_ID, "0"); // MSGID for reference with empty value(value not used)
      }
      else
      {
        s(MSG_ID, "0"); // Save sent msgid to dictionary
      }
      doc["status"] = "0";
      doc["ERROR"] = "4";
      serializeJson(doc, msg);
      mesh.sendBroadcast(msg);
      // Serial.println(msg);
      taskSendMessage.disable();
      taskFiredetect.disable();
      taskSensorRead.disable();
      taskESPReset.disable();
      SysOn = false;
    }
    FinalTemp = 0; // reset FinalTemp
    SumTemp = 0;   // reset sumTemp
  }
}

//---------------------------Extra MQ2 code--------------------------------------------------
void SensorRead()
{
  STR_A1 = analogRead(Sen1Pin);
  STR_A2 = analogRead(Sen2Pin);
  STR_A3 = analogRead(Sen3Pin);
}
float MQRead(int analogValue)
{
  float rs = 0;
  rs = MQResistanceCalculation(analogValue);
  return rs;
}
void ReadSensor()
{
  if (SerialDebug)
  {
    Serial.println("Sen1 = " + String(STR_A1) + ": Sen2 = " + String(STR_A2) + ": Sen3 = " + String(STR_A3));
  }
  CO1 = MQGetGasPercentage(MQRead(STR_A1) / Ro1, GAS_CO);
  Smoke1 = MQGetGasPercentage(MQRead(STR_A1) / Ro1, GAS_SMOKE);
  CO2 = MQGetGasPercentage(MQRead(STR_A2) / Ro2, GAS_CO);
  Smoke2 = MQGetGasPercentage(MQRead(STR_A2) / Ro2, GAS_SMOKE);
  CO3 = MQGetGasPercentage(MQRead(STR_A3) / Ro3, GAS_CO);
  Smoke3 = MQGetGasPercentage(MQRead(STR_A3) / Ro3, GAS_SMOKE);
}
void CalibrateSensor()
{
  Ro1 = MQCalibration(Sen1Pin);
  Ro2 = MQCalibration(Sen2Pin);
  Ro3 = MQCalibration(Sen3Pin);
  int SavedRo1 = (EEPROM.read(1)); // Read Saved RO from rom
  int SavedRo2 = (EEPROM.read(5));
  int SavedRo3 = (EEPROM.read(10));
  int dif = Ro1 - SavedRo1;
  if (abs(dif) <= RoTresh)
  { // CompareNew Ro1
    {
      Serial.print("Ro1=");
      Serial.println(Ro1);
    }
  }
  else
  {
    Serial.print("New Ro1 Unreliable = ");
    Serial.println(Ro1);
    Serial.print("Old Ro1 =");
    Serial.println(SavedRo1);
    Ro1 = SavedRo1;
  }
  dif = Ro2 - SavedRo2;
  if (abs(dif) <= RoTresh)
  { // CompareNew Ro2

    Serial.print("Ro2=");
    Serial.println(Ro2);
  }
  else
  {
    Serial.print("New Ro2 Unreliable = ");
    Serial.println(Ro2);
    Serial.print("Old Ro2 =");
    Serial.println(SavedRo2);

    Ro2 = SavedRo2;
  }
  dif = Ro3 - SavedRo3;
  if (abs(dif) <= RoTresh)
  { // CompareNew Ro3

    Serial.print("Ro3 =");
    Serial.println(Ro3);
  }
  else
  {

    Serial.print("New Ro3 Unreliable = ");
    Serial.println(Ro3);
    Serial.print("Old Ro3 =");
    Serial.println(SavedRo3);

    Ro3 = SavedRo3;
  }
}