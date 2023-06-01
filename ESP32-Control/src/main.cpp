// For esp32 ttgo screen  Only
#include <Arduino.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>
#include <Wire.h>
#include "OneButton.h"
#include "TFT_eSPI.h" /* Please use the TFT library provided in the library. */
#include "TouchLib.h"
#include "pin_config.h"
#include <Pangodream_18650_CL.h>
#define TOUCH_MODULES_CST_MUTUAL
#define _DICT_PACK_STRUCTURES
#define MESH_PREFIX "FireAlarm"
#define MESH_PASSWORD "19283746"
#define MESH_PORT 1010
#include <Dictionary.h>
#define RXD2 16
#define TXD2 17
#define WFCPin 39
#define ChaosPin 12
Dictionary &d = *(new Dictionary()); // Onfire
Dictionary &s = *(new Dictionary()); // relay
Dictionary &N = *(new Dictionary()); // relay
Dictionary &ErrorD = *(new Dictionary());
Scheduler userScheduler; // to control your personal task
painlessMesh mesh;
#define LineSize 15
int ChaosTick = 0;
int HarmonyTick = 0;
int ID = 0;
int IDcount;
int ErrorNum;
int DisplayPosi;
int ERTick;
int buzpin = 43; // pin for buzzer pin
int PageAlpha = 1;
String PAmsg;
int PaOrder;
int Buzlevel = 0;
char Room[] = "987"; // change room name
bool Onfire = false;
bool SentFire = false;
bool BuzzerON = false;
bool ovr_Status = false; // Overide for buzzer
bool ChaosOn = false;
bool BHold, RunBOnce, OnceHold, RunBRepeat, RepeatBHold, OrderHold;
bool Backhold, DelHold, KBsave;
bool NextHold, FRAlphaHold, SAlphaHold, FAlphaHold, TAlphaHold;
String KBOUTPUT;
String Finalstr;
//----------------------------------------------------------------------
const char *Recieved_ID;
String MSG_ID;
String RE_ID;
String Recieved_status;
String Relay_Message;
String Error_Status;
int CurrentRoom = 0;
String json1;
SimpleList<uint32_t> nodes;
//---------For Serial------------------------------
String message = "";
bool messageReady = false;
int Req_ID;
bool Doc_Created = false;
bool DocRecieve_Created = false;
//--------
DynamicJsonDocument doc(1024);
DynamicJsonDocument doc_Recieved(1024);

void sendMessage(); // Prototype so PlatformIO doesn't complain
void Database();
void BuzzerAlert102();
void BuzzerAlert104();
void BuzzerAlert1010();
void Beep();
void Relay();
void ESPReset();
void Loop2Code(void *pvParameters);
//--------------------------
void InfoSet();
void BGRefreshCode();
void TouchCode();
void BGimage();
void ButtonTouch();
void Topinfo();
void PlusMinusT();
void CancelButton();
void LeftRightArrow();
void SixtySec();
void ClearEmpty();
void KBCreate();
void KBTouch();
void SclOne();
void SclTwo();
void SclThree();
int ResetTimer;
int SumScroll;
//----------------------For Screen--------------------------------------------
bool debug = false;
TaskHandle_t BGRefresh;
TaskHandle_t Loop2;
int ScreenNum = 1;
int NTick, STick;
TouchLib touch(Wire, PIN_IIC_SDA, PIN_IIC_SCL, CTS328_SLAVE_ADDRESS);
#define TOUCH_GET_FORM_INT 0
int SimulateRoom = 1;
TFT_eSPI tft = TFT_eSPI();
bool flags_sleep = false;
int XCoord, YCoord, PrCoord;
bool ModeEnter = false;
int SimulateTick;
bool SCRHoldL = false;
bool SCRHoldR = false;
bool KBholdR, KBholdL;
bool StartScr = true;
bool SWHold = false;
bool CCHold = false;
bool SWFinal = false;
bool SWClicked = false;
bool PlusHold = false;
bool SaveHold, CancelHold;
bool MinusHold = false;
int OFF_SimulateTick;
bool StartBroadcast = false;
int XCoordLS, XCoordRS;
bool TopNewHd, MidNewHd, BtNewHd, TopDelHd, MidDelHd, BtDelHd;
bool ScrollOne, ScrollTwo, ScrollThree;
bool TopDelCf, MidDelCf, BtDelCf;
bool KBSaveHold, KBCancelHold, KBsymbolHold;
bool OneEd, TwoEd, ThreeEd;
String OneMsg, TwoMsg, ThreeMsg;
bool OutputScr;
int Onedelay, Twodelay, Threedelay;
bool KBran, Symbol;
String KeyboardStr;
String Keyboard(int Pick);
int Keyboardlen;
String FDigit, SDigit, TDigit, FrDigit;
int xt = 8;
int yt = 8;
int SendTick = 1;
#if TOUCH_GET_FORM_INT
bool get_int = false;
#endif
//---------------------Battery---------------------
#define MIN_USB_VOL 4.9
#define CONV_FACTOR 1.8
#define READS 20
Pangodream_18650_CL BL(PIN_BAT_VOLT, CONV_FACTOR, READS);
//-------------------------------------------------

OneButton button(PIN_BUTTON_1, true);
OneButton button2(PIN_BUTTON_2, true);

TFT_eSprite sprite = TFT_eSprite(&tft);
TFT_eSprite spr2 = TFT_eSprite(&tft);
TFT_eSprite KBsprite = TFT_eSprite(&tft);
//-------------------------------------
Task taskSendMessage(TASK_SECOND * 2, TASK_FOREVER, &sendMessage);
Task taskBuzzer102(TASK_SECOND * 2, TASK_FOREVER, &BuzzerAlert102);
Task taskBuzzer104(TASK_SECOND * 4, TASK_FOREVER, &BuzzerAlert104);
Task taskBuzzer1010(TASK_SECOND * 10, TASK_FOREVER, &BuzzerAlert1010);
Task taskBeep(TASK_SECOND * 0.5, TASK_FOREVER, &Beep);
Task taskESPReset(TASK_SECOND * 1, TASK_FOREVER, &ESPReset);
Task taskSixtySec(TASK_SECOND * 5, TASK_FOREVER, &SixtySec);

