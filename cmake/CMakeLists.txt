set(PKG_DESC "Image Tools for Wildbook IA")
set(pkg_conf_file ${CMAKE_CURRENT_BINARY_DIR}/pydarknet.pc)
configure_file(pydarknet.pc.in ${pkg_conf_file} @ONLY)
install(
  FILES ${pkg_conf_file}
  DESTINATION ${PYDARKNET_LIB_INSTALL_DIR}/pkgconfig/
  COMPONENT pkgconfig)
