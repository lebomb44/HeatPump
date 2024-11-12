#include "HeatPump.h"
#include "Arduino.h"

// Constructor /////////////////////////////////////////////////////////////////

HeatPump::HeatPump() {
  infoMode = 0;
  tempMode = false;
  wideVaneAdj = false;
  healthCount = 0;
  functions = heatpumpFunctions();
}

// Public Methods //////////////////////////////////////////////////////////////

bool HeatPump::connect(HardwareSerial *serial) {
  if(serial != NULL) {
    _HardSerial = serial;
  }
  _HardSerial->begin(2400, SERIAL_8E1);
  
  // settle before we start sending packets
  delay(2000);

  // send the CONNECT packet twice - need to copy the CONNECT packet locally
  byte packet[CONNECT_LEN];
  memcpy(packet, CONNECT, CONNECT_LEN);

  writePacket(packet, CONNECT_LEN);
  return true;
}

bool HeatPump::update() {
  readAllPackets();
  byte packet[PACKET_LEN] = {};
  createPacket(packet, wantedSettings);
  writePacket(packet, PACKET_LEN);
  delay(100);
  writePacket(packet, PACKET_LEN);
  return true;
}

void HeatPump::sync() {
  readAllPackets();
  //OCM printAllPackets();
  byte packet[PACKET_LEN] = {};
  createInfoPacket(packet);
  writePacket(packet, PACKET_LEN);
  delay(500);
  writePacket(packet, PACKET_LEN);
}

bool HeatPump::getPowerSettingBool() {
  return currentSettings.power == POWER_MAP[1] ? true : false;
}

void HeatPump::setPowerSetting(bool setting) {
  wantedSettings = currentSettings;
  wantedSettings.power = lookupByteMapIndex(POWER_MAP, 2, POWER_MAP[setting ? 1 : 0]) > -1 ? POWER_MAP[setting ? 1 : 0] : POWER_MAP[0];
}

const char* HeatPump::getPowerSetting() {
  return currentSettings.power;
}

void HeatPump::setPowerSetting(const char* setting) {
  wantedSettings = currentSettings;
  int index = lookupByteMapIndex(POWER_MAP, 2, setting);
  if (index > -1) {
    wantedSettings.power = POWER_MAP[index];
  } else {
    wantedSettings.power = POWER_MAP[0];
  }
}

const char* HeatPump::getModeSetting() {
  return currentSettings.mode;
}

void HeatPump::setModeSetting(const char* setting) {
  wantedSettings = currentSettings;
  int index = lookupByteMapIndex(MODE_MAP, 5, setting);
  if (index > -1) {
    wantedSettings.mode = MODE_MAP[index];
  } else {
    wantedSettings.mode = MODE_MAP[0];
  }
}

float HeatPump::getTemperature() {
  return currentSettings.temperature;
}

void HeatPump::setTemperature(float setting) {
  wantedSettings = currentSettings;
  if(!tempMode){
    wantedSettings.temperature = lookupByteMapIndex(TEMP_MAP, 16, (int)(setting + 0.5)) > -1 ? setting : TEMP_MAP[0];
  }
  else {
    setting = setting * 2;
    setting = round(setting);
    setting = setting / 2;
    wantedSettings.temperature = setting < 10 ? 10 : (setting > 31 ? 31 : setting);
  }
}

const char* HeatPump::getFanSpeed() {
  return currentSettings.fan;
}

void HeatPump::setFanSpeed(const char* setting) {
  wantedSettings = currentSettings;
  int index = lookupByteMapIndex(FAN_MAP, 6, setting);
  if (index > -1) {
    wantedSettings.fan = FAN_MAP[index];
  } else {
    wantedSettings.fan = FAN_MAP[0];
  }
}

const char* HeatPump::getVaneSetting() {
  return currentSettings.vane;
}

void HeatPump::setVaneSetting(const char* setting) {
  wantedSettings = currentSettings;
  int index = lookupByteMapIndex(VANE_MAP, 7, setting);
  if (index > -1) {
    wantedSettings.vane = VANE_MAP[index];
  } else {
    wantedSettings.vane = VANE_MAP[0];
  }
}

