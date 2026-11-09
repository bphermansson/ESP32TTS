# ESP32TTS
An ESP32 that uses a MAX98357A DAC/Amplifier and a speaker to speak.

- Configure connections to the MAX98357A in main.cpp.

- Create 'settings.h" in folder include with this content:

'''
#ifndef SETTINGS_H
#define SETTINGS_H
    #define WIFI_SSID "SSID"
    #define WIFI_PASSWORD "Password"
#endif // SETTINGS_H
'''