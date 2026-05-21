#include "bk7670_sms.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bk7670_sms {

static const char *const TAG = "bk7670_sms";

void BK7670SMS::loop() {
  while (this->available()) {
    char c;
    this->read_byte(&c);
    // traiter si besoin
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

void BK7670SMS::modem_reboot() {
  if (this->gpio_powerkey_ != nullptr) {
    this->gpio_powerkey_->turn_on();
    delay(1500);
    this->gpio_powerkey_->turn_off();
  }
}

}  // namespace bk7670_sms
}  // namespace esphome
