#include "logger.h"
#include "config.h"
#include "module/rf.h"
#include "module/led.h"
#include "module/power.h"
#include "module/audio/encoder.h"
#include "module/audio/buffer.h"

#include "queue"

/*****************************
          传输层协议
*****************************/
#include "lwip/err.h"
#include "lwip/api.h"
static bool socketIsOpen = false;
static netconn *socketSendInstance = NULL;
static netconn *socketReceiveInstance = NULL;
static ip_addr_t socketDestination;

// 发送数据 需要 netbuf_new
static bool socket_send(netbuf *buf)
{
  err_t err = netconn_sendto(socketSendInstance, buf, &socketDestination, WIFI_NO_PORT);
  if (err != ERR_OK)
  {
    logger::warnln("Socket send failed: %d", err);
    return false;
  }
  // 释放
  netbuf_delete(buf);
  return true;
}

// 读取数据 需要 netbuf_delete
static bool socket_receive(netbuf **buf)
{
  err_t err = netconn_recv(socketReceiveInstance, buf);
  if (err == ERR_WOULDBLOCK)
  {
    return false;
  }
  else if (err != ERR_OK)
  {
    logger::warnln("Socket receive failed: %d", err);
    return false;
  }
  return true;
}

static bool socket_close()
{
  if (socketIsOpen)
  {
    socketIsOpen = false;
    if (socketSendInstance != NULL)
    {
      netconn_delete(socketSendInstance);
      socketSendInstance = NULL;
    }
    if (socketReceiveInstance != NULL)
    {
      netconn_delete(socketReceiveInstance);
      socketReceiveInstance = NULL;
    }
  }
  logger::debugln("Socket is shutdown.");
  return true;
}

static bool socket_open(uint32_t localIP, uint32_t destIP)
{
  if (socketIsOpen)
  {
    socket_close();
  }

  // 创建
  socketSendInstance = netconn_new_with_proto_and_callback(NETCONN_RAW, WIFI_IP_PROTOCOL, NULL);
  if (socketSendInstance == NULL)
  {
    logger::warnln("Socket unable to create:!");
    return false;
  }
  socketReceiveInstance = netconn_new_with_proto_and_callback(NETCONN_RAW, WIFI_IP_PROTOCOL, NULL);
  if (socketReceiveInstance == NULL)
  {
    netconn_delete(socketSendInstance);
    logger::warnln("Socket unable to create:!");
    return false;
  }

  // 绑定到指定地址
  ip_addr_t local_ip = {.addr = localIP};
  err_t ret = netconn_bind(socketSendInstance, &local_ip, WIFI_NO_PORT);
  if (ret != ERR_OK)
  {
    netconn_delete(socketSendInstance);
    socketSendInstance = NULL;
    netconn_delete(socketReceiveInstance);
    socketReceiveInstance = NULL;
    logger::warnln("Socket netconn bind failed: %d", ret);
    return false;
  }
  ret = netconn_bind(socketReceiveInstance, IP_ADDR_ANY, WIFI_NO_PORT);
  if (ret != ERR_OK)
  {
    netconn_delete(socketSendInstance);
    socketSendInstance = NULL;
    netconn_delete(socketReceiveInstance);
    socketReceiveInstance = NULL;
    logger::warnln("Socket netconn bind failed: %d", ret);
    return false;
  }
  // 远程地址
  socketDestination.addr = destIP;

  // 设置非阻塞模式
  netconn_set_nonblocking(socketReceiveInstance, true);

  socketIsOpen = true;
  logger::debugln("Socket is started.");
  return true;
}
/****************************/

/*****************************
          WIFI协议
*****************************/
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
static bool wifiIsOpen = false;
static uint8_t wifiRetryTime = 0;
static uint8_t wifiMAC[6] = {0};
static uint32_t wifiIP;
static uint32_t wifiGatewayIP;
static esp_netif_t *wifiNetIF;
static EventGroupHandle_t wifiEventGroup;
static esp_event_handler_instance_t wifiHandleInstance1;
static esp_event_handler_instance_t wifiHandleInstance2;
static const wifi_init_config_t wifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();
static wifi_config_t wifiConfig;

