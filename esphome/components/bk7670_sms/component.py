import esphome.codegen as cg
from esphome.components import uart, text_sensor
from esphome.const import CONF_ID

from .schema import (
    BK7670SMS,
    CONF_GPIO_AD,
    CONF_GPIO_HE,
    CONF_GPIO_HG,
    CONF_PIN_CODE,
    CONF_ACL_NUMBERS,
    CONF_INCOMING_SMS,
    CONF_LAST_SENDER,
    CONF_LAST_CMD,
)

async def to_code(config):
    uart_parent = await cg.get_variable(config["uart_id"])
    var = cg.new_Pvariable(config[CONF_ID], uart_parent)

    # Enregistrement UART
    await uart.register_uart_device(var, config)

    # GPIO
    ad = await cg.get_variable(config[CONF_GPIO_AD])
    he = await cg.get_variable(config[CONF_GPIO_HE])
    hg = await cg.get_variable(config[CONF_GPIO_HG])

    cg.add(var.set_gpio_ad(ad))
    cg.add(var.set_gpio_he(he))
    cg.add(var.set_gpio_hg(hg))

    # Sécurité
    cg.add(var.set_pin_code(config[CONF_PIN_CODE]))

    for number in config[CONF_ACL_NUMBERS]:
        cg.add(var.add_acl_number(number))

    # Text sensors (SMS, sender, commande)
    if CONF_INCOMING_SMS in config:
        sms_sensor = await cg.get_variable(config[CONF_INCOMING_SMS])
        cg.add(var.set_incoming_sms_sensor(sms_sensor))

    if CONF_LAST_SENDER in config:
        sender_sensor = await cg.get_variable(config[CONF_LAST_SENDER])
        cg.add(var.set_last_sender_sensor(sender_sensor))

    if CONF_LAST_CMD in config:
        cmd_sensor = await cg.get_variable(config[CONF_LAST_CMD])
        cg.add(var.set_last_cmd_sensor(cmd_sensor))
