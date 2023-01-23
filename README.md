# Home-Assist_sensors
For those boards you need a ESP and for the Openterm device you need a special board that can handle >10V to TLL voltage.

# energie-meter P!-sensor
The P1-sensor with bitbang serial communication. So you don't need external chips to invert the signal from the energy meter. This causes the communication to be less stable, however you can easily miss a message without problems.

# ESP WiFi thermostat standalone
When you do not have Home-assistant you can use this thermostat.

# ESP Home Assistant OpenTerm
Connect any compatible OpenTerm CV with Home Assistant. Using MQTT

# ESP thermal-camera
The thermal camera is connected using the esp to wifi. Goal is to make a hotspot people can join to go to the webpage to stream the realtime camera.
[See blogpost](https://martijnvwezel.com/blogs/esp32_heatcamera/)
<img src="https://martijnvwezel.com/img/heatcamera/Heatcamera.png" alt="side" height="350" class="center"/>
# installations for Arduino IDE:

Under additional board manager URL's:
```
http://arduino.esp8266.com/stable/package_esp8266com_index.json
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json
```
Under board Manage boards:
```
* Openterm (Ihor Melnyk)
* Adafruit_MLX90640
* Adafruit esp8266
* More I guess

```



