import esphome.codegen as cg
from esphome.components import uart, output
from esphome.const import CONF_ID

from .schema import (
    BK7670SMS,
    CONF_GPIO_AD,
    CONF_GPIO_HE,
    CONF_GPIO_HG,
    CONF_GPIO_POWERKEY,
)

async def to_code(config):
    # Récupérer l’UART parent
    uart_parent = await cg.get_variable(config["uart_id"])

    # Créer l’objet C++ AVEC le parent UART
    var = cg.new_Pvariable(config[CONF_ID], uart_parent)
  
    # Enregistrer comme UARTDevice
    await uart.register_uart_device(var, config)

    # Outputs
    ad = await cg.get_variable(config[CONF_GPIO_AD])
    he = await cg.get_variable(config[CONF_GPIO_HE])
    hg = await cg.get_variable(config[CONF_GPIO_HG])
    pwr = await cg.get_variable(config[CONF_GPIO_POWERKEY])

    cg.add(var.set_gpio_ad(ad))
    cg.add(var.set_gpio_he(he))
    cg.add(var.set_gpio_hg(hg))
    cg.add(var.set_gpio_powerkey(pwr))