static bool wifi_close();
static bool ble_open();

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT)
  {
    if (event_id == WIFI_EVENT_STA_START)
    {
      wifiRetryTime = 0;
      esp_wifi_connect();
      logger::debugln("WiFi is starting to connect.");
    }
    else if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
      logger::debugln("WiFi success to connect.");
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
      logger::warnln("WiFi failed to connect.");
      if (wifiRetryTime < WIFI_RETRY)
      {
        esp_wifi_connect();
        wifiRetryTime++;
        logger::warnln("WiFi retry to connect to the AP.");
      }
      else
      {
        if (wifiIsOpen)
        {
          // TODO: 断开连接直接重启
          power::core_restart();
        }
        else
        {
          xEventGroupSetBits(wifiEventGroup, WIFI_FAIL_BIT);
          logger::warnln("WiFi connect to the AP fail!");
        }
      }
    }
  }
  else if (event_base == IP_EVENT)
  {
    if (event_id == IP_EVENT_STA_GOT_IP)
    {
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
      wifiIP = event->ip_info.ip.addr;
      wifiGatewayIP = event->ip_info.gw.addr;
      logger::debugln("WiFi got ip:" IPSTR, IP2STR(&event->ip_info.ip));
      xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
    }
    else if (event_id == IP_EVENT_STA_LOST_IP)
    {
      wifiIP = 0;
      wifiGatewayIP = 0;
    }
  }
}

static bool wifi_close()
{
  if (wifiIsOpen)
  {
    socket_close();
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifiHandleInstance1));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifiHandleInstance2));
    ESP_ERROR_CHECK(esp_wifi_deinit());
    esp_netif_destroy(wifiNetIF);
    vEventGroupDelete(wifiEventGroup);
    wifiNetIF = NULL;
    wifiIsOpen = false;
  }
  logger::debugln("WiFi is shutdown.");
  return true;
}

static bool wifi_open(const char *ssid, const char *password)
{
  if (wifiIsOpen)
  {
    wifi_close();
  }

  wifiEventGroup = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifiNetIF = esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = wifiInitConfig;
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &wifiHandleInstance1));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &wifiHandleInstance2));

  strcpy((char *)wifiConfig.sta.ssid, ssid);
  strcpy((char *)wifiConfig.sta.password, password);
  wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
  ESP_ERROR_CHECK(esp_wifi_start());

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
   * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(wifiEventGroup,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE,
                                         pdFALSE,
                                         portMAX_DELAY);

  wifiIsOpen = true;
  logger::debugln("WiFi is started for local IP %d.%d.%d.%d, gateway IP %d.%d.%d.%d.",
                  ((uint8_t *)&wifiIP)[0], ((uint8_t *)&wifiIP)[1], ((uint8_t *)&wifiIP)[2], ((uint8_t *)&wifiIP)[3],
                  ((uint8_t *)&wifiGatewayIP)[0], ((uint8_t *)&wifiGatewayIP)[1], ((uint8_t *)&wifiGatewayIP)[2], ((uint8_t *)&wifiGatewayIP)[3]);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
   * happened. */
  if (bits & WIFI_CONNECTED_BIT)
  {
    esp_read_mac(wifiMAC, ESP_MAC_WIFI_STA);
    socket_open(wifiIP, wifiGatewayIP);
    return true;
  }
  else
  {
    return false;
  }
}
/****************************/

/*****************************
          BLE协议
*****************************/
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

static bool bleIsOpen = false;
static bool bleIsClosing = false;
static bool bleIsAdvertising = false;
static uint8_t bleMAC[6] = {0};
static uint16_t bleConnectionHandle = 0;
static ble_l2cap_chan *bleChannel = NULL;
// 储存池
#define BLE_L2CAP_COC_BUF_COUNT (20 * MYNEWT_VAL(BLE_L2CAP_COC_MAX_NUM))
static os_membuf_t bleMemory[OS_MEMPOOL_SIZE(BLE_L2CAP_COC_BUF_COUNT, BLE_L2CAP_MTU)];
static os_mempool bleMemoryPool;
static os_mbuf_pool bleBufferpool;
static std::queue<uint8_t *> bleReceive;

static void ble_start_advertising();
static void ble_stop_advertising();

