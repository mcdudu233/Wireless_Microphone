#include "logger.h"
#include "tool/device.h"

// 地址转换为字符串
static std::string macToStr(const uint8_t mac[6])
{
  char bda_str[18];
  snprintf(bda_str, sizeof(bda_str), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return std::string(bda_str);
}
// 字符串转换为地址
static uint64_t strToMac(const std::string &str)
{
  uint64_t result;
  uint8_t *pt = (uint8_t *)&result;
  sscanf(str.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
         &pt[0], &pt[1], &pt[2], &pt[3], &pt[4], &pt[5]);
  return result;
}
// 地址转换为uint64_t
static uint64_t macToUint64(const uint8_t mac[6])
{
  uint64_t result = 0;
  memcpy(&result, mac, 6);
  return result;
}

Device::Device()
    : id(0), battery(100), rssi(0),
      bleConnected(false), bleDoConnect(true), bleMAC{0},
      bleHandle(0), bleChannel(nullptr),
      wifiConnected(false), wifiMAC{0}, wifiIP(0) {}

Device::Device(uint8_t battery, int8_t rssi,
               bool bleConnected, bool bleDoConnect, uint8_t *bleMAC, uint16_t bleHandle, ble_l2cap_chan *bleChannel,
               bool wifiConnected, uint8_t *wifiMAC, uint32_t wifiIP)
    : id(0), battery(battery), rssi(rssi),
      bleConnected(bleConnected), bleDoConnect(bleDoConnect), bleMAC{0},
      bleHandle(bleHandle), bleChannel(bleChannel),
      wifiConnected(wifiConnected), wifiMAC{0}, wifiIP(wifiIP)
{
  memcpy(this->bleMAC, bleMAC, 6);
  memcpy(this->wifiMAC, wifiMAC, 6);
}

Device::Device(const Device &other)
    : id(other.id), battery(other.battery), rssi(other.rssi),
      bleConnected(other.bleConnected), bleDoConnect(other.bleDoConnect), bleHandle(other.bleHandle),
      bleChannel(other.bleChannel),
      wifiConnected(other.wifiConnected), wifiIP(other.wifiIP)
{
  memcpy(this->bleMAC, other.bleMAC, 6);
  memcpy(this->wifiMAC, other.wifiMAC, 6);
}

Device::Device(Device &&other) noexcept
    : id(other.id), battery(other.battery), rssi(other.rssi),
      bleConnected(other.bleConnected), bleDoConnect(other.bleDoConnect), bleHandle(other.bleHandle),
      bleChannel(other.bleChannel),
      wifiConnected(other.wifiConnected), wifiIP(other.wifiIP)
{
  memcpy(this->bleMAC, other.bleMAC, 6);
  memcpy(this->wifiMAC, other.wifiMAC, 6);
}

Device &Device::operator=(const Device &other)
{
  if (this != &other)
  {
    id = other.id;
    battery = other.battery;
    rssi = other.rssi;
    bleConnected = other.bleConnected;
    bleDoConnect = other.bleDoConnect;
    memcpy(this->bleMAC, other.bleMAC, 6);
    bleHandle = other.bleHandle;
    bleChannel = other.bleChannel;
    wifiConnected = other.wifiConnected;
    memcpy(this->wifiMAC, other.wifiMAC, 6);
    wifiIP = other.wifiIP;
  }
  return *this;
}

Device &Device::operator=(Device &&other) noexcept
{
  if (this != &other)
  {
    id = other.id;
    battery = other.battery;
    rssi = other.rssi;
    bleConnected = other.bleConnected;
    bleDoConnect = other.bleDoConnect;
    memcpy(this->bleMAC, other.bleMAC, 6);
    bleHandle = other.bleHandle;
    bleChannel = other.bleChannel;
    wifiConnected = other.wifiConnected;
    memcpy(this->wifiMAC, other.wifiMAC, 6);
    wifiIP = other.wifiIP;
  }
  return *this;
}

// 每个参数的getter和setter方法
uint8_t Device::getId() const
{
  return id;
}
uint8_t Device::getBattery() const
{
  return battery;
}
void Device::setBattery(uint8_t battery)
{
  this->battery = battery;
}
int8_t Device::getRssi() const
{
  return rssi;
}
void Device::setRssi(int8_t rssi)
{
  this->rssi = rssi;
}
bool Device::isBleConnected() const
{
  return bleConnected;
}
void Device::setBleConnected(bool connected)
{
  bleConnected = connected;
}
bool Device::isBleDoConnect() const
{
  return bleDoConnect;
}
void Device::setBleDoConnect(bool doConnect)
{
  bleDoConnect = doConnect;
}
uint8_t *Device::getBleMAC()
{
  return bleMAC;
}
void Device::setBleMAC(uint8_t *mac)
{
  memcpy(bleMAC, mac, 6);
}
uint16_t Device::getBleHandle() const
{
  return bleHandle;
}
void Device::setBleHandle(uint16_t handle)
{
  bleHandle = handle;
}
ble_l2cap_chan *Device::getBleChannel() const
{
  return bleChannel;
}
void Device::setBleChannel(ble_l2cap_chan *channel)
{
  bleChannel = channel;
}
bool Device::isWifiConnected() const
{
  return wifiConnected;
}
void Device::setWifiConnected(bool connected)
{
  wifiConnected = connected;
}
uint8_t *Device::getWifiMAC()
{
  return wifiMAC;
}
void Device::setWifiMAC(uint8_t *mac)
{
  memcpy(wifiMAC, mac, 6);
}
uint32_t Device::getWifiIP() const
{
  return wifiIP;
}
void Device::setWifiIP(uint32_t ip)
{
  wifiIP = ip;
}
std::string Device::getBleMACString()
{
  return macToStr(bleMAC);
}
std::string Device::getWifiMACString()
{
  return macToStr(wifiMAC);
}
std::string Device::getWifiIPString() const
{
  uint8_t bytes[4];
  bytes[0] = (wifiIP >> 24) & 0xFF;
  bytes[1] = (wifiIP >> 16) & 0xFF;
  bytes[2] = (wifiIP >> 8) & 0xFF;
  bytes[3] = wifiIP & 0xFF;
  char ip_str[16];
  snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
           bytes[0], bytes[1], bytes[2], bytes[3]);
  return std::string(ip_str);
}
void Device::print()
{
  LOGGER_INFO("Device{%d}: Battery=%d%%, RSSI=%d dBm, BLE Connected=%s, BLE MAC=%s, BLE Handle=%d, WiFi Connected=%s, WiFi MAC=%s, WiFi IP=%s",
                  id, battery, rssi,
                  bleConnected ? "Yes" : "No", getBleMACString().c_str(), bleHandle,
                  wifiConnected ? "Yes" : "No", getWifiMACString().c_str(), getWifiIPString().c_str());
}

