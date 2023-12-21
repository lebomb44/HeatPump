#ifndef __HeatPump_H__
#define __HeatPump_H__
#include <stdint.h>
#include <math.h>
#include <HardwareSerial.h>

typedef uint8_t byte;

struct heatpumpSettings {
  const char* power;
  const char* mode;
  float temperature;
  const char* fan;
  const char* vane; //vertical vane, up/down
  const char* wideVane; //horizontal vane, left/right
  bool iSee;   //iSee sensor, at the moment can only detect it, not set it
  bool connected;
};

struct heatpumpTimers {
  const char* mode;
  int onMinutesSet;
  int onMinutesRemaining;
  int offMinutesSet;
  int offMinutesRemaining;
};

bool operator==(const heatpumpTimers& lhs, const heatpumpTimers& rhs);
bool operator!=(const heatpumpTimers& lhs, const heatpumpTimers& rhs);

struct heatpumpStatus {
  float roomTemperature;
  bool operating; // if true, the heatpump is operating to reach the desired temperature
  heatpumpTimers timers;
  int compressorFrequency;
};

#define MAX_FUNCTION_CODE_COUNT 30

struct heatpumpFunctionCodes {
  bool valid[MAX_FUNCTION_CODE_COUNT];
  int code[MAX_FUNCTION_CODE_COUNT];
};

class heatpumpFunctions  {
  private:
    byte raw[MAX_FUNCTION_CODE_COUNT];
    bool _isValid1;
    bool _isValid2;

    int getCode(byte b);
    int getValue(byte b);

  public:
    heatpumpFunctions();

    bool isValid() const;
    
    // data must be 15 bytes
    void setData1(byte* data);
    void setData2(byte* data);
    void getData1(byte* data) const;
    void getData2(byte* data) const;
    
    void clear();

    int getValue(int code);
    bool setValue(int code, int value);

    heatpumpFunctionCodes getAllCodes();   

};

class HeatPump
{
  private:
    static const int PACKET_LEN = 22;
    static const int PACKET_SENT_INTERVAL_MS = 1000;
    static const int PACKET_INFO_INTERVAL_MS = 2000;
    static const int PACKET_TYPE_DEFAULT = 99;

    static const int CONNECT_LEN = 8;
    const byte CONNECT[CONNECT_LEN] = {0xfc, 0x5a, 0x01, 0x30, 0x02, 0xca, 0x01, 0xa8};
    static const int HEADER_LEN  = 8;
    const byte HEADER[HEADER_LEN]  = {0xfc, 0x41, 0x01, 0x30, 0x10, 0x01, 0x00, 0x00};

    static const int INFOHEADER_LEN  = 5;
    //OCM const byte INFOHEADER[INFOHEADER_LEN]  = {0xfc, 0x42, 0x02, 0x7A, 0x10};
    const byte INFOHEADER[INFOHEADER_LEN]  = {0xfc, 0x42, 0x01, 0x30, 0x10};

    static const int INFOMODE_LEN = 6;
    const byte INFOMODE[INFOMODE_LEN] = {
      0x02, // request a settings packet - RQST_PKT_SETTINGS
      0x03, // request the current room temp - RQST_PKT_ROOM_TEMP
      0x04, // unknown
      0x05, // request the timers - RQST_PKT_TIMERS
      0x06, // request status - RQST_PKT_STATUS
      0x09  // request standby mode (maybe?) RQST_PKT_STANDBY
    };

    const int RCVD_PKT_FAIL            = 0;
    const int RCVD_PKT_CONNECT_SUCCESS = 1;
    const int RCVD_PKT_SETTINGS        = 2;
    const int RCVD_PKT_ROOM_TEMP       = 3;
    const int RCVD_PKT_UPDATE_SUCCESS  = 4;
    const int RCVD_PKT_STATUS          = 5;
    const int RCVD_PKT_TIMER           = 6;
    const int RCVD_PKT_FUNCTIONS       = 7;

