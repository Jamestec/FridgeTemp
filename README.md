# FridgeTemp
Wireless and automatic fridge temperature monitoring with an ESP32 and Raspberry Pi. Supports multiple sensors concurrently.

ESP32_FridgeTemp.ino is main code for ESP32. The board I'm using is the Lolin D32 Pro and the sensor is a Si7021 to TMP117. There is usable EPD code for the is Wemos EPD (it sucks since the MISO cable isn't used and could've carried BUSY instead), but I'm currently not using it.

If you're using a Sparkfun TMP117, it is possible to use the I2C port on the Lolin D32 Pro with Sparkfun's Qwiic connectors, but you'll have to rearrange some wires.
<details>
  <summary>Instructions on rearranging a wire</summary>
  I will start to explain how to rearrange a wire by first defining that the all plastic side of the wire end/housing is the top part and the side with some exposed metal is the bottom part of the wire end.

* The I2C interface looking at the top with wires going out to the right, should be ordered from top to bottom: ground (Black), SDA (Blue), SCL (Yellow) and 3.3V (Red).
* The QWIIC connector looking at the top with wires going out to the left, should be ordered from top to bottom: ground (Black), 3.3V (Red), SDA (Blue) and SCL (Yellow).
* You will have to disconnect all the individual wires from its housing on one end of the wire. I did this by getting something small and pokey (a Dupont wire) and pressing down on the exposed metal on the bottom side of the housing: you should feel something depress and the wire should slide out of it's housing. After rearranging the wires and inserting them back in, they will be a bit loose, but once you connect the wire to the boards, they won't fall out. If you forget which way the wire goes in the hole, the side with two slits faces the top part of the housing.

If you do this wrong, you might smell an electrical burning smell: this may be from the resettable fuse inside the Lolin D32 Pro's voltage regulator. Simply for some time and the voltage regulator should start working again.

Note that if you're looking at Wemos's schematic of the Lolin D32 Pro, you can interpret the I2C port from either the perspective of the "back" of the board, where "back" means there are no surface-mount components visible, or from the perspective of looking into the I2C port and with the micro-USB port above the ESP-32 chip itself.
</details>

Placeholder:
Hello Every-nyan,

I wish I were a bird.

Thank you for reading my speech.
