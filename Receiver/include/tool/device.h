#pragma once

#include "module/rf.h"
#include "cinttypes"
#include "string"
#include "unordered_map"
#include "host/ble_hs.h"

class Device
{
private:
  // 设备唯一ID
  uint8_t id;
  // 设备信息
  uint8_t battery;
  int8_t rssi;
  // 蓝牙记录
  bool bleConnected;
  bool bleDoConnect;
  uint8_t bleMAC[6]; // 6字节MAC地址
  uint16_t bleHandle;
  ble_l2cap_chan *bleChannel;
  // WIFI记录
  bool wifiConnected;
  uint8_t wifiMAC[6]; // 6字节MAC地址
  uint32_t wifiIP;    // IPv4地址

public:
  // 构造函数
  Device();
  Device(uint8_t battery, int8_t rssi,
         bool bleConnected, bool bleDoConnect, uint8_t *bleMAC, uint16_t bleHandle, ble_l2cap_chan *bleChannel,
         bool wifiConnected, uint8_t *wifiMAC, uint32_t wifiIP);
  // 拷贝构造函数
  Device(const Device &other);
  // 移动构造函数
  Device(Device &&other) noexcept;
  // 拷贝赋值运算符
  Device &operator=(const Device &other);
  // 移动赋值运算符
  Device &operator=(Device &&other) noexcept;

  uint8_t getId() const;
  uint8_t getBattery() const;
  void setBattery(uint8_t battery);
  int8_t getRssi() const;
  void setRssi(int8_t rssi);
  bool isBleConnected() const;
  void setBleConnected(bool connected);
  bool isBleDoConnect() const;
  void setBleDoConnect(bool doConnect);
  uint8_t *getBleMAC();
  void setBleMAC(uint8_t *mac);
  uint16_t getBleHandle() const;
  void setBleHandle(uint16_t handle);
  ble_l2cap_chan *getBleChannel() const;
  void setBleChannel(ble_l2cap_chan *channel);
  bool isWifiConnected() const;
  void setWifiConnected(bool connected);
  uint8_t *getWifiMAC();
  void setWifiMAC(uint8_t *mac);
  uint32_t getWifiIP() const;
  void setWifiIP(uint32_t ip);
  std::string getBleMACString();
  std::string getWifiMACString();
  std::string getWifiIPString() const;

  // 打印设备信息
  void print();
};

class DeviceManager
{
private:
  uint8_t deviceCount = 0;
  Device devices[RF_MAX_CONNECTION];
  std::unordered_map<int64_t, Device *> bleMACToDevice;
  std::unordered_map<int64_t, Device *> wifiMACToDevice;
  std::unordered_map<uint32_t, Device *> wifiIPToDevice;

  std::unordered_map<int64_t, uint32_t> wifiMACToIP;
public:
  // 添加设备
  Device *addDevice(uint8_t bleMAC[6]);

  // 通过BLE MAC地址获取设备指针
  Device *getDeviceByBleMAC(uint8_t bleMAC[6]);
  Device *getDeviceByBleMAC(const std::string bleMAC);

  // 通过WiFi MAC地址获取设备指针
  Device *getDeviceByWifiMAC(uint8_t wifiMAC[6]);
  Device *getDeviceByWifiMAC(const std::string wifiMAC);

  // 通过WiFi IP地址获取设备指针
  Device *getDeviceByWifiIP(uint32_t wifiIP);

  // 绑定WiFi MAC地址到设备
  void bindDevice(uint8_t bleMAC[6], uint8_t wifiMAC[6]);

  // 绑定WiFi MAC地址到IP地址
  void bindWifiMACToIP(uint8_t wifiMAC[6], uint32_t wifiIP);

  // 获取所有设备
  Device *getAllDevices();

  // 清空所有设备
  void clear();

  // 获取设备数量
  uint8_t size() const;
};