#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/components/output/binary_output.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include <vector>
#include <string>
#include <queue>

namespace esphome {
namespace bk7670_sms {

class BK7670SMS : public uart::UARTDevice, public Component {
 public:
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

  // --- Ajouts pour lecture non bloquante et état SMS ---
  std::string uart_buffer_;               // buffer persistant pour la lecture UART
  bool awaiting_cmgr_body_{false};        // si la ligne suivante est le corps après +CMGR

  enum SmsState { SMS_IDLE, SMS_WAIT_PROMPT, SMS_SENDING } sms_state_{SMS_IDLE};
  unsigned long sms_ts_{0};               // timestamp pour timeout
  std::string sms_pending_number_;
  std::string sms_pending_text_;

  struct SmsItem { std::string number; std::string text; };
  std::queue<SmsItem> sms_queue_;
  unsigned long last_retry_ts_{0};
  unsigned long backoff_ms_{2000};

  // helpers
  void handle_line(const std::string &line);
  void start_send_sms_sequence();
  void queue_sms(const std::string &number, const std::string &text);
};

}  // namespace bk7670_sms
}  // namespace esphome
