idf_component_register(
    SRCS main.cpp i2c.cpp input.cpp power_supply.cpp screen.cpp settings.cpp
    PRIV_REQUIRES sc8815 esp_driver_gpio esp_driver_i2c esp_hw_support esp_timer
                  freertos log nvs_flash preferences
)
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)
