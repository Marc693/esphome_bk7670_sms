#pragma once
#include "esphome.h"
#include <vector>
#include <string>

namespace esphome {
namespace bk7670_sms {

class BK7670SMS : public Component, public UARTDevice {
 public:
  BK7670SMS(UARTComponent *parent) : UARTDevice(parent) {}

  void set_pin_code(const std::string &pin) { pin_code = pin; }
  void set_acl_numbers(const std::vector<std::string> &acl) { acl_numbers = acl; }

  void set_gpio_ad(GPIOPin *p) { gpio_ad = p; }
  void set_gpio_he(GPIOPin *p) { gpio_he = p; }
  void set_gpio_hg(GPIOPin *p) { gpio_hg = p; }
  void set_gpio_powerkey(GPIOPin *p) { gpio_powerkey = p; }

  void setup() override;
  void loop() override;

  void send_sms(const std::string &number, const std::string &message);
  void modem_reboot();

 protected:
  std::vector<std::string> acl_numbers;
  std::string pin_code;

  GPIOPin *gpio_ad = nullptr;
  GPIOPin *gpio_he = nullptr;
  GPIOPin *gpio_hg = nullptr;
  GPIOPin *gpio_powerkey = nullptr;

  std::string rx_buffer;

  void process_line(const std::string &line);
};

}  // namespace bk7670_sms
}  // namespace esphome