    const byte CONTROL_PACKET_1[5] = {0x01,    0x02,  0x04,  0x08, 0x10};
                                   //{"POWER","MODE","TEMP","FAN","VANE"};
    const byte CONTROL_PACKET_2[1] = {0x01};
                                   //{"WIDEVANE"};
    const byte POWER[2]            = {0x00, 0x01};
    const char* POWER_MAP[2]       = {"OFF", "ON"};
    const byte MODE[5]             = {0x01,   0x02,  0x03, 0x07, 0x08};
    const char* MODE_MAP[5]        = {"HEAT", "DRY", "COOL", "FAN", "AUTO"};
    const byte TEMP[16]            = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    const int TEMP_MAP[16]         = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16};
    const byte FAN[6]              = {0x00,  0x01,   0x02, 0x03, 0x05, 0x06};
    const char* FAN_MAP[6]         = {"AUTO", "QUIET", "1", "2", "3", "4"};
    const byte VANE[7]             = {0x00,  0x01, 0x02, 0x03, 0x04, 0x05, 0x07};
    const char* VANE_MAP[7]        = {"AUTO", "1", "2", "3", "4", "5", "SWING"};
    const byte WIDEVANE[7]         = {0x01, 0x02, 0x03, 0x04, 0x05, 0x08, 0x0c};
    const char* WIDEVANE_MAP[7]    = {"<<", "<",  "|",  ">",  ">>", "<>", "SWING"};
    const byte ROOM_TEMP[32]       = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                                      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const int ROOM_TEMP_MAP[32]    = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
                                      26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41};
    const byte TIMER_MODE[4]       = {0x00,  0x01,  0x02, 0x03};
    const char* TIMER_MODE_MAP[4]  = {"NONE", "OFF", "ON", "BOTH"};

    static const int TIMER_INCREMENT_MINUTES = 10;

    const byte FUNCTIONS_SET_PART1 = 0x1F;
    const byte FUNCTIONS_GET_PART1 = 0x20;
    const byte FUNCTIONS_SET_PART2 = 0x21;
    const byte FUNCTIONS_GET_PART2 = 0x22;

    // these settings will be initialised in connect()
    heatpumpSettings currentSettings {};
    heatpumpSettings wantedSettings {};

    // initialise to all off, then it will update shortly after connect;
    heatpumpStatus currentStatus {0, false, {TIMER_MODE_MAP[0], 0, 0, 0, 0}, 0};

    heatpumpFunctions functions;
  
    HardwareSerial * _HardSerial {nullptr};
    int infoMode;
    bool tempMode;
    bool wideVaneAdj;

    const char* lookupByteMapValue(const char* valuesMap[], const byte byteMap[], int len, byte byteValue);
    int    lookupByteMapValue(const int valuesMap[], const byte byteMap[], int len, byte byteValue);
    int    lookupByteMapIndex(const char* valuesMap[], int len, const char* lookupValue);
    int    lookupByteMapIndex(const int valuesMap[], int len, int lookupValue);

    byte checkSum(byte bytes[], int len);
    void createPacket(byte *packet, heatpumpSettings settings);
    void createInfoPacket(byte *packet);
    int readPacket();
    void readAllPackets();
    void writePacket(byte *packet, int length);
    void prepareInfoPacket(byte* packet, int length);
    void prepareSetPacket(byte* packet, int length);

  public:
    // indexes for INFOMODE array (public so they can be optionally passed to sync())
    const int RQST_PKT_SETTINGS  = 0;
    const int RQST_PKT_ROOM_TEMP = 1;
    const int RQST_PKT_TIMERS    = 3;
    const int RQST_PKT_STATUS    = 4;
    const int RQST_PKT_STANDBY   = 5;

    // general
    HeatPump();
    bool connect(HardwareSerial *serial);
    bool update();
    void sync();

    // settings
    void setPowerSetting(bool setting);
    bool getPowerSettingBool(); 
    const char* getPowerSetting();
    void setPowerSetting(const char* setting);
    const char* getModeSetting();
    void setModeSetting(const char* setting);
    float getTemperature();
    void setTemperature(float setting);
    const char* getFanSpeed();
    void setFanSpeed(const char* setting);
    const char* getVaneSetting();
    void setVaneSetting(const char* setting);
    const char* getWideVaneSetting();
    void setWideVaneSetting(const char* setting);
    bool getIseeBool();

    // status
    heatpumpStatus getStatus();
    float getRoomTemperature();
    bool getOperating();
    bool isConnected();

    // functions
    // NOTE: These methods have been tested with a PVA (P-series air handler) unit and has not been tested with anything else. Use at your own risk.
    heatpumpFunctions getFunctions();
    bool setFunctions(heatpumpFunctions const& functions);
    
    // helpers
    float FahrenheitToCelsius(int tempF);
    int CelsiusToFahrenheit(float tempC);
    void printAllPackets();
};
#endif
