#include "bk7670_sms.h"

namespace esphome {
namespace bk7670_sms {

void BK7670SMS::setup() {
  rx_buffer.clear();
  ESP_LOGI("bk7670_sms", "BK7670 SMS component initialized");
}

void BK7670SMS::loop() {
  while (available()) {
    char c = read();
    if (c == '\n') {
      process_line(rx_buffer);
      rx_buffer.clear();
    } else if (c != '\r') {
      rx_buffer.push_back(c);
    }
  }
}

void BK7670SMS::send_sms(const std::string &number, const std::string &message) {
  write_str("AT+CMGF=1\r\n");
  delay(200);
  write_str(("AT+CMGS=\"" + number + "\"\r\n").c_str());
  delay(200);
  write_str((message + "\x1A").c_str());
  ESP_LOGI("bk7670_sms", "SMS envoyé à %s", number.c_str());
}

void BK7670SMS::modem_reboot() {
  if (!gpio_powerkey) return;

  ESP_LOGW("bk7670_sms", "Reboot modem...");
  gpio_powerkey->digital_write(true);
  delay(1500);
  gpio_powerkey->digital_write(false);
}

void BK7670SMS::process_line(const std::string &line) {
  ESP_LOGD("bk7670_sms", "RX: %s", line.c_str());
}

}  // namespace bk7670_sms
}  // namespace esphome
