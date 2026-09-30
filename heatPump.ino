#include "src/HeatPump.h"
#include <CnC.h>
#include <stdlib.h>

#define __HEATPUMP__
#define __TIC__
#define __BOILER__
#define __GRID_VOLTAGE_CURRENT__

#ifdef __TIC__
#define TIC_MAX_MSG_SIZE 25
#endif
#ifdef __BOILER__
#define BOILER_POWER_PIN_OUT 11
#define BOILER_POWER_PIN_IN 2
#endif
#ifdef __GRID_VOLTAGE_CURRENT__
#define GRID_VOLTAGE_PIN A0
#define GRID_CURRENT_PIN A1
#define GRID_HALF_PERIOD_SAMPLES_NB 45
#define GRID_SAMPLES_NB (GRID_HALF_PERIOD_SAMPLES_NB*9)
#endif

#ifdef __HEATPUMP__
HeatPump hp;
#endif

const char nodeName[] PROGMEM = "heatpump";
const char sepName[] PROGMEM = " ";
const char hkName[] PROGMEM = "val";
const char cmdGetName[] PROGMEM = "get";
const char cmdSetName[] PROGMEM = "set";

const char pingName[] PROGMEM = "ping";
#ifdef __HEATPUMP__
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
#endif
#ifdef __TIC__
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
#endif
#ifdef __BOILER__
const char boilerPowerName[] PROGMEM = "boilerpower";
const char boilerAutoName[] PROGMEM = "boilerauto";
const char boilerAutoFloorName[] PROGMEM = "boilerautofloor";

const uint16_t boiler_timesOn[] PROGMEM = {0,50,100,150,200,250,300,350,400,450,500,550,600,650,700,750,800,850,900,950,1000,1050,1100,1150,1200,1250,1300,1350,1400,1450,1500,1550,1600,1650,1700,1750,1800,1850,1900,1950,2000};
const uint16_t boiler_powers[] PROGMEM = {0,22,53,101,156,220,294,379,469,562,655,747,849,938,1033,1120,1201,1281,1342,1424,1495,1563,1636,1695,1750,1810,1855,1895,1910,1935,1950,1980,2000,2000,2000,2015,2015,2017,2020,2025,2027};
#endif
#ifdef __GRID_VOLTAGE_CURRENT__
const char gridVoltageRefName[] PROGMEM = "gridvoltageref";
const char gridVoltageEffName[] PROGMEM = "gridvoltageeff";
const char gridCurrentRefName[] PROGMEM = "gridcurrentref";
const char gridPowerName[] PROGMEM = "gridpower";
const char gridPowerConsoName[] PROGMEM = "gridpowerconso";
const char gridPowerInjectName[] PROGMEM = "gridpowerinject";
const char gridInjectName[] PROGMEM = "gridinject";
const char gridHealthName[] PROGMEM = "gridhealth";
#endif
#ifdef __TIC__
char tic_c = 0;
char tic_msg[TIC_MAX_MSG_SIZE] = {0};
uint8_t tic_msg_index = 0;
uint8_t tic_checksum = 0;
uint32_t tic_health_count = 0;
uint32_t tic_irms1 = 0;
uint32_t tic_urms1 = 0;
#endif
#ifdef __BOILER__
uint16_t boiler_timeOn = 0;
uint16_t boiler_power = 0;
uint8_t boiler_auto = 2;
uint16_t boiler_auto_floor = 200;
#endif
#ifdef __GRID_VOLTAGE_CURRENT__
volatile int32_t grid_voltage[GRID_SAMPLES_NB] = {0};
volatile int32_t grid_current[GRID_SAMPLES_NB] = {0};
volatile int32_t grid_power = 0;
volatile uint32_t grid_power_conso = 0;
volatile uint32_t grid_power_inject = 0;
volatile bool grid_inject = false;
volatile uint32_t grid_health_count = 0;
#endif
uint32_t previousTime_10s = 0;
uint32_t previousTime_60s = 0;
uint32_t currentTime = 0;

