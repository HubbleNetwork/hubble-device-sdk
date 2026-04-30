zephyr_include_directories(
	../
	../../../
)

zephyr_library_sources(nrf52_soc.c)

add_library(libhubble_sat_nrf52 STATIC IMPORTED GLOBAL)
set_target_properties(libhubble_sat_nrf52 PROPERTIES
	IMPORTED_LOCATION ${HUBBLE_BLOBS_DIR}/nrf52/libhubble_sat_nrf52.a
)
zephyr_link_libraries(libhubble_sat_nrf52)

if(CONFIG_BOARD_NRF21540DK_NRF52840)
	zephyr_include_directories(${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic/hal/nrf5/radio/)
	zephyr_library_sources(fem.c)
	zephyr_library_sources(nrf21540dk.c)
endif()