static int ble_l2cap_handler(struct ble_l2cap_event *event, void *arg)
{
  int rc;
  switch (event->type)
  {
  // 连接事件
  case BLE_L2CAP_EVENT_COC_CONNECTED:
  {
    if (event->connect.status == 0)
    {
      ble_l2cap_chan_info chan_info;
      rc = ble_l2cap_get_chan_info(event->connect.chan, &chan_info);
      if (rc != 0)
      {
        logger::warnln("Failed to get L2CAP channel info: %d", rc);
        break;
      }
      if (chan_info.psm == BLE_L2CAP_PSM)
      {
        bleChannel = event->connect.chan;
        logger::debugln("BLE channel connected.");
      }
    }
    else
    {
      logger::warnln("BLE L2CAP COC error: %d", event->connect.status);
    }
    break;
  }

  // 断开连接事件
  case BLE_L2CAP_EVENT_COC_DISCONNECTED:
  {
    ble_l2cap_chan_info chan_info;
    rc = ble_l2cap_get_chan_info(event->disconnect.chan, &chan_info);
    if (rc != 0)
    {
      logger::warnln("Failed to get L2CAP channel info: %d", rc);
      break;
    }
    if (chan_info.psm == BLE_L2CAP_PSM)
    {
      bleChannel == NULL;
      logger::debugln("BLE channel disconnected.");
    }
    break;
  }

  // 接收连接事件
  case BLE_L2CAP_EVENT_COC_ACCEPT:
  {
    ble_l2cap_chan_info chan_info;
    rc = ble_l2cap_get_chan_info(event->accept.chan, &chan_info);
    if (rc != 0)
    {
      logger::warnln("Failed to get L2CAP channel info: %d", rc);
      break;
    }
    // 接受连接
    os_mbuf *sdu_rx;
    sdu_rx = os_mbuf_get_pkthdr(&bleBufferpool, 0);
    if (!sdu_rx)
    {
      logger::warnln("BLE L2CAP accept no memory!");
      break;
    }
    rc = ble_l2cap_recv_ready(event->accept.chan, sdu_rx);
    if (rc != 0)
    {
      logger::warnln("BLE L2CAP accept failed!");
      break;
    }
    logger::debugln("BLE L2CAP accept request for psm=%d.", chan_info.psm);
    break;
  }

  // 接收到数据事件
  case BLE_L2CAP_EVENT_COC_DATA_RECEIVED:
  {
    if (event->receive.sdu_rx != NULL)
    {
      uint16_t data_len = OS_MBUF_PKTLEN(event->receive.sdu_rx);
      // 放进接收队列
      uint8_t *packet = (uint8_t *)heap_caps_malloc(event->receive.sdu_rx->om_len, MALLOC_CAP_SPIRAM);
      memcpy(packet, event->receive.sdu_rx->om_data, event->receive.sdu_rx->om_len);
      bleReceive.push(packet);
      os_mbuf_free(event->receive.sdu_rx);
      logger::debugln("BLE received %d bytes on L2CAP channel.", event->receive.sdu_rx->om_len);
    }

    // 响应数据 准备接收下一个数据包
    os_mbuf *sdu_rx;
    sdu_rx = os_mbuf_get_pkthdr(&bleBufferpool, 0);
    if (!sdu_rx)
    {
      logger::warnln("BLE L2CAP accept no memory!");
      break;
    }
    rc = ble_l2cap_recv_ready(event->receive.chan, sdu_rx);
    if (rc != 0)
    {
      logger::warnln("BLE L2CAP accept failed!");
      break;
    }
    break;
  }

  default:
  {
    break;
  }
  }
  return 0;
}

