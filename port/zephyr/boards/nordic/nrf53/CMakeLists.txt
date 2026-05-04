zephyr_include_directories(
	../
	../../../
)

zephyr_library_sources(nrf53_soc.c)

add_library(libhubble_sat_nrf53 STATIC IMPORTED GLOBAL)
set_target_properties(libhubble_sat_nrf53 PROPERTIES
	IMPORTED_LOCATION ${HUBBLE_BLOBS_DIR}/nrf53/libhubble_sat_nrf53.a
)
zephyr_link_libraries(libhubble_sat_nrf53)

if(CONFIG_BOARD_THINGY53_NRF5340_CPUNET)
	# Thingy53 uses nrf21540 fem. Let's just use the
	# implementation in nrf52 for now, but if it is
	# probably worth moving it to a common location.
	zephyr_include_directories(${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic/hal/nrf5/radio/)
	zephyr_include_directories(../nrf52/)
	zephyr_library_sources(../nrf52/fem.c)
	zephyr_library_sources(thingy53.c)
endif()


