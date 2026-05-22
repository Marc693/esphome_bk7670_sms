#include "bk7670_sms.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bk7670_sms {

static const char *TAG = "bk7670_sms";

void BK7670SMS::handle_line(const std::string &line) {
  // Filtrage basique des lignes suspectes
  if (line.rfind("edge_all_open_tabs", 0) == 0 ||
      line.find("<WebsiteContent_") != std::string::npos ||
      line.size() > 2000) {
    ESP_LOGW(TAG, "Ignored suspicious line");
    return;
  }

  ESP_LOGI(TAG, "LINE: %s", line.c_str());

  // +CMTI: "SM",index  ou +CMTI: "ME",index
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

  // Entête +CMGR: on marque que la ligne suivante est le corps
  if (line.rfind("+CMGR:", 0) == 0) {
    awaiting_cmgr_body_ = true;
    return;
  }

  // Si on attend le corps du SMS, traiter la ligne comme body
  if (awaiting_cmgr_body_) {
    awaiting_cmgr_body_ = false;
    std::string body = line;
    // Exemple simple: détecter mot clé et actionner relais
    if (body.find("24052005") != std::string::npos) {
      ESP_LOGI(TAG, "PW detected in SMS body");
      if (this->gpio_ad_) this->gpio_ad_->turn_on();
      // Non bloquant : stocker timestamp pour éteindre plus tard si nécessaire
    }
    // Appel de la méthode publique pour traitement plus complet
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
    return;
  }
}

void BK7670SMS::start_send_sms_sequence() {
  // Assure que sms_pending_number_ et sms_pending_text_ sont définis
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
}

void BK7670SMS::send_sms(const std::string &number, const std::string &text) {
  if (sms_state_ != SMS_IDLE) {
    ESP_LOGW(TAG, "SMS busy");
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
  const int max_reads = 16;
  while (this->available() && count < max_reads) {
    if (!this->read_byte(&c)) break;
    count++;
    uart_buffer_.push_back((char)c);
    ESP_LOGD(TAG, "RX 0x%02X", c);
    // Si CTRL+Z reçu dans flux inattendu, on le logge et on continue
    if (c == 0x1A) {
      ESP_LOGI(TAG, "Detected CTRL+Z in stream");
    }
  }

  // Extraire lignes terminées par CRLF
  size_t pos;
  while ((pos = uart_buffer_.find("\r\n")) != std::string::npos) {
    std::string line = uart_buffer_.substr(0, pos);
    // retirer CR si présent
    if (!line.empty() && line.back() == '\r') line.pop_back();
    handle_line(line);
    uart_buffer_.erase(0, pos + 2);
  }

  // Gestion des timeouts SMS
  if (sms_state_ == SMS_WAIT_PROMPT) {
    if (millis() - sms_ts_ > 15000) {
      ESP_LOGW(TAG, "SMS prompt timeout");
      sms_state_ = SMS_IDLE;
    }
  } else if (sms_state_ == SMS_SENDING) {
    if (millis() - sms_ts_ > 60000) {
      ESP_LOGW(TAG, "SMS send timeout");
      sms_state_ = SMS_IDLE;
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
  // Implémentation métier ici
}

void BK7670SMS::do_arm() {
  if (this->gpio_ad_) this->gpio_ad_->turn_on();
}
void BK7670SMS::do_disarm() {
  if (this->gpio_ad_) this->gpio_ad_->turn_off();
}
void BK7670SMS::do_status(const std::string &sender) {
  // envoyer status par SMS ou log
  ESP_LOGI(TAG, "do_status requested by %s", sender.c_str());
}

}  // namespace bk7670_sms
}  // namespace esphome
