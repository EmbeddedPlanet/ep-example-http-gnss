# HTTP and GNSS Example for Embedded Planet Modules

This repository contains an example of how to use the HTTP and GNSS features on Embedded Planet's Agora and Chronos modules.

To build for Agora, clone this repository, download all dependencies (`mbed deploy`). Then, build the program for the `EP_AGORA` mbed target.

Make sure your module has a GNSS antenna properly installed and that the antenna has an unobstructed view of the sky (ie: outside works best). It may take a few minutes before a "fix" is achieved and position data starts streaming.

To view the output of the example, use a terminal program to view the debug UART output. You must configure the baud rate to 115200 to ensure the output is displayed properly.