//---------------------------------------------------
void receivedCallback(uint32_t from, String &Recievedmsg)
{ // Recieved messages
  Relay_Message = Recievedmsg;
  json1 = Recievedmsg.c_str();
  DeserializationError error = deserializeJson(doc_Recieved, json1); // If error
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
  }
  Recieved_ID = doc_Recieved["Room"];
  Recieved_status = doc_Recieved["status"].as<String>();
  RE_ID = doc_Recieved["MSG_ID"].as<String>();
  Error_Status = doc_Recieved["ERROR"].as<String>();
  if (debug)
  {
    Serial.print("RoomID = " + String(Recieved_ID));
    Serial.print(" Recieved_status = " + String(Recieved_status));
    Serial.print(" ERROR = " + String(Error_Status));
    Serial.println(" RE_ID = " + String(RE_ID));
  }
  Relay();
}
void OverrideBuzzer()
{
  if ((d.count()) >= 10)
  { // Rooms on fire >= 10
    ovr_Status = true;
    if (Buzlevel < 3)
    { // if Buzzlevel lower than highest(3)
      taskBuzzer102.enable();
      taskBuzzer104.disable();
      taskBuzzer1010.disable();
      if (debug)
        Serial.println("Overide Buzzer102");
      Buzlevel = 3;
    }
  }
  else
  {
    ovr_Status = false;
  }
}
void TouchCode()
{
  char str_buf[100];
  static uint8_t last_finger;
#if TOUCH_GET_FORM_INT
  if (get_int)
  {
    get_int = 0;
    touch.read();
#else
  if (touch.read())
  {
#endif
    uint8_t n = touch.getPointNum();
    // sprintf(str_buf, "finger num :%d \r\n", n);
    // tft.drawString(str_buf, 0, 15);
    for (uint8_t i = 0; i < n; i++)
    {
      TP_Point t = touch.getPoint(i);
      //sprintf(str_buf, "x:%04d y:%04d p:%04d \r\n", t.x, t.y, t.pressure);
      //tft.drawString(str_buf, 0, 35 + (20 * i));
      XCoord = t.x;
      YCoord = t.y;
      PrCoord = t.pressure;
      // Serial.println("Xcoord = " + String(XCoord) + " Ycoord = " + String(YCoord) + " Prcoord = " + String(PrCoord));
    }
    if (last_finger > n) // if finger count goes down
    {
      // tft.fillRect(0, 55, 320, 100, TFT_BLACK);
    }
    last_finger = n;
  }
  else
  {
    // No finger detected
    XCoord = 0;
    YCoord = 0;
    PrCoord = 0;
    // Serial.println("Xcoord = " + String(XCoord) + " Ycoord = " + String(YCoord) + " Prcoord = " + String(PrCoord));
    //  tft.fillScreen(TFT_GREEN);
  }
}
void BGRefreshCode()
{
  sprite.fillScreen(TFT_LIGHTGREY);
  ButtonTouch();
  Topinfo();

  if (!KBran)
  {
    sprite.pushSprite(0, 0);
  }
  else
  {
    KBsprite.pushSprite(0, 0);
  }
}
void Topinfo()
{
  if (ScreenNum == 1)
  {
    LeftRightArrow();
    spr2.fillSprite(TFT_BLACK);
    spr2.setFreeFont(&Orbitron_Light_24);
    spr2.fillRoundRect(2 + 5, 0 + 5, 75, 20, 3, TFT_MAGENTA);
    spr2.setTextColor(TFT_GREEN); // Turns red for some fucking reason
    spr2.drawString("Nodes : ", 36 + 5, 10 + 5, 2);
    spr2.drawString(String(nodes.size()), 65 + 5, 10 + 5, 2);
    spr2.fillRoundRect(84 + 5, 0 + 5, 75, 20, 3, TFT_MAGENTA);
    spr2.drawString("Normal : ", 115 + 5, 10 + 5, 2);
    spr2.drawString(String(nodes.size() - d.count()), 145 + 5, 10 + 5, 2);
    //-----------------------------------------------------------
    spr2.fillRoundRect(2 + 5, 25 + 5, 75, 20, 3, TFT_MAGENTA);
    spr2.drawString("Fire : ", 41 + 5, 36 + 5, 2);
    spr2.drawString(String(d.count()), 65 + 5, 36 + 5, 2);
    spr2.fillRoundRect(84 + 5, 25 + 5, 75, 20, 3, TFT_MAGENTA);
    spr2.drawString("Errors : ", 115 + 5, 36 + 5, 2);
    spr2.drawString(String(ErrorNum), 145 + 5, 36 + 5, 2);
    //-----------------------------------------------------------
    spr2.fillRoundRect(5, 53 + 5, 160, 55, 3, TFT_SILVER);
    spr2.fillRect(5 + 5, 60 + 5, 150, 40, TFT_DARKGREY);
    if (!SWClicked)
    {
      sprite.fillRoundRect(28, 283, 115, 25, 5, TFT_BLACK);
      sprite.setTextColor(TFT_WHITE);
      sprite.drawCentreString("TEST-SW", 85, 287, 2);
      spr2.setTextColor(TFT_SILVER);
      spr2.fillRect(78 + 5, 60 + 5, 6, 40, TFT_SILVER);
      spr2.drawString("RST : ", 33 + 5, 80 + 5, 2);
      spr2.drawString(String(ResetTimer), 65 + 5, 80 + 5, 2);
      if (BL.getBatteryVolts() >= MIN_USB_VOL)
      {
        spr2.setTextColor(TFT_GREEN);
        spr2.drawString("CHARGING", 120 + 5, 80 + 5, 2);
      }
      else
      {
        spr2.drawString("BATT : ", 113 + 5, 80 + 5, 2);
        spr2.drawString(String(BL.getBatteryChargeLevel()), 143 + 5, 80 + 5, 2);
      }
    }
    else
    {
      if (!SWFinal) // If Not final button yet (Green)
      {
        spr2.fillRect(5 + 5, 60 + 5, 45, 40, TFT_BLUE);
        spr2.fillRect(110 + 5, 60 + 5, 45, 40, TFT_GREEN);
        spr2.fillRect(45 + 5, 60 + 5, 6, 40, TFT_SILVER);
        spr2.fillRect(110 + 5, 60 + 5, 6, 40, TFT_SILVER);
        spr2.fillRect(22 + 5, 67 + 5, 7, 25, TFT_SILVER);  // Plus sign Y
        spr2.fillRect(13 + 5, 75 + 5, 25, 7, TFT_SILVER);  // Plus sign X
        spr2.fillRect(123 + 5, 75 + 5, 25, 7, TFT_SILVER); // Minus sign
        spr2.drawString("Sim : ", 80 + 5, 80 + 5, 2);
        spr2.drawString(String(SimulateRoom), 100 + 5, 80 + 5, 2);
      }
      else
      {
        spr2.drawString("BROADCASTING", 80, 80, 2);
      }
    }
    spr2.pushToSprite(&sprite, 0, 0, TFT_BLACK);
  }
  else if (ScreenNum == 2)
  {
    if (OneEd || TwoEd || ThreeEd) // KB is running
    {
    }
    else
    {
      LeftRightArrow();
      if (OutputScr)
      {
      }
      else
      {
        sprite.fillRoundRect(110, 70 - 40, 40, 15, 3, TFT_LIGHTGREY);  // top grey bt
        sprite.fillRoundRect(110, 125 - 40, 40, 15, 3, TFT_LIGHTGREY); // mid grey bt
        sprite.fillRoundRect(110, 180 - 40, 40, 15, 3, TFT_LIGHTGREY); // bt grey bt
        if (OneMsg.length() != 0)
        {
          sprite.setTextColor(TFT_BLACK);
          sprite.fillRoundRect(15, 70 - 40, 70, 15, 3, TFT_GREEN); // top green bt
          sprite.drawCentreString("OCUPIED", 50, 70 - 40, 2);
          sprite.drawCentreString(String(OneMsg.length()), 130, 70 - 40, 2);
        }
        else
        {
          sprite.fillRoundRect(15, 70 - 40, 70, 15, 3, TFT_RED); // top green bt
          sprite.setTextColor(TFT_WHITE);
          sprite.drawCentreString("Empty", 50, 70 - 40 - 2, 2);
        }

        if (TwoMsg.length() != 0)
        {
          sprite.setTextColor(TFT_BLACK);
          sprite.fillRoundRect(15, 125 - 40, 70, 15, 3, TFT_GREEN); // mid green bt
          sprite.drawCentreString("OCUPIED", 50, 125 - 40, 2);
          sprite.drawCentreString(String(TwoMsg.length()), 130, 125 - 40, 2);
        }
        else
        {
          sprite.fillRoundRect(15, 125 - 40, 70, 15, 3, TFT_RED); // mid green bt
          sprite.setTextColor(TFT_WHITE);
          sprite.drawCentreString("Empty", 50, 125 - 40 - 2, 2);
        }
        if (ThreeMsg.length() != 0)
        {
          sprite.setTextColor(TFT_BLACK);
          sprite.fillRoundRect(15, 180 - 40, 70, 15, 3, TFT_GREEN); // bt green bt
          sprite.drawCentreString("OCUPIED", 50, 180 - 40, 2);
          sprite.drawCentreString(String(ThreeMsg.length()), 130, 180 - 40, 2);
        }
        else
        {
          sprite.fillRoundRect(15, 180 - 40, 70, 15, 3, TFT_RED); // mid green bt
          sprite.setTextColor(TFT_WHITE);
          sprite.drawCentreString("Empty", 50, 180 - 40 - 2, 2);
        }
        sprite.setTextColor(TFT_BLACK);
      }
      sprite.setFreeFont(&Orbitron_Light_24);
      sprite.fillRoundRect(28, 283, 115, 25, 5, TFT_BLACK);
      sprite.setTextColor(TFT_WHITE);
      sprite.drawCentreString("PA-SYSTEM", 85, 287, 2);
    }
  }
  else if (ScreenNum == 3)
  {
    LeftRightArrow();
    spr2.fillSprite(TFT_BLACK);
    spr2.setFreeFont(&Orbitron_Light_24);
    spr2.fillRoundRect(2, 0, 150, 135, 3, TFT_SKYBLUE); // BG
    spr2.fillRoundRect(30, 4, 100, 20, 3, TFT_BLUE);    // SIGN BG
    spr2.setTextColor(TFT_SILVER);
    spr2.drawString("Status : Normal", 79, 12, 2); // Sign
    if (N.count() > 0)
    { // if there is Room not on Fire
      while (NTick + 1 <= N.count())
      {
        if (debug)
          Serial.println("ROOM_NOT_ON_FIRE = " + (String(N(NTick))));
        spr2.drawString(String(NTick + 1), 10, 32 + (NTick * LineSize), 2);
        spr2.drawString(". ", 20, 32 + (NTick * LineSize), 2);
        spr2.drawString(String(N(NTick)), 30 + ((String(N(NTick)).length()) * 3), 32 + (NTick * LineSize), 2);
        NTick++;
      }
      NTick = 0; // cycle send
    }
    else
    {
      spr2.fillRoundRect(55, 68, 50, 20, 3, TFT_SILVER); // SIGN BG
      spr2.setTextColor(TFT_WHITE);
      spr2.drawString("NONE", 80, 78, 2);
    }
    //---------------------Rooms on FIRE------------------------
    spr2.fillRoundRect(2, 140, 150, 130, 2, TFT_DARKGREEN); // BG
    spr2.fillRoundRect(23, 144, 110, 20, 3, TFT_CYAN);      // SIGN BG
    spr2.setTextColor(TFT_SILVER);
    spr2.drawString("Status : On Fire", 79, 155, 2); // Sign
    if (d.count() > 0)
    {
      while (NTick + 1 <= d.count())
      {
        if (debug)
          Serial.println("Room on fire = " + (String(d.count()) + " Rooms"));
        if (debug)
          Serial.println("ROOM_ON_FIRE = " + (String(d(NTick))));
        spr2.drawString(String(NTick + 1), 10, 180 + (NTick * LineSize), 2);
        spr2.drawString(". ", 20, 180 + (NTick * LineSize), 2);
        spr2.drawString(String(d(NTick)), 30 + ((String(d(NTick)).length()) * 3), 180 + (NTick * LineSize), 2);
        NTick++;
      }
      NTick = 0; // cycle send
    }
    else
    {
      spr2.fillRoundRect(55, 220, 50, 20, 3, TFT_SILVER); // SIGN BG
      spr2.setTextColor(TFT_WHITE);
      spr2.drawString("NONE", 80, 230, 2);
    }
    sprite.fillRoundRect(28, 283, 115, 25, 5, TFT_BLACK);
    sprite.setTextColor(TFT_WHITE);
    sprite.drawString("Devices", 65, 285, 2);
    spr2.pushToSprite(&sprite, xt, yt, TFT_BLACK);
  }
  else if (ScreenNum == 4)
  {
    LeftRightArrow();
    spr2.fillSprite(TFT_BLACK);
    spr2.setFreeFont(&Orbitron_Light_24);
    spr2.fillRoundRect(2, 0, 150, 270, 3, TFT_DARKGREEN); // BG
    spr2.fillRoundRect(30, 4, 100, 20, 3, TFT_CYAN);      // SIGN BG
    spr2.setTextColor(TFT_SILVER);
    spr2.drawString("Status : ERROR", 79, 12, 2); // Sign
    if (ErrorD.count() > 0)
    { // if there is Room error
      while (ERTick + 1 <= ErrorD.count())
      {
        if (debug)
          Serial.println("ROOM_NOT_ON_FIRE = " + (String(N(ERTick))));
        spr2.drawString(String(ERTick + 1), 10, 32 + (ERTick * LineSize), 2);
        spr2.drawString(". ", 20, 32 + (NTick * LineSize), 2);
        spr2.drawString(String(N(ERTick)), 30, 32 + (ERTick * LineSize), 2);
        ERTick++;
      }
      ERTick = 0; // cycle send
    }
    else
    {
      spr2.fillRoundRect(55, 68 + 150, 50, 20, 3, TFT_SILVER); // SIGN BG
      spr2.setTextColor(TFT_WHITE);
      spr2.drawString("NONE", 80, 78 + 150, 2);
    }
    sprite.fillRoundRect(28, 283, 115, 25, 5, TFT_BLACK);
    sprite.setTextColor(TFT_WHITE);
    sprite.drawString("ERRORS", 65, 285, 2);
    spr2.pushToSprite(&sprite, xt, yt, TFT_BLACK);
  }
}

void ButtonTouch()
{
  if (ScreenNum == 1)
  {
    if (!SWFinal)
    {
      if (!SWClicked) // For Switch
      {
        if (!SWHold)
        {
          if ((XCoord <= 120) && (XCoord >= 30) && (YCoord <= 160) && (YCoord >= 60))
          {
            sprite.fillSmoothCircle(85, 200, 65, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 60, TFT_WHITE, TFT_BLACK);
            sprite.fillSmoothCircle(85, 200, 50, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 45, TFT_DARKGREEN, TFT_BLACK);
            XCoordLS = XCoord;
            SWHold = true;
          }
          else
          {
            sprite.fillSmoothCircle(85, 200, 65, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 60, TFT_WHITE, TFT_BLACK);
            sprite.fillSmoothCircle(85, 200, 50, TFT_BLACK, TFT_WHITE);
          }
        }
        else
        {
          if ((XCoord == 0) && (YCoord == 0))
          {
            sprite.fillSmoothCircle(85, 200, 65, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 60, TFT_WHITE, TFT_BLACK);
            sprite.fillSmoothCircle(85, 200, 50, TFT_BLACK, TFT_WHITE);
            // Serial.println("Finger lifted");
            SWClicked = true;
            SWHold = false;
          }
          else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
          {
            sprite.fillSmoothCircle(85, 200, 65, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 60, TFT_WHITE, TFT_BLACK);
            sprite.fillSmoothCircle(85, 200, 50, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 45, TFT_DARKGREEN, TFT_BLACK);
            // Serial.println("Finger in range");
          }
          else
          {
            sprite.fillSmoothCircle(85, 200, 65, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 60, TFT_WHITE, TFT_BLACK);
            sprite.fillSmoothCircle(85, 200, 50, TFT_BLACK, TFT_WHITE);
            // Serial.println("Finger not in range");
            SWHold = false;
          }
        }
      }
      else
      {
        CancelButton();
        PlusMinusT(); // Plus Minus touch
        if (!SWHold)
        {
          if ((XCoord <= 120) && (XCoord >= 50) && (YCoord <= 160) && (YCoord >= 60)) // clicked
          {
            sprite.fillSmoothCircle(85, 200, 65, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 60, TFT_WHITE, TFT_BLACK);
            sprite.fillSmoothCircle(85, 200, 50, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 45, TFT_GREEN, TFT_BLACK);
            XCoordLS = XCoord;
            SWHold = true;
          }
          else
          {
            sprite.fillSmoothCircle(85, 200, 65, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 60, TFT_WHITE, TFT_BLACK);
            sprite.fillSmoothCircle(85, 200, 50, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 45, TFT_DARKGREEN, TFT_BLACK);
          }
        }
        else
        {
          if ((XCoord == 0) && (YCoord == 0))
          {
            sprite.fillSmoothCircle(85, 200, 65, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 60, TFT_WHITE, TFT_BLACK);
            sprite.fillSmoothCircle(85, 200, 50, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 45, TFT_DARKGREEN, TFT_BLACK);
            // Serial.println("Finger lifted");
            SWFinal = true;
            SWHold = false;
          }
          else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
          {
            sprite.fillSmoothCircle(85, 200, 65, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 60, TFT_WHITE, TFT_BLACK);
            sprite.fillSmoothCircle(85, 200, 50, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 45, TFT_GREEN, TFT_BLACK);
            // Serial.println("Finger in range");
          }
          else
          {
            sprite.fillSmoothCircle(85, 200, 65, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 60, TFT_WHITE, TFT_BLACK);
            sprite.fillSmoothCircle(85, 200, 50, TFT_BLACK, TFT_WHITE);
            sprite.fillSmoothCircle(85, 200, 45, TFT_DARKGREEN, TFT_BLACK);
            // Serial.println("Finger not in range");
            SWHold = false;
          }
        }
      }
    }
    else
    {
      CancelButton();
      sprite.fillSmoothCircle(85, 200, 65, TFT_BLACK, TFT_WHITE);
      sprite.fillSmoothCircle(85, 200, 60, TFT_WHITE, TFT_BLACK);
      sprite.fillSmoothCircle(85, 200, 50, TFT_BLACK, TFT_WHITE);
      sprite.fillSmoothCircle(85, 200, 45, TFT_DARKGREY, TFT_BLACK);
    }
  }
  else if (ScreenNum == 2) // The Broadcast Screen
  {
    if (KBsave)
    {
      if (OneEd)
      {
        OneMsg = Finalstr;
      }
      else if (TwoEd)
      {
        TwoMsg = Finalstr;
      }
      else if (ThreeEd)
      {
        ThreeMsg = Finalstr;
      }
      KBsave = false;
      OneEd = false;
      TwoEd = false;
      ThreeEd = false;
    }
    if (OneEd)
    {
      Finalstr = Keyboard(1);
    }
    else if (TwoEd)
    {
      Finalstr = Keyboard(2);
    }
    else if (ThreeEd)
    {
      Finalstr = Keyboard(3);
    }
    else
    {
      if (KBran)
      {
        tft.setRotation(4);
        KBran = false;
      }
      sprite.fillSprite(TFT_LIGHTGREY);
      sprite.fillRoundRect(5, 53 + 5 + 135, 160, 78 + 5, 3, TFT_DARKGREY);
      if (!OnceHold)
      {
        if ((XCoord <= 170) && (XCoord >= 0) && (YCoord <= 90) && (YCoord >= 55))
        {
          sprite.fillRect(5 + 5, 60 + 5 + 160, 150, 40 - 20, TFT_BLACK);
          XCoordLS = XCoord;
          OnceHold = true;
        }
        else
        {
          sprite.fillRect(5 + 5, 60 + 5 + 160, 150, 40 - 20, TFT_SILVER);
        }
      }
      else
      {
        if ((XCoord == 0) && (YCoord == 0))
        {
          sprite.fillRect(5 + 5, 60 + 5 + 160, 150, 40 - 20, TFT_SILVER);
          // Serial.println("Finger lifted");
          SendTick = 0;
          RunBOnce = true;
          OutputScr = true;
          OnceHold = false;
        }
        else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
        {
          sprite.fillRect(5 + 5, 60 + 5 + 160, 150, 40 - 20, TFT_BLACK);
          // Serial.println("Finger in range");
        }
        else
        {
          sprite.fillRect(5 + 5, 60 + 5 + 160, 150, 40 - 20, TFT_SILVER);
          OnceHold = false;
        }
      }
      if (!RepeatBHold)
      {
        if ((XCoord <= 170) && (XCoord >= 0) && (YCoord <= 54) && (YCoord >= 15))
        {
          sprite.fillRect(5 + 5, 60 + 5 + 185, 150, 40 - 20, TFT_BLACK);
          XCoordLS = XCoord;
          RepeatBHold = true;
        }
        else
        {
          sprite.fillRect(5 + 5, 60 + 5 + 185, 150, 40 - 20, TFT_SILVER);
        }
      }
      else
      {
        if ((XCoord == 0) && (YCoord == 0))
        {
          sprite.fillRect(5 + 5, 60 + 5 + 185, 150, 40 - 20, TFT_SILVER);
          if (!RunBRepeat)
          {
            RunBRepeat = true;
            OutputScr = true;
          }
          else
          {
            RunBRepeat = false;
          }
          RepeatBHold = false;
        }
        else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
        {
          sprite.fillRect(5 + 5, 60 + 5 + 185, 150, 40 - 20, TFT_BLACK);
        }
        else
        {
          sprite.fillRect(5 + 5, 60 + 5 + 185, 150, 40 - 20, TFT_SILVER);
          RepeatBHold = false;
        }
      }
      if (!OrderHold)
      {
        if ((XCoord <= 170) && (XCoord >= 0) && (YCoord <= 125) && (YCoord >= 95))
        {
          sprite.fillRect(5 + 5, 60 + 5 + 135, 150, 40 - 20, TFT_BLACK);
          XCoordLS = XCoord;
          OrderHold = true;
        }
        else
        {
          sprite.fillRect(5 + 5, 60 + 5 + 135, 150, 40 - 20, TFT_SILVER);
        }
      }
      else
      {
        if ((XCoord == 0) && (YCoord == 0))
        {
          sprite.fillRect(5 + 5, 60 + 5 + 135, 150, 40 - 20, TFT_SILVER);
          if (PaOrder < 4) // 0,1,2,3,4
          {
            PaOrder++;
          }
          else
          {
            PaOrder = 0;
          }
          OrderHold = false;
        }
        else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
        {
          sprite.fillRect(5 + 5, 60 + 5 + 135, 150, 40 - 20, TFT_BLACK);
        }
        else
        {
          sprite.fillRect(5 + 5, 60 + 5 + 135, 150, 40 - 20, TFT_SILVER);
          OrderHold = false;
        }
      }
      sprite.setTextColor(TFT_BLACK);
      if (PaOrder == 0)
      {
        sprite.drawCentreString("1-2-3", 85, 60 + 5 + 137, 2);
      }
      else if (PaOrder == 1)
      {
        sprite.drawCentreString("1-3-2", 85, 60 + 5 + 137, 2);
      }
      else if (PaOrder == 2)
      {
        sprite.drawCentreString("2-3-1", 85, 60 + 5 + 137, 2);
      }
      else if (PaOrder == 3)
      {
        sprite.drawCentreString("2-1-3", 85, 60 + 5 + 137, 2);
      }
      else if (PaOrder == 4)
      {
        sprite.drawCentreString("3-2-1", 85, 60 + 5 + 137, 2);
      }
      sprite.drawCentreString("BROADCAST ONCE", 85, 60 + 5 + 160, 2);
      if (!RunBRepeat)
      {
        sprite.drawCentreString("BROADCAST REPEAT", 85, 60 + 5 + 185, 2);
      }
      else
      {
        sprite.drawCentreString("BROADCAST STOP", 85, 60 + 5 + 185, 2);
      }
      if (OutputScr)
      {
        sprite.drawLine(10, 20, 160, 20, TFT_BLACK);
        sprite.drawLine(10, 20, 10, 60, TFT_BLACK);
        sprite.drawLine(160, 20, 160, 60, TFT_BLACK);
        sprite.drawLine(10, 60, 160, 60, TFT_BLACK);
        sprite.drawCentreString("OUTPUT-1", 85, 5, 2);
        String sub;
        if (PaOrder == 0) // 123
        {
          if (!ScrollOne)
          {
            SclOne();
          }
          else if (!ScrollTwo)
          {
            SclTwo();
          }
          else if (!ScrollThree)
          {
            SclThree();
          }
          else
          {
            if (RunBRepeat)
            {
              ScrollOne = false;
              ScrollTwo = false;
              ScrollThree = false;
            }
            else
            {
              ScrollOne = false;
              ScrollTwo = false;
              ScrollThree = false;
              OutputScr = false;
            }
          }
        }
        else if (PaOrder == 1) // 132
        {
          if (!ScrollOne)
          {
            SclOne();
          }
          else if (!ScrollThree)
          {
            SclThree();
          }
          else if (!ScrollTwo)
          {
            SclTwo();
          }
          else
          {
            if (RunBRepeat)
            {
              ScrollOne = false;
              ScrollTwo = false;
              ScrollThree = false;
            }
            else
            {
              ScrollOne = false;
              ScrollTwo = false;
              ScrollThree = false;
              OutputScr = false;
            }
          }
        }
        else if (PaOrder == 2) // 231
        {
          if (!ScrollTwo)
          {
            SclTwo();
          }
          else if (!ScrollThree)
          {
            SclThree();
          }
          else if (!ScrollOne)
          {
            SclOne();
          }
          else
          {
            if (RunBRepeat)
            {
              ScrollOne = false;
              ScrollTwo = false;
              ScrollThree = false;
            }
            else
            {
              ScrollOne = false;
              ScrollTwo = false;
              ScrollThree = false;
              OutputScr = false;
            }
          }
        }
        else if (PaOrder == 3) // 213
        {

          if (!ScrollTwo)
          {
            SclTwo();
          }
          else if (!ScrollOne)
          {
            SclOne();
          }

          else if (!ScrollThree)
          {
            SclThree();
          }
          else
          {
            if (RunBRepeat)
            {
              ScrollOne = false;
              ScrollTwo = false;
              ScrollThree = false;
            }
            else
            {
              ScrollOne = false;
              ScrollTwo = false;
              ScrollThree = false;
              OutputScr = false;
            }
          }
        }
        else if (PaOrder == 4) // 321
        {
          if (!ScrollThree)
          {
            SclThree();
          }
          else if (!ScrollTwo)
          {
            SclTwo();
          }
          else if (!ScrollOne)
          {
            SclOne();
          }
          else
          {
            if (RunBRepeat)
            {
              ScrollOne = false;
              ScrollTwo = false;
              ScrollThree = false;
            }
            else
            {
              ScrollOne = false;
              ScrollTwo = false;
              ScrollThree = false;
              OutputScr = false;
            }
          }
        }
        sprite.drawLine(10, 20 + 60, 160, 20 + 60, TFT_BLACK);
        sprite.drawLine(10, 20 + 60, 10, 60 + 60, TFT_BLACK);
        sprite.drawLine(160, 20 + 60, 160, 60 + 60, TFT_BLACK);
        sprite.drawLine(10, 60 + 60, 160, 60 + 60, TFT_BLACK);
        sprite.drawCentreString("OUTPUT-2", 85, 5 + 60, 2);
        sprite.drawLine(10, 20 + 60 + 60, 160, 20 + 60 + 60, TFT_BLACK);
        sprite.drawLine(10, 20 + 60 + 60, 10, 60 + 60 + 60, TFT_BLACK);
        sprite.drawLine(160, 20 + 60 + 60, 160, 60 + 60 + 60, TFT_BLACK);
        sprite.drawLine(10, 60 + 60 + 60, 160, 60 + 60 + 60, TFT_BLACK);
        sprite.drawCentreString("OUTPUT-3", 85, 5 + 60 + 60, 2);
      }
      else
      {
        DisplayPosi = 0;
        sprite.drawCentreString("PA-INPUT", 85, 5, 2);
        sprite.fillRoundRect(5, 65 - 40, 160, 50, 5, TFT_DARKGREY);  // top bg
        sprite.fillRoundRect(5, 120 - 40, 160, 50, 5, TFT_DARKGREY); // mid bg
        sprite.fillRoundRect(5, 175 - 40, 160, 50, 5, TFT_DARKGREY); // bt bg
        if (!TopNewHd)
        {
          if ((XCoord <= 85) && (XCoord >= 5) && (YCoord <= 270) && (YCoord >= 230))
          {
            if (OneMsg.length() == 0)
              sprite.fillRoundRect(15, 70 - 20, 70, 15, 3, TFT_DARKGREEN); // top new bt
            else
              sprite.fillRoundRect(15, 70 - 20, 70, 15, 3, TFT_NAVY); // top new bt
            XCoordLS = XCoord;
            TopNewHd = true;
          }
          else
          {
            if (OneMsg.length() == 0)
              sprite.fillRoundRect(15, 70 - 20, 70, 15, 3, TFT_GREEN); // top new bt
            else
              sprite.fillRoundRect(15, 70 - 20, 70, 15, 3, TFT_BLUE); // top new bt
          }
        }
        else
        {
          if ((XCoord == 0) && (YCoord == 0))
          {
            OneEd = true;
            KeyboardStr = OneMsg;
            if (OneMsg.length() == 0)
              sprite.fillRoundRect(15, 70 - 20, 70, 15, 3, TFT_GREEN); // top new bt
            else
              sprite.fillRoundRect(15, 70 - 20, 70, 15, 3, TFT_BLUE); // top new bt
            TopNewHd = false;
          }
          else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
          {
            if (OneMsg.length() == 0)
              sprite.fillRoundRect(15, 70 - 20, 70, 15, 3, TFT_DARKGREEN); // top new bt
            else
              sprite.fillRoundRect(15, 70 - 20, 70, 15, 3, TFT_NAVY); // top new bt
          }
          else
          {
            if (OneMsg.length() == 0)
              sprite.fillRoundRect(15, 70 - 20, 70, 15, 3, TFT_GREEN); // top new bt
            else
              sprite.fillRoundRect(15, 70 - 20, 70, 15, 3, TFT_BLUE); // top new bt
            TopNewHd = false;
          }
        }
        if (OneMsg.length() == 0)
        {
          sprite.drawCentreString("NEW", 50, 70 - 20, 2);
        }
        else
        {
          sprite.drawCentreString("EDIT", 50, 70 - 20, 2);
        }
        if (!MidNewHd)
        {
          if ((XCoord <= 85) && (XCoord >= 5) && (YCoord <= 220) && (YCoord >= 170))
          {
            if (TwoMsg.length() == 0)
              sprite.fillRoundRect(15, 125 - 20, 70, 15, 3, TFT_DARKGREEN); // mid new bt
            else
              sprite.fillRoundRect(15, 125 - 20, 70, 15, 3, TFT_NAVY); // mid new bt
            XCoordLS = XCoord;
            MidNewHd = true;
          }
          else
          {
            if (TwoMsg.length() == 0)
              sprite.fillRoundRect(15, 125 - 20, 70, 15, 3, TFT_GREEN); // mid new bt
            else
              sprite.fillRoundRect(15, 125 - 20, 70, 15, 3, TFT_BLUE); // mid new bt
          }
        }
        else
        {
          if ((XCoord == 0) && (YCoord == 0))
          {
            TwoEd = true;
            KeyboardStr = TwoMsg;
            if (TwoMsg.length() == 0)
              sprite.fillRoundRect(15, 125 - 20, 70, 15, 3, TFT_GREEN); // mid new bt
            else
              sprite.fillRoundRect(15, 125 - 20, 70, 15, 3, TFT_BLUE); // mid new bt
            MidNewHd = false;
          }
          else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
          {
            if (TwoMsg.length() == 0)
              sprite.fillRoundRect(15, 125 - 20, 70, 15, 3, TFT_DARKGREEN); // mid new bt
            else
              sprite.fillRoundRect(15, 125 - 20, 70, 15, 3, TFT_NAVY); // mid new bt
          }
          else
          {
            if (TwoMsg.length() == 0)
              sprite.fillRoundRect(15, 70 - 20, 70, 15, 3, TFT_GREEN); // top new bt
            else
              sprite.fillRoundRect(15, 70 - 20, 70, 15, 3, TFT_BLUE); // top new bt
            MidNewHd = false;
          }
        }
        if (TwoMsg.length() == 0)
        {
          sprite.drawCentreString("NEW", 50, 125 - 20, 2);
        }
        else
        {
          sprite.drawCentreString("EDIT", 50, 125 - 20, 2);
        }
        if (!BtNewHd)
        {
          if ((XCoord <= 85) && (XCoord >= 5) && (YCoord <= 160) && (YCoord >= 126))
          {
            if (ThreeMsg.length() == 0)
              sprite.fillRoundRect(15, 180 - 20, 70, 15, 3, TFT_DARKGREEN); // bt new bt
            else
              sprite.fillRoundRect(15, 180 - 20, 70, 15, 3, TFT_NAVY); // bt new bt
            XCoordLS = XCoord;
            BtNewHd = true;
          }
          else
          {
            if (ThreeMsg.length() == 0)
              sprite.fillRoundRect(15, 180 - 20, 70, 15, 3, TFT_GREEN); // bt new bt
            else
              sprite.fillRoundRect(15, 180 - 20, 70, 15, 3, TFT_BLUE); // bt new bt
          }
        }
        else
        {
          if ((XCoord == 0) && (YCoord == 0))
          {
            ThreeEd = true;
            KeyboardStr = ThreeMsg;
            if (ThreeMsg.length() == 0)
              sprite.fillRoundRect(15, 180 - 20, 70, 15, 3, TFT_GREEN); // bt new bt
            else
              sprite.fillRoundRect(15, 180 - 20, 70, 15, 3, TFT_BLUE); // bt new bt

            BtNewHd = false;
          }
          else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
          {
            if (ThreeMsg.length() == 0)
              sprite.fillRoundRect(15, 180 - 20, 70, 15, 3, TFT_DARKGREEN); // bt new bt
            else
              sprite.fillRoundRect(15, 180 - 20, 70, 15, 3, TFT_NAVY); // bt new bt
          }
          else
          {
            ThreeEd = true;
            KeyboardStr = ThreeMsg;
            if (ThreeMsg.length() == 0)
              sprite.fillRoundRect(15, 180 - 20, 70, 15, 3, TFT_GREEN); // bt new bt
            else
              sprite.fillRoundRect(15, 180 - 20, 70, 15, 3, TFT_BLUE); // bt new bt
            BtNewHd = false;
          }
        }
        if (ThreeMsg.length() == 0)
        {
          sprite.drawCentreString("NEW", 50, 180 - 20, 2);
        }
        else
        {
          sprite.drawCentreString("EDIT", 50, 180 - 20, 2);
        }
        //--------------------------------------------------------------------------------
        if (TopDelCf)
        {
          if (!TopDelHd)
          {
            if ((XCoord <= 170) && (XCoord >= 100) && (YCoord <= 270) && (YCoord >= 230))
            {
              sprite.fillRoundRect(90, 70 - 20, 70, 15, 3, TFT_MAGENTA); // top del bt
              XCoordLS = XCoord;
              TopDelHd = true;
            }
            else
            {
              sprite.fillRoundRect(90, 70 - 20, 70, 15, 3, TFT_RED); // top del bt
            }
          }
          else
          {
            if ((XCoord == 0) && (YCoord == 0))
            {

              sprite.fillRoundRect(90, 70 - 20, 70, 15, 3, TFT_RED); // top del bt
              OneMsg = "";
              TopDelHd = false;
              TopDelCf = false;
            }
            else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
            {
              sprite.fillRoundRect(90, 70 - 20, 70, 15, 3, TFT_MAGENTA); // top del bt
            }
            else
            {
              sprite.fillRoundRect(90, 70 - 20, 70, 15, 3, TFT_RED); // top del bt
              TopDelHd = false;
            }
          }
          sprite.drawCentreString("CONFIRM", 90 + 35, 70 - 20, 2);
          if (millis() - Onedelay >= 4000)
          {
            TopDelCf = false;
          }
        }
        else
        {
          if (!TopDelHd)
          {
            if ((XCoord <= 170) && (XCoord >= 100) && (YCoord <= 270) && (YCoord >= 230))
            {
              sprite.fillRoundRect(90, 70 - 20, 70, 15, 3, TFT_RED); // top del bt
              XCoordLS = XCoord;
              TopDelHd = true;
            }
            else
            {
              sprite.fillRoundRect(90, 70 - 20, 70, 15, 3, TFT_ORANGE); // top del bt
            }
          }
          else
          {
            if ((XCoord == 0) && (YCoord == 0))
            {
              if (OneMsg.length() > 0)
              {
                TopDelCf = true;
              }
              Onedelay = millis();
              sprite.fillRoundRect(90, 70 - 20, 70, 15, 3, TFT_ORANGE); // top del bt
              TopDelHd = false;
            }
            else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
            {
              sprite.fillRoundRect(90, 70 - 20, 70, 15, 3, TFT_RED); // top del bt
            }
            else
            {
              sprite.fillRoundRect(90, 70 - 20, 70, 15, 3, TFT_ORANGE); // top del bt
              TopDelHd = false;
            }
          }
          sprite.drawCentreString("DELETE", 90 + 35, 70 - 20, 2);
        }
        if (MidDelCf)
        {
          if (!MidDelHd)
          {
            if ((XCoord <= 170) && (XCoord >= 100) && (YCoord <= 220) && (YCoord >= 170))
            {
              sprite.fillRoundRect(90, 125 - 20, 70, 15, 3, TFT_MAGENTA); // mid del bt
              XCoordLS = XCoord;
              MidDelHd = true;
            }
            else
            {
              sprite.fillRoundRect(90, 125 - 20, 70, 15, 3, TFT_RED); // mid del bt
            }
          }
          else
          {
            if ((XCoord == 0) && (YCoord == 0))
            {
              sprite.fillRoundRect(90, 125 - 20, 70, 15, 3, TFT_RED); // mid del bt
              TwoMsg = "";
              MidDelHd = false;
              MidDelCf = false;
            }
            else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
            {
              sprite.fillRoundRect(90, 125 - 20, 70, 15, 3, TFT_MAGENTA); // mid del bt
            }
            else
            {
              sprite.fillRoundRect(90, 125 - 20, 70, 15, 3, TFT_RED); // mid del bt
              MidDelHd = false;
            }
          }
          sprite.drawCentreString("CONFIRM", 90 + 35, 125 - 20, 2);
          if (millis() - Twodelay >= 4000)
          {
            MidDelCf = false;
          }
        }
        else
        {
          if (!MidDelHd)
          {
            if ((XCoord <= 170) && (XCoord >= 100) && (YCoord <= 220) && (YCoord >= 170))
            {
              sprite.fillRoundRect(90, 125 - 20, 70, 15, 3, TFT_MAGENTA); // mid del bt
              XCoordLS = XCoord;
              MidDelHd = true;
            }
            else
            {
              sprite.fillRoundRect(90, 125 - 20, 70, 15, 3, TFT_ORANGE); // mid del bt
            }
          }
          else
          {
            if ((XCoord == 0) && (YCoord == 0))
            {
              if (TwoMsg.length() > 0)
              {
                Twodelay = millis();
              }
              MidDelCf = true;
              sprite.fillRoundRect(90, 125 - 20, 70, 15, 3, TFT_ORANGE); // mid del bt
              MidDelHd = false;
            }
            else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
            {
              sprite.fillRoundRect(90, 125 - 20, 70, 15, 3, TFT_MAGENTA); // mid del bt
            }
            else
            {
              sprite.fillRoundRect(90, 125 - 20, 70, 15, 3, TFT_ORANGE); // mid del bt
              MidDelHd = false;
            }
          }
          sprite.drawCentreString("DELETE", 90 + 35, 125 - 20, 2);
        }
        if (BtDelCf)
        {
          if (!BtDelHd)
          {
            if ((XCoord <= 170) && (XCoord >= 100) && (YCoord <= 160) && (YCoord >= 126))
            {
              sprite.fillRoundRect(90, 180 - 20, 70, 15, 3, TFT_RED); // bt del bt
              XCoordLS = XCoord;
              BtDelHd = true;
            }
            else
            {
              sprite.fillRoundRect(90, 180 - 20, 70, 15, 3, TFT_RED); // bt del bt
            }
          }
          else
          {
            if ((XCoord == 0) && (YCoord == 0))
            {
              sprite.fillRoundRect(90, 180 - 20, 70, 15, 3, TFT_RED); // bt del bt
              ThreeMsg = "";
              BtDelHd = false;
              BtDelCf = false;
            }
            else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
            {
              sprite.fillRoundRect(90, 180 - 20, 70, 15, 3, TFT_RED); // bt del bt
            }
            else
            {
              sprite.fillRoundRect(90, 180 - 20, 70, 15, 3, TFT_RED); // bt del bt
              BtDelHd = false;
            }
          }
          sprite.drawCentreString("CONFIRM", 90 + 35, 160, 2);
          if (millis() - Threedelay >= 4000)
          {
            BtDelCf = false;
          }
        }
        else
        {
          if (!BtDelHd)
          {
            if ((XCoord <= 170) && (XCoord >= 100) && (YCoord <= 160) && (YCoord >= 126))
            {
              sprite.fillRoundRect(90, 180 - 20, 70, 15, 3, TFT_RED); // bt del bt
              XCoordLS = XCoord;
              BtDelHd = true;
            }
            else
            {
              sprite.fillRoundRect(90, 180 - 20, 70, 15, 3, TFT_ORANGE); // bt del bt
            }
          }
          else
          {
            if ((XCoord == 0) && (YCoord == 0))
            {
              if (ThreeMsg.length() > 0)
              {
                BtDelCf = true;
              }
              Threedelay = millis();
              sprite.fillRoundRect(90, 180 - 20, 70, 15, 3, TFT_ORANGE); // bt del bt
              BtDelHd = false;
            }
            else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
            {
              sprite.fillRoundRect(90, 180 - 20, 70, 15, 3, TFT_RED); // bt del bt
            }
            else
            {
              sprite.fillRoundRect(90, 180 - 20, 70, 15, 3, TFT_ORANGE); // bt del bt
              BtDelHd = false;
            }
          }
          sprite.drawCentreString("DELETE", 90 + 35, 160, 2);
        }
      }
    }
  }
}
void LeftRightArrow()
{
  // For Left Arrow
  if (!SCRHoldL)
  {
    if ((XCoord <= 50) && (XCoord >= 1) && (YCoord <= 60) && (YCoord >= 0))
    {
      // sprite.fillTriangle(15 - 9, 85 + 210, 35 - 9, 65 + 210, 35 - 9, 105 + 210, TFT_LIGHTGREY);
      sprite.fillTriangle(20 - 9, 85 + 210, 31 - 9, 74 + 210, 31 - 9, 96 + 210, TFT_BLACK);
      XCoordLS = XCoord;
      SCRHoldL = true;
    }
    else
    {
      // sprite.fillTriangle(15 - 9, 85 + 210, 35 - 9, 65 + 210, 35 - 9, 105 + 210, TFT_LIGHTGREY);
      sprite.fillTriangle(20 - 9, 85 + 210, 31 - 9, 74 + 210, 31 - 9, 96 + 210, TFT_DARKGREY);
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      // Serial.println("Finger lifted");
      // sprite.fillTriangle(15 - 9, 85 + 210, 35 - 9, 65 + 210, 35 - 9, 105 + 210, TFT_LIGHTGREY);
      sprite.fillTriangle(20 - 9, 85 + 210, 31 - 9, 74 + 210, 31 - 9, 96 + 210, TFT_BLACK);
      if (ScreenNum > 1) // Loop 1-4 temp untill touch buttons
      {
        ScreenNum--;
      }
      else
      {
        ScreenNum = 3;
      }
      // Serial.println("ScreenNum = " + String(ScreenNum));
      SCRHoldL = false;
    }
    else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
    {
      // Serial.println("Finger in range");
      // sprite.fillTriangle(15 - 9, 85 + 210, 35 - 9, 65 + 210, 35 - 9, 105 + 210, TFT_LIGHTGREY);
      sprite.fillTriangle(20 - 9, 85 + 210, 31 - 9, 74 + 210, 31 - 9, 96 + 210, TFT_BLACK);
    }
    else
    {
      if (debug)
        Serial.println("Finger not in range");
      // sprite.fillTriangle(15 - 9, 85 + 210, 35 - 9, 65 + 210, 35 - 9, 105 + 210, TFT_LIGHTGREY);
      sprite.fillTriangle(20 - 9, 85 + 210, 31 - 9, 74 + 210, 31 - 9, 96 + 210, TFT_WHITE);
      SCRHoldL = false;
    }
  }
  // For Right Arrow
  if (!SCRHoldR)
  {
    if ((XCoord <= 170) && (XCoord >= 120) && (YCoord <= 50) && (YCoord >= 0))
    {
      XCoordRS = XCoord;
      // sprite.fillTriangle(305 - 140, 85 + 210, 285 - 140, 65 + 210, 285 - 140, 105 + 210, TFT_LIGHTGREY);
      sprite.fillTriangle(300 - 140, 85 + 210, 289 - 140, 74 + 210, 289 - 140, 96 + 210, TFT_BLACK);
      SCRHoldR = true;
    }
    else
    {
      // sprite.fillTriangle(305 - 140, 85 + 210, 285 - 140, 65 + 210, 285 - 140, 105 + 210, TFT_LIGHTGREY);
      sprite.fillTriangle(300 - 140, 85 + 210, 289 - 140, 74 + 210, 289 - 140, 96 + 210, TFT_DARKGREY);
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      if (debug)
        Serial.println("Finger lifted");
      // sprite.fillTriangle(305 - 140, 85 + 210, 285 - 140, 65 + 210, 285 - 140, 105 + 210, TFT_LIGHTGREY);
      sprite.fillTriangle(300 - 140, 85 + 210, 289 - 140, 74 + 210, 289 - 140, 96 + 210, TFT_BLACK);
      if (ScreenNum <= 3) // Loop 1-4 temp untill touch buttons
      {
        ScreenNum++;
      }
      else
      {
        ScreenNum = 1;
      }
      if (debug)
        Serial.println("ScreenNum = " + String(ScreenNum));
      SCRHoldR = false;
    }
    else if ((XCoord <= XCoordRS + 5) && (XCoord >= XCoordRS - 5)) // Don't need Y X alone works well
    {
      // sprite.fillTriangle(305 - 140, 85 + 210, 285 - 140, 65 + 210, 285 - 140, 105 + 210, TFT_LIGHTGREY);
      sprite.fillTriangle(300 - 140, 85 + 210, 289 - 140, 74 + 210, 289 + -140, 96 + 210, TFT_BLACK);
      if (debug)
        Serial.println("Finger in range");
    }
    else
    {
      // sprite.fillTriangle(305 - 140, 85 + 210, 285 - 140, 65 + 210, 285 - 140, 105 + 210, TFT_LIGHTGREY);
      sprite.fillTriangle(300 - 140, 85 + 210, 289 - 140, 74 + 210, 289 - 140, 96 + 210, TFT_DARKGREY);

      SCRHoldR = false;
    }
  }
}
void CancelButton()
{
  if (!CCHold)
  {
    if ((XCoord <= 120) && (XCoord >= 55) && (YCoord <= 25) && (YCoord >= 0)) // clicked
    {
      sprite.fillRoundRect(28, 283, 115, 25, 5, TFT_RED);
      sprite.setTextColor(TFT_BLACK);
      sprite.drawString("STOP_TEST", 53, 287, 2);
      XCoordLS = XCoord;
      CCHold = true;
    }
    else
    {
      sprite.fillRoundRect(28, 283, 115, 25, 5, TFT_RED);
      sprite.setTextColor(TFT_BLACK);
      sprite.drawString("STOP_TEST", 53, 287, 2);
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      sprite.fillRoundRect(28, 283, 115, 25, 5, TFT_RED);
      sprite.setTextColor(TFT_BLACK);
      sprite.drawString("STOPPING", 55, 287, 2);
      if (debug)
        Serial.println("Finger lifted");
      SWFinal = false;
      SWClicked = false;
      CCHold = false;
    }
    else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
    {
      sprite.fillRoundRect(28, 283, 115, 25, 5, TFT_RED);
      sprite.setTextColor(TFT_BLACK);
      sprite.drawString("STOP_TEST", 53, 287, 2);
      if (debug)
        Serial.println("Finger in range");
    }
    else
    {
      sprite.fillRoundRect(28, 283, 115, 25, 5, TFT_RED);
      sprite.setTextColor(TFT_BLACK);
      sprite.drawString("STOP_TEST", 53, 287, 2);
      SWHold = false;
    }
  }
}
void PlusMinusT()
{
  // For Plus
  if (!PlusHold)
  {
    if ((XCoord <= 50) && (XCoord >= 1) && (YCoord <= 260) && (YCoord >= 180))
    {
      XCoordLS = XCoord;
      PlusHold = true;
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      if (SimulateRoom <= 9)
      {
        SimulateRoom++;
      }
      else
      {
        SimulateRoom = 1;
      }
      if (debug)
        Serial.println("Finger lifted");

      PlusHold = false;
    }
    else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
    {
      // Nothing
    }
    else
    {
      PlusHold = false;
    }
  }
  // For Minus
  if (!MinusHold)
  {
    if ((XCoord <= 170) && (XCoord >= 120) && (YCoord <= 260) && (YCoord >= 180))
    {
      XCoordLS = XCoord;
      MinusHold = true;
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      if (SimulateRoom >= 2)
      {
        SimulateRoom--;
      }
      else
      {
        SimulateRoom = 10;
      }
      if (debug)
        Serial.println("Finger lifted");
      MinusHold = false;
    }
    else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
    {
      // Nothing
    }
    else
    {
      MinusHold = false;
    }
  }
}
void Relay()
{ // Filter massages to relay forward
  bool IDMatch;
  IDMatch = s(RE_ID); // chexk in the database for sent ids
  if (IDMatch == true)
  {
    // Do nothing bc dupicate message
  }
  else
  {
    IDcount = s.count(); // Numbers of ID saved in "s" Dictionary
    if (IDcount >= 99)
    {
      s.remove(s(0)); // remove oldest input data to prevent overflow
      s(RE_ID, "0");  // MSGID for reference with empty value(value not used)
    }
    else
    {
      s(RE_ID, "0");
    }
    // mesh.sendBroadcast(Relay_Message);
    // Serial.print("Relay_Message = ");
    // Serial.println(Relay_Message);
    Database();
  }
}
void SixtySec()
{
  N.remove(N(0));
  d.remove(d(0));           // delete entry in onfire dict
  ErrorD.remove(ErrorD(0)); // delete entry in Error dict
  if (debug)
    Serial.println("Clearing Dictionary");
}
void ClearEmpty() // Clear dict when no devices connected
{
  if (nodes.size() <= 0)
  {
    if (d.count() > 0)
    {
      while (d.count())
        d.remove(d(0));
    }
    if (ErrorD.count() > 0)
    {
      while (ErrorD.count())
        ErrorD.remove(ErrorD(0));
    }
  }
}
void Database()
{
  bool DataMatch;
  DataMatch = d(Recieved_ID); // check in the database for rooms on fire
  if (DataMatch == true)
  { // Found match in database
    if (debug)
      Serial.println("DataMatch true");
    if (Recieved_status == "1")
    {
      if (debug)
        Serial.println(String(Recieved_ID) + " Status is still on fire");
    }
    else if (Recieved_status == "0")
    {
      if (debug)
        Serial.println("Fire stopped in room " + String(Recieved_ID));
      d.remove(Recieved_ID);
    }
  }
  else
  {
    if (Recieved_status == "1")
    {
      if (debug)
        Serial.println("New fire reported in room " + String(Recieved_ID));
      d(Recieved_ID, "1");
    }
    else if (Recieved_status == "0")
    {
      if (debug)
        Serial.println("Room" + String(Recieved_ID) + " Status Normal : Not saving");
    }
  }
  OverrideBuzzer();
  //-----------------------------------------------------------------------------
  DataMatch = ErrorD(Recieved_ID); // check in the database for room Error
  if (DataMatch == true)
  {                          // Found match in database
    if (Error_Status == "1") // ERRORED
    {
    }
    else if (Error_Status == "0")
    {
      ErrorD.remove(Recieved_ID);
    }
  }
  else
  {
    if (Error_Status == "1")
    {
      ErrorD(Recieved_ID, "1");
    }
    else if (Error_Status == "0")
    {
      // NOthing
    }
  }
  ErrorNum = ErrorD.count();
  //-----------------------------------------------------------------------------
  DataMatch = N(Recieved_ID); // Save all rooms not on fire
  if (DataMatch == true)
  { // Found match in database
    if (Recieved_status == "1")
    {
      N.remove(Recieved_ID);
    }
    else if (Recieved_status == "0")
    {
      N(Recieved_ID, "0"); // refresh data
    }
  }
  else
  {
    if (Recieved_status == "1")
    {
      // do nothing
    }
    else if (Recieved_status == "0")
    {
      N(Recieved_ID, "0"); // add data
    }
  }
}

void newConnectionCallback(uint32_t nodeId)
{
  if (debug)
    Serial.printf("-- > startHere: New Connection, nodeId = % u\n", nodeId);
}
void changedConnectionCallback()
{
  if (debug)
    Serial.printf("Changed connections\n");
}
void nodeTimeAdjustedCallback(int32_t offset)
{
  if (debug)
    Serial.printf("Adjusted time % u. Offset = % d\n", mesh.getNodeTime(), offset);
}
String msg;
int lastsend;
void sendMessage()
{
  String msg;
  if (SWFinal) // Broadcast rooms on fire
  {
    OFF_SimulateTick = 0;
    if (SimulateTick <= SimulateRoom)
    {
      if (SimulateTick == 1)
      {
        doc["Room"] = "111";
        doc["status"] = "1";
        if (debug)
          Serial.println("111 : 1 send");
      }
      else if (SimulateTick == 2)
      {
        doc["Room"] = "Kitchen";
        doc["status"] = "1";
        if (debug)
          Serial.println("Kitchen : 1 send");
      }
      else if (SimulateTick == 3)
      {
        doc["Room"] = "Bedroom";
        doc["status"] = "1";
        if (debug)
          Serial.println("Bedroom : 1 send");
      }
      else if (SimulateTick == 4)
      {
        doc["Room"] = "114";
        doc["status"] = "1";
        if (debug)
          Serial.println("114 : 1 send");
      }
      else if (SimulateTick == 5)
      {
        doc["Room"] = "115";
        doc["status"] = "1";
        if (debug)
          Serial.println("115 : 1 send");
      }
      else if (SimulateTick == 6)
      {
        doc["Room"] = "116";
        doc["status"] = "1";
        if (debug)
          Serial.println("116 : 1 send");
      }
      else if (SimulateTick == 7)
      {
        doc["Room"] = "117";
        doc["status"] = "1";
        if (debug)
          Serial.println("117 : 1 send");
      }
      else if (SimulateTick == 8)
      {
        doc["Room"] = "118";
        doc["status"] = "1";
        if (debug)
          Serial.println("118 : 1 send");
      }
      else if (SimulateTick == 9)
      {
        doc["Room"] = "119";
        doc["status"] = "1";
        if (debug)
          Serial.println("119 : 1 send");
      }
      else if (SimulateTick == 10)
      {
        doc["Room"] = "120";
        doc["status"] = "1";
        if (debug)
          Serial.println("120 : 1 send");
      }
      if (debug)
        Serial.println("SimulateTick =" + String(SimulateTick));
      SimulateTick++;
    }
    else
    {
      SimulateTick = 1;
    }
  }
  else if (RunBOnce) // Run Broadcast
  {
    doc["Room"] = "BA";
    doc["status"] = "3";
    if (PaOrder == 0) // 123
    {
      if (SendTick == 1)
      {
        doc["PAmsg"] = OneMsg;
      }
      else if (SendTick == 2)
      {
        doc["PAmsg"] = TwoMsg;
      }
      else if (SendTick == 3)
      {
        doc["PAmsg"] = ThreeMsg;
      }
    }
    else if (PaOrder == 1) // 132
    {
      if (SendTick == 1)
      {
        doc["PAmsg"] = OneMsg;
      }
      else if (SendTick == 2)
      {
        doc["PAmsg"] = ThreeMsg;
      }
      else if (SendTick == 3)
      {
        doc["PAmsg"] = TwoMsg;
      }
    }
    else if (PaOrder == 2) // 231
    {
      if (SendTick == 1)
      {
        doc["PAmsg"] = TwoMsg;
      }
      else if (SendTick == 2)
      {
        doc["PAmsg"] = ThreeMsg;
      }
      else if (SendTick == 3)
      {
        doc["PAmsg"] = OneMsg;
      }
    }
    else if (PaOrder == 3) // 213
    {
      if (SendTick == 1)
      {
        doc["PAmsg"] = TwoMsg;
      }
      else if (SendTick == 2)
      {
        doc["PAmsg"] = OneMsg;
      }
      else if (SendTick == 3)
      {
        doc["PAmsg"] = ThreeMsg;
      }
    }
    else if (PaOrder == 4) // 321
    {
      if (SendTick == 1)
      {
        doc["PAmsg"] = ThreeMsg;
      }
      else if (SendTick == 2)
      {
        doc["PAmsg"] = TwoMsg;
      }
      else if (SendTick == 3)
      {
        doc["PAmsg"] = OneMsg;
      }
    }
  }
  else // If SW_Final hasn't been on
  {
    SimulateTick = 0;
    if (OFF_SimulateTick <= SimulateRoom)
    {
      if (OFF_SimulateTick == 1)
      {
        doc["Room"] = "111";
        doc["status"] = "0";
        if (debug)
          Serial.println("111 : 0 send");
      }
      else if (OFF_SimulateTick == 2)
      {
        doc["Room"] = "Kitchen";
        doc["status"] = "0";
        if (debug)
          Serial.println("Kitchen : 0 send");
      }
      else if (OFF_SimulateTick == 3)
      {
        doc["Room"] = "Bedroom";
        doc["status"] = "0";
        if (debug)
          Serial.println("Bedroom : 0 send");
      }
      else if (OFF_SimulateTick == 4)
      {
        doc["Room"] = "114";
        doc["status"] = "0";
        if (debug)
          Serial.println("114 : 0 send");
      }
      else if (OFF_SimulateTick == 5)
      {
        doc["Room"] = "115";
        doc["status"] = "0";
        if (debug)
          Serial.println("115 : 0 send");
      }
      else if (OFF_SimulateTick == 6)
      {
        doc["Room"] = "116";
        doc["status"] = "0";
        if (debug)
          Serial.println("116 : 0 send");
      }
      else if (OFF_SimulateTick == 7)
      {
        doc["Room"] = "117";
        doc["status"] = "0";
        if (debug)
          Serial.println("117 : 0 send");
      }
      else if (OFF_SimulateTick == 8)
      {
        doc["Room"] = "118";
        doc["status"] = "0";
        if (debug)
          Serial.println("118 : 0 send");
      }
      else if (OFF_SimulateTick == 9)
      {
        doc["Room"] = "119";
        doc["status"] = "0";
        if (debug)
          Serial.println("119 : 0 send");
      }
      else if (OFF_SimulateTick == 10)
      {
        doc["Room"] = "120";
        doc["status"] = "0";
        if (debug)
          Serial.println("120 : 0 send");
      }
      if (debug)
        Serial.println("OFF_SimulateTick =" + String(OFF_SimulateTick));
      OFF_SimulateTick++;
    }
    else
    {
      OFF_SimulateTick = 1;
    }
  }
  msg = ""; // reset msg string
  String RoomStr = String(Room);
  int Roomint = RoomStr.toInt();
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
  doc["MSG_ID"] = MSG_ID; // Save sent msgid to dictionary
  if (IDcount >= 99)
  {
    s.remove(s(0)); // remove oldest input data to prevent overflow
    s(MSG_ID, "0"); // MSGID for reference with empty value(value not used)
  }
  else
  {
    s(MSG_ID, "0");
  }
  serializeJson(doc, msg);
  if (debug)
    Serial.println(msg);
  mesh.sendBroadcast(msg);
  if ((RunBOnce) || (RunBRepeat))
  {
    if (SendTick < 3)
      SendTick++;
    else
    {
      SendTick = 1;
      RunBOnce = false;
    }
  }
  else
  {
    SendTick = 1;
  }
  if (RunBRepeat){
    RunBOnce = true;
  }
  doc["PAmsg"] = ""; // reset before mew msg
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

  void ESPReset()
{
  nodes = mesh.getNodeList();
  if (debug)
    Serial.printf("Num nodes: %d\n ", nodes.size());
  if (debug)
    Serial.println("Reset Timer = " + String(ResetTimer));
  if (nodes.size() <= 0)
  { // no connected devices
    if (ResetTimer >= 60)
    {
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
void setup()
{
  OneMsg = "";
  TwoMsg = "";
  ThreeMsg = "";
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  gpio_hold_dis((gpio_num_t)PIN_TOUCH_RES);
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  pinMode(PIN_TOUCH_RES, OUTPUT);
  digitalWrite(PIN_TOUCH_RES, LOW);
  delay(500);
  digitalWrite(PIN_TOUCH_RES, HIGH);
  pinMode(buzpin, OUTPUT);
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(4);
  tft.setSwapBytes(true);
  ledcSetup(0, 2000, 8);
  ledcAttachPin(PIN_LCD_BL, 0);
  ledcWrite(0, 100);
  KBsprite.createSprite(320, 170);
  KBsprite.setSwapBytes(true);
  KBsprite.setTextColor(TFT_BLACK);
  KBsprite.setTextSize(2);
  KBsprite.setSwapBytes(true);
  KBsprite.setTextDatum(4);
  sprite.createSprite(170, 320);
  sprite.setSwapBytes(true);
  delay(1000);
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  // touch funtion uses pin 18,17
  Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
  if (!touch.init())
  {
    if (debug)
      Serial.println("Touch IC not found");
  }
  button.attachClick([]
                     { flags_sleep = 1; });
  button2.attachClick([]
                      { if (ScreenNum <= 2) //Loop 1-3 temp untill touch buttons
                     {
                      ScreenNum++;
                     }
                     else{ScreenNum = 1;}
                     if (debug)Serial.println("B2 Clicked"); });
  spr2.createSprite(170, 320);
  spr2.fillSprite(TFT_GREEN);
  spr2.setTextDatum(4);
  spr2.setFreeFont(&Orbitron_Light_24);
  xTaskCreatePinnedToCore(Loop2Code, "Loop2", 8192, NULL, 1, &Loop2, 1);
  userScheduler.addTask(taskSendMessage);
  userScheduler.addTask(taskBuzzer102);
  userScheduler.addTask(taskBuzzer104);
  userScheduler.addTask(taskBuzzer1010);
  userScheduler.addTask(taskBeep);
  userScheduler.addTask(taskESPReset);
  userScheduler.addTask(taskSixtySec);
  taskSixtySec.enable();
  // taskTouchCode.enable();
  // taskBGRefresh.enable();
  taskESPReset.enable();
  taskSendMessage.enable();
}
void Loop2Code(void *pvParameters) // On core 0
{
  for (;;)
  {
    mesh.update();
    button.tick(); // for checking button input
    button2.tick();
    vTaskDelay(1);
  }
}

void loop() // On core 1
{
  BGRefreshCode();
  TouchCode();
  ClearEmpty();
  vTaskDelay(1);
}

String Keyboard(int Pick)
{
  KBOUTPUT = "demo";
  if (!KBran)
  {
    tft.setRotation(3);
    KBran = true;
  }
  KBCreate();
  KBsprite.fillSprite(TFT_LIGHTGREY);
  KBTouch();
  KBsprite.fillRoundRect(45, 42, 230, 50, 8, TFT_WHITE); // BG NAME BOX
  Keyboardlen = KeyboardStr.length();
  if (Keyboardlen < 13)
  {
    KBsprite.drawCentreString(KeyboardStr, 160, 52, 2);
  }
  else
  {
    String sub = KeyboardStr.substring(Keyboardlen - 13, Keyboardlen);
    KBsprite.drawCentreString(sub, 160, 52, 2);
  }
  KBsprite.drawString(String(FDigit), 30 + 27, 107 + 8 + 6, 2);
  KBsprite.drawString(String(SDigit), 100 + 27, 107 + 8 + 6, 2);
  if (!Symbol)
  {
    KBsprite.drawString(String(TDigit), 170 + 27, 107 + 8 + 6, 2);
    KBsprite.drawString(String(FrDigit), 240 + 27, 107 + 8 + 6, 2);
  }
  else
  {
    KBsprite.drawString(String(TDigit), 170 + 27, 107 + 8 + 3, 2);
    KBsprite.drawString(String(FrDigit), 240 + 27, 107 + 8 + 3, 2);
  }

  KBsprite.setTextColor(TFT_WHITE);
  KBsprite.drawString("SPACE", 90, 155);
  KBsprite.drawString("DELETE", 230, 155);
  return KBOUTPUT;
}
void KBCreate()
{
  if (Symbol)
  {
    FDigit = "!";
    SDigit = "?";
    TDigit = ".";
    FrDigit = ",";
  }
  else
  {
    if (PageAlpha == 1)
    { // 1-6 A-X
      FDigit = "A";
      SDigit = "B";
      TDigit = "C";
      FrDigit = "D";
    }
    else if (PageAlpha == 2)
    {
      FDigit = "E";
      SDigit = "F";
      TDigit = "G";
      FrDigit = "H";
    }
    else if (PageAlpha == 3)
    {
      FDigit = "I";
      SDigit = "J";
      TDigit = "K";
      FrDigit = "L";
    }
    else if (PageAlpha == 4)
    {
      FDigit = "M";
      SDigit = "N";
      TDigit = "O";
      FrDigit = "P";
    }
    else if (PageAlpha == 5)
    {
      FDigit = "Q";
      SDigit = "R";
      TDigit = "S";
      FrDigit = "T";
    }
    else if (PageAlpha == 6)
    {
      FDigit = "U";
      SDigit = "V";
      TDigit = "W";
      FrDigit = "X";
    }
    else if (PageAlpha == 7)
    {
      FDigit = "Y";
      SDigit = "Z";
      TDigit = "A";
      FrDigit = "B";
    }
  }
}
void KBTouch()
{
  KBsprite.setTextColor(TFT_BLACK);
  // For Left Arrow
  if (!KBholdL)
  {
    if ((XCoord <= 80) && (XCoord >= 40) && (YCoord <= 70) && (YCoord >= 0))
    {
      KBsprite.fillTriangle(15, 85 - 20, 35, 65 - 20, 35, 105 - 20, TFT_LIGHTGREY);
      KBsprite.fillTriangle(20, 85 - 20, 31, 74 - 20, 31, 96 - 20, TFT_BLACK);
      XCoordLS = XCoord;
      KBholdL = true;
    }
    else
    {
      KBsprite.fillTriangle(15, 85 - 20, 35, 65 - 20, 35, 105 - 20, TFT_LIGHTGREY);
      KBsprite.fillTriangle(20, 85 - 20, 31, 74 - 20, 31, 96 - 20, TFT_DARKGREY);
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      KBsprite.fillTriangle(15, 85 - 20, 35, 65 - 20, 35, 105 - 20, TFT_LIGHTGREY);
      KBsprite.fillTriangle(20, 85 - 20, 31, 74 - 20, 31, 96 - 20, TFT_BLACK);
      if (PageAlpha > 1)
      {
        PageAlpha--;
      }
      else
      {
        PageAlpha = 7;
      }
      KBholdL = false;
    }
    else if ((XCoord <= XCoordLS + 5) && (XCoord >= XCoordLS - 5)) // Don't need Y X alone works well
    {
      KBsprite.fillTriangle(15, 85 - 20, 35, 65 - 20, 35, 105 - 20, TFT_LIGHTGREY);
      KBsprite.fillTriangle(20, 85 - 20, 31, 74 - 20, 31, 96 - 20, TFT_BLACK);
    }
    else
    {
      KBsprite.fillTriangle(15, 85 - 20, 35, 65 - 20, 35, 105 - 20, TFT_LIGHTGREY);
      KBsprite.fillTriangle(20, 85 - 20, 31, 74 - 20, 31, 96 - 20, TFT_WHITE);
      KBholdL = false;
    }
  }
  // For Right Arrow
  if (!KBholdR)
  {
    if ((XCoord <= 85) && (XCoord >= 40) && (YCoord <= 320) && (YCoord >= 270))
    {
      XCoordRS = XCoord;
      KBsprite.fillTriangle(305, 85 - 20, 285, 65 - 20, 285, 105 - 20, TFT_LIGHTGREY);
      KBsprite.fillTriangle(300, 85 - 20, 289, 74 - 20, 289, 96 - 20, TFT_BLACK);
      KBholdR = true;
    }
    else
    {
      KBsprite.fillTriangle(305, 85 - 20, 285, 65 - 20, 285, 105 - 20, TFT_LIGHTGREY);
      KBsprite.fillTriangle(300, 85 - 20, 289, 74 - 20, 289, 96 - 20, TFT_DARKGREY);
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      KBsprite.fillTriangle(305, 85 - 20, 285, 65 - 20, 285, 105 - 20, TFT_LIGHTGREY);
      KBsprite.fillTriangle(300, 85 - 20, 289, 74 - 20, 289, 96 - 20, TFT_BLACK);
      if (PageAlpha <= 6)
      {
        PageAlpha++;
      }
      else
      {
        PageAlpha = 1;
      }
      KBholdR = false;
    }
    else if ((XCoord <= XCoordRS + 15) && (XCoord >= XCoordRS - 15)) // Don't need Y X alone works well
    {
      KBsprite.fillTriangle(305, 85 - 20, 285, 65 - 20, 285, 105 - 20, TFT_LIGHTGREY);
      KBsprite.fillTriangle(300, 85 - 20, 289, 74 - 20, 289, 96 - 20, TFT_BLACK);
    }
    else
    {
      KBsprite.fillTriangle(305, 85 - 20, 285, 65 - 20, 285, 105 - 20, TFT_LIGHTGREY);
      KBsprite.fillTriangle(300, 85 - 20, 289, 74 - 20, 289, 96 - 20, TFT_DARKGREY);
      KBholdR = false;
    }
  }
  // For spacebt
  if (!Backhold)
  {
    if ((XCoord <= 170) && (XCoord >= 130) && (YCoord <= 140) && (YCoord >= 10))
    {
      XCoordRS = XCoord;
      KBsprite.fillRoundRect(40 - 17 + 8, 142, 120, 25, 4, TFT_DARKGREY); // space bg
      Backhold = true;
    }
    else
    {
      KBsprite.fillRoundRect(40 - 17 + 8, 142, 120, 25, 4, TFT_BLACK); // space bg
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      KeyboardStr += " ";
      KBsprite.fillRoundRect(40 - 17 + 8, 142, 120, 25, 4, TFT_BLACK); // space bg
      Backhold = false;
    }
    else if ((XCoord <= XCoordRS + 15) && (XCoord >= XCoordRS - 15)) // Don't need Y X alone works well
    {
      KBsprite.fillRoundRect(40 - 17 + 8, 142, 120, 25, 4, TFT_DARKGREY); // space bg
    }
    else
    {
      KBsprite.fillRoundRect(40 - 17 + 8, 142, 120, 25, 4, TFT_BLACK); // space bg
      Backhold = false;
    }
  }
  // For del bt
  if (!DelHold)
  {
    if ((XCoord <= 170) && (XCoord >= 130) && (YCoord <= 310) && (YCoord >= 160))
    {
      XCoordRS = XCoord;
      KBsprite.fillRoundRect(160 + 8, 142, 120, 25, 4, TFT_DARKGREY); // del bg
      DelHold = true;
    }
    else
    {
      KBsprite.fillRoundRect(160 + 8, 142, 120, 25, 4, TFT_BLACK); // del bg
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      int NameLenth = KeyboardStr.length();
      int i;
      String NewNameMinus;
      for (i = 1; i < NameLenth; i++)
      {
        NewNameMinus += KeyboardStr[i - 1];
      }
      KeyboardStr = NewNameMinus;
      KBsprite.fillRoundRect(160 + 8, 142, 120, 25, 4, TFT_BLACK); // del bg
      DelHold = false;
    }
    else if ((XCoord <= XCoordRS + 15) && (XCoord >= XCoordRS - 15)) // Don't need Y X alone works well
    {
      KBsprite.fillRoundRect(160 + 8, 142, 120, 25, 4, TFT_DARKGREY); // del bg
    }
    else
    {
      KBsprite.fillRoundRect(160 + 8, 142, 120, 25, 4, TFT_BLACK); // del bg
      DelHold = false;
    }
  }
  //--------------------------------------------------------------------------------
  KBsprite.fillRoundRect(40 - 17, 100, 65, 38, 4, TFT_DARKGREY);
  KBsprite.fillRoundRect(110 - 17, 100, 65, 38, 4, TFT_DARKGREY);
  KBsprite.fillRoundRect(180 - 17, 100, 65, 38, 4, TFT_DARKGREY);
  KBsprite.fillRoundRect(250 - 17, 100, 65, 38, 4, TFT_DARKGREY);
  if (!FAlphaHold) // first alphabet
  {
    if ((XCoord <= 140) && (XCoord >= 100) && (YCoord <= 70) && (YCoord >= 0))
    {
      XCoordRS = XCoord;
      KBsprite.fillRoundRect(40 - 10, 100 + 7, 50, 25, 4, TFT_BLACK); // b1
      FAlphaHold = true;
    }
    else
    {
      KBsprite.fillRoundRect(40 - 10, 100 + 7, 50, 25, 4, TFT_LIGHTGREY); // b1
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      if (KeyboardStr.length() <= 900)
        KeyboardStr += FDigit;
      KBsprite.fillRoundRect(40 - 10, 100 + 7, 50, 25, 4, TFT_LIGHTGREY); // b1
      FAlphaHold = false;
    }
    else if ((XCoord <= XCoordRS + 15) && (XCoord >= XCoordRS - 15)) // Don't need Y X alone works well
    {
      KBsprite.fillRoundRect(40 - 10, 100 + 7, 50, 25, 4, TFT_BLACK); // b1
    }
    else
    {
      KBsprite.fillRoundRect(40 - 10, 100 + 7, 50, 25, 4, TFT_LIGHTGREY); // b1

      FAlphaHold = false;
    }
  }
  if (!SAlphaHold) // Second alphabet
  {
    if ((XCoord <= 140) && (XCoord >= 100) && (YCoord <= 160) && (YCoord >= 80))
    {
      XCoordRS = XCoord;
      KBsprite.fillRoundRect(110 - 10, 100 + 7, 50, 25, 4, TFT_BLACK); // b2
      SAlphaHold = true;
    }
    else
    {
      KBsprite.fillRoundRect(110 - 10, 100 + 7, 50, 25, 4, TFT_LIGHTGREY); // b2
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      if (KeyboardStr.length() <= 900)
        KeyboardStr += SDigit;
      KBsprite.fillRoundRect(110 - 10, 100 + 7, 50, 25, 4, TFT_LIGHTGREY); // b2
      SAlphaHold = false;
    }
    else if ((XCoord <= XCoordRS + 15) && (XCoord >= XCoordRS - 15)) // Don't need Y X alone works well
    {
      KBsprite.fillRoundRect(110 - 10, 100 + 7, 50, 25, 4, TFT_BLACK); // b2
    }
    else
    {
      KBsprite.fillRoundRect(110 - 10, 100 + 7, 50, 25, 4, TFT_LIGHTGREY); // b2
      SAlphaHold = false;
    }
  }
  if (!TAlphaHold) // Third alphabet
  {
    if ((XCoord <= 140) && (XCoord >= 100) && (YCoord <= 220) && (YCoord >= 170))
    {
      XCoordRS = XCoord;
      KBsprite.fillRoundRect(180 - 10, 100 + 7, 50, 25, 4, TFT_BLACK); // b3
      TAlphaHold = true;
    }
    else
    {
      KBsprite.fillRoundRect(180 - 10, 100 + 7, 50, 25, 4, TFT_LIGHTGREY); // b3
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      if (KeyboardStr.length() <= 900)
        KeyboardStr += TDigit;
      KBsprite.fillRoundRect(180 - 10, 100 + 7, 50, 25, 4, TFT_LIGHTGREY); // b3
      TAlphaHold = false;
    }
    else if ((XCoord <= XCoordRS + 15) && (XCoord >= XCoordRS - 15)) // Don't need Y X alone works well
    {
      KBsprite.fillRoundRect(180 - 10, 100 + 7, 50, 25, 4, TFT_BLACK); // b3
    }
    else
    {
      KBsprite.fillRoundRect(180 - 10, 100 + 7, 50, 25, 4, TFT_LIGHTGREY); // b3

      TAlphaHold = false;
    }
  }
  if (!FRAlphaHold) // Forth alphabet
  {
    if ((XCoord <= 140) && (XCoord >= 100) && (YCoord <= 290) && (YCoord >= 230))
    {
      XCoordRS = XCoord;
      KBsprite.fillRoundRect(250 - 10, 100 + 7, 50, 25, 4, TFT_BLACK); // b4
      FRAlphaHold = true;
    }
    else
    {
      KBsprite.fillRoundRect(250 - 10, 100 + 7, 50, 25, 4, TFT_LIGHTGREY); // b4
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {

      if (KeyboardStr.length() <= 900)
        KeyboardStr += FrDigit;
      KBsprite.fillRoundRect(250 - 10, 100 + 7, 50, 25, 4, TFT_LIGHTGREY); // b4
      FRAlphaHold = false;
    }
    else if ((XCoord <= XCoordRS + 15) && (XCoord >= XCoordRS - 15)) // Don't need Y X alone works well
    {
      KBsprite.fillRoundRect(250 - 10, 100 + 7, 50, 25, 4, TFT_BLACK); // b4
    }
    else
    {
      KBsprite.fillRoundRect(250 - 10, 100 + 7, 50, 25, 4, TFT_LIGHTGREY); // b4
      FRAlphaHold = false;
    }
  }
  if (!KBSaveHold) // savebt
  {
    if ((XCoord <= 50) && (XCoord >= 0) && (YCoord <= 120) && (YCoord >= 30))
    {
      XCoordRS = XCoord;
      KBsprite.fillRoundRect(35, 12, 75, 25, 8, TFT_BLUE);
      KBSaveHold = true;
    }
    else
    {
      KBsprite.fillRoundRect(35, 12, 75, 25, 8, TFT_DARKCYAN);
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      KBOUTPUT = KeyboardStr;
      KBsave = true;
      KBsprite.fillRoundRect(35, 12, 75, 25, 8, TFT_DARKCYAN);
      KBSaveHold = false;
    }
    else if ((XCoord <= XCoordRS + 15) && (XCoord >= XCoordRS - 15)) // Don't need Y X alone works well
    {
      KBsprite.fillRoundRect(35, 12, 75, 25, 8, TFT_BLUE);
    }
    else
    {
      KBsprite.fillRoundRect(35, 12, 75, 25, 8, TFT_DARKCYAN);
      KBSaveHold = false;
    }
  }
  if (!KBCancelHold) // cancel
  {
    if ((XCoord <= 50) && (XCoord >= 0) && (YCoord <= 180) && (YCoord >= 130))
    {
      KBsprite.fillRoundRect(120, 12, 75, 25, 8, TFT_BLUE);
      XCoordRS = XCoord;
      KBCancelHold = true;
    }
    else
    {
      KBsprite.fillRoundRect(120, 12, 75, 25, 8, TFT_DARKCYAN);
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      if (OneEd)
      {
        Finalstr = OneMsg;
        OneEd = false;
      }
      else if (TwoEd)
      {
        Finalstr = TwoMsg;
        TwoEd = false;
      }
      else if (ThreeEd)
      {
        Finalstr = ThreeMsg;
        ThreeEd = false;
      }
      KBsprite.fillRoundRect(120, 12, 75, 25, 8, TFT_DARKCYAN);
      KBCancelHold = false;
    }
    else if ((XCoord <= XCoordRS + 15) && (XCoord >= XCoordRS - 15)) // Don't need Y X alone works well
    {
      KBsprite.fillRoundRect(120, 12, 75, 25, 8, TFT_BLUE);
    }
    else
    {
      KBsprite.fillRoundRect(120, 12, 75, 25, 8, TFT_DARKCYAN);
      KBCancelHold = false;
    }
  }
  if (!KBsymbolHold) // symbol
  {
    if ((XCoord <= 50) && (XCoord >= 0) && (YCoord <= 300) && (YCoord >= 185))
    {
      KBsprite.fillRoundRect(205, 12, 75, 25, 8, TFT_BLUE);
      XCoordRS = XCoord;
      KBsymbolHold = true;
    }
    else
    {
      KBsprite.fillRoundRect(205, 12, 75, 25, 8, TFT_DARKCYAN);
    }
  }
  else
  {
    if ((XCoord == 0) && (YCoord == 0))
    {
      if (!Symbol)
        Symbol = true;
      else
        Symbol = false;
      KBsprite.fillRoundRect(205, 12, 75, 25, 8, TFT_DARKCYAN);
      KBsymbolHold = false;
    }
    else if ((XCoord <= XCoordRS + 15) && (XCoord >= XCoordRS - 15)) // Don't need Y X alone works well
    {
      KBsprite.fillRoundRect(205, 12, 75, 25, 8, TFT_BLUE);
    }
    else
    {
      KBsprite.fillRoundRect(205, 12, 75, 25, 8, TFT_DARKCYAN);
      KBsymbolHold = false;
    }
  }
  KBsprite.drawCentreString("SAVE", 35 + (75 / 2), 16, 1);
  KBsprite.drawCentreString("CANCEL", 120 + (75 / 2), 16, 1);
  KBsprite.drawCentreString("SYMBOL", 205 + (75 / 2), 16, 1);
}
void SclOne()
{
  String OneCopy = OneMsg;
  String sub;
  if (OneCopy.length() > 22)
  {
    if (DisplayPosi + 22 < OneCopy.length())
    {
      sub = OneCopy.substring(DisplayPosi, DisplayPosi + 25);
      DisplayPosi++;
    }
    else
    {
      delay(500);
      DisplayPosi = 0;
      ScrollOne = true;
    }
    sprite.drawString(String(sub), 10, 35, 2);
  }
  else
  {
    sprite.drawCentreString(OneMsg, 85, 35, 2);
    delay(500);
    ScrollOne = true;
  }
}
void SclTwo()
{
  String TwoCopy = TwoMsg;
  String sub;
  if (TwoCopy.length() > 22)
  {
    if (DisplayPosi + 22 < TwoCopy.length())
    {
      sub = TwoCopy.substring(DisplayPosi, DisplayPosi + 25);
      DisplayPosi++;
    }
    else
    {
      delay(500);
      DisplayPosi = 0;
      ScrollTwo = true;
    }
    sprite.drawString(String(sub), 10, 35 + 65, 2);
  }
  else
  {
    sprite.drawCentreString(TwoMsg, 85, 35 + 65, 2);
    delay(500);
    ScrollTwo = true;
  }
}
void SclThree()
{
  String ThreeCopy = ThreeMsg;
  String sub;
  if (ThreeCopy.length() > 22)
  {
    if (DisplayPosi + 22 < ThreeCopy.length())
    {
      sub = ThreeMsg.substring(DisplayPosi, DisplayPosi + 25);
      DisplayPosi++;
    }
    else
    {
      delay(500);
      DisplayPosi = 0;
      ScrollThree = true;
    }
    sprite.drawString(String(sub), 10, 35 + 65 + 60, 2);
  }
  else
  {
    sprite.drawCentreString(ThreeCopy, 85, 35 + 60 + 65, 2);
    delay(500);
    ScrollThree = true;
  }
}