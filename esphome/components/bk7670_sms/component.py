import esphome.codegen as cg
from esphome.components import uart, binary_sensor
from esphome.const import CONF_ID

from .schema import (
    BK7670SMS,
    CONF_GPIO_AD,
    CONF_GPIO_HE,
    CONF_GPIO_HG,
    CONF_GPIO_RESET,
    CONF_PIN_CODE,
    CONF_ACL_NUMBERS,
    CONF_INPUT_SIRENE_INT,
    CONF_INPUT_SIRENE_EXT,
    CONF_INPUT_ARME_TOTAL,
    CONF_INPUT_PANNE_SECTEUR,
    CONF_INPUT_ALARME_DECLENCHEE,
    CONF_INPUT_ARME_PARTIEL,
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

    cg.add(var.set_pin_code(config[CONF_PIN_CODE]))

    for number in config[CONF_ACL_NUMBERS]:
        cg.add(var.add_acl_number(number))

    if CONF_GPIO_RESET in config:
        reset_out = await cg.get_variable(config[CONF_GPIO_RESET])
        cg.add(var.set_gpio_reset(reset_out))

    if CONF_INPUT_SIRENE_INT in config:
        sirene_int = await cg.get_variable(config[CONF_INPUT_SIRENE_INT])
        cg.add(var.set_input_sirene_int(sirene_int))
    if CONF_INPUT_SIRENE_EXT in config:
        sirene_ext = await cg.get_variable(config[CONF_INPUT_SIRENE_EXT])
        cg.add(var.set_input_sirene_ext(sirene_ext))
    if CONF_INPUT_ARME_TOTAL in config:
        arme_total = await cg.get_variable(config[CONF_INPUT_ARME_TOTAL])
        cg.add(var.set_input_armee_total(arme_total))
    if CONF_INPUT_PANNE_SECTEUR in config:
        panne_secteur = await cg.get_variable(config[CONF_INPUT_PANNE_SECTEUR])
        cg.add(var.set_input_panne_secteur(panne_secteur))
    if CONF_INPUT_ALARME_DECLENCHEE in config:
        alarme_declenchee = await cg.get_variable(config[CONF_INPUT_ALARME_DECLENCHEE])
        cg.add(var.set_input_alarme_declenchee(alarme_declenchee))
    if CONF_INPUT_ARME_PARTIEL in config:
        arme_partiel = await cg.get_variable(config[CONF_INPUT_ARME_PARTIEL])
        cg.add(var.set_input_armee_partiel(arme_partiel))
