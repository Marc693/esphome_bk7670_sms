import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, switch
from esphome.const import CONF_ID

CONF_GPIO_AD = "gpio_ad"
CONF_GPIO_HE = "gpio_he"
CONF_GPIO_HG = "gpio_hg"
CONF_GPIO_POWERKEY = "gpio_powerkey"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(cg.Component),  # placeholder, remplacé dans component.py
    cv.Required("uart_id"): cv.use_id(uart.UARTComponent),
    cv.Required("pin_code"): cv.string,
    cv.Required("acl_numbers"): cv.ensure_list(cv.string),

    cv.Required(CONF_GPIO_AD): cv.use_id(switch.Switch),
    cv.Required(CONF_GPIO_HE): cv.use_id(switch.Switch),
    cv.Required(CONF_GPIO_HG): cv.use_id(switch.Switch),
    cv.Required(CONF_GPIO_POWERKEY): cv.use_id(switch.Switch),
})
