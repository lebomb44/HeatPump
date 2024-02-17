#include "src/HeatPump.h"
#include <CnC.h>
#include <stdlib.h>

#define TIC_MAX_MSG_SIZE 25

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

const char optarifName[] PROGMEM = "OPTARIF";
const char hchcName[] PROGMEM = "HCHC";
const char hchpName[] PROGMEM = "HCHP";
const char ptecName[] PROGMEM = "PTEC";
const char baseName[] PROGMEM = "BASE";
const char iinstName[] PROGMEM = "IINST";
const char pappName[] PROGMEM = "PAPP";

char tic_msg[TIC_MAX_MSG_SIZE] = {0};
uint8_t tic_msg_index = 0;

uint32_t previousTime_10s = 0;
uint32_t currentTime = 0;

void ping_cmdGet(int arg_cnt, char **args) { cnc_print_cmdGet_u32(pingName, currentTime); }
void power_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) { hp.setPowerSetting(args[3]); hp.update(); } }
void mode_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) { hp.setModeSetting(args[3]); hp.update(); } }
void temp_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) { hp.setTemperature(atof(args[3])); hp.update(); } }
void fanSpeed_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) { hp.setFanSpeed(args[3]); hp.update(); } }

void setup() {
  //Serial.begin(115200);
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
  Serial.begin(115200);

  tic_msg_index = 0;
  pinMode(16, OUTPUT);
  pinMode(17, INPUT_PULLUP);
  Serial2.begin(1200, SERIAL_7E1);
  Serial2.setTimeout(0);
}

void loop() {
  currentTime = millis(); cncPoll();
  /* HK @ 0.5Hz */
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
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    switch (c) {
      case '\r':
      case '\n':
        tic_msg[tic_msg_index] = '\0';
        if (0 == strncmp_P(tic_msg, optarifName, strnlen_P(optarifName, 50))) {
          tic_msg[10] = 0;
          cnc_print_hk_str(optarifName, &tic_msg[8]);
        }
        if (0 == strncmp_P(tic_msg, hchcName, strnlen_P(hchcName, 50))) {
          tic_msg[14] = 0;
          cnc_print_hk_str(hchcName, &tic_msg[5]);
        }
        if (0 == strncmp_P(tic_msg, hchpName, strnlen_P(hchpName, 50))) {
          tic_msg[14] = 0;
          cnc_print_hk_str(hchpName, &tic_msg[5]);
        }
        if (0 == strncmp_P(tic_msg, ptecName, strnlen_P(ptecName, 50))) {
          tic_msg[7] = 0;
          cnc_print_hk_str(ptecName, &tic_msg[5]);
        }
        if (0 == strncmp_P(tic_msg, baseName, strnlen_P(baseName, 50))) {
          tic_msg[14] = 0;
          cnc_print_hk_str(baseName, &tic_msg[5]);
        }
        if (0 == strncmp_P(tic_msg, iinstName, strnlen_P(iinstName, 50))) {
          tic_msg[9] = 0;
          cnc_print_hk_str(iinstName, &tic_msg[6]);
        }
        if (0 == strncmp_P(tic_msg, pappName, strnlen_P(pappName, 50))) {
          tic_msg[10] = 0;
          cnc_print_hk_str(pappName, &tic_msg[5]);
        }
        tic_msg_index = 0;
        break;
      default:
        // normal character entered. add it to the buffer
        if((TIC_MAX_MSG_SIZE-1) > tic_msg_index) {
            tic_msg[tic_msg_index] = c;
            tic_msg_index++;
        }
        break;
    }
  }
}