Device *DeviceManager::addDevice(uint8_t bleMAC[6])
{
  if (deviceCount < RF_MAX_CONNECTION)
  {
    Device *newDevice = &devices[deviceCount++];
    newDevice->setBleMAC(bleMAC);
    bleMACToDevice[macToUint64(bleMAC)] = newDevice;
    return newDevice;
  }
  return nullptr;
}

Device *DeviceManager::getDeviceByBleMAC(uint8_t bleMAC[6])
{
  if (bleMACToDevice.contains(macToUint64(bleMAC)))
  {
    return bleMACToDevice[macToUint64(bleMAC)];
  }
  return nullptr;
}

Device *DeviceManager::getDeviceByBleMAC(const std::string bleMAC)
{
  uint64_t mac = strToMac(bleMAC);
  return getDeviceByBleMAC((uint8_t *)&mac);
}

Device *DeviceManager::getDeviceByWifiMAC(uint8_t wifiMAC[6])
{
  if (wifiMACToDevice.contains(macToUint64(wifiMAC)))
  {
    return wifiMACToDevice[macToUint64(wifiMAC)];
  }
  return nullptr;
}

Device *DeviceManager::getDeviceByWifiMAC(const std::string wifiMAC)
{
  uint64_t mac = strToMac(wifiMAC);
  return getDeviceByWifiMAC((uint8_t *)&mac);
}

Device *DeviceManager::getDeviceByWifiIP(uint32_t wifiIP)
{
  if (wifiIPToDevice.contains(wifiIP))
  {
    return wifiIPToDevice[wifiIP];
  }
  return nullptr;
}

void DeviceManager::bindDevice(uint8_t bleMAC[6], uint8_t wifiMAC[6])
{
  Device *device = getDeviceByBleMAC(bleMAC);
  if (device != nullptr)
  {
    wifiMACToDevice[macToUint64(wifiMAC)] = device;
    device->setWifiMAC(wifiMAC);
    if (wifiMACToIP.contains(macToUint64(wifiMAC)))
    {
      device->setWifiIP(wifiMACToIP[macToUint64(wifiMAC)]);
    }
  }
}

void DeviceManager::bindWifiMACToIP(uint8_t wifiMAC[6], uint32_t wifiIP)
{
  wifiMACToIP[macToUint64(wifiMAC)] = wifiIP;
  Device *device = getDeviceByWifiMAC(wifiMAC);
  if (device != nullptr)
  {
    device->setWifiIP(wifiIP);
  }
}

Device *DeviceManager::getAllDevices()
{
  return devices;
}

void DeviceManager::clear()
{
  deviceCount = 0;
  bleMACToDevice.clear();
  wifiMACToDevice.clear();
  wifiIPToDevice.clear();
}

uint8_t DeviceManager::size() const
{
  return deviceCount;
}