#include "bk7670_sms.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bk7670_sms {

static const char *TAG = "bk7670_sms";

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
    return;
  }

  // Si on attend le corps du SMS, traiter la ligne comme body
  if (awaiting_cmgr_body_) {
    awaiting_cmgr_body_ = false;
    std::string body = line;
    ESP_LOGI(TAG, "SMS body received: %s", body.c_str());
    // Exemple d'action testée : log uniquement
    // Pour action réelle, décommenter la ligne suivante
    // if (this->gpio_ad_) this->gpio_ad_->turn_on();
    this->process_sms("", body);
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

  // Gestion des timeouts SMS
  unsigned long now = millis();
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

// Stubs pour méthodes publiques à compléter par toi si besoin
void BK7670SMS::process_sms(const std::string &sender, const std::string &body) {
  ESP_LOGI(TAG, "process_sms called. sender='%s' body='%s'", sender.c_str(), body.c_str());
  // Exemple : publier dans text_sensor si défini dans YAML
  // id(incoming_sms).publish_state(body); // décommenter si incoming_sms existe
  // Exemple d'action sur mot-clé (commentée pour test)
  // if (body.find("ARM") != std::string::npos) { if (this->gpio_ad_) this->gpio_ad_->turn_on(); }
}

void BK7670SMS::do_arm() {
  // if (this->gpio_ad_) this->gpio_ad_->turn_on();
  ESP_LOGI(TAG, "do_arm called (action commented in test)");
}
void BK7670SMS::do_disarm() {
  // if (this->gpio_ad_) this->gpio_ad_->turn_off();
  ESP_LOGI(TAG, "do_disarm called (action commented in test)");
}
void BK7670SMS::do_status(const std::string &sender) {
  ESP_LOGI(TAG, "do_status requested by %s", sender.c_str());
}

}  // namespace bk7670_sms
}  // namespace esphome
