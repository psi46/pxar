# Add source files for the devices library

# add HV power supply source files (depending on the device used)
IF(BUILD_HVSUPPLY MATCHES "Keithley237")
  MESSAGE("-- HV Supply Support: building Keithley 237 support.")
  SET(SOURCE_FILES "hvsupply.keithley237.cc")
ELSEIF(BUILD_HVSUPPLY MATCHES "Keithley2410")
  MESSAGE("-- HV Supply Support: building Keithley 2410 support.")
  SET(SOURCE_FILES "hvsupply.keithley2410.cc")
ELSEIF(BUILD_HVSUPPLY MATCHES "Iseg")
  MESSAGE("-- HV Supply Support: building Iseg support.")
  SET(SOURCE_FILES "hvsupply.iseg.cc" "rs232.cc")
ENDIF()

IF(SOURCE_FILES)
  MESSAGE("--   Source files: " ${SOURCE_FILES})
  SET(DEVICES TRUE PARENT_SCOPE)

  ADD_LIBRARY(devices SHARED ${SOURCE_FILES})
  SET(DEVICES_LINK_LIBRARY devices)

  INSTALL(TARGETS devices
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

ELSE(SOURCE_FILES)
  MESSAGE("-- HV Supply Support: none.")
ENDIF(SOURCE_FILES)