#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/components/output/binary_output.h"
#include "esphome/core/component.h"

namespace esphome {
namespace bk7670_sms {

class BK7670SMS : public uart::UARTDevice, public Component {
 public:
  void set_gpio_ad(output::BinaryOutput *out) { this->gpio_ad_ = out; }
  void set_gpio_he(output::BinaryOutput *out) { this->gpio_he_ = out; }
  void set_gpio_hg(output::BinaryOutput *out) { this->gpio_hg_ = out; }

  void loop() override;
  void send_sms(const std::string &number, const std::string &text);

 protected:
  output::BinaryOutput *gpio_ad_{nullptr};
  output::BinaryOutput *gpio_he_{nullptr};
  output::BinaryOutput *gpio_hg_{nullptr};
};

}  // namespace bk7670_sms
}  // namespace esphome
