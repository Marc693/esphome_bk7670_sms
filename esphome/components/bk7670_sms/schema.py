import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, switch
from esphome.const import CONF_ID

bk7670_ns = cg.esphome_ns.namespace("bk7670_sms")
BK7670SMSComponent = bk7670_ns.class_("BK7670SMSComponent", cg.Component, uart.UARTDevice)

CONF_GPIO_AD = "gpio_ad"
CONF_GPIO_HE = "gpio_he"
CONF_GPIO_HG = "gpio_hg"
CONF_GPIO_POWERKEY = "gpio_powerkey"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BK7670SMSComponent),
    cv.Required("uart_id"): cv.use_id(uart.UARTComponent),
    cv.Required("pin_code"): cv.string,
    cv.Required("acl_numbers"): cv.ensure_list(cv.string),

    cv.Required(CONF_GPIO_AD): cv.use_id(switch.Switch),
    cv.Required(CONF_GPIO_HE): cv.use_id(switch.Switch),
    cv.Required(CONF_GPIO_HG): cv.use_id(switch.Switch),
    cv.Required(CONF_GPIO_POWERKEY): cv.use_id(switch.Switch),
})
