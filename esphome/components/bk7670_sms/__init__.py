from esphome import automation
import esphome.config_validation as cv
import esphome.codegen as cg

from .schema import CONFIG_SCHEMA, BK7670SMSComponent
from .component import to_code

# Déclaration du domaine
AUTO_LOAD = ["uart", "output"]
CODEOWNERS = ["@Marc693"]

CONFIG_SCHEMA = CONFIG_SCHEMA
