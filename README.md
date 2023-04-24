GarageDoorOpener

A ESP32 with LORA module (in this case a TTGO LoRa32-Oled) is connected to the garage door UHF remote at the contact of the "open" button and ground (battery -). 
The Garage is opened either via MQTT or by receiving the LORA signal transmitted from the LoRa sender in the car.

The sketch is OTA enabled. The LoRa receiver is a TTGO LoRa32-Oled and the LoRa transmitter in the car is a Heltec 151 LoRa Node.
The TTGO LoRa32-Oled is service a simple web page in order to be monitored for being alive via a web request on its IP.

The LORA sender is connected to the rear light which turn on when selecting reverse gear and therefore it turns on and sends 
the LORA packet when the reverse gear is selected. That is done in this way because the car gets parked by reversing into the garage.

You can see a short video here: https://youtu.be/jy5k5pi7eWQ