const char* HeatPump::getWideVaneSetting() {
  return currentSettings.wideVane;
}

void HeatPump::setWideVaneSetting(const char* setting) {
  wantedSettings = currentSettings;
  int index = lookupByteMapIndex(WIDEVANE_MAP, 7, setting);
  if (index > -1) {
    wantedSettings.wideVane = WIDEVANE_MAP[index];
  } else {
    wantedSettings.wideVane = WIDEVANE_MAP[0];
  }
}

bool HeatPump::getIseeBool() { //no setter yet
  return currentSettings.iSee;
}

heatpumpStatus HeatPump::getStatus() {
  return currentStatus;
}

float HeatPump::getRoomTemperature() {
  return currentStatus.roomTemperature;
}

bool HeatPump::getOperating() {
  return currentStatus.operating;
}

uint32_t HeatPump::getHealthCount() {
  return healthCount;
}

float HeatPump::FahrenheitToCelsius(int tempF) {
  float temp = (tempF - 32) / 1.8;                
  return ((float)round(temp*2))/2;                 //Round to nearest 0.5C
}

int HeatPump::CelsiusToFahrenheit(float tempC) {
  float temp = (tempC * 1.8) + 32;                //round up if heat, down if cool or any other mode
  return (int)(temp + 0.5);
}

// Private Methods //////////////////////////////////////////////////////////////

int HeatPump::lookupByteMapIndex(const int valuesMap[], int len, int lookupValue) {
  for (int i = 0; i < len; i++) {
    if (valuesMap[i] == lookupValue) {
      return i;
    }
  }
  return -1;
}

int HeatPump::lookupByteMapIndex(const char* valuesMap[], int len, const char* lookupValue) {
  for (int i = 0; i < len; i++) {
    if (strcasecmp(valuesMap[i], lookupValue) == 0) {
      return i;
    }
  }
  return -1;
}


const char* HeatPump::lookupByteMapValue(const char* valuesMap[], const byte byteMap[], int len, byte byteValue) {
  for (int i = 0; i < len; i++) {
    if (byteMap[i] == byteValue) {
      return valuesMap[i];
    }
  }
  return valuesMap[0];
}

int HeatPump::lookupByteMapValue(const int valuesMap[], const byte byteMap[], int len, byte byteValue) {
  for (int i = 0; i < len; i++) {
    if (byteMap[i] == byteValue) {
      return valuesMap[i];
    }
  }
  return valuesMap[0];
}

byte HeatPump::checkSum(byte bytes[], int len) {
  byte sum = 0;
  for (int i = 0; i < len; i++) {
    sum += bytes[i];
  }
  return (0xfc - sum) & 0xff;
}

void HeatPump::createPacket(byte *packet, heatpumpSettings settings) {
  prepareSetPacket(packet, PACKET_LEN);
  
  if(settings.power != currentSettings.power) {
    packet[8]  = POWER[lookupByteMapIndex(POWER_MAP, 2, settings.power)];
    packet[6] += CONTROL_PACKET_1[0];
  }
  if(settings.mode!= currentSettings.mode) {
    packet[9]  = MODE[lookupByteMapIndex(MODE_MAP, 5, settings.mode)];
    packet[6] += CONTROL_PACKET_1[1];
  }
  if(!tempMode && settings.temperature!= currentSettings.temperature) {
    packet[10] = TEMP[lookupByteMapIndex(TEMP_MAP, 16, settings.temperature)];
    packet[6] += CONTROL_PACKET_1[2];
  }
  else if(tempMode && settings.temperature!= currentSettings.temperature) {
    float temp = (settings.temperature * 2) + 128;
    packet[19] = (int)temp;
    packet[6] += CONTROL_PACKET_1[2];
  }
  if(settings.fan!= currentSettings.fan) {
    packet[11] = FAN[lookupByteMapIndex(FAN_MAP, 6, settings.fan)];
    packet[6] += CONTROL_PACKET_1[3];
  }
  if(settings.vane!= currentSettings.vane) {
    packet[12] = VANE[lookupByteMapIndex(VANE_MAP, 7, settings.vane)];
    packet[6] += CONTROL_PACKET_1[4];
  }
  if(settings.wideVane!= currentSettings.wideVane) {
    packet[18] = WIDEVANE[lookupByteMapIndex(WIDEVANE_MAP, 7, settings.wideVane)] | (wideVaneAdj ? 0x80 : 0x00);
    packet[7] += CONTROL_PACKET_2[0];
  }
  // add the checksum
  byte chkSum = checkSum(packet, 21);
  packet[21] = chkSum;
}

