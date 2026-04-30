/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <nrfx_gpiote.h>

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/gpio/gpio.h>

#include "fem.h"


#if DT_NODE_HAS_PROP(DT_NODELABEL(radio), fem)
#define FEM_NODE DT_PHANDLE(DT_NODELABEL(radio), fem)
#if DT_NODE_HAS_STATUS_OKAY(FEM_NODE)
#define HAL_RADIO_HAVE_FEM
#endif /* DT_NODE_HAS_STATUS_OKAY(FEM_NODE) */
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(radio), fem)) */

/* Does FEM_NODE have a particular DT compatible? */
#define FEM_HAS_COMPAT(compat) DT_NODE_HAS_COMPAT(FEM_NODE, compat)

/* Does FEM_NODE have a particular DT property defined? */
#define FEM_HAS_PROP(prop)     DT_NODE_HAS_PROP(FEM_NODE, prop)

/*
 * Device-specific settings are pulled in based FEM_NODE's compatible
 * property.
 */

#ifdef HAL_RADIO_HAVE_FEM
#if FEM_HAS_COMPAT(generic_fem_two_ctrl_pins)
#include "radio_nrf5_fem_generic.h"
#elif FEM_HAS_COMPAT(nordic_nrf21540_fem)
#include "radio_nrf5_fem_nrf21540.h"
#else
#error "radio node fem property has an unsupported compatible"
#endif /* FEM_HAS_COMPAT(generic_fem_two_ctrl_pins) */
#endif /* HAL_RADIO_HAVE_FEM */

#define NRF_FEM_GPIO(prop)                                                     \
	((NRF_GPIO_Type *)DT_REG_ADDR(DT_GPIO_CTLR(FEM_NODE, prop)))

/*
 * Define POL_INV macros expected by radio_nrf5_dppi as needed.
 */

#ifdef HAL_RADIO_GPIO_HAVE_PA_PIN
#if DT_GPIO_FLAGS(FEM_NODE, HAL_RADIO_GPIO_PA_PROP) & GPIO_ACTIVE_LOW
#define HAL_RADIO_GPIO_PA_POL_INV 1
#endif
#define NRF_GPIO_PA       NRF_FEM_GPIO(HAL_RADIO_GPIO_PA_PROP)
#define NRF_GPIO_PA_PIN   DT_GPIO_PIN(FEM_NODE, HAL_RADIO_GPIO_PA_PROP)
#define NRF_GPIO_PA_FLAGS DT_GPIO_FLAGS(FEM_NODE, HAL_RADIO_GPIO_PA_PROP)
#define NRF_GPIO_PA_PSEL  NRF_FEM_PSEL(HAL_RADIO_GPIO_PA_PROP)
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

#ifdef HAL_RADIO_GPIO_HAVE_LNA_PIN
#if DT_GPIO_FLAGS(FEM_NODE, HAL_RADIO_GPIO_LNA_PROP) & GPIO_ACTIVE_LOW
#define HAL_RADIO_GPIO_LNA_POL_INV 1
#endif
#define NRF_GPIO_LNA       NRF_FEM_GPIO(HAL_RADIO_GPIO_LNA_PROP)
#define NRF_GPIO_LNA_PIN   DT_GPIO_PIN(FEM_NODE, HAL_RADIO_GPIO_LNA_PROP)
#define NRF_GPIO_LNA_FLAGS DT_GPIO_FLAGS(FEM_NODE, HAL_RADIO_GPIO_LNA_PROP)
#define NRF_GPIO_LNA_PSEL  NRF_FEM_PSEL(HAL_RADIO_GPIO_LNA_PROP)
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

#ifdef HAL_RADIO_FEM_IS_NRF21540
#if DT_NODE_HAS_PROP(FEM_NODE, pdn_gpios)
#define NRF_GPIO_PDN        NRF_FEM_GPIO(pdn_gpios)
#define NRF_GPIO_PDN_PIN    DT_GPIO_PIN(FEM_NODE, pdn_gpios)
#define NRF_GPIO_PDN_OFFSET DT_PROP(FEM_NODE, pdn_settle_time_us)
#endif /* DT_NODE_HAS_PROP(FEM_NODE, pdn_gpios) */

