import esphome.codegen as cg
from esphome.components import uart, switch
from esphome.const import CONF_ID

from .schema import (
    BK7670SMSComponent,
    CONF_GPIO_AD,
    CONF_GPIO_HE,
    CONF_GPIO_HG,
    CONF_GPIO_POWERKEY,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    ad = await cg.get_variable(config[CONF_GPIO_AD])
    he = await cg.get_variable(config[CONF_GPIO_HE])
    hg = await cg.get_variable(config[CONF_GPIO_HG])
    pwr = await cg.get_variable(config[CONF_GPIO_POWERKEY])

    cg.add(var.set_gpio_ad(ad))
    cg.add(var.set_gpio_he(he))
    cg.add(var.set_gpio_hg(hg))
    cg.add(var.set_gpio_powerkey(pwr))
