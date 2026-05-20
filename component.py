import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_UART_ID
from .schema import (
    CONFIG_SCHEMA,
    CONF_PIN_CODE,
    CONF_ACL_NUMBERS,
    CONF_GPIO_AD,
    CONF_GPIO_HE,
    CONF_GPIO_HG,
    CONF_GPIO_POWERKEY,
)

bk7670_sms_ns = cg.esphome_ns.namespace("bk7670_sms")
BK7670SMSComponent = bk7670_sms_ns.class_("BK7670SMS", cg.Component, cg.UARTDevice)

CONFIG_SCHEMA = CONFIG_SCHEMA

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], config[CONF_UART_ID])
    await cg.register_component(var, config)
    await cg.register_uart_device(var, config)

    cg.add(var.set_pin_code(config[CONF_PIN_CODE]))
    cg.add(var.set_acl_numbers(config[CONF_ACL_NUMBERS]))

    cg.add(var.set_gpio_ad(await cg.gpio_pin_expression(config[CONF_GPIO_AD])))
    cg.add(var.set_gpio_he(await cg.gpio_pin_expression(config[CONF_GPIO_HE])))
    cg.add(var.set_gpio_hg(await cg.gpio_pin_expression(config[CONF_GPIO_HG])))
    cg.add(var.set_gpio_powerkey(await cg.gpio_pin_expression(config[CONF_GPIO_POWERKEY])))
