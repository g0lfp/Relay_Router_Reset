# Relay_Router_Reset
ESP8266 module based relay board router resetter

Calls a page on my 5and9.co.uk server, if it can't get a reply, it waits 10 minutes and tries a different page. If after calling the second page it receives no reply, it initiates a power cycle of the router. From v0.2 it waits before trying to reconnect/continue, in case the router takes longer to restart the wifi than it should. This is set to 10 seconds. Feel free to shorten it. 
There are two pages that I call. I expect to see the first page being called at regular intervals. The second page only gets called if there's a problem. 

v0.0 Initial commit

v0.1 Bug fixes

v0.2 12.06.2020 Added delays to stop the ESP trying to connect to the WiFi before the router had finished booting. These are way longer than needed. Feel free to shorten them. 

Comments to my twitter @g0lfp please.