static int ble_gap_handler(struct ble_gap_event *event, void *arg)
{
  int rc;
  switch (event->type)
  {
  // 连接事件
  case BLE_GAP_EVENT_CONNECT:
  {
    if (event->connect.status == 0)
    {
      // 连接成功
      bleConnectionHandle = event->connect.conn_handle;
      // 创建 L2CAP 服务器
      rc = ble_l2cap_create_server(BLE_L2CAP_PSM, BLE_L2CAP_MTU, ble_l2cap_handler, NULL);

      if (rc != 0)
      {
        logger::warnln("BLE failed to create config L2CAP server: %d", rc);
      }
      ble_stop_advertising();
      logger::debugln("BLE connected, conn_handle: %d", event->connect.conn_handle);
    }
    else
    {
      // 连接失败
      ble_start_advertising();
      logger::warnln("BLE connected failed! status %d", event->connect.status);
    }
    break;
  }

  // 断开连接事件
  case BLE_GAP_EVENT_DISCONNECT:
  {
    bleConnectionHandle = BLE_HS_CONN_HANDLE_NONE;
    // 清理L2CAP通道
    bleChannel = NULL;
    if (!bleIsClosing && bleIsOpen)
    {
      ble_start_advertising();
    }
    logger::debugln("BLE disconnected. reason=%d", event->disconnect.reason);
    break;
  }

  // 蓝牙广告停止事件
  case BLE_GAP_EVENT_ADV_COMPLETE:
  {
    // 重新开始广告
    ble_start_advertising();
    logger::debugln("BLE restart to advertising!");
    break;
  }

  default:
  {
    break;
  }
  }
  return 0;
}

// 重置
static void ble_on_reset(int reason)
{
  logger::warnln("BLE reset, reason: %d", reason);
  ble_stop_advertising();
  bleConnectionHandle = BLE_HS_CONN_HANDLE_NONE;
  bleChannel = NULL;
  bleIsOpen = false;
}

// NimBLE 协议栈加载完成
static void ble_on_sync(void)
{
  int rc;

  // 获取MAC地址
  esp_read_mac(bleMAC, ESP_MAC_BT);

  // 设置设备名称
  rc = ble_svc_gap_device_name_set(BLE_NAME);
  if (rc != 0)
  {
    logger::warnln("BLE set device name failed! reason=%d", rc);
    return;
  }

  // 开始广告
  ble_start_advertising();
}

void ble_host_task(void *param)
{
  logger::debugln("BLE Host Task Started.");
  /* This function will return only when nimble_port_stop() is executed */
  nimble_port_run();
  nimble_port_freertos_deinit();
}

static void ble_stop_advertising()
{
  if (bleIsAdvertising)
  {
    led::black();
    ble_gap_adv_stop();
    bleIsAdvertising = false;
  }
}

static void ble_start_advertising()
{
  if (!bleIsAdvertising)
  {
    led::blue();

    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name;
    int rc;

    /**
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info).
     *     o Advertising tx power.
     *     o Device name.
     *     o 16-bit service UUIDs (alert notifications).
     */
    memset(&fields, 0, sizeof(fields));
    // 配置设备名称
    name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;
    // 配置发射功率
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    // 配置标志:
    // o Discoverability in forthcoming advertisement (general)
    // o BLE-only (BR/EDR unsupported).
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0)
    {
      logger::warnln("BLE error setting advertisement data! rc=%d\n", rc);
      return;
    }

    // 开始广告
    memset(&adv_params, 0, sizeof(adv_params));
    // adv_params.itvl_min = 0x00A0;
    // adv_params.itvl_max = 0x00B0;
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_handler, NULL);
    if (rc != 0)
    {
      logger::warnln("BLE error enabling advertisement! rc=%d\n", rc);
      return;
    }

    bleIsAdvertising = true;
  }
}

static bool ble_close()
{
  bleIsClosing = true;
  if (bleIsOpen)
  {
    int rc;
    // 停止广告
    ble_stop_advertising();

    // 停止NimBLE主机任务
    nimble_port_stop();
    if (rc != 0)
    {
      logger::warnln("NimBLE stop failed: %d", rc);
      bleIsClosing = false;
      return false;
    }
    // 反初始化NimBLE
    rc = nimble_port_deinit();
    if (rc != 0)
    {
      logger::warnln("NimBLE deinit failed: %d", rc);
      bleIsClosing = false;
      return false;
    }

    // 清理内存池os_error_t
    rc = os_mempool_clear(&bleMemoryPool);
    if (rc != OS_OK)
    {
      logger::warnln("NimBLE os_mempool_clear failed: %d", rc);
      bleIsClosing = false;
      return false;
    }

    // 清理全局状态
    bleConnectionHandle = BLE_HS_CONN_HANDLE_NONE;
    bleChannel = NULL;
    bleIsOpen = false;
  }
  bleIsClosing = false;
  logger::debugln("BLE is close.");
  return true;
}

