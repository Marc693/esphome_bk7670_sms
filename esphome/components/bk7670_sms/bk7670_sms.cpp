#include "bk7670_sms.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bk7670_sms {

static const char *const TAG = "bk7670_sms";

void BK7670SMS::loop() {
  ESP_LOGD(TAG, "loop running");

  while (this->available()) {
    uint8_t c;
    this->read_byte(&c);
    ESP_LOGD(TAG, "RX: %c (0x%02X)", c, c);
  }
}

void BK7670SMS::send_sms(const std::string &number, const std::string &text) {
  this->write_str("AT+CMGF=1\r\n");
  delay(200);

  this->write_str("AT+CMGS=\"");
  this->write_str(number.c_str());
  this->write_str("\"\r\n");
  delay(200);

  this->write_str(text.c_str());
  this->write_byte(26);  // CTRL+Z
}

}  // namespace bk7670_sms
}  // namespace esphome
