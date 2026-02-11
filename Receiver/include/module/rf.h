#pragma once

#define RF_MAX_CONNECTION 4
// 蓝牙
#define BLE_NAME "MicTx"
#define BLE_L2CAP_UUID 0x1812
#define BLE_L2CAP_PSM 0x1001
#define BLE_L2CAP_MTU 512
// WIFI
#define WIFI_NAME "MicRx"
#define WIFI_PASSWORD "Cx^9Xbg5wih3"
#define WIFI_CHANNEL 8
#define WIFI_IP_PROTOCOL 0xE9
#define WIFI_NO_PORT 0
#define WIFI_IP_HEAD_LEN 20

// 客户端状态
#define PACKET_CLIENT_STATUS_SIZE (sizeof(uint8_t) + sizeof(ClientStatusPacket))
#define PACKET_CLIENT_STATUS_BLE_MAC_NONE 0
#define PACKET_CLIENT_STATUS_WIFI_MAC_NONE 0
struct __attribute__((packed)) ClientStatusPacket
{
  uint8_t bleMAC[6] = {PACKET_CLIENT_STATUS_BLE_MAC_NONE};
  uint8_t wifiMAC[6] = {PACKET_CLIENT_STATUS_WIFI_MAC_NONE};
  uint8_t battery = 0x00;
};

// 服务端控制设备
#define PACKET_SERVER_CONTROL_DEVICE_SIZE (sizeof(uint8_t) + sizeof(ServerControlDevicePacket))
struct __attribute__((packed)) ServerControlDevicePacket
{
  bool start = false;
  bool startWiFi = false;
  bool startBLE = true;
  bool mode = false; // true: WiFi模式; false: BLE模式
  char name[32] = "";
  char password[32] = "";
};

// 服务端控制音频
#define PACKET_SERVER_CONTROL_AUDIO_SIZE (sizeof(uint8_t) + sizeof(ServerControlAudioPacket))
struct __attribute__((packed)) ServerControlAudioPacket
{
  bool start = false;
  uint8_t channel = 2;
  uint32_t rate = 48000;
  uint8_t bit = 16;
  bool autoVolumn = false;
  bool peekVolumn = false;
  uint8_t volumn = 0;
};

// WIFI传输包
#define PACKET_WIFI_AUDIO_HEAD_SIZE (sizeof(uint8_t) + sizeof(WiFiAudioPacket) - PACKET_WIFI_AUDIO_DATA_MAX_SIZE)
#define PACKET_WIFI_AUDIO_DATA_MAX_SIZE 1420
struct __attribute__((packed)) WiFiAudioPacket
{
  uint16_t size;
  uint32_t number;
  uint8_t part;
  uint8_t data[PACKET_WIFI_AUDIO_DATA_MAX_SIZE];
};

// BLE传输包
#define PACKET_BLE_AUDIO_HEAD_SIZE (sizeof(uint8_t) + sizeof(BLEAudioPacket) - PACKET_BLE_AUDIO_DATA_MAX_SIZE)
#define PACKET_BLE_AUDIO_DATA_MAX_SIZE 384
struct __attribute__((packed)) BLEAudioPacket
{
  uint16_t size;
  uint32_t number;
  uint8_t part;
  uint8_t data[PACKET_BLE_AUDIO_DATA_MAX_SIZE];
};

// 统一协议
enum PacketType
{
  PACKET_TYPE_WIFI_AUDIO = 0,
  PACKET_TYPE_BLE_AUDIO = 1,
  PACKET_TYPE_CLIENT_ACK = 2,
  PACKET_TYPE_SERVER_ACK = 3,
  PACKET_TYPE_CLIENT_STATUS = 4,
  PACKET_TYPE_SERVER_CONTROL_DEVICE = 5,
  PACKET_TYPE_SERVER_CONTROL_AUDIO = 6,
};
struct __attribute__((packed)) Packet
{
  uint8_t type; // 标识是哪个包
  union
  {
    // 音频包
    WiFiAudioPacket audioDataWiFi;
    BLEAudioPacket audioDataBLE;
    // 状态配置包
    ClientStatusPacket clientStatus;
    ServerControlDevicePacket serverControlDevice;
    ServerControlAudioPacket serverControlAudio;
  } packet;
};

namespace rf
{
  void setup();
  void reconfigure();
}