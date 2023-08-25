
#include <HeatPump.h>
#include <CnC.h>

HeatPump hp;

const char nodeName[] PROGMEM = "heatpump";
const char sepName[] PROGMEM = " ";
const char hkName[] PROGMEM = "val";
const char cmdGetName[] PROGMEM = "get";
const char cmdSetName[] PROGMEM = "set";

const char pingName[] PROGMEM = "ping";
const char windowShutterButtonName[] PROGMEM = "windowShutterButton";
const char windowShutterUpRelayName[] PROGMEM = "windowShutterUpRelay";
const char windowShutterDownRelayName[] PROGMEM = "windowShutterDownRelay";
const char windowWindowContactName[] PROGMEM = "windowWindowContact";
const char windowShutterContactName[] PROGMEM = "windowShutterContact";
const char doorShutterButtonName[] PROGMEM = "doorShutterButton";
const char doorWindowContactName[] PROGMEM = "doorWindowContact";
const char doorShutterContactName[] PROGMEM = "doorShutterContact";
const char lightRelayName[] PROGMEM = "lightRelay";
const char entryRelayName[] PROGMEM = "entryRelay";
const char tempSensorsName[] PROGMEM = "tempSensors";

uint32_t previousTime_10s = 0;
uint32_t currentTime = 0;

void setup() {
  hp.enableExternalUpdate();
  hp.connect(&Serial1);

  Serial.begin(115200);
  cncInit(nodeName);
  cnc_hkName_set(hkName);
  cnc_cmdGetName_set(cmdGetName);
  cnc_cmdSetName_set(cmdSetName);
  cnc_sepName_set(sepName);
  cnc_cmdGet_Add(pingName, ping_cmdGet);
  cnc_cmdGet_Add(windowWindowContactName , windowWindowContact_cmdGet);
  cnc_cmdGet_Add(windowShutterContactName, windowShutterContact_cmdGet);
  cnc_cmdGet_Add(doorShutterButtonName, doorShutterButton_cmdGet);
  cnc_cmdGet_Add(doorWindowContactName, doorWindowContact_cmdGet);
  cnc_cmdGet_Add(doorShutterContactName, doorShutterContact_cmdGet);
  cnc_cmdGet_Add(lightRelayName, lightRelay_cmdGet);
  cnc_cmdSet_Add(lightRelayName, lightRelay_cmdSet);
  cnc_cmdGet_Add(entryRelayName, entryRelay_cmdGet);
  cnc_cmdSet_Add(entryRelayName, entryRelay_cmdSet);

  previousTime_10s = millis();
}

void loop() {
  currentTime = millis(); cncPoll();
  /* HK @ 0.1Hz */
  if((uint32_t)(currentTime - previousTime_10s) >= 10000) {
    hp.sync();
    hp.getPowerSetting();
    hp.getModeSetting();
    hp.getTemperature();
    hp.getFanSpeed();
    hp.getVaneSetting();
    hp.getWideVaneSetting();
    hp.getIseeBool();
    hp.getStatus();
    hp.getRoomTemperature();
    hp.getOperating();

    cnc_print_hk_bool();

    doorShutterContact.run(true); cncPoll();
    previousTime_10s = currentTime;
  }
}