# Mongoose OS app to test ESP32 and CAP1106 touch IC

## Overview

This is an example app to test CAP1106 touch with ESP32 over mongoose-os

## Hardware setup

connect following pins between CAP1106 and ESP32:
```
SDA -> IO26
SCL -> IO32
ALERT -> IO35
```

## How to install this app

See Mongoose OS page https://mongoose-os.com/software.html

## Runing

Test over RPC service
```powershell
mos --port COM7 call I2C.ReadRegB '{\"addr\":40,\"reg\":255}'
```
