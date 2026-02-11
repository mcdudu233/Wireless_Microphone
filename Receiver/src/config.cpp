#include "logger.h"
#include "config.h"

#include "nvs_flash.h"
#include "Preferences.h"

config::ConfigValue config::config;

static Preferences prefs;

void config::setup()
{
  LOGGER_INFO("Config is starting...");

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
    if (partition != NULL)
    {
      err = esp_partition_erase_range(partition, 0, partition->size);
      if (!err)
      {
        err = nvs_flash_init();
      }
      else
      {
        LOGGER_WARN("Config failed to format the broken NVS partition!");
      }
    }
    else
    {
      LOGGER_WARN("Config could not find NVS partition");
    }
  }
  if (err)
  {
    LOGGER_WARN("Config failed to initialize NVS! Error: %u", err);
  }

  prefs.begin(CONFIG_NAME);

  if (!prefs.isKey(CONFIG_VERSION_NAME))
  {
    // 不存则新建配置
    prefs.putUShort(CONFIG_VERSION_NAME, CONFIG_VERSION_VALUE);
    prefs.putBytes(CONFIG_DATA_NAME, &config, sizeof(ConfigValue));
  }
  else
  {
    if (prefs.getUShort(CONFIG_VERSION_NAME) != CONFIG_VERSION_VALUE)
    {
      // 版本不一致重置配置
      prefs.clear();
      prefs.putUShort(CONFIG_VERSION_NAME, CONFIG_VERSION_VALUE);
      prefs.putBytes(CONFIG_DATA_NAME, &config, sizeof(ConfigValue));
    }
  }

  // 读取配置
  prefs.getBytes(CONFIG_DATA_NAME, &config, sizeof(ConfigValue));

  LOGGER_INFO("Config is started.");
}

void config::save()
{
  prefs.putBytes(CONFIG_DATA_NAME, &config, sizeof(ConfigValue));
}