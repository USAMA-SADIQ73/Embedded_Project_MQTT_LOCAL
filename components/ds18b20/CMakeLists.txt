idf_component_register(SRCS "ds18b20.c"
                    INCLUDE_DIRS "."
                    PRIV_INCLUDE_DIRS ""
                    PRIV_REQUIRES "driver"
                    REQUIRES "esp_timer")
