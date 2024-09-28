# This file is only used to publish this project as an ESP-IDF component.
# See https://docs.espressif.com/projects/idf-component-manager.
idf_component_register(SRCS "src/jsonst.c" "src/jsonst_util.c"
                       INCLUDE_DIRS "src/include")
