#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/components/output/binary_output.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include <vector>
#include <string>
#include <queue>
#include <functional>
#include <array>

namespace esphome {
namespace bk7670_sms {

class BK7670SMS : public uart::UARTDevice, public Component
#ifdef USE_API
    , public api::CustomAPIDevice
#endif
{
 public:
  BK7670SMS(uart::UARTComponent *parent) : uart::UARTDevice(parent) {}

  void setup() override;
  void loop() override;

  void set_gpio_ad(output::BinaryOutput *out) { this->gpio_ad_ = out; }
  void set_gpio_he(output::BinaryOutput *out) { this->gpio_he_ = out; }
  void set_gpio_hg(output::BinaryOutput *out) { this->gpio_hg_ = out; }
  void set_gpio_reset(output::BinaryOutput *out) { this->gpio_reset_ = out; }

  void set_input_sirene_int(binary_sensor::BinarySensor *sensor) { this->input_sirene_int_ = sensor; }
  void set_input_sirene_ext(binary_sensor::BinarySensor *sensor) { this->input_sirene_ext_ = sensor; }
  void set_input_armee_total(binary_sensor::BinarySensor *sensor) { this->input_armee_total_ = sensor; }
  void set_input_panne_secteur(binary_sensor::BinarySensor *sensor) { this->input_panne_secteur_ = sensor; }
  void set_input_alarme_declenchee(binary_sensor::BinarySensor *sensor) { this->input_alarme_declenchee_ = sensor; }
  void set_input_armee_partiel(binary_sensor::BinarySensor *sensor) { this->input_armee_partiel_ = sensor; }

  void send_at_command(std::string command);

  void set_pin_code(const std::string &pin) { this->pin_code_ = pin; }
  void add_acl_number(const std::string &num) { this->acl_numbers_.push_back(num); }

  void send_sms(const std::string &number, const std::string &text);
  void send_at_command(std::string command);
  void check_modem();
  void reset_modem();

  // Traitement des SMS entrants
  void process_sms(const std::string &sender, const std::string &body);

  // Commandes logiques
  void do_he();
  void do_hg();
  void do_desarm();
  void send_status(const std::string &sender);

 protected:
  output::BinaryOutput *gpio_ad_{nullptr};
  output::BinaryOutput *gpio_he_{nullptr};
  output::BinaryOutput *gpio_hg_{nullptr};

  binary_sensor::BinarySensor *input_sirene_int_{nullptr};
  binary_sensor::BinarySensor *input_sirene_ext_{nullptr};
  binary_sensor::BinarySensor *input_armee_total_{nullptr};
  binary_sensor::BinarySensor *input_panne_secteur_{nullptr};
  binary_sensor::BinarySensor *input_alarme_declenchee_{nullptr};
  binary_sensor::BinarySensor *input_armee_partiel_{nullptr};

  output::BinaryOutput *gpio_reset_{nullptr};
  std::string pin_code_{""};
  std::vector<std::string> acl_numbers_;

  bool gpio_reset_state_{false};
  unsigned long reset_end_ts_{0};

  // --- Ajouts pour lecture non bloquante et état SMS ---
  std::string uart_buffer_;               // buffer persistant pour la lecture UART
  bool awaiting_cmgr_body_{false};        // si la ligne suivante est le corps après +CMGR
  std::string last_sms_sender_;

  enum SmsState { SMS_IDLE, SMS_WAIT_PROMPT, SMS_SENDING } sms_state_{SMS_IDLE};
  unsigned long sms_ts_{0};               // timestamp pour timeout
  std::string sms_pending_number_;
  std::string sms_pending_text_;

  struct SmsItem { std::string number; std::string text; };
  std::queue<SmsItem> sms_queue_;
  unsigned long last_retry_ts_{0};
  unsigned long backoff_ms_{2000};

  bool gpio_ad_state_{false};
  bool gpio_he_state_{false};
  bool gpio_hg_state_{false};
  unsigned long desarm_end_ts_{0};

  bool input_sirene_int_state_{true};
  bool input_sirene_ext_state_{true};
  bool input_armee_total_state_{true};
  bool input_panne_secteur_state_{true};
  bool input_alarme_declenchee_state_{true};
  bool input_armee_partiel_state_{true};

  void handle_line(const std::string &line);
  void start_send_sms_sequence();
  void queue_sms(const std::string &number, const std::string &text);

  bool is_acl_number(const std::string &sender);
  bool check_pin(const std::string &body);
  void handle_incoming_sms(const std::string &sender, const std::string &body);
  void send_sms_to_acl(const std::string &text);
  void update_desarm_timer(unsigned long now);
  void update_reset_timer(unsigned long now);

  void on_sirene_int_state(bool state);
  void on_sirene_ext_state(bool state);
  void on_armee_total_state(bool state);
  void on_panne_secteur_state(bool state);
  void on_alarme_declenchee_state(bool state);
  void on_armee_partiel_state(bool state);
};

}  // namespace bk7670_sms
}  // namespace esphome
