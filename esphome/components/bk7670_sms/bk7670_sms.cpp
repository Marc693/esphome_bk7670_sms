#include "bk7670_sms.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include <sstream>

namespace esphome {
namespace bk7670_sms {

static const char *TAG = "bk7670_sms";

void BK7670SMS::setup() {
  if (this->input_sirene_int_) {
    this->input_sirene_int_->add_on_state_callback([this](bool state) { this->on_sirene_int_state(state); });
  }
  if (this->input_sirene_ext_) {
    this->input_sirene_ext_->add_on_state_callback([this](bool state) { this->on_sirene_ext_state(state); });
  }
  if (this->input_armee_total_) {
    this->input_armee_total_->add_on_state_callback([this](bool state) { this->on_armee_total_state(state); });
  }
  if (this->input_panne_secteur_) {
    this->input_panne_secteur_->add_on_state_callback([this](bool state) { this->on_panne_secteur_state(state); });
  }
  if (this->input_alarme_declenchee_) {
    this->input_alarme_declenchee_->add_on_state_callback([this](bool state) { this->on_alarme_declenchee_state(state); });
  }
  if (this->input_armee_partiel_) {
    this->input_armee_partiel_->add_on_state_callback([this](bool state) { this->on_armee_partiel_state(state); });
  }

  if (this->gpio_reset_) {
    this->gpio_reset_->turn_off();
    this->gpio_reset_state_ = false;
    this->reset_end_ts_ = 0;
  }

#ifdef USE_API_CUSTOM_SERVICES
  this->register_service(&BK7670SMS::send_at_command, "send_at",
                         std::array<std::string, 1>{std::string("command")});
  this->register_service(&BK7670SMS::reset_modem, "reset_modem");
#else
  ESP_LOGW(TAG, "API custom services are not enabled; send_at and reset_modem services are unavailable");
#endif
}

void BK7670SMS::handle_line(const std::string &line) {
  // Filtrage basique : ignorer lignes vides
  if (line.empty()) return;

  ESP_LOGI(TAG, "LINE: %s", line.c_str());

  // +CMTI: notification d'un nouveau SMS
  if (line.rfind("+CMTI:", 0) == 0) {
    size_t comma = line.find(',');
    if (comma != std::string::npos) {
      std::string idx = line.substr(comma + 1);
      while (!idx.empty() && isspace((unsigned char)idx.front())) idx.erase(0,1);
      while (!idx.empty() && isspace((unsigned char)idx.back())) idx.pop_back();
      std::string cmd = "AT+CMGR=" + idx + "\r";
      ESP_LOGI(TAG, "Requesting CMGR for index %s", idx.c_str());
      this->write_str(cmd.c_str());
    }
    return;
  }

  // Entête +CMGR: la ligne suivante sera le corps
  if (line.rfind("+CMGR:", 0) == 0) {
    awaiting_cmgr_body_ = true;
    last_sms_sender_.clear();
    size_t first_quote = line.find('"');
    if (first_quote != std::string::npos) {
      size_t second_quote = line.find('"', first_quote + 1);
      if (second_quote != std::string::npos) {
        size_t third_quote = line.find('"', second_quote + 1);
        if (third_quote != std::string::npos) {
          size_t fourth_quote = line.find('"', third_quote + 1);
          if (fourth_quote != std::string::npos)
            last_sms_sender_ = line.substr(third_quote + 1, fourth_quote - third_quote - 1);
        }
      }
    }
    return;
  }

  // Si on attend le corps du SMS, traiter la ligne comme body
  if (awaiting_cmgr_body_) {
    awaiting_cmgr_body_ = false;
    std::string body = line;
    ESP_LOGI(TAG, "SMS body received from '%s': %s", last_sms_sender_.c_str(), body.c_str());
    this->process_sms(last_sms_sender_, body);
    last_sms_sender_.clear();
    return;
  }

  // Réponses AT basiques
  if (line == "OK") {
    ESP_LOGD(TAG, "AT OK");
    return;
  }
  if (line == "ERROR") {
    ESP_LOGW(TAG, "AT ERROR");
    return;
  }

  // Détection du prompt '>' pour envoi SMS
  if (line.find(">") != std::string::npos && sms_state_ == SMS_WAIT_PROMPT) {
    // envoyer le texte + CTRL+Z
    this->write_array((const uint8_t*)sms_pending_text_.c_str(), sms_pending_text_.length());
    uint8_t ctrlz = 0x1A;
    this->write_array(&ctrlz, 1);
    sms_state_ = SMS_SENDING;
    sms_ts_ = millis();
    ESP_LOGI(TAG, "Sent SMS payload, waiting for confirmation");
    return;
  }

  // Détection +CMGS: confirmation d'envoi
  if (line.rfind("+CMGS:", 0) == 0) {
    ESP_LOGI(TAG, "SMS sent: %s", line.c_str());
    sms_state_ = SMS_IDLE;
    // reset backoff
    backoff_ms_ = 2000;
    return;
  }

  // Gestion d'erreurs liées à l'envoi
  if (line.find("SMS busy") != std::string::npos || line.find("CMS ERROR") != std::string::npos) {
    ESP_LOGW(TAG, "Modem reports busy or CMS ERROR: %s", line.c_str());
    sms_state_ = SMS_IDLE;
    last_retry_ts_ = millis();
    backoff_ms_ = std::min<unsigned long>(backoff_ms_ * 2, 60000);
    return;
  }
}

void BK7670SMS::start_send_sms_sequence() {
  if (sms_pending_number_.empty()) {
    ESP_LOGW(TAG, "No SMS number pending");
    sms_state_ = SMS_IDLE;
    return;
  }
  // Set text mode
  this->write_str("AT+CMGF=1\r");
  // Demander prompt pour envoi
  std::string cmgs = "AT+CMGS=\"" + sms_pending_number_ + "\"\r";
  this->write_str(cmgs.c_str());
  sms_state_ = SMS_WAIT_PROMPT;
  sms_ts_ = millis();
  ESP_LOGI(TAG, "Started CMGS for %s", sms_pending_number_.c_str());
}

void BK7670SMS::queue_sms(const std::string &number, const std::string &text) {
  SmsItem it; it.number = number; it.text = text;
  sms_queue_.push(it);
  ESP_LOGI(TAG, "Queued SMS to %s (queue size %u)", number.c_str(), (unsigned int)sms_queue_.size());
}

void BK7670SMS::send_sms(const std::string &number, const std::string &text) {
  // API publique : ajoute en file si occupé
  if (sms_state_ != SMS_IDLE || !sms_queue_.empty()) {
    queue_sms(number, text);
    return;
  }
  sms_pending_number_ = number;
  sms_pending_text_ = text;
  start_send_sms_sequence();
}

void BK7670SMS::send_at_command(const api::StringRef &command) {
  std::string cmd(command.data(), command.size());
  if (!cmd.empty() && cmd.back() != '\r') {
    cmd += '\r';
  }
  ESP_LOGI(TAG, "send_at_command: %s", cmd.c_str());
  this->write_str(cmd.c_str());
}

void BK7670SMS::reset_modem() {
  if (!this->gpio_reset_) {
    ESP_LOGW(TAG, "reset_modem called but gpio_reset_ is not configured");
    return;
  }
  ESP_LOGI(TAG, "Resetting modem via GPIO");
  this->gpio_reset_->turn_on();
  this->gpio_reset_state_ = true;
  this->reset_end_ts_ = millis() + 200;
}

bool BK7670SMS::is_acl_number(const std::string &sender) {
  if (sender.empty()) return false;
  for (const auto &num : this->acl_numbers_) {
    if (sender == num) return true;
    if (!num.empty() && sender.find(num) != std::string::npos) return true;
  }
  return false;
}

bool BK7670SMS::check_pin(const std::string &body) {
  if (this->pin_code_.empty()) return true;
  std::istringstream iss(body);
  std::string token;
  if (!(iss >> token)) return false;
  return token == this->pin_code_;
}

void BK7670SMS::handle_incoming_sms(const std::string &sender, const std::string &body) {
  if (!this->is_acl_number(sender)) {
    ESP_LOGW(TAG, "SMS ignored from unauthorized sender %s", sender.c_str());
    return;
  }
  std::string text = body;
  while (!text.empty() && isspace((unsigned char)text.front())) text.erase(0, 1);
  while (!text.empty() && isspace((unsigned char)text.back())) text.pop_back();
  if (text.empty()) {
    ESP_LOGW(TAG, "SMS body empty");
    return;
  }

  if (!this->check_pin(text)) {
    ESP_LOGW(TAG, "SMS ignored: invalid PIN from %s", sender.c_str());
    return;
  }

  std::istringstream iss(text);
  std::string pin_token;
  std::string command;
  iss >> pin_token >> command;
  if (command.empty()) {
    ESP_LOGW(TAG, "SMS ignored: no command after PIN");
    return;
  }

  if (command == "CDE_HE") {
    this->do_he();
    this->send_sms(sender, "CDE_HE executed");
  } else if (command == "CDE_HG") {
    this->do_hg();
    this->send_sms(sender, "CDE_HG executed");
  } else if (command == "DESARM") {
    this->do_desarm();
    this->send_sms(sender, "DESARM executed");
  } else if (command == "STATUT") {
    this->send_status(sender);
  } else {
    ESP_LOGW(TAG, "SMS ignored: unknown command '%s'", command.c_str());
    this->send_sms(sender, "Unknown command: " + command);
  }
}

void BK7670SMS::send_sms_to_acl(const std::string &text) {
  for (const auto &number : this->acl_numbers_) {
    if (!number.empty())
      this->send_sms(number, text);
  }
}

void BK7670SMS::send_status(const std::string &sender) {
  std::string status = "HE:" + std::string(this->gpio_he_state_ ? "ON" : "OFF");
  status += " HG:" + std::string(this->gpio_hg_state_ ? "ON" : "OFF");
  status += " DESARM:" + std::string(this->gpio_ad_state_ ? "ON" : "OFF");
  status += " Sirene_Int:" + std::string(this->input_sirene_int_state_ ? "HIGH" : "LOW");
  status += " Sirene_Ext:" + std::string(this->input_sirene_ext_state_ ? "HIGH" : "LOW");
  status += " Armement_Total:" + std::string(this->input_armee_total_state_ ? "HIGH" : "LOW");
  status += " Panne_Secteur:" + std::string(this->input_panne_secteur_state_ ? "HIGH" : "LOW");
  status += " Alarme_Declenchee:" + std::string(this->input_alarme_declenchee_state_ ? "HIGH" : "LOW");
  status += " Armement_Partiel:" + std::string(this->input_armee_partiel_state_ ? "HIGH" : "LOW");
  this->send_sms(sender, status);
}

void BK7670SMS::do_he() {
  if (this->gpio_he_) {
    this->gpio_he_->turn_on();
    this->gpio_he_state_ = true;
  }
  ESP_LOGI(TAG, "do_he executed");
}

void BK7670SMS::do_hg() {
  if (this->gpio_hg_) {
    this->gpio_hg_->turn_on();
    this->gpio_hg_state_ = true;
  }
  ESP_LOGI(TAG, "do_hg executed");
}

void BK7670SMS::do_desarm() {
  if (this->gpio_ad_) {
    this->gpio_ad_->turn_on();
    this->gpio_ad_state_ = true;
    this->desarm_end_ts_ = millis() + 2000;
  }
  ESP_LOGI(TAG, "do_desarm executed");
}

void BK7670SMS::update_desarm_timer(unsigned long now) {
  if (this->gpio_ad_state_ && this->desarm_end_ts_ != 0 && now >= this->desarm_end_ts_) {
    if (this->gpio_ad_) {
      this->gpio_ad_->turn_off();
    }
    this->gpio_ad_state_ = false;
    this->desarm_end_ts_ = 0;
    ESP_LOGI(TAG, "DESARM output returned to off after delay");
  }
}

void BK7670SMS::update_reset_timer(unsigned long now) {
  if (this->gpio_reset_state_ && this->reset_end_ts_ != 0 && now >= this->reset_end_ts_) {
    if (this->gpio_reset_) {
      this->gpio_reset_->turn_off();
    }
    this->gpio_reset_state_ = false;
    this->reset_end_ts_ = 0;
    ESP_LOGI(TAG, "Modem reset line returned to off");
  }
}

void BK7670SMS::on_sirene_int_state(bool state) {
  if (this->input_sirene_int_state_ && !state) {
    ESP_LOGI(TAG, "Sirene_Int triggered on falling edge");
    this->send_sms_to_acl("Sirene_Int");
  }
  this->input_sirene_int_state_ = state;
}

void BK7670SMS::on_sirene_ext_state(bool state) {
  if (this->input_sirene_ext_state_ && !state) {
    ESP_LOGI(TAG, "Sirene_Ext triggered on falling edge");
    this->send_sms_to_acl("Sirene_Ext");
  }
  this->input_sirene_ext_state_ = state;
}

void BK7670SMS::on_armee_total_state(bool state) {
  if (this->input_armee_total_state_ && !state) {
    ESP_LOGI(TAG, "Armement_Total triggered on falling edge");
    this->send_sms_to_acl("Armement_Total");
  }
  this->input_armee_total_state_ = state;
}

void BK7670SMS::on_panne_secteur_state(bool state) {
  if (this->input_panne_secteur_state_ && !state) {
    ESP_LOGI(TAG, "Panne_Secteur triggered on falling edge");
    this->send_sms_to_acl("Panne_Secteur");
  }
  this->input_panne_secteur_state_ = state;
}

void BK7670SMS::on_alarme_declenchee_state(bool state) {
  if (this->input_alarme_declenchee_state_ && !state) {
    ESP_LOGI(TAG, "Alarme_Declenchee triggered on falling edge");
    this->send_sms_to_acl("Alarme_Declenchee");
  }
  this->input_alarme_declenchee_state_ = state;
}

void BK7670SMS::on_armee_partiel_state(bool state) {
  if (this->input_armee_partiel_state_ && !state) {
    ESP_LOGI(TAG, "Armement_Partiel triggered on falling edge");
    this->send_sms_to_acl("Armement_Partiel");
  }
  this->input_armee_partiel_state_ = state;
}

void BK7670SMS::loop() {
  // Lecture non bloquante
  uint8_t c;
  int count = 0;
  const int max_reads = 32;
  while (this->available() && count < max_reads) {
    if (!this->read_byte(&c)) break;
    count++;
    uart_buffer_.push_back((char)c);
    ESP_LOGD(TAG, "RX 0x%02X", c);
  }

  // Extraire lignes terminées par CRLF
  size_t pos;
  while ((pos = uart_buffer_.find("\r\n")) != std::string::npos) {
    std::string line = uart_buffer_.substr(0, pos);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    handle_line(line);
    uart_buffer_.erase(0, pos + 2);
  }

  unsigned long now = millis();
  this->update_desarm_timer(now);
  this->update_reset_timer(now);

  // Gestion des timeouts SMS
  if (sms_state_ == SMS_WAIT_PROMPT) {
    if (now - sms_ts_ > 15000) {
      ESP_LOGW(TAG, "SMS prompt timeout");
      sms_state_ = SMS_IDLE;
      last_retry_ts_ = now;
      backoff_ms_ = std::min<unsigned long>(backoff_ms_ * 2, 60000);
    }
  } else if (sms_state_ == SMS_SENDING) {
    if (now - sms_ts_ > 60000) {
      ESP_LOGW(TAG, "SMS send timeout");
      sms_state_ = SMS_IDLE;
      last_retry_ts_ = now;
      backoff_ms_ = std::min<unsigned long>(backoff_ms_ * 2, 60000);
    }
  }

  // Retry/backoff et traitement de la file
  if (sms_state_ == SMS_IDLE && !sms_queue_.empty()) {
    if (now - last_retry_ts_ >= backoff_ms_) {
      auto next = sms_queue_.front();
      sms_queue_.pop();
      sms_pending_number_ = next.number;
      sms_pending_text_ = next.text;
      start_send_sms_sequence();
      last_retry_ts_ = now;
    }
  }

  // Sécurité buffer
  const size_t max_buffer_len = 8192;
  if (uart_buffer_.size() > max_buffer_len) {
    ESP_LOGW(TAG, "UART buffer overflow, clearing");
    uart_buffer_.clear();
  }
}

void BK7670SMS::process_sms(const std::string &sender, const std::string &body) {
  ESP_LOGI(TAG, "process_sms called. sender='%s' body='%s'", sender.c_str(), body.c_str());
  this->handle_incoming_sms(sender, body);
}

}  // namespace bk7670_sms
}  // namespace esphome
