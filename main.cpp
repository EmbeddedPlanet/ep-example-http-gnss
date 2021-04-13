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

#include "http_request.h"

#define TRACE_GROUP   "MAIN"

using namespace ep;

#if TELIT_ME910_GNSS_ENABLED
using gnss_type = TELIT_ME910_GNSS;
#elif TELIT_ME310_GNSS_ENABLED
using gnss_type = TELIT_ME310_GNSS;
#endif

void dump_response(HttpResponse* res) {
    printf("Status: %d - %s\n", res->get_status_code(), res->get_status_message().c_str());
    printf("Headers:\n");
    for (size_t ix = 0; ix < res->get_headers_length(); ix++) {
        printf("\t%s: %s\n", res->get_headers_fields()[ix]->c_str(), res->get_headers_values()[ix]->c_str());
    }
    printf("\nBody (%d bytes):\n\n%s\n", res->get_body_length(), res->get_body_as_string().c_str());
}

int main()
{

    // Initialize mbed trace
    mbed_trace_init();

#if !MBED_CONF_CELLULAR_DEBUG_AT
    mbed_trace_exclude_filters_set("CELL");
#endif

    tr_info("********************************************");
    tr_info("* Embedded Planet HTTP GNSS Example v0.1.0 *");
    tr_info("********************************************");

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
// #else
//     gnss.configure_gnss_data_stream(gnss_type::NMEA_STREAM_ENABLE_SECOND_FORMAT, true, false, false, true, true, false);
#endif
    // Configure the extended GNSS data stream (enable GNRMC and GPGGA)
#if TELIT_ME310_GNSS_ENABLED
    gnss.configure_gnss_data_stream_extended(false, false, false, false, false, false, false, true, false, false, false, true, false);
#endif

    uint32_t device_address = NRF_FICR->DEVICEADDR[0];

    char addr_buf[255] = "";
    char msg_buf[60] = "";

    // Print Dweet web addres to the serial terminal
    snprintf(addr_buf, 255, "https://dweet.io/follow/ep-example-http-gnss_%lu", device_address);
    tr_info("Device Dweet Address: %s", addr_buf);

    NetworkInterface *network = NetworkInterface::get_default_instance();
    if (!network) {
        tr_error("No network interface found");
        while(1) {
            ThisThread::sleep_for(10ms);
        }
    }

    nsapi_error_t connect_status = network->connect();

    if (connect_status != NSAPI_ERROR_OK) {
        tr_error("Failed to connect to network (%d)", connect_status);
        while(1) {
            ThisThread::sleep_for(10ms);
        }
    }

    tr_info("Connected to the network");
    SocketAddress socket_address;
    network->get_ip_address(&socket_address);
    tr_info("IP address: %s", socket_address.get_ip_address());

    while (1) {
        TCPSocket *socket;
        nsapi_error_t open_result;
        nsapi_error_t connect_result;
        SocketAddress sa;

        // Sleep for 10s
        ThisThread::sleep_for(10s);

        // Print out current position info
#if TELIT_ME310_GNSS_ENABLED
        position_info = gnss.get_current_position();
#else
        position_info = gnss.get_current_position(false);
#endif

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

                tr_info("----- Setting up TCP connection -----");
                socket = new TCPSocket();
                open_result = socket->open(network);
                if (open_result != NSAPI_ERROR_OK) {
                    tr_error("Opening TCPSocket failed... %d", open_result);
                    break;
                }

                network->gethostbyname("dweet.io", &sa);
                sa.set_port(80);
                connect_result = socket->connect(sa);
                if (connect_result != NSAPI_ERROR_OK) {
                    tr_error("Connecting over TCPSocket failed... %d", connect_result);
                    break;
                }
                tr_info("Connected over TCP to dweet.io:80");

                {
                    snprintf(addr_buf, 255, "https://dweet.io:443/dweet/for/ep-example-http-gnss_%lu", device_address);
                    HttpRequest* post_req = new HttpRequest(socket, HTTP_POST, addr_buf);
                    post_req->set_header("Content-Type", "application/json");

                    // Send a simple http request
                    tr_info("Sending HTTP POST");
                    double lat = position_info.Latitude.degrees + (position_info.Latitude.minutes / 60.0f);
                    if (position_info.Latitude.cardinal == NMEA_CARDINAL_DIR_SOUTH) {
                        lat = -lat;
                    }
                    double lon = position_info.Longitude.degrees + (position_info.Longitude.minutes / 60.0f);
                    if (position_info.Longitude.cardinal == NMEA_CARDINAL_DIR_WEST) {
                        lon = -lon;
                    }
                    snprintf(msg_buf,60,"{\"lat\":%.5f,\"lon\":%.5f}", lat, lon);            
                    HttpResponse* post_res = post_req->send(msg_buf, strlen(msg_buf));
                    if (!post_res) {
                        tr_error("HttpRequest failed (error code %d)\n", post_req->get_error());
                        socket->close();
                        delete socket;
                        delete post_req;
                        break;
                    }
                    else
                    {
                        tr_info("----- HTTP POST response -----");
                        dump_response(post_res);
                        delete post_req;
                    }
                }
                socket->close();

                break;
            default:
            case GNSS::FIX_TYPE_UNKNOWN:
                tr_error("Unknown fix, error occurred");
                break;
        }
    }
}
