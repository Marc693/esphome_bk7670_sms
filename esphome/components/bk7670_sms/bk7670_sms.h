#include "bk7670_sms.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bk7670_sms {

static const char *const TAG = "bk7670_sms";

void BK7670SMS::loop() {
  // Lecture ligne par ligne sur l'UART
  while (this->available()) {
    std::string line;
    char c;
    // Lire jusqu'à fin de ligne
    while (this->available()) {
      this->read_byte(reinterpret_cast<uint8_t *>(&c));
      if (c == '\n')
        break;
      if (c != '\r')
        line.push_back(c);
    }

    if (line.empty())
      continue;

    ESP_LOGV(TAG, "RX: %s", line.c_str());

    // Détection d'un SMS entrant en mode texte
    // Exemple: +CMT: "+33650855673","","24/05/21,14:22:10+08"
    if (line.rfind("+CMT:", 0) == 0) {
      // Extraction du numéro entre guillemets
      std::string sender;
      auto first_quote = line.find('"');
      if (first_quote != std::string::npos) {
        auto second_quote = line.find('"', first_quote + 1);
        if (second_quote != std::string::npos && second_quote > first_quote + 1) {
          sender = line.substr(first_quote + 1, second_quote - first_quote - 1);
        }
      }

      // Ligne suivante = corps du SMS
      std::string body;
      while (this->available()) {
        body.clear();
        while (this->available()) {
          this->read_byte(reinterpret_cast<uint8_t *>(&c));
          if (c == '\n')
            break;
          if (c != '\r')
            body.push_back(c);
        }
        if (!body.empty())
          break;
      }

      ESP_LOGI(TAG, "SMS reçu de %s : %s", sender.c_str(), body.c_str());
      if (!sender.empty() && !body.empty()) {
        this->process_sms(sender, body);
      }
    }
  }
}

void BK7670SMS::send_sms(const std::string &number, const std::string &text) {
  ESP_LOGI(TAG, "Envoi SMS à %s : %s", number.c_str(), text.c_str());

  this->write_str("AT+CMGF=1\r\n");
  delay(200);

  this->write_str("AT+CMGS=\"");
  this->write_str(number.c_str());
  this->write_str("\"\r\n");
  delay(200);

  this->write_str(text.c_str());
  this->write_byte(26);  // CTRL+Z
}

void BK7670SMS::process_sms(const std::string &sender, const std::string &body) {
  // Vérification ACL
  bool allowed = false;
  for (auto &num : this->acl_numbers_) {
    if (num == sender) {
      allowed = true;
      break;
    }
  }
  if (!allowed) {
    ESP_LOGW(TAG, "SMS rejeté : numéro non autorisé (%s)", sender.c_str());
    return;
  }

  // Vérification PIN
  if (this->pin_code_.empty() || body.rfind(this->pin_code_, 0) != 0) {
    ESP_LOGW(TAG, "SMS rejeté : PIN incorrect");
    return;
  }

  // Extraction commande après le PIN
  std::string cmd = body.substr(this->pin_code_.size());
  while (!cmd.empty() && cmd[0] == ' ')
    cmd.erase(0, 1);

  ESP_LOGI(TAG, "Commande SMS reçue : '%s'", cmd.c_str());

  if (cmd == "ARM") {
    this->do_arm();
    this->send_sms(sender, "Alarme ARMEE");
  } else if (cmd == "DISARM") {
    this->do_disarm();
    this->send_sms(sender, "Alarme DESARMEE");
  } else if (cmd == "STATUS") {
    this->do_status(sender);
  } else {
    this->send_sms(sender, "Commande inconnue");
  }
}

void BK7670SMS::do_arm() {
  ESP_LOGI(TAG, "Commande ARM");
  if (this->gpio_hg_ != nullptr)
    this->gpio_hg_->turn_on();
}

void BK7670SMS::do_disarm() {
  ESP_LOGI(TAG, "Commande DISARM");
  if (this->gpio_hg_ != nullptr)
    this->gpio_hg_->turn_off();
}

void BK7670SMS::do_status(const std::string &sender) {
  ESP_LOGI(TAG, "Commande STATUS");
  // Ici, on envoie un statut simple. Tu pourras enrichir avec des id() via lambdas YAML si besoin.
  this->send_sms(sender, "STATUS: commande OK (etat detaille a implementer via YAML ou extension)");
}

}  // namespace bk7670_sms
}  // namespace esphome
