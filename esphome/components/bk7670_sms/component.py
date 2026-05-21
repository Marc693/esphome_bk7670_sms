import esphome.codegen as cg
from esphome.components import uart
from esphome.const import CONF_ID

from .schema import (
    BK7670SMS,
    CONF_GPIO_AD,
    CONF_GPIO_HE,
    CONF_GPIO_HG,
)

async def to_code(config):
    uart_parent = await cg.get_variable(config["uart_id"])
    var = cg.new_Pvariable(config[CONF_ID], uart_parent)

    await uart.register_uart_device(var, config)

    ad = await cg.get_variable(config[CONF_GPIO_AD])
    he = await cg.get_variable(config[CONF_GPIO_HE])
    hg = await cg.get_variable(config[CONF_GPIO_HG])

    cg.add(var.set_gpio_ad(ad))
    cg.add(var.set_gpio_he(he))
    cg.add(var.set_gpio_hg(hg))
