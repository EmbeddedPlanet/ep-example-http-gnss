/**
 * Embedded Planet GNSS Example
 *
 * Built with ARM Mbed-OS
 *
 * Copyright (c) 2021 Embedded Planet, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "mbed.h"

#include "TELIT_ME910_GNSS.h"
#include "mbed_trace.h"

#define TRACE_GROUP   "MAIN"

using namespace ep;

#if TELIT_ME910_GNSS_ENABLED
using gnss_type = TELIT_ME910_GNSS;
#elif TELIT_ME310_GNSS_ENABLED
using gnss_type = TELIT_ME310_GNSS;
#endif

int main()
{

    // Initialize mbed trace
    mbed_trace_init();

#if !MBED_CONF_CELLULAR_DEBUG_AT
    mbed_trace_exclude_filters_set("CELL");
#endif

    tr_info("***************************************");
    tr_info("* Embedded Planet GNSS Example v0.1.0 *");
    tr_info("***************************************");

    // GNSS object
    gnss_type gnss;

    // PositionInfo object
    GNSS::PositionInfo position_info;

    // Initialize the connection
    tr_info("Initializing MEx10 module...");
    gnss.init();

#if TELIT_ME310_GNSS_ENABLED
    // Set eDRX paramters according to recommendations in the ME310
    // GNSS application note for use with Cat-M1
    gnss.set_edrx_parameters(gnss_type::EDRX_MODE_ENABLE, gnss_type::EDRX_ACT_CAT_M1, "0101", "0001");

    // Set WWAN as the priority
    tr_info("Setting WWAN as the priority");
    gnss.set_gnss_priority(gnss_type::PRIORITY_WWAN);

    // // Set GNSS as the priority
    // tr_info("Setting GNSS as the priority");
    // gnss.set_gnss_priority(gnss_type::PRIORITY_GNSS);

#endif

    // Enable power to the GNSS controller
    tr_info("Enabling MEx10 GNSS controller");
    gnss.enable();

    // Configure the GNSS data stream (enable GGA and RMC)
#if TELIT_ME310_GNSS_ENABLED
    gnss.configure_gnss_data_stream(gnss_type::NMEA_STREAM_ENABLE_SECOND_FORMAT, true, false, false, false, true, false);
#else
    gnss.configure_gnss_data_stream(gnss_type::NMEA_STREAM_ENABLE_SECOND_FORMAT, true, false, false, true, true, false);
#endif
    // Configure the extended GNSS data stream (enable GNRMC and GPGGA)
#if TELIT_ME310_GNSS_ENABLED
    gnss.configure_gnss_data_stream_extended(false, false, false, false, false, false, false, true, false, false, false, true, false);
#endif

    while (1) {
        // Sleep for 10s
        ThisThread::sleep_for(10s);

        // Print out current position info
        position_info = gnss.get_current_position();

        tr_info("Satellites in View:                            %d", position_info.NumberOfSatellites);

        switch (position_info.Fix) {
            case GNSS::FIX_TYPE_INVALID:
                tr_warn("Invalid fix");
                break;
            case GNSS::FIX_TYPE_2D:
            case GNSS::FIX_TYPE_3D:
                tr_info("FIX INFO:");
                tr_info("\tPosition:                            %d° %.5f' %c %d° %.5f' %c",
                    position_info.Latitude.degrees, 
                    position_info.Latitude.minutes,
                    position_info.Latitude.cardinal,
                    position_info.Longitude.degrees, 
                    position_info.Longitude.minutes,
                    position_info.Longitude.cardinal);
                tr_info("\tHorizontal dilution of precision:    %.1f m", position_info.HorizontalDilutionOfPrecision);
                tr_info("\tAltitude:                            %.1f m", position_info.Altitude);
                tr_info("\tFix type:                            %s", position_info.Fix == GNSS::FIX_TYPE_2D ? "2D" : "3D");
                tr_info("\tCourse over ground:                  %.1f", position_info.CourseOverGround);
                tr_info("\tSpeed over ground:                   %.1f km/hr", position_info.SpeedOverGround);
                tr_info("\tTimestamp:                           %s", ctime(&position_info.UtcTimestamp));
                break;
            default:
            case GNSS::FIX_TYPE_UNKNOWN:
                tr_error("Unknown fix, error occurred");
                break;
        }
    }
}
