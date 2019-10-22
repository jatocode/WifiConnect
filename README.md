# WifiConnect
Arduino/ESP32 code for creating an AP and selecting a WiFi to connect to.
At start tries to connect to saved WiFi, if it fails it switches over to an Access Point and serves a wifi-selection page.

Feel free to use and modify as you wish. pull-requests appreciated! 

## Usage

Serves two HTML pages
* one main page when connected that can run actions on the ESP (without reloads) 
* one access point page that gives you the possibility to logon to another network

## Neat stuff

* Simple way to create somewhat dynamic HTML-pages
* Easy way to design HTML-pages and serve from your Arduino without a WebServer-library
* How to use the ESP32 as a very simple backend
* How to setup AP and WiFi and mDNS on ESP32
* How to read/write to EEPROM on ESP32/Arduino
* How to parse a simple FORM GET request

## How to change HTML

Included are two demo HTML-pages, main.html and accesspoint.html. 

### To update the HTML
You need to join all lines in those to one (ctrl-A and then ctrl-J in VSCode), also replace all " with \\".  
**OR** use the htmlString.sh script that does that for you.

Copy/paste the oneliner into one of the variables in the main program.