#if DT_NODE_HAS_PROP(FEM_NODE, ant_sel_gpios)
#define NRF_GPIO_ANT_SEL     NRF_FEM_GPIO(ant_sel_gpios)
#define NRF_GPIO_ANT_SEL_PIN DT_GPIO_PIN(FEM_NODE, ant_sel_gpios)
#endif /* DT_NODE_HAS_PROP(FEM_NODE, ant_sel_gpios) */

#if DT_NODE_HAS_PROP(FEM_NODE, mode_gpios)
#define NRF_GPIO_MODE     NRF_FEM_GPIO(mode_gpios)
#define NRF_GPIO_MODE_PIN DT_GPIO_PIN(FEM_NODE, mode_gpios)
#endif /* DT_NODE_HAS_PROP(FEM_NODE, mode_gpios) */
#endif /* HAL_RADIO_FEM_IS_NRF21540 */

void hubble_board_fem_setup(void)
{
#ifdef HAL_RADIO_GPIO_HAVE_PA_PIN
	NRF_GPIO_PA->DIRSET = BIT(NRF_GPIO_PA_PIN);
	NRF_GPIO_PA->OUTSET = BIT(NRF_GPIO_PA_PIN);
#endif

#ifdef HAL_RADIO_GPIO_HAVE_LNA_PIN
	NRF_GPIO_LNA->DIRSET = BIT(NRF_GPIO_LNA_PIN);
	NRF_GPIO_LNA->OUTCLR = BIT(NRF_GPIO_LNA_PIN);
#endif

#ifdef NRF_GPIO_PDN_PIN
	NRF_GPIO_PDN->DIRSET = BIT(NRF_GPIO_PDN_PIN);
	NRF_GPIO_PDN->OUTSET = BIT(NRF_GPIO_PDN_PIN);
#endif

/* Enable ANT1, disable ANT2 */
#ifdef NRF_GPIO_ANT_SEL_PIN
	NRF_GPIO_ANT_SEL->DIRSET = BIT(NRF_GPIO_ANT_SEL_PIN);
	NRF_GPIO_ANT_SEL->OUTCLR = BIT(NRF_GPIO_ANT_SEL_PIN);
#endif

/* Mode = 0 = 20 dBm factory default */
#ifdef NRF_GPIO_MODE_PIN
	NRF_GPIO_MODE->DIRSET = BIT(NRF_GPIO_MODE_PIN);
	NRF_GPIO_MODE->OUTCLR = BIT(NRF_GPIO_MODE_PIN);
#endif
}


void hubble_board_fem_enable(void)
{
#ifdef NRF_GPIO_PDN_PIN
	NRF_GPIO_PDN->OUTSET = BIT(NRF_GPIO_PDN_PIN);
#endif

#ifdef HAL_RADIO_GPIO_HAVE_PA_PIN
	NRF_GPIO_PA->OUTSET = BIT(NRF_GPIO_PA_PIN);
#endif

#ifdef HAL_RADIO_GPIO_HAVE_LNA_PIN
	NRF_GPIO_LNA->OUTCLR = BIT(NRF_GPIO_LNA_PIN);
#endif
}

void hubble_board_fem_bypass(void)
{
#ifdef HAL_RADIO_GPIO_HAVE_PA_PIN
	NRF_GPIO_PA->OUTSET = BIT(NRF_GPIO_PA_PIN);
#endif

#ifdef HAL_RADIO_GPIO_HAVE_LNA_PIN
	NRF_GPIO_LNA->OUTSET = BIT(NRF_GPIO_LNA_PIN);
#endif

/* nRF21 doesn't have bypass -> this let the pa sleep */
#ifdef NRF_GPIO_PDN_PIN
	NRF_GPIO_PDN->OUTCLR = BIT(NRF_GPIO_PDN_PIN);
#endif
}

void hubble_board_fem_sleep(void)
{
#ifdef HAL_RADIO_GPIO_HAVE_PA_PIN
	NRF_GPIO_PA->OUTCLR = BIT(NRF_GPIO_PA_PIN);
#endif

#ifdef HAL_RADIO_GPIO_HAVE_LNA_PIN
	NRF_GPIO_LNA->OUTCLR = BIT(NRF_GPIO_LNA_PIN);
#endif

#ifdef NRF_GPIO_PDN_PIN
	NRF_GPIO_PDN->OUTCLR = BIT(NRF_GPIO_PDN_PIN);
#endif
}
