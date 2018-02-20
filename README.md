# homie-cpp
Homie C++ header only library

Supports Homie version 3.0 (redesign branch) both as a device and a master role.
It is not built around a specific mqtt library, so you can use what ever you like to.

#### Devicemode
In device mode you can publish a device and react to changes sent via mqtt.
Because mqtt allows only one testatment topic you need multiple connections
to the broker if you want to implement more than one device.

#### Master
Allows you to discover devices connected to a broker and set properties.
One connection can be used for multiple master instances (e.g. for multiple basetopics)
because we do not need a testament in master mode.
