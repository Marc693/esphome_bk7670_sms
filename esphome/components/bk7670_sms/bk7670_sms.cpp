#include "bk7670_sms.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bk7670_sms {

static const char *const TAG = "bk7670_sms";

void BK7670SMS::loop() {
  // Lecture ligne par ligne sur l'UART
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

  // Gestion file AT : envoi si libre
  if (!at_busy_ && !at_queue_.empty()) {
    std::string cmd = at_queue_.front();
    at_queue_.pop();
    at_busy_ = true;
    at_last_send_ = millis();

    ESP_LOGI(TAG, "AT >> %s", cmd.c_str());
    // stocker la dernière commande sans CRLF pour ignorer l'écho
    this->last_at_sent_ = cmd;
    this->write_str((cmd + "\r\n").c_str());
  }

  // Timeout AT (plus permissif)
  if (at_busy_ && millis() - at_last_send_ > 5000) {
    ESP_LOGW(TAG, "AT timeout, libération");
    at_busy_ = false;
  }

  // Watchdog modem
  if (millis() - last_modem_ok_ > 30000) {
    ESP_LOGW(TAG, "Modem silencieux, reboot via PWRKEY");
    reboot_count_++;
    last_modem_ok_ = millis();
  }
}

void BK7670SMS::queue_at(const std::string &cmd) {
  // stocke la commande (sans CRLF) pour ignorer son écho
  this->at_queue_.push(cmd);
}

void BK7670SMS::process_line_(const std::string &line) {
  last_uart_activity_ = millis();

  // 1) Ignorer l'écho exact de la dernière commande envoyée
  if (!last_at_sent_.empty() && line == last_at_sent_) {
    ESP_LOGD(TAG, "Echo modem ignoré: %s", line.c_str());
    return;
  }

  // 2) Si on attend le prompt '>' pour CMGS
  if (this->expecting_cmgs_prompt_) {
    std::string s = line;
    // trim espaces
    while (!s.empty() && isspace((unsigned char)s.front())) s.erase(0,1);
    while (!s.empty() && isspace((unsigned char)s.back())) s.pop_back();
    if (s == ">") {
      ESP_LOGI(TAG, "Prompt '>' reçu, envoi du corps SMS");
      // envoyer le corps directement (pas via queue_at) + CTRL+Z
      this->write_str(this->pending_sms_body_.c_str());
      this->write_byte(0x1A);
      this->expecting_cmgs_prompt_ = false;
      this->pending_sms_body_.clear();
      return;
    }
  }

  // 3) Réponses classiques
  if (line == "OK") {
    at_busy_ = false;
    last_modem_ok_ = millis();
    ESP_LOGD(TAG, "AT OK reçu");
    return;
  }

  if (line == "ERROR") {
    at_busy_ = false;
    ESP_LOGW(TAG, "AT ERROR reçu");
    return;
  }

  // Notification SMS : +CMTI: "SM",3
  if (line.rfind("+CMTI:", 0) == 0) {
    handle_cmti_(line);
    return;
  }

  // Réponse CMGR (header)
  if (line.rfind("+CMGR:", 0) == 0) {
    incoming_header_ = line;
    return;
  }

  // Corps du SMS (après CMGR)
  if (!incoming_header_.empty()) {
    parse_cmgr_(incoming_header_, line);
    incoming_header_.clear();
    return;
  }

  // +CMGS (confirmation d'envoi)
  if (line.rfind("+CMGS:", 0) == 0) {
    ESP_LOGI(TAG, "CMGS: %s", line.c_str());
    // laisser OK suivant gérer la libération
    return;
  }

  ESP_LOGD(TAG, "RX non traité: %s", line.c_str());
}

void BK7670SMS::handle_cmti_(const std::string &line) {
  auto pos = line.find(',');
  if (pos == std::string::npos)
    return;

  int index = atoi(line.substr(pos + 1).c_str());
  ESP_LOGI(TAG, "Nouveau SMS index %d", index);

  read_sms_index_(index);
}

void BK7670SMS::read_sms_index_(int index) {
  queue_at("AT+CMGF=1");
  queue_at("AT+CMGR=" + std::to_string(index));
}

void BK7670SMS::parse_cmgr_(const std::string &header, const std::string &body) {
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

bool BK7670SMS::is_acl_allowed_(const std::string &num) {
  for (auto &n : acl_numbers_) {
    if (n == num)
      return true;
  }
  return false;
}

bool BK7670SMS::check_pin_(const std::string &body) {
  if (pin_code_.empty())
    return true;

  return body.rfind(pin_code_, 0) == 0;
}

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

void BK7670SMS::send_sms(const std::string &number, const std::string &text) {
  ESP_LOGI(TAG, "Queue SMS à %s", number.c_str());
  // Mode texte
  queue_at("AT+CMGF=1");
  // Demander l'envoi ; on attendra le prompt '>' pour envoyer le corps
  queue_at("AT+CMGS=\"" + number + "\"");
  // stocker le corps et activer le flag d'attente
  this->pending_sms_body_ = text;
  this->expecting_cmgs_prompt_ = true;
}

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