static bool ble_open()
{
  if (bleIsOpen)
  {
    ble_close();
  }

  // 初始化 NimBLE
  esp_err_t ret = nimble_port_init();
  if (ret != ESP_OK)
  {
    logger::warnln("BLE port init failed: %d", ret);
    return false;
  }

  // 初始化配置
  ble_hs_cfg.reset_cb = ble_on_reset;
  ble_hs_cfg.sync_cb = ble_on_sync;
  ble_hs_cfg.store_status_cb = NULL;

  // 创建接受池
  ret = os_mempool_init(&bleMemoryPool, BLE_L2CAP_COC_BUF_COUNT, BLE_L2CAP_MTU, bleMemory, "coc_sdu_pool");
  if (ret != 0)
  {
    logger::warnln("BLE os_mempool_init failed: %d", ret);
    return false;
  }
  ret = os_mbuf_pool_init(&bleBufferpool, &bleMemoryPool, BLE_L2CAP_MTU, BLE_L2CAP_COC_BUF_COUNT);
  if (ret != 0)
  {
    logger::warnln("BLE os_mbuf_pool_init failed: %d", ret);
    return false;
  }

  // 启动 NimBLE 主机任务
  nimble_port_freertos_init(ble_host_task);

  bleIsOpen = true;
  logger::debugln("BLE is started.");
  return true;
}

// 发送数据
bool ble_send(const uint8_t *data, uint16_t len)
{
  if (!bleIsOpen || !bleChannel)
  {
    logger::warnln("BLE channel not ready.");
    return false;
  }

  if (len > BLE_L2CAP_MTU)
  {
    logger::warnln("BLE data too large for L2CAP MTU.");
    return false;
  }

  struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
  if (!om)
  {
    logger::warnln("BLE failed to allocate mbuf.");
    return false;
  }

  int rc = ble_l2cap_send(bleChannel, om);
  if (rc != 0)
  {
    logger::warnln("BLE failed to send data: %d", rc);
    os_mbuf_free_chain(om);
    return false;
  }

  logger::debugln("BLE sent %d bytes.", len);
  return true;
}
/****************************/

// 解析数据包
static void rf_receive_packet(const uint8_t *data)
{
  Packet *packet = (Packet *)data;
  switch (packet->type)
  {
  // 服务器响应ACK
  case PACKET_TYPE_SERVER_ACK:
  {
    break;
  }

  // 配置设备
  case PACKET_TYPE_SERVER_CONTROL_DEVICE:
  {
    logger::debugln("RF get config control.");
    ServerControlDevicePacket *src = &packet->packet.serverControlDevice;
    if (config::status.device.mode != src->mode)
    {
      config::status.device.mode = src->mode;
    }
    if (strcmp(config::status.device.name, src->name) != 0)
    {
      strcpy(config::status.device.name, src->name);
    }
    if (strcmp(config::status.device.password, src->password) != 0)
    {
      strcpy(config::status.device.password, src->password);
    }
    if (config::status.device.startWiFi != src->startWiFi)
    {
      if (src->startWiFi)
      {
        if (wifi_open(config::status.device.name, config::status.device.password))
        {
          config::status.device.startWiFi = src->startWiFi;
        }
      }
      else
      {
        if (wifi_close())
        {
          config::status.device.startWiFi = src->startWiFi;
        }
      }
    }
    if (config::status.device.startBLE != src->startBLE)
    {
      if (src->startBLE)
      {
        if (ble_open())
        {
          config::status.device.startBLE = src->startBLE;
        }
      }
      else
      {
        if (ble_close())
        {
          config::status.device.startBLE = src->startBLE;
        }
      }
    }
    if (config::status.device.start != src->start)
    {
      config::status.device.start = src->start;
    }
    break;
  }

  // 配置音频
  case PACKET_TYPE_SERVER_CONTROL_AUDIO:
  {
    logger::debugln("BLE get audio control.");
    ServerControlAudioPacket *src = &packet->packet.serverControlAudio;
    if (config::status.audio.channel != src->channel)
    {
      config::status.audio.channel = src->channel;
      audio::encoder::setChannel(config::status.audio.channel);
    }
    if (config::status.audio.rate != src->rate)
    {
      config::status.audio.rate = src->rate;
      audio::encoder::setRate(config::status.audio.rate);
    }
    if (config::status.audio.bit != src->bit)
    {
      config::status.audio.bit = src->bit;
      audio::encoder::setBit(config::status.audio.bit);
    }
    if (config::status.audio.autoVolumn != src->autoVolumn)
    {
      config::status.audio.autoVolumn = src->autoVolumn;
      audio::encoder::setGain(config::status.audio.volumn);
    }
    if (config::status.audio.peekVolumn != src->peekVolumn)
    {
      config::status.audio.peekVolumn = src->peekVolumn;
      audio::encoder::setPeek(config::status.audio.peekVolumn);
    }
    if (config::status.audio.volumn != src->volumn)
    {
      config::status.audio.volumn = src->volumn;
      audio::encoder::setAuto(config::status.audio.autoVolumn);
    }
    if (config::status.audio.start != src->start)
    {
      config::status.audio.start = src->start;
      if (config::status.audio.start)
      {
        audio::encoder::setRate(config::status.audio.rate);
        audio::encoder::setChannel(config::status.audio.channel);
        audio::encoder::setBit(config::status.audio.bit);
        audio::encoder::on();
        audio::encoder::setPeek(config::status.audio.peekVolumn);
        audio::encoder::setAuto(config::status.audio.autoVolumn);
        audio::encoder::setGain(config::status.audio.volumn);
      }
      else
      {
        audio::encoder::off();
      }
    }
    break;
  }

  default:
  {
    logger::warnln("RF unknow packet type=%d", packet->type);
    break;
  }
  }
}