void ping_cmdGet(int arg_cnt, char **args) { cnc_print_cmdGet_u32(pingName, currentTime); }
#ifdef __HEATPUMP__
void power_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) { hp.setPowerSetting(args[3]); hp.update(); } }
void mode_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) { hp.setModeSetting(args[3]); hp.update(); } }
void temp_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) { hp.setTemperature(atof(args[3])); hp.update(); } }
void fanSpeed_cmdSet(int arg_cnt, char **args) { if(4==arg_cnt) { hp.setFanSpeed(args[3]); hp.update(); } }
#endif
#ifdef __BOILER__
void boilerPower_cmdSet(int arg_cnt, char **args) {
  uint16_t _power = 0;
  if(4 == arg_cnt) {
    _power = strtoul(args[3], NULL, 10);
    boiler_power_set(_power);
  }
  //Serial.print("Boiler Power = "); Serial.print(boiler_power); Serial.print(" / Time On = "); Serial.println(boiler_timeOn); 
}

void boiler_power_set(int32_t _power) {
  if(_power < 0) {
    _power = 0;
  }
  if(2000 < _power) {
    _power = 2000;
  }
  if((0 <= _power) && (_power <= 2000)) {
    boiler_power = _power;
    if(boiler_power == 0) { digitalWrite(BOILER_POWER_PIN_OUT, LOW); }
    boiler_timeOn = power2time(boiler_power);
  }
}

uint16_t power2time(uint16_t power) {
  for(uint16_t i=0; i<((sizeof(boiler_powers)/sizeof(uint16_t))-1); i++) {
    uint32_t power0 = pgm_read_word(&boiler_powers[i]);
    uint32_t power1 = pgm_read_word(&boiler_powers[i+1]);
    //Serial.print("Index="); Serial.print(i); Serial.print(" power0=");Serial.print(power0); Serial.print(" power1=");Serial.println(power1);
    if((power0 <= power) && (power < power1)) {
      uint32_t time0 = pgm_read_word(&boiler_timesOn[i]);
      uint32_t time1 = pgm_read_word(&boiler_timesOn[i+1]);
      //Serial.print("time0="); Serial.print(time0); Serial.print(" time1="); Serial.println(time1);
      uint16_t _timeOn = 0;
      _timeOn = (((time1-time0)*(power-power0) + (time0*(power1-power0))) / (power1-power0));
      if(_timeOn > 2000) {
        _timeOn = 2000;
      }
      //Serial.print("timeOn="); Serial.println(_timeOn);
      return _timeOn;
    }
  }
  return 0;
}

void boilerAuto_cmdSet(int arg_cnt, char **args) {
  uint16_t _auto = 0;
  if(4 == arg_cnt) {
    _auto = strtoul(args[3], NULL, 10);
    if((0 <= _auto) && (_auto <=2)) {
      boiler_auto = _auto;
    }
  }
}

void boilerAutoFloor_cmdSet(int arg_cnt, char **args) {
  uint16_t _auto_floor = 0;
  if(4 == arg_cnt) {
    _auto_floor = strtoul(args[3], NULL, 10);
    if(2000 < _auto_floor) {
      _auto_floor = 2000;
    }
    boiler_auto_floor = _auto_floor;
  }
}

ISR(TIMER1_OVF_vect) {
  TCCR1B = B00000000;  // Stop timer
  digitalWrite(BOILER_POWER_PIN_OUT, HIGH);
}

void zero_crossing() {
  uint16_t _boiler_timeOn = 0;
  uint8_t lowADC = 0;
  uint8_t highADC = 0;

  TCCR1B = B00000000;  // Stop timer
  digitalWrite(BOILER_POWER_PIN_OUT, LOW);

  if((0 <= boiler_timeOn) && (boiler_timeOn <= 2000)) {
    _boiler_timeOn = boiler_timeOn;
  }

  if(0 != _boiler_timeOn) {
    TCNT1 = 65535 - (20000 - (_boiler_timeOn * 10));            // Timer Preloading
    // 0W    -> 0ms  = 0ticks
    // 1000W -> 5ms  = 10000ticks
    // 2000W -> 10ms = 20000ticks
    TCCR1B = B00000010;  // Start timer clk_16Mhz/8
  }

  grid_health_count++;
}
#endif

