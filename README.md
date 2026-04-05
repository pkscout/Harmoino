# Harmoino for Home Assistant
The primary focus of this fork is a new HomeAssistantHub that combines all the remote discovery and operations from the original into one sketch and automatically creates a device in Home Assistant so that you can use your Harmony remote to control things in Home Assistant.

Please see the wiki pages for details on the hardware setup, software install, and operation.

None of this would have been possible without the work joakimjalden did to reverse engineer the radio signals between the Harmony Hub and the remote.  [Here's the original repository](https://github.com/joakimjalden/Harmoino).  The original readme and code are still here are well.  And here's joakimjalden's original acknowledgements too.

## Acknowledgements
None of this would have been possible without the work joakimjalden did to reverse engineer the radio signals between the Harmony Hub and the remote.  [Here's the original repository](https://github.com/joakimjalden/Harmoino).  The original readme and code are still here are well.  And here's joakimjalden's original acknowledgements too.

> First, I wish to acknowledge the [Hacking the Harmony RF Remote](https://haukcode.wordpress.com/2015/04/16/hacking-the-harmony-rf-remote/) blog post on Hakan's Coding and Stuff. This post gave me crucial pointers early on, especially with the radio hardware. I can also recommend the discussion thread for helpful information on pairing the Harmony Remote with the Logitech Unifying Receiver on a PC if you are looking for a more software-based solution. Second, the [Simple nRF24L01+ 2.4GHz transceiver demo](https://forum.arduino.cc/t/simple-nrf24l01-2-4ghz-transceiver-demo/405123) by Robin2 was tremendously helpful in getting started with the RF24 library for the Arduino, and it inspired the minimalistic SimpleHub implementation.