static void rf_handle(void *arg)
{
  // 缓存
  netbuf *receiveBuffer = NULL;
  netbuf **sendBuffer = NULL;
  // 定时发送设备状态
  uint16_t statusNumber = 0;

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(TASK_RF_PERIOD);
  while (true)
  {
    xTaskDelayUntil(&xLastWakeTime, xFrequency);

    /* 处理 BLE 模块 */
    if (bleIsOpen && bleChannel != NULL)
    {
      /* 发送 */
      if (config::status.device.start && !config::status.device.mode)
      {
      }

      /* 接收 */
      while (!bleReceive.empty())
      {
        uint8_t *packet = bleReceive.front();
        // 解析数据包
        rf_receive_packet(packet);
        heap_caps_free(packet);
        bleReceive.pop();
      }
    }

    /* 处理 WIFI 模块 */
    if (wifiIsOpen && socketIsOpen)
    {
      /* 发送 */
      if (config::status.device.start && config::status.device.mode)
      {
        uint8_t size = audio::buffer::getWiFiPacketFront(&sendBuffer);
        for (int part = 0; part < size; part++)
        {
          socket_send(sendBuffer[part]);
        }
        if (sendBuffer != NULL)
        {
          free(sendBuffer);
          sendBuffer = NULL;
        }
      }
      if (statusNumber++ == RF_CLIENT_STATUS_PERIOD)
      {
        // 发送状态包
        netbuf *buf = netbuf_new();
        if (buf != NULL)
        {
          Packet *packet = (Packet *)netbuf_alloc(buf, PACKET_CLIENT_STATUS_SIZE);
          packet->type = PACKET_TYPE_CLIENT_STATUS;
          memcpy(packet->packet.clientStatus.bleMAC, bleMAC, 6);
          memcpy(packet->packet.clientStatus.wifiMAC, wifiMAC, 6);
          packet->packet.clientStatus.battery = (uint8_t)power::getBATPercent();
          socket_send(buf);
        }
        statusNumber = 0;
      }

      /* 接收 */
      if (socket_receive(&receiveBuffer))
      {
        uint8_t *data;
        uint16_t len;
        do
        {
          netbuf_data(receiveBuffer, (void **)&data, &len);
          data += WIFI_IP_HEAD_LEN;
          len -= WIFI_IP_HEAD_LEN;
          // 解析数据包
          rf_receive_packet(data);

        } while (netbuf_next(receiveBuffer) >= 0);
        netbuf_delete(receiveBuffer);
      }
    }
  }
}

void rf::setup()
{
  ble_open();
  // 启动发送接收线程
  xTaskCreatePinnedToCore(rf_handle, "rf_handle", TASK_RF_STACK, NULL, TASK_RF_PRIORITY, NULL, TASK_RF_CORE);
}