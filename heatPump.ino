#include "src/HeatPump.h"
#include <CnC.h>
#include <stdlib.h>

HeatPump hp;

const char nodeName[] PROGMEM = "heatpump";
const char sepName[] PROGMEM = " ";
const char hkName[] PROGMEM = "val";
const char cmdGetName[] PROGMEM = "get";
const char cmdSetName[] PROGMEM = "set";

const char pingName[] PROGMEM = "ping";
const char powerName[] PROGMEM = "power";
const char modeName[] PROGMEM = "mode";
const char tempName[] PROGMEM = "temp";
const char fanSpeedName[] PROGMEM = "fanspeed";
const char vaneName[] PROGMEM = "vane";
const char wideVaneName[] PROGMEM = "widevane";
const char iseeName[] PROGMEM = "isee";
const char roomTempName[] PROGMEM = "roomtemp";
const char operatingName[] PROGMEM = "operating";

uint32_t previousTime_10s = 0;
uint32_t currentTime = 0;

void ping_cmdGet(int arg_cnt, char **args) { cnc_print_cmdGet_u32(pingName, currentTime); }
void power_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) {hp.setPowerSetting(args[3]); hp.update(); } }
void mode_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) {hp.setModeSetting(args[3]); hp.update(); } }
void temp_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) { hp.setTemperature(atof(args[3])); hp.update(); } }
void fanSpeed_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) {hp.setFanSpeed(args[3]); hp.update(); } }

void setup() {
  Serial.begin(115200);
  cncInit(nodeName);
  cnc_hkName_set(hkName);
  cnc_cmdGetName_set(cmdGetName);
  cnc_cmdSetName_set(cmdSetName);
  cnc_sepName_set(sepName);
  cnc_cmdGet_Add(pingName, ping_cmdGet);
  cnc_cmdSet_Add(powerName, power_cmdSet);
  cnc_cmdSet_Add(modeName, mode_cmdSet);
  cnc_cmdSet_Add(tempName, temp_cmdSet);
  cnc_cmdSet_Add(fanSpeedName, fanSpeed_cmdSet);

  previousTime_10s = millis();

  pinMode(18, OUTPUT);
  pinMode(19, INPUT_PULLUP);
  hp.connect(&Serial1);
  delay(1000);
}

void loop() {
  currentTime = millis(); cncPoll();
  /* HK @ 0.1Hz */
  if((uint32_t)(currentTime - previousTime_10s) >= 2000) {
    hp.sync();
    
    cnc_print_hk_str(powerName, hp.getPowerSetting());
    cnc_print_hk_str(modeName, hp.getModeSetting());
    cnc_print_hk_float(tempName, hp.getTemperature());
    cnc_print_hk_str(fanSpeedName, hp.getFanSpeed());
    cnc_print_hk_str(vaneName, hp.getVaneSetting());
    cnc_print_hk_str(wideVaneName, hp.getWideVaneSetting());
    cnc_print_hk_bool(iseeName, hp.getIseeBool());
    cnc_print_hk_float(roomTempName, hp.getRoomTemperature());
    cnc_print_hk_bool(operatingName, hp.getOperating());
    
    previousTime_10s = currentTime;
  }
}
