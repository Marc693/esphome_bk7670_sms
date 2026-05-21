import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, output

bk7670_ns = cg.esphome_ns.namespace("bk7670_sms")
BK7670SMS = bk7670_ns.class_("BK7670SMS", uart.UARTDevice)

CONF_GPIO_AD = "gpio_ad"
CONF_GPIO_HE = "gpio_he"
CONF_GPIO_HG = "gpio_hg"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BK7670SMS),
    cv.Required(CONF_GPIO_AD): cv.use_id(output.BinaryOutput),
    cv.Required(CONF_GPIO_HE): cv.use_id(output.BinaryOutput),
    cv.Required(CONF_GPIO_HG): cv.use_id(output.BinaryOutput),
}).extend(uart.UART_DEVICE_SCHEMA)
