/*
 * Copyright (c) 2025 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ephemeris.h
 * @brief Hubble Network Satellite Ephemeris APIs
 **/

#ifndef INCLUDE_HUBBLE_SAT_EPHEMERIS_H
#define INCLUDE_HUBBLE_SAT_EPHEMERIS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct hubble_sat_orbital_params
 * @brief Represents the orbital parameters of a satellite.
 *
 * This structure contains the parameters required to describe the orbit
 * of a satellite, including its position, motion, and orientation.
 */
struct hubble_sat_orbital_params {
	/** Reference epoch time (seconds since Unix epoch) */
	uint64_t t0;
	/** Mean motion at epoch (radians per second) */
	double n0;
	/** Rate of change of mean motion (radians per second^2) */
	double ndot;
	/** Right ascension of ascending node at epoch (radians) */
	double raan0;
	/** Rate of change of RAAN (radians per second) */
	double raandot;
	/** Argument of perigee at epoch (radians) */
	double aop0;
	/** Rate of change of argument of perigee (radians per second) */
	double aopdot;
	/** Inclination (degrees) */
	double inclination;
	/** Eccentricity (unitless, 0=circular) */
	double eccentricity;
	/** NORAD ID of the satellite */
	uint32_t sat_id;
};

/**
 * @struct hubble_sat_device_pos
 * @brief Represents the location of a device.
 *
 * This structure contains the latitude and longitude of a
 * device on Earth.
 */
struct hubble_sat_device_pos {
	/** Latitude in degrees. */
	double lat;
	/** Longitude in degrees. */
	double lon;
};

/**
 * @struct hubble_sat_device_region
 * @brief Represents a rectangular geographic region.
 *
 * This structure defines a rectangular region using
 * a center point (latitude/longitude) and range values.
 */
struct hubble_sat_device_region {
	/** Latitude of the region center in degrees. */
	double lat_mid;
	/** Total latitude range in degrees (region extends lat_range/2 above and below lat_mid). */
	double lat_range;
	/** Longitude of the region center in degrees. */
	double lon_mid;
	/** Total longitude range in degrees (region extends lon_range/2 east and west of lon_mid). */
	double lon_range;
};

/**
 * @struct hubble_sat_pass_info
 * @brief Represents information about a satellite pass.
 *
 * This structure contains details about a satellite's pass over a
 * specific location, including the time, longitude, and whether the
 * satellite is ascending or descending.
 */
struct hubble_sat_pass_info {
	/** Longitude of the satellite pass (degrees, East positive). */
	double lon;
	/** Time of the satellite pass (Unix time, seconds since epoch). */
	uint64_t t;
	/** Time duration of the pass in seconds. */
	uint32_t duration;
	/** True if the satellite is ascending (moving northward), false if descending. */
	bool ascending;
};

/**
 * @brief Set orbital information for satellites.
 *
 * This function stores orbital parameters for one or more satellites. The parameters
 * are used by hubble_next_pass_get() and hubble_next_pass_region_get() to compute
 * the next satellite pass across all configured satellites.
 *
 * @note The caller must ensure the @p satellites array remains valid for the lifetime
 *       of subsequent pass calculations, as the data is not copied internally.
 *
 * @note To clear the satellite list, call with @p satellites set to NULL and
 *       @p count set to 0.
 *
 * @param satellites Pointer to an array of satellite orbital parameters. May be NULL
 *                   only if @p count is 0.
 * @param count Number of entries in the array. Pass 0 to clear the satellite list.
 * @return 0 on success.
 * @retval -EINVAL if @p satellites is NULL and @p count is greater than 0.
 */
int hubble_sat_satellites_set(
	const struct hubble_sat_orbital_params *const satellites, size_t count);

/**
 * @brief Get the next satellite pass.
 *
 * This function calculates the next pass of a satellite over a
 * given location, based on satellites orbital parameters
 * and the device's location.
 *
 * @param t Current time or the time from which to start the calculation.
 * @param pos Pointer to the device's location.
 * @param pass The next satellite pass in case of success.
 * @return 0 on success or a negative value in case of error.
 */
int hubble_next_pass_get(uint64_t t, const struct hubble_sat_device_pos *pos,
			 struct hubble_sat_pass_info *pass);

/**
 * @brief Get the next satellite pass over a geographic region.
 *
 * This function calculates the next pass of a satellite over a
 * rectangular geographic region defined by latitude and longitude
 * bounds, based on satellites orbital parameters.
 *
 * @param t Current time or the time from which to start the calculation.
 * @param region Pointer to the geographic region definition.
 * @param pass The next satellite pass in case of success.
 * @return 0 on success or a negative value in case of error.
 */
int hubble_next_pass_region_get(uint64_t t,
				const struct hubble_sat_device_region *region,
				struct hubble_sat_pass_info *pass);
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_HUBBLE_SAT_EPHEMERIS_H */