void HeatPump::createInfoPacket(byte *packet) {
  // add the header to the packet
  for (int i = 0; i < INFOHEADER_LEN; i++) {
    packet[i] = INFOHEADER[i];
  }
  
  // request current infoMode, and increment for the next request
  packet[5] = INFOMODE[infoMode];
  if(infoMode == (INFOMODE_LEN - 1)) {
    infoMode = 0;
  } else {
    infoMode++;
  }

  // pad the packet out
  for (int i = 0; i < 15; i++) {
    packet[i + 6] = 0x00;
  }

  // add the checksum
  byte chkSum = checkSum(packet, 21);
  packet[21] = chkSum;
}

void HeatPump::writePacket(byte *packet, int length) {
  for (int i = 0; i < length; i++) {
     _HardSerial->write((uint8_t)packet[i]);
  }
}

int HeatPump::readPacket() {
  byte header[INFOHEADER_LEN] = {};
  byte data[PACKET_LEN] = {};
  bool foundStart = false;
  int dataSum = 0;
  byte checksum = 0;
  byte dataLength = 0;

  if(_HardSerial->available() > 0) {
    // read until we get start byte 0xfc
    while(_HardSerial->available() > 0 && !foundStart) {
      header[0] = _HardSerial->read();
      if(header[0] == HEADER[0]) {
        foundStart = true;
        //delay(100); // found that this delay increases accuracy when reading, might not be needed though
      }
    }

    if(!foundStart) {
      Serial.println("heatpump start not found");
      return RCVD_PKT_FAIL;
    }
    //read header
    for(int i=1;i<5;i++) {
      header[i] =  _HardSerial->read();
    }
    //check header
    if(header[0] == HEADER[0] && header[2] == HEADER[2] && header[3] == HEADER[3]) {
      dataLength = header[4];
      
      for(int i=0;i<dataLength;i++) {
        data[i] = _HardSerial->read();
      }
  
      // read checksum byte
      data[dataLength] = _HardSerial->read();
  
      // sum up the header bytes...
      for (int i = 0; i < INFOHEADER_LEN; i++) {
        dataSum += header[i];
      }

      // ...and add to that the sum of the data bytes
      for (int i = 0; i < dataLength; i++) {
        dataSum += data[i];
      }
  
      // calculate checksum
      checksum = (0xfc - dataSum) & 0xff;
      if(data[dataLength] == checksum) {
        if(header[1] == 0x62) {
          switch(data[0]) {
            case 0x02: { // setting information
              heatpumpSettings receivedSettings;
              receivedSettings.power       = lookupByteMapValue(POWER_MAP, POWER, 2, data[3]);
              receivedSettings.iSee = data[4] > 0x08 ? true : false;
              receivedSettings.mode = lookupByteMapValue(MODE_MAP, MODE, 5, receivedSettings.iSee  ? (data[4] - 0x08) : data[4]);

              if(data[11] != 0x00) {
                int temp = data[11];
                temp -= 128;
                receivedSettings.temperature = (float)temp / 2;
                tempMode =  true;
              } else {
                receivedSettings.temperature = lookupByteMapValue(TEMP_MAP, TEMP, 16, data[5]);
              }

              receivedSettings.fan         = lookupByteMapValue(FAN_MAP, FAN, 6, data[6]);
              receivedSettings.vane        = lookupByteMapValue(VANE_MAP, VANE, 7, data[7]);
              receivedSettings.wideVane    = lookupByteMapValue(WIDEVANE_MAP, WIDEVANE, 7, data[10] & 0x0F);
		          wideVaneAdj = (data[10] & 0xF0) == 0x80 ? true : false;
              
              currentSettings = receivedSettings;
              return RCVD_PKT_SETTINGS;
            }

            case 0x03: { //Room temperature reading
              heatpumpStatus receivedStatus;

              if(data[6] != 0x00) {
                int temp = data[6];
                temp -= 128;
                receivedStatus.roomTemperature = (float)temp / 2;
              } else {
                receivedStatus.roomTemperature = lookupByteMapValue(ROOM_TEMP_MAP, ROOM_TEMP, 32, data[3]);
              }

              currentStatus.roomTemperature = receivedStatus.roomTemperature;

              return RCVD_PKT_ROOM_TEMP;
            }

            case 0x04: { // unknown
                break; 
            }

            case 0x05: { // timer packet
              heatpumpTimers receivedTimers;

              receivedTimers.mode                = lookupByteMapValue(TIMER_MODE_MAP, TIMER_MODE, 4, data[3]);
              receivedTimers.onMinutesSet        = data[4] * TIMER_INCREMENT_MINUTES;
              receivedTimers.onMinutesRemaining  = data[6] * TIMER_INCREMENT_MINUTES;
              receivedTimers.offMinutesSet       = data[5] * TIMER_INCREMENT_MINUTES;
              receivedTimers.offMinutesRemaining = data[7] * TIMER_INCREMENT_MINUTES;

              currentStatus.timers = receivedTimers;

              return RCVD_PKT_TIMER;
            }

            case 0x06: { // status
              heatpumpStatus receivedStatus;
              receivedStatus.operating = data[4];
              receivedStatus.compressorFrequency = data[3];

              currentStatus.operating = receivedStatus.operating;
              currentStatus.compressorFrequency = receivedStatus.compressorFrequency;

              return RCVD_PKT_STATUS;
            }

            case 0x09: { // standby mode maybe?
              break;
            }
            
            case 0x20:
            case 0x22: {
              if (dataLength == 0x10) {
                if (data[0] == 0x20) {
                  functions.setData1(&data[1]);
                } else {
                  functions.setData2(&data[1]);
                }
                  
                return RCVD_PKT_FUNCTIONS;
              }
              break;
            }
          } 
        } 
        
        if(header[1] == 0x61) { //Last update was successful 
          return RCVD_PKT_UPDATE_SUCCESS;
        } else if(header[1] == 0x7a) { //Last update was successful 
          return RCVD_PKT_CONNECT_SUCCESS;
        }
      }
    }
  }

  return RCVD_PKT_FAIL;
}

