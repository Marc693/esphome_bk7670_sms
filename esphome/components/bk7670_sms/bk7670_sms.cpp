#include "bk7670_sms.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bk7670_sms {

static const char *const TAG = "bk7670_sms";

//
// ──────────────────────────────────────────────────────────────
//   LOOP PRINCIPALE
// ──────────────────────────────────────────────────────────────
//
void BK7670SMS::loop() {
  // Lecture UART ligne par ligne
  while (this->available()) {
    char c;
    this->read_byte(reinterpret_cast<uint8_t *>(&c));

    if (c == '\r')
      continue;

    if (c == '\n') {
      if (!rx_buffer_.empty()) {
        ESP_LOGV(TAG, "RX: %s", rx_buffer_.c_str());
        process_line_(rx_buffer_);
        rx_buffer_.clear();
      }
    } else {
      rx_buffer_.push_back(c);
    }
  }

  // Gestion file AT
  if (!at_busy_ && !at_queue_.empty()) {
    std::string cmd = at_queue_.front();
    at_queue_.pop();
    at_busy_ = true;
    at_last_send_ = millis();

    ESP_LOGI(TAG, "AT >> %s", cmd.c_str());
    this->write_str((cmd + "\r\n").c_str());
  }

  // Timeout AT
  if (at_busy_ && millis() - at_last_send_ > 1500) {
    ESP_LOGW(TAG, "AT timeout, libération");
    at_busy_ = false;
  }

  // Watchdog modem
  if (millis() - last_modem_ok_ > 30000) {
    ESP_LOGW(TAG, "Modem silencieux, reboot via PWRKEY");
    reboot_count_++;
    // Ici tu ajouteras ton GPIO PWRKEY si nécessaire
    last_modem_ok_ = millis();
  }
}

//
// ──────────────────────────────────────────────────────────────
//   FILE D’ATTENTE AT
// ──────────────────────────────────────────────────────────────
//
void BK7670SMS::queue_at(const std::string &cmd) {
  at_queue_.push(cmd);
}

//
// ──────────────────────────────────────────────────────────────
//   TRAITEMENT DES LIGNES UART
// ──────────────────────────────────────────────────────────────
//
void BK7670SMS::process_line_(const std::string &line) {
  last_uart_activity_ = millis();

  if (line == "OK") {
    at_busy_ = false;
    last_modem_ok_ = millis();
    return;
  }

  if (line == "ERROR") {
    at_busy_ = false;
    ESP_LOGE(TAG, "Erreur AT");
    return;
  }

  // Notification SMS : +CMTI: "SM",3
  if (line.rfind("+CMTI:", 0) == 0) {
    handle_cmti_(line);
    return;
  }

  // Réponse CMGR (header)
  if (line.rfind("+CMGR:", 0) == 0) {
    // Le corps du SMS arrivera dans la ligne suivante
    // On stocke le header temporairement
    incoming_header_ = line;
    return;
  }

  // Corps du SMS (après CMGR)
  if (!incoming_header_.empty()) {
    parse_cmgr_(incoming_header_, line);
    incoming_header_.clear();
    return;
  }
}

//
// ──────────────────────────────────────────────────────────────
//   TRAITEMENT CMTI (nouveau SMS)
// ──────────────────────────────────────────────────────────────
//
void BK7670SMS::handle_cmti_(const std::string &line) {
  // Exemple : +CMTI: "SM",3
  auto pos = line.find(',');
  if (pos == std::string::npos)
    return;

  int index = atoi(line.substr(pos + 1).c_str());
  ESP_LOGI(TAG, "Nouveau SMS index %d", index);

  read_sms_index_(index);
}

//
// ──────────────────────────────────────────────────────────────
//   LECTURE DU SMS VIA CMGR
// ──────────────────────────────────────────────────────────────
//
void BK7670SMS::read_sms_index_(int index) {
  queue_at("AT+CMGF=1");
  queue_at("AT+CMGR=" + std::to_string(index));
}

