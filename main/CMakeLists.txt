# Declaring requirements here opts out of the build system's convenience
# dependency of main on every component, so anything main.cpp includes beyond
# the common components has to be listed. led_strip arrives from
# idf_component.yml and must not be repeated.
idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_wifi bootloader_support
)
