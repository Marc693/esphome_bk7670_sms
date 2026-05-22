#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/components/output/binary_output.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include <vector>
#include <queue>

namespace esphome {
namespace bk7670_sms {

static const char *const TAG = "bk7670_sms";

class BK7670SMS : public uart::UARTDevice, public Component {
 public:
  // Constructeur conforme à UARTDevice
  BK7670SMS(uart::UARTComponent *parent) : uart::UARTDevice(parent) {}

  // GPIO pilotés par commandes SMS
  void set_gpio_ad(output::BinaryOutput *out) { this->gpio_ad_ = out; }
  void set_gpio_he(output::BinaryOutput *out) { this->gpio_he_ = out; }
  void set_gpio_hg(output::BinaryOutput *out) { this->gpio_hg_ = out; }

  // Sécurité
  void set_pin_code(const std::string &pin) { this->pin_code_ = pin; }
  void add_acl_number(const std::string &num) { this->acl_numbers_.push_back(num); }

  // Sensors HA
  void set_incoming_sms_sensor(text_sensor::TextSensor *s) { this->incoming_sms_sensor_ = s; }
  void set_last_sender_sensor(text_sensor::TextSensor *s) { this->last_sender_sensor_ = s; }
  void set_last_cmd_sensor(text_sensor::TextSensor *s) { this->last_cmd_sensor_ = s; }

  // Boucle principale
  void loop() override;

  // Envoi AT sécurisé (file d’attente)
  void queue_at(const std::string &cmd);

  // Envoi SMS sortant
  void send_sms(const std::string &number, const std::string &text);

  // Traitement SMS entrants
  void process_sms(const std::string &sender, const std::string &body);

  // Commandes logiques
  void do_arm();
  void do_disarm();
  void do_status(const std::string &sender);

 protected:
  // GPIO
  output::BinaryOutput *gpio_ad_{nullptr};
  output::BinaryOutput *gpio_he_{nullptr};
  output::BinaryOutput *gpio_hg_{nullptr};

  // Sécurité
  std::string pin_code_{""};
  std::vector<std::string> acl_numbers_;

  // Buffers UART
  std::string rx_buffer_;
  uint32_t last_uart_activity_{0};

  // File d’attente AT
  std::queue<std::string> at_queue_;
  bool at_busy_{false};
  uint32_t at_last_send_{0};

  // Watchdog modem
  uint32_t last_modem_ok_{0};
  int reboot_count_{0};

  // Sensors HA
  text_sensor::TextSensor *incoming_sms_sensor_{nullptr};
  text_sensor::TextSensor *last_sender_sensor_{nullptr};
  text_sensor::TextSensor *last_cmd_sensor_{nullptr};

  // Méthodes internes
  void process_line_(const std::string &line);
  void handle_cmti_(const std::string &line);
  void read_sms_index_(int index);
  void parse_cmgr_(const std::string &header, const std::string &body);
  bool is_acl_allowed_(const std::string &num);
  bool check_pin_(const std::string &body);
};

}  // namespace bk7670_sms
}  // namespace esphome