void setup() {
  //Serial.begin(115200);
  cncInit(nodeName);
  cnc_hkName_set(hkName);
  cnc_cmdGetName_set(cmdGetName);
  cnc_cmdSetName_set(cmdSetName);
  cnc_sepName_set(sepName);
  cnc_cmdGet_Add(pingName, ping_cmdGet);
#ifdef __HEATPUMP__
  cnc_cmdSet_Add(powerName, power_cmdSet);
  cnc_cmdSet_Add(modeName, mode_cmdSet);
  cnc_cmdSet_Add(tempName, temp_cmdSet);
  cnc_cmdSet_Add(fanSpeedName, fanSpeed_cmdSet);
#endif
#ifdef __BOILER__
  cnc_cmdSet_Add(boilerPowerName, boilerPower_cmdSet);
  cnc_cmdSet_Add(boilerAutoName, boilerAuto_cmdSet);
  cnc_cmdSet_Add(boilerAutoFloorName, boilerAutoFloor_cmdSet);
#endif
  previousTime_10s = millis();
  previousTime_60s = millis();
#ifdef __HEATPUMP__
  pinMode(18, OUTPUT);
  pinMode(19, INPUT_PULLUP);
  hp.init(&Serial1);
  hp.connect();
#endif
#ifdef __GRID_VOLTAGE_CURRENT__
  pinMode(GRID_VOLTAGE_PIN, INPUT);
  pinMode(GRID_CURRENT_PIN, INPUT);
  grid_power = 0;
  grid_power_conso = 0;
  grid_power_inject = 0;
  grid_inject = false;
  grid_health_count = 0;
#endif
  delay(1000);
  Serial.begin(115200);
#ifdef __TIC__
  tic_c = 0;
  tic_msg_index = 0;
  tic_checksum = 0;
  tic_health_count = 0;
  tic_irms1 = 0;
  tic_urms1 = 0;

  pinMode(16, OUTPUT);
  pinMode(17, INPUT);
  Serial2.begin(9600, SERIAL_7E1);
  Serial2.setTimeout(0);
#endif
#ifdef __BOILER__
  boiler_timeOn = 0;
  boiler_power = 0;
  boiler_auto = 2;
  boiler_auto_floor = 200;

  pinMode(BOILER_POWER_PIN_OUT, OUTPUT);
  digitalWrite(BOILER_POWER_PIN_OUT, LOW);
  pinMode(BOILER_POWER_PIN_IN, INPUT_PULLUP);
  TCCR1A = 0;           // Init Timer1
  TCCR1B = 0;           // Init Timer1
  TCNT1 = 0;            // Timer Preloading
  TIMSK1 |= B00000001;  // Enable Timer Overflow Interrupt
  attachInterrupt(digitalPinToInterrupt(BOILER_POWER_PIN_IN), zero_crossing, RISING);
#endif
}

