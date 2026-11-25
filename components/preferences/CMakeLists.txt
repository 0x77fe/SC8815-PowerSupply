idf_component_register(
    SRCS upstream/Preferences.cpp
    INCLUDE_DIRS upstream
    PRIV_REQUIRES nvs_flash
    REQUIRES log
)
