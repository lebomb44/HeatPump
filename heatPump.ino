#include "src/HeatPump.h"
#include <CnC.h>
#include <stdlib.h>

#define TIC_MAX_MSG_SIZE 25
#define BOILER_POWER_PIN_OUT 11
#define BOILER_POWER_PIN_IN 2

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
const char healthCountName[] PROGMEM = "healthcount";

const char ltarfName[] PROGMEM = "LTARF";
const char ntarfName[] PROGMEM = "NTARF";
const char eastName[] PROGMEM = "EAST";
const char irms1Name[] PROGMEM = "IRMS1";
const char urms1Name[] PROGMEM = "URMS1";
const char sinstsName[] PROGMEM = "SINSTS";
const char stgeName[] PROGMEM = "STGE";
const char contactTICName[] PROGMEM = "contacttic";
const char protectTICName[] PROGMEM = "protecttic";
const char ovTICName[] PROGMEM = "ovtic";
const char opTICName[] PROGMEM = "optic";
const char injectionTICName[] PROGMEM = "injecttic";
const char healthTICName[] PROGMEM = "healthtic";

const char boilerPowerName[] PROGMEM = "boilerpower";

char tic_msg[TIC_MAX_MSG_SIZE] = {0};
uint8_t tic_msg_index = 0;
uint32_t tic_health_count = 0;

uint16_t boiler_power = 0;

uint32_t previousTime_10s = 0;
uint32_t currentTime = 0;

void ping_cmdGet(int arg_cnt, char **args) { cnc_print_cmdGet_u32(pingName, currentTime); }
void power_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) { hp.setPowerSetting(args[3]); hp.update(); } }
void mode_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) { hp.setModeSetting(args[3]); hp.update(); } }
void temp_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) { hp.setTemperature(atof(args[3])); hp.update(); } }
void fanSpeed_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) { hp.setFanSpeed(args[3]); hp.update(); } }
void boilerPower_cmdSet(int arg_cnt, char **args) {
  uint16_t _power = 0;
  if(4 == arg_cnt) {
    _power = strtoul(args[3], NULL, 10);
    if((0 <= _power) && (_power < 2500)) {
      boiler_power = _power;
      if(boiler_power == 0) { digitalWrite(BOILER_POWER_PIN_OUT, LOW); }
    }
  }
}

ISR(TIMER1_OVF_vect) {
  TCCR1B = B00000000;  // Stop timer
  digitalWrite(BOILER_POWER_PIN_OUT, HIGH);
}

void zero_crossing() {
  TCCR1B = B00000000;  // Stop timer
  digitalWrite(BOILER_POWER_PIN_OUT, LOW);

  uint16_t _boiler_power = 0;
  if((0 <= boiler_power) && (boiler_power <= 2000)) {
    _boiler_power = boiler_power;
  }

  TCNT1 = 65535 - (20000 - (_boiler_power * 10));            // Timer Preloading
  // 0W    -> 0ms  = 0
  // 1000W -> 5ms  = 10000ticks
  // 2000W -> 10ms = 20000ticks
  TCCR1B = B00000010;  // Start timer clk_16Mhz/8
}

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
  cnc_cmdSet_Add(boilerPowerName, boilerPower_cmdSet);

  previousTime_10s = millis();

  pinMode(18, OUTPUT);
  pinMode(19, INPUT_PULLUP);
  hp.connect(&Serial1);
  delay(1000);
  Serial.begin(115200);

  tic_msg_index = 0;
  tic_health_count = 0;
  pinMode(16, OUTPUT);
  pinMode(17, INPUT);
  Serial2.begin(9600, SERIAL_7E1);
  Serial2.setTimeout(0);

  boiler_power = 0;
  pinMode(BOILER_POWER_PIN_OUT, OUTPUT);
  digitalWrite(BOILER_POWER_PIN_OUT, LOW);
  pinMode(BOILER_POWER_PIN_IN, INPUT_PULLUP);
  TCCR1A = 0;           // Init Timer1
  TCCR1B = 0;           // Init Timer1
  TCNT1 = 0;            // Timer Preloading
  TIMSK1 |= B00000001;  // Enable Timer Overflow Interrupt
  attachInterrupt(digitalPinToInterrupt(BOILER_POWER_PIN_IN), zero_crossing, RISING);
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
    cnc_print_hk_u32(healthCountName, hp.getHealthCount());
    cnc_print_hk_u32(boilerPowerName, boiler_power);
    
    cnc_print_hk_u32(healthTICName, tic_health_count);

    previousTime_10s = currentTime;
  }

  while (Serial2.available() > 0) {
    char c = Serial2.read();
    //Serial.write(c);
    switch (c) {
      case '\r':
      case '\n':
        tic_msg[tic_msg_index] = '\0';
        if (0 == strncmp_P(tic_msg, ltarfName, strnlen_P(ltarfName, 50))) {
          tic_health_count++;
          tic_msg[20] = 0;
          cnc_print_hk_str(ltarfName, &tic_msg[14]);
        }
        if (0 == strncmp_P(tic_msg, ntarfName, strnlen_P(ntarfName, 50))) {
          tic_health_count++;
          tic_msg[8] = 0;
          cnc_print_hk_str(ntarfName, &tic_msg[6]);
        }
        if (0 == strncmp_P(tic_msg, eastName, strnlen_P(eastName, 50))) {
          tic_health_count++;
          tic_msg[14] = 0;
          cnc_print_hk_str(eastName, &tic_msg[5]);
        }
        if (0 == strncmp_P(tic_msg, irms1Name, strnlen_P(irms1Name, 50))) {
          tic_health_count++;
          tic_msg[9] = 0;
          cnc_print_hk_str(irms1Name, &tic_msg[6]);
        }
        if (0 == strncmp_P(tic_msg, urms1Name, strnlen_P(urms1Name, 50))) {
          tic_health_count++;
          tic_msg[9] = 0;
          cnc_print_hk_str(urms1Name, &tic_msg[6]);
        }
        if (0 == strncmp_P(tic_msg, sinstsName, strnlen_P(sinstsName, 50))) {
          tic_health_count++;
          tic_msg[12] = 0;
          cnc_print_hk_str(sinstsName, &tic_msg[7]);
        }
        if (0 == strncmp_P(tic_msg, stgeName, strnlen_P(stgeName, 50))) {
          tic_health_count++;
          tic_msg[13] = 0;
          cnc_print_hk_str(stgeName, &tic_msg[5]);
          uint32_t stge = 0;
          stge = strtoul(&tic_msg[5], NULL, 16);
          cnc_print_hk_bool(contactTICName, !(stge & 0x00000001));
          cnc_print_hk_u32(protectTICName, (stge >> 1) & 0x00000007);
          cnc_print_hk_bool(ovTICName, (stge >> 6) & 0x00000001);
          cnc_print_hk_bool(opTICName, (stge >> 7) & 0x00000001);
          cnc_print_hk_bool(injectionTICName, (stge >> 9) & 0x00000001);
        }
        tic_msg_index = 0;
        tic_msg[tic_msg_index] = '\0';
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
