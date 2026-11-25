idf_component_register(SRCS sc8815.cpp INCLUDE_DIRS ".")
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_20)
