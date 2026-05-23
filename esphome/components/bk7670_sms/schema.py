import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, output, binary_sensor

bk7670_ns = cg.esphome_ns.namespace("bk7670_sms")
BK7670SMS = bk7670_ns.class_("BK7670SMS", uart.UARTDevice)

CONF_GPIO_AD = "gpio_ad"
CONF_GPIO_HE = "gpio_he"
CONF_GPIO_HG = "gpio_hg"

CONF_PIN_CODE = "pin_code"
CONF_ACL_NUMBERS = "acl_numbers"

CONF_INPUT_SIRENE_INT = "input_sirene_int"
CONF_INPUT_SIRENE_EXT = "input_sirene_ext"
CONF_INPUT_ARME_TOTAL = "input_armee_total"
CONF_INPUT_PANNE_SECTEUR = "input_panne_secteur"
CONF_INPUT_ALARME_DECLENCHEE = "input_alarme_declenchee"
CONF_INPUT_ARME_PARTIEL = "input_armee_partiel"
CONF_GPIO_RESET = "gpio_reset"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BK7670SMS),

    cv.Required(CONF_GPIO_AD): cv.use_id(output.BinaryOutput),
    cv.Required(CONF_GPIO_HE): cv.use_id(output.BinaryOutput),
    cv.Required(CONF_GPIO_HG): cv.use_id(output.BinaryOutput),
    cv.Optional(CONF_GPIO_RESET): cv.use_id(output.BinaryOutput),

    cv.Optional(CONF_PIN_CODE, default=""): cv.string,
    cv.Optional(CONF_ACL_NUMBERS, default=[]): cv.ensure_list(cv.string),

    cv.Optional(CONF_INPUT_SIRENE_INT): cv.use_id(binary_sensor.BinarySensor),
    cv.Optional(CONF_INPUT_SIRENE_EXT): cv.use_id(binary_sensor.BinarySensor),
    cv.Optional(CONF_INPUT_ARME_TOTAL): cv.use_id(binary_sensor.BinarySensor),
    cv.Optional(CONF_INPUT_PANNE_SECTEUR): cv.use_id(binary_sensor.BinarySensor),
    cv.Optional(CONF_INPUT_ALARME_DECLENCHEE): cv.use_id(binary_sensor.BinarySensor),
    cv.Optional(CONF_INPUT_ARME_PARTIEL): cv.use_id(binary_sensor.BinarySensor),

}).extend(uart.UART_DEVICE_SCHEMA)