void HeatPump::readAllPackets() {
  while (_HardSerial->available() > 0) {
    if(RCVD_PKT_FAIL != readPacket()) {
      healthCount++;
    }
  }
}

void HeatPump::printAllPackets() {
  byte b = 0;
  Serial.print("Received:                                                                   ");
  while (_HardSerial->available() > 0) {
    b = _HardSerial->read();
    if ((uint8_t)b < 16) { Serial.print("0"); }
    Serial.print((uint8_t)b, HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void HeatPump::prepareInfoPacket(byte* packet, int length) {
  memset(packet, 0, length * sizeof(byte));
  
  for (int i = 0; i < INFOHEADER_LEN && i < length; i++) {
    packet[i] = INFOHEADER[i];
  }  
}

void HeatPump::prepareSetPacket(byte* packet, int length) {
  memset(packet, 0, length * sizeof(byte));
  
  for (int i = 0; i < HEADER_LEN && i < length; i++) {
    packet[i] = HEADER[i];
  }  
}

heatpumpFunctions HeatPump::getFunctions() {
  functions.clear();
  
  byte packet1[PACKET_LEN] = {};
  byte packet2[PACKET_LEN] = {};

  prepareInfoPacket(packet1, PACKET_LEN);
  packet1[5] = FUNCTIONS_GET_PART1;
  packet1[21] = checkSum(packet1, 21);

  prepareInfoPacket(packet2, PACKET_LEN);
  packet2[5] = FUNCTIONS_GET_PART2;
  packet2[21] = checkSum(packet2, 21);
  
  writePacket(packet1, PACKET_LEN);
  readPacket();

  writePacket(packet2, PACKET_LEN);
  readPacket();

  // retry reading a few times in case responses were related
  // to other requests
  for (int i = 0; i < 5 && !functions.isValid(); ++i) {
    delay(100);
    readPacket();
  }

  return functions;
}

bool HeatPump::setFunctions(heatpumpFunctions const& functions) {
  if (!functions.isValid()) {
    return false;
  }

  byte packet1[PACKET_LEN] = {};
  byte packet2[PACKET_LEN] = {};

  prepareSetPacket(packet1, PACKET_LEN);
  packet1[5] = FUNCTIONS_SET_PART1;
  
  prepareSetPacket(packet2, PACKET_LEN);
  packet2[5] = FUNCTIONS_SET_PART2;
  
  functions.getData1(&packet1[6]);
  functions.getData2(&packet2[6]);

  // sanity check, we expect data byte 15 (index 20) to be 0
  if (packet1[20] != 0 || packet2[20] != 0)
    return false;
    
  // make sure all the other data bytes are set
  for (int i = 6; i < 20; ++i) {
    if (packet1[i] == 0 || packet2[i] == 0)
      return false;
  }

  packet1[21] = checkSum(packet1, 21);
  packet2[21] = checkSum(packet2, 21);

  writePacket(packet1, PACKET_LEN);
  readPacket();

  writePacket(packet2, PACKET_LEN);
  readPacket();

  return true;
}


heatpumpFunctions::heatpumpFunctions() {
  clear();
}

bool heatpumpFunctions::isValid() const {
  return _isValid1 && _isValid2;
}

void heatpumpFunctions::setData1(byte* data) {
  memcpy(raw, data, 15);
  _isValid1 = true;
}

void heatpumpFunctions::setData2(byte* data) {
  memcpy(raw + 15, data, 15);
  _isValid2 = true;
}

void heatpumpFunctions::getData1(byte* data) const {
  memcpy(data, raw, 15);
}

void heatpumpFunctions::getData2(byte* data) const {
  memcpy(data, raw + 15, 15);
}

void heatpumpFunctions::clear() {
  memset(raw, 0, sizeof(raw));
  _isValid1 = false;
  _isValid2 = false;
}

int heatpumpFunctions::getCode(byte b) {
  return ((b >> 2) & 0xff) + 100;
}

int heatpumpFunctions::getValue(byte b) {
  return b & 3;
}
    
int heatpumpFunctions::getValue(int code) {
  if (code > 128 || code < 101)
    return 0;
    
  for (int i = 0; i < MAX_FUNCTION_CODE_COUNT; ++i) {
    if (getCode(raw[i]) == code)
      return getValue(raw[i]);
  }

  return 0;
}

bool heatpumpFunctions::setValue(int code, int value) {
  if (code > 128 || code < 101)
    return false;

  if (value < 1 || value > 3)
    return false;
    
  for (int i = 0; i < MAX_FUNCTION_CODE_COUNT; ++i) {
    if (getCode(raw[i]) == code) {
      raw[i] = ((code - 100) << 2) + value;
      return true;
    }
  }

  return false;
}

heatpumpFunctionCodes heatpumpFunctions::getAllCodes() {
  heatpumpFunctionCodes result;
  for (int i = 0; i < MAX_FUNCTION_CODE_COUNT; ++i) {
    int code = getCode(raw[i]);
    result.code[i] = code;
    result.valid[i] = (code >= 101 && code <= 128);
  }

  return result;
}
