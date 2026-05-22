from .schema import (
    CONFIG_SCHEMA,
    BK7670SMS,
    CONF_INCOMING_SMS,
    CONF_LAST_SENDER,
    CONF_LAST_CMD,
)
from .component import to_code

AUTO_LOAD = ["uart", "output", "text_sensor"]
CODEOWNERS = ["@Marc693"]