//
// ──────────────────────────────────────────────────────────────
//   PARSING DU SMS (CMGR)
// ──────────────────────────────────────────────────────────────
//
void BK7670SMS::parse_cmgr_(const std::string &header, const std::string &body) {
  // Exemple header :
  // +CMGR: "REC UNREAD","+33650855673","","24/05/21,14:22:10+08"

  std::string sender;
  auto first = header.find('"');
  if (first != std::string::npos) {
    auto second = header.find('"', first + 1);
    if (second != std::string::npos)
      sender = header.substr(first + 1, second - first - 1);
  }

  ESP_LOGI(TAG, "SMS reçu de %s : %s", sender.c_str(), body.c_str());

  if (incoming_sms_sensor_)
    incoming_sms_sensor_->publish_state(body);

  if (last_sender_sensor_)
    last_sender_sensor_->publish_state(sender);

  process_sms(sender, body);
}

//
// ──────────────────────────────────────────────────────────────
//   SÉCURITÉ : ACL
// ──────────────────────────────────────────────────────────────
//
bool BK7670SMS::is_acl_allowed_(const std::string &num) {
  for (auto &n : acl_numbers_) {
    if (n == num)
      return true;
  }
  return false;
}

//
// ──────────────────────────────────────────────────────────────
//   SÉCURITÉ : PIN
// ──────────────────────────────────────────────────────────────
//
bool BK7670SMS::check_pin_(const std::string &body) {
  if (pin_code_.empty())
    return true;

  return body.rfind(pin_code_, 0) == 0;
}

//
// ──────────────────────────────────────────────────────────────
//   TRAITEMENT LOGIQUE DES SMS
// ──────────────────────────────────────────────────────────────
//
void BK7670SMS::process_sms(const std::string &sender, const std::string &body) {
  if (!is_acl_allowed_(sender)) {
    ESP_LOGW(TAG, "Numéro non autorisé : %s", sender.c_str());
    return;
  }

  if (!check_pin_(body)) {
    ESP_LOGW(TAG, "PIN incorrect");
    return;
  }

  std::string cmd = body.substr(pin_code_.size());
  while (!cmd.empty() && cmd[0] == ' ')
    cmd.erase(0, 1);

  ESP_LOGI(TAG, "Commande SMS : %s", cmd.c_str());

  if (last_cmd_sensor_)
    last_cmd_sensor_->publish_state(cmd);

  if (cmd == "ARM") {
    do_arm();
    send_sms(sender, "Alarme ARMEE");
  } else if (cmd == "DISARM") {
    do_disarm();
    send_sms(sender, "Alarme DESARMEE");
  } else if (cmd == "STATUS") {
    do_status(sender);
  } else {
    send_sms(sender, "Commande inconnue");
  }
}

//
// ──────────────────────────────────────────────────────────────
//   ENVOI SMS SORTANT
// ──────────────────────────────────────────────────────────────
//
void BK7670SMS::send_sms(const std::string &number, const std::string &text) {
  ESP_LOGI(TAG, "Envoi SMS à %s : %s", number.c_str(), text.c_str());

  queue_at("AT+CMGF=1");
  queue_at("AT+CMGS=\"" + number + "\"");
  queue_at(text + "\x1A");
}

//
// ──────────────────────────────────────────────────────────────
//   COMMANDES GPIO
// ──────────────────────────────────────────────────────────────
//
void BK7670SMS::do_arm() {
  ESP_LOGI(TAG, "ARM");
  if (gpio_hg_)
    gpio_hg_->turn_on();
}

void BK7670SMS::do_disarm() {
  ESP_LOGI(TAG, "DISARM");
  if (gpio_hg_)
    gpio_hg_->turn_off();
}

void BK7670SMS::do_status(const std::string &sender) {
  send_sms(sender, "STATUS OK");
}

}  // namespace bk7670_sms
}  // namespace esphome
