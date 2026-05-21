#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/components/output/binary_output.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include <vector>

namespace esphome {
namespace bk7670_sms {

class BK7670SMS : public uart::UARTDevice, public Component {
 public:
  // Constructeur conforme à UARTDevice
  BK7670SMS(uart::UARTComponent *parent) : uart::UARTDevice(parent) {}

  void set_gpio_ad(output::BinaryOutput *out) { this->gpio_ad_ = out; }
  void set_gpio_he(output::BinaryOutput *out) { this->gpio_he_ = out; }
  void set_gpio_hg(output::BinaryOutput *out) { this->gpio_hg_ = out; }

  void set_pin_code(const std::string &pin) { this->pin_code_ = pin; }
  void add_acl_number(const std::string &num) { this->acl_numbers_.push_back(num); }

  void loop() override;
  void send_sms(const std::string &number, const std::string &text);

  // Traitement des SMS entrants
  void process_sms(const std::string &sender, const std::string &body);

  // Commandes logiques
  void do_arm();
  void do_disarm();
  void do_status(const std::string &sender);

 protected:
  output::BinaryOutput *gpio_ad_{nullptr};
  output::BinaryOutput *gpio_he_{nullptr};
  output::BinaryOutput *gpio_hg_{nullptr};

  std::string pin_code_{""};
  std::vector<std::string> acl_numbers_;
};

}  // namespace bk7670_sms
}  // namespace esphome
