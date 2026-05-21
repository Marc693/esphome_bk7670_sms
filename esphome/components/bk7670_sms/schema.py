import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_UART_ID
from esphome.components.uart import UARTComponent
from esphome import pins

# Import obligatoire : sinon NameError
from .component import BK7670SMSComponent

CONF_PIN_CODE = "pin_code"
CONF_ACL_NUMBERS = "acl_numbers"
CONF_GPIO_AD = "gpio_ad"
CONF_GPIO_HE = "gpio_he"
CONF_GPIO_HG = "gpio_hg"
CONF_GPIO_POWERKEY = "gpio_powerkey"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BK7670SMSComponent),
    cv.Required(CONF_UART_ID): cv.use_id(UARTComponent),

    cv.Required(CONF_PIN_CODE): cv.string,
    cv.Required(CONF_ACL_NUMBERS): cv.ensure_list(cv.string),

    cv.Required(CONF_GPIO_AD): pins.gpio_output_pin_schema,
    cv.Required(CONF_GPIO_HE): pins.gpio_output_pin_schema,
    cv.Required(CONF_GPIO_HG): pins.gpio_output_pin_schema,
    cv.Required(CONF_GPIO_POWERKEY): pins.gpio_output_pin_schema,
})
