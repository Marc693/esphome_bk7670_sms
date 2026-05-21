#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace bk7670_sms {

class BK7670SMS : public uart::UARTDevice {
 public:
  BK7670SMS(uart::UARTComponent *parent) : uart::UARTDevice(parent) {}

  void loop() override;

  void set_gpio_ad(output::BinaryOutput *out) { this->gpio_ad_ = out; }
  void set_gpio_he(output::BinaryOutput *out) { this->gpio_he_ = out; }
  void set_gpio_hg(output::BinaryOutput *out) { this->gpio_hg_ = out; }
  void set_gpio_powerkey(output::BinaryOutput *out) { this->gpio_powerkey_ = out; }

  void send_sms(const std::string &number, const std::string &text);
  void modem_reboot();

 protected:
  output::BinaryOutput *gpio_ad_{nullptr};
  output::BinaryOutput *gpio_he_{nullptr};
  output::BinaryOutput *gpio_hg_{nullptr};
  output::BinaryOutput *gpio_powerkey_{nullptr};
};

}  // namespace bk7670_sms
}  // namespace esphome