void loop() {
  currentTime = millis(); cncPoll();
  /* HK @ 0.5Hz */
  if((uint32_t)(currentTime - previousTime_10s) >= 2000) {
#ifdef __HEATPUMP__
    hp.sync(); cncPoll();
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
#endif
#ifdef __BOILER__
    cnc_print_hk_u32(boilerPowerName, boiler_power);
    cnc_print_hk_u32(boilerAutoName, boiler_auto);
    cnc_print_hk_u32(boilerAutoFloorName, boiler_auto_floor);
#endif
#ifdef __TIC__
    cnc_print_hk_u32(healthTICName, tic_health_count);
#endif
#ifdef __GRID_VOLTAGE_CURRENT__
    volatile uint32_t t_before = 0;
    volatile uint32_t t_after = 0;
    t_before = millis();
    for(uint16_t i=0; i<(GRID_SAMPLES_NB); i++) {
      grid_voltage[i] = analogRead(GRID_VOLTAGE_PIN);
      grid_current[i] = analogRead(GRID_CURRENT_PIN);
    }
    t_after = millis();
    //Serial.print("####### T="); Serial.println(t_after - t_before);
    uint32_t _voltageMean = 0;
    uint32_t _currentMean = 0;
    for(uint16_t i=0; i<GRID_SAMPLES_NB; i++) {
      _voltageMean = _voltageMean + grid_voltage[i];
      _currentMean = _currentMean + grid_current[i];
    }
    _voltageMean = _voltageMean / GRID_SAMPLES_NB;
    _currentMean = _currentMean / GRID_SAMPLES_NB;
    //Serial.print("####### vMean="); Serial.println(_voltageMean);
    //Serial.print("####### iMean="); Serial.println(_currentMean);
    for(uint16_t i=0; i<GRID_SAMPLES_NB; i++) {
      grid_voltage[i] = grid_voltage[i] - _voltageMean;
      grid_current[i] = grid_current[i] - _currentMean;
    }

    uint8_t zero_crossing = 0;
    for(uint16_t i=GRID_HALF_PERIOD_SAMPLES_NB; i<(GRID_HALF_PERIOD_SAMPLES_NB*4); i++) {
      if((0 > grid_voltage[i]) && (0 > grid_voltage[i+1]) && (0 <= grid_voltage[i+2])) {
        zero_crossing = i+2;
        break;
      }
    }
    grid_power = 0;
    uint32_t _voltageEff = 0;
    int32_t _power = 0;
    for(uint16_t i=zero_crossing; i<(zero_crossing+(GRID_HALF_PERIOD_SAMPLES_NB*4)); i++) {
      _voltageEff = _voltageEff + abs(grid_voltage[i]);
      _power = grid_voltage[i] * grid_current[i];
      //Serial.print("####### @="); Serial.print(i); Serial.print(" V="); Serial.print(grid_voltage[i]); Serial.print(" I="); Serial.println(grid_current[i]);
      grid_power = grid_power + _power;
    }
    _voltageEff = (_voltageEff * 10) / 1784;
    if(0 > grid_power) { grid_inject = true; } else { grid_inject = false; }
    grid_power = abs(grid_power);
    grid_power = grid_power / 221;
    if(true == grid_inject) { grid_power_conso = 0; grid_power_inject = grid_power; }
    else { grid_power_conso = grid_power; grid_power_inject = 0; }
    cnc_print_hk_u32(gridVoltageRefName, _voltageMean);
    cnc_print_hk_u32(gridVoltageEffName, _voltageEff);
    cnc_print_hk_u32(gridCurrentRefName, _currentMean);
    cnc_print_hk_u32(gridPowerName, grid_power);
    cnc_print_hk_u32(gridPowerConsoName, grid_power_conso);
    cnc_print_hk_u32(gridPowerInjectName, grid_power_inject);
    cnc_print_hk_bool(gridInjectName, grid_inject);
    cnc_print_hk_u32(gridHealthName, grid_health_count);
#endif

    previousTime_10s = currentTime;
  }
  if((uint32_t)(currentTime - previousTime_60s) >= 60000) {
    hp.connect(); cncPoll();
    previousTime_60s = currentTime;
  }
#ifdef __TIC__
  while (Serial2.available() > 0) {
    tic_c = Serial2.read(); cncPoll();
    //Serial.write(tic_c);
    switch (tic_c) {
      case '\r':
      case '\n':
        tic_msg[tic_msg_index] = '\0';
        if (tic_msg_index > 4) {
          tic_checksum = 0;
          for (uint8_t i=0; i<(tic_msg_index-1); i++) {
            tic_checksum = tic_checksum + tic_msg[i];
          }
          if (tic_msg[tic_msg_index-1] == ((tic_checksum & 0x3F) + 0x20)) {
            if (0 == strncmp_P(tic_msg, ltarfName, strnlen_P(ltarfName, 50))) {
              tic_health_count++;
              tic_msg[20] = 0;
              cnc_print_hk_str(ltarfName, &tic_msg[14]);
            }
            else if (0 == strncmp_P(tic_msg, ntarfName, strnlen_P(ntarfName, 50))) {
              tic_health_count++;
              tic_msg[8] = 0;
              cnc_print_hk_str(ntarfName, &tic_msg[6]);
            }
            else if (0 == strncmp_P(tic_msg, eastName, strnlen_P(eastName, 50))) {
              tic_health_count++;
              tic_msg[14] = 0;
              cnc_print_hk_str(eastName, &tic_msg[5]);
            }
            else if (0 == strncmp_P(tic_msg, irms1Name, strnlen_P(irms1Name, 50))) {
              tic_health_count++;
              tic_msg[9] = 0;
              cnc_print_hk_str(irms1Name, &tic_msg[6]);
              tic_irms1 = strtoul(&tic_msg[6], NULL, 10);
            }
            else if (0 == strncmp_P(tic_msg, urms1Name, strnlen_P(urms1Name, 50))) {
              tic_health_count++;
              tic_msg[9] = 0;
              cnc_print_hk_str(urms1Name, &tic_msg[6]);
              tic_urms1 = strtoul(&tic_msg[6], NULL, 10);
            }
            else if (0 == strncmp_P(tic_msg, sinstsName, strnlen_P(sinstsName, 50))) {
              tic_health_count++;
              tic_msg[12] = 0;
              cnc_print_hk_str(sinstsName, &tic_msg[7]);
            }
            else if (0 == strncmp_P(tic_msg, stgeName, strnlen_P(stgeName, 50))) {
              tic_health_count++;
              tic_msg[13] = 0;
              cnc_print_hk_str(stgeName, &tic_msg[5]);
              uint32_t stge = 0;
              stge = strtoul(&tic_msg[5], NULL, 16);
              cnc_print_hk_bool(contactTICName, !(stge & 0x00000001));
              cnc_print_hk_u32(protectTICName, (stge >> 1) & 0x00000007);
              cnc_print_hk_bool(ovTICName, (stge >> 6) & 0x00000001);
              cnc_print_hk_bool(opTICName, (stge >> 7) & 0x00000001);
              bool _tic_inject = false;
              _tic_inject = (stge >> 9) & 0x00000001;
              cnc_print_hk_bool(injectionTICName, _tic_inject);
#ifdef __BOILER__
              if(0 < boiler_auto) {
                if(true == _tic_inject) {
                  //int32_t delta_power = (tic_irms1 * tic_urms1) - boiler_auto_floor;
                  //boiler_power_set(((int32_t)boiler_power) + (delta_power/3));
                  int32_t delta_power = 0;
                  if(1 == boiler_auto) {
                    delta_power = (tic_irms1 * tic_urms1);
                  }
#ifdef __GRID_VOLTAGE_CURRENT__
                  if(2 == boiler_auto) {
                    delta_power = grid_power;
                  }
#endif
                  if(delta_power > boiler_auto_floor) {
                    boiler_power_set(((int32_t)boiler_power) + 1);
                  }
                  else {
                    boiler_power_set(((int32_t)boiler_power) - 1);
                  }
                }
                else {
                  boiler_power_set(((int32_t)boiler_power) - 10);
                }
              }
#endif
            }
          }
        }
        tic_msg_index = 0;
        tic_msg[tic_msg_index] = '\0';
        break;
      default:
        // normal character entered. add it to the buffer
        if((TIC_MAX_MSG_SIZE-1) > tic_msg_index) {
            tic_msg[tic_msg_index] = tic_c;
            tic_msg_index++;
        }
        break;
    }
  }
#endif
}
