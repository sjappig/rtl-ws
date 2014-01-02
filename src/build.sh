#!/bin/bash
echo [!] testing
gcc -DTEST resample.c common_sp.c
./a.out
echo [!] compiling c
gcc -c -DINSTALL_DATADIR=\"/home/pi/codebase/github/rtl-ws/resources\" -o rtl-ws-server.o rtl-ws-server.c
gcc -c -o resample.o resample.c 
gcc -c -o common_sp.o common_sp.c
gcc -c -o rtl_sensor.o rtl_sensor.c
gcc -c -o common.o common.c
echo [!] compiling c++
g++ -std=c++0x -c -o spectrum.o spectrum.cpp
echo [!] linking
gcc -o rtl-ws-server -lwebsockets -lrtlsdr -lpthread -lstdc++ spectrum.o rtl-ws-server.o resample.o common_sp.o rtl_sensor.o common.o
