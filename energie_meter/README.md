Install the ESP8266 (add link of additional boards)
then under devices install ESP8266 and than SELECT the ESP8266


# before upload to the esp
make sure speed is 160MHz of the cpu, you can simply do this in the arduino -> tools -> cpu frequency



# links
* http://domoticx.com/p1-poort-slimme-meter-data-naar-domoticz-esp8266/
* https://github.com/jantenhove/P1-Meter-ESP8266/issues/3
* http://domoticx.com/p1-poort-slimme-meter-hardware/
* http://domoticx.com/arduino-p1-poort-telegrammen-uitlezen/
* https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_a727fce1f1.pdf
* http://www.gejanssen.com/howto/Slimme-meter-uitlezen/
* https://github.com/gejanssen/slimmemeter-rpi/blob/master/P1uitlezer-ESMR50.py
* http://gejanssen.com/howto/Slimme-meter-uitlezen/Slimme_meter_ESMR50.pdf

#  install mqtt with docker with the following config:
The config file: `vim /srv/mosquitto/config/mosquitto.conf`
``` config

persistence true
persistence_location /mosquitto/data/

user mosquitto

# Listen on all interfaces
listener 1883

#Allow connection without authentication
allow_anonymous true

log_dest file /mosquitto/log/mosquitto.log
log_dest stdout

password_file /mosquitto/config/credentials
```
Start the docker
``` bash
sudo docker run -itd \
--name=mqtt \
--restart=always \
--net=host \
-v /srv/mosquitto/config:/mosquitto/config \
-v /srv/mosquitto/data:/mosquitto/data \
-v /srv/mosquitto/log:/mosquitto/log \
eclipse-mosquitto

```
Setup username
``` bash
docker exec -it mqtt sh
mosquitto_passwd -c /mosquitto/config/credentials username
# enter poassword
# done
````



# home assist part


Change the TZ, see home-assist webpage
``` bash
docker run -d
            --name homeassistant2
            --privileged
            --restart=unless-stopped
            -e TZ=NL
            -v /srv/homeassistant_docker:/config
            --device=/dev/ttyACM0
            --network=host
            ghcr.io/home-assistant/home-assistant:stable

```
> Note: not all of the units are correctly added.
``` yaml

mqtt:
  sensor:
    - name: "consumption_low_tarif"
      state_topic: "sensors/power/p1meter/consumption_low_tarif"
      unit_of_measurement: "KWh"
    - name: "consumption_high_tarif"
      state_topic: "sensors/power/p1meter/consumption_high_tarif"
      unit_of_measurement: "KWh"
    - name: "returndelivery_low_tarif"
      state_topic: "sensors/power/p1meter/returndelivery_low_tarif"
      unit_of_measurement: "KWh"
    - name: "returndelivery_high_tarif"
      state_topic: "sensors/power/p1meter/returndelivery_high_tarif"
      unit_of_measurement: "KWh"
    - name: "actual_consumption"
      state_topic: "sensors/power/p1meter/actual_consumption"
      unit_of_measurement: "W"
    - name: "actual_returndelivery"
      state_topic: "sensors/power/p1meter/actual_returndelivery"
      unit_of_measurement: "KW"
    - name: "l1_instant_power_usage"
      state_topic: "sensors/power/p1meter/l1_instant_power_usage"
      unit_of_measurement: "KW"
    - name: "l2_instant_power_usage"
      state_topic: "sensors/power/p1meter/l2_instant_power_usage"
      unit_of_measurement: "KW"
    - name: "l3_instant_power_usage"
      state_topic: "sensors/power/p1meter/l3_instant_power_usage"
      unit_of_measurement: "kW"
    - name: "l1_instant_power_current"
      state_topic: "sensors/power/p1meter/l1_instant_power_current"
      unit_of_measurement: "A"
    - name: "l2_instant_power_current"
      state_topic: "sensors/power/p1meter/l2_instant_power_current"
      unit_of_measurement: "A"
    - name: "l3_instant_power_current"
      state_topic: "sensors/power/p1meter/l3_instant_power_current"
      unit_of_measurement: "A"
    - name: "l1_voltage"
      state_topic: "sensors/power/p1meter/l1_voltage"
      unit_of_measurement: "V"
    - name: "l2_voltage"
      state_topic: "sensors/power/p1meter/l2_voltage"
      unit_of_measurement: "V"
    - name: "l3_voltage"
      state_topic: "sensors/power/p1meter/l3_voltage"
      unit_of_measurement: "V"
    - name: "gas_meter_m3"
      state_topic: "sensors/power/p1meter/gas_meter_m3"
      unit_of_measurement: "m³"
    - name: "actual_tarif_group"
      state_topic: "sensors/power/p1meter/actual_tarif_group"
    - name: "short_power_outages"
      state_topic: "sensors/power/p1meter/short_power_outages"
    - name: "long_power_outages"
      state_topic: "sensors/power/p1meter/long_power_outages"
    - name: "short_power_drops"
      state_topic: "sensors/power/p1meter/short_power_drops"
    - name: "short_power_peaks"
      state_topic: "sensors/power/p1meter/short_power_peaks"
```


# example data
``` log
1-3:0.2.8(50)
0-0:1.0.0(220807181627S)
0-0:96.1.1(4530303433303037303730333134363138)
1-0:1.8.1(005284.087*kWh)
1-0:1.8.2(006582.992*kWh)
1-0:2.8.1(000030.868*kWh)
1-0:2.8.2(000060.777*kWh)
0-0:96.14.0(0001)
1-0:1.7.0(00.000*kW)
1-0:2.7.0(00.822*kW)
0-0:96.7.21(00008)
0-0:96.7.9(00003)
1-0:99.97.0(1)(0-0:96.7.19)(191214015112W)(0000004571*s)
1-0:32.32.0(00010)
1-0:32.36.0(00001)
0-0:96.13.0()
1-0:32.7.0(240.8*V)
1-0:31.7.0(003*A)
1-0:21.7.0(00.000*kW)
1-0:22.7.0(00.820*kW)
0-1:24.1.0(003)
0-1:96.1.0(4730303339303031383037363739343138)
0-1:24.2.1(220807181505S)(04042.770*m3)
!E752
/ISK5\2M550E-1012

1-3:0.2.8(50)
0-0:1.0.0(220807181628S)
0-0:96.1.1(4530303433303037303730333134363138)
1-0:1.8.1(005284.087*kWh)
1-0:1.8.2(006582.992*kWh)
1-0:2.8.1(000030.868*kWh)
1-0:2.8.2(000060.777*kWh)
0-0:96.14.0(0001)
1-0:1.7.0(00.000*kW)
1-0:2.7.0(00.834*kW)
0-0:96.7.21(00008)
0-0:96.7.9(00003)
1-0:99.97.0(1)(0-0:96.7.19)(191214015112W)(0000004571*s)
1-0:32.32.0(00010)
1-0:32.36.0(00001)
0-0:96.13.0()
1-0:32.7.0(240.7*V)
1-0:31.7.0(003*A)
1-0:21.7.0(00.000*kW)
1-0:22.7.0(00.841*kW)
0-1:24.1.0(003)
0-1:96.1.0(4730303339303031383037363739343138)
0-1:24.2.1(220807181505S)(04042.770*m3)
!7A22

```