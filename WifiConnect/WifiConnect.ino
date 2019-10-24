/*
* Try to connect to wifi. Setup access point for selection if fail.
* http://github.com/jatocode/WifiConnect
* 
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <ESPmDNS.h>
#include <EEPROM.h>
#include <DNSServer.h>

#define EEPROM_SIZE 200
#define EEPROM_SSID 0
#define EEPROM_PASS 100
#define MAX_EEPROM_LEN 50 // Max length to read ssid/passwd

// Set these to your desired credentials.
const char *ssid = "esp32wifi";
const char *password = "supersecret";
const char *mdnsname = "esp32wifi";

String ipaddress = "";
String savedSSID = "";
String savedPASS = "";
String networks = "";
bool connected = false;

WiFiServer server(80);
DNSServer dnsServer;

// See README.md for an easy way to create these strings
// main html page as one line
const String mainHtmlOutput = "<!DOCTYPE html><html><head>		<title>ESP32</title>		<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/mini.css/3.0.1/mini-default.min.css\">		<style>.button.large {text-align:center;padding:2em ; margin: 1em;font-size:2em;color:black}</style></head><body>		<h1 align=\"center\">ESP32 action</h1>		<br/><br/>		<div class=\"row cols-sm-10\">				<a class=\"button large\" onClick='run(\"A\")' href=\"#\">Do one thing</a>				<a class=\"button large\" onClick='run(\"B\")' href=\"#\">Do another thing</a>		</div>		<div><small>Connected to WiFi: %WIFI%</small></div>		<script>				async function run(param) {						let result = await fetch('/' + param);						/* Use result for something - or not */				}		</script>		</body></html>";
// access point, select wifi as one line
const String apHtmlOutput = "<!DOCTYPE html><html><head>	<title>ESP32 connect to Wifi</title>	<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/mini.css/3.0.1/mini-default.min.css\"></head><body>	<h1 align=\"center\">ESP32 connect to Wifi</h1>	<br /><br />	<form action=\"/C\" method=\"GET\">		<fieldset>			<legend>Connect to wifi</legend>			<div class=\"col-sm-12 col-md-6\">				<select name=\"ssid\">					%SSIDLIST%				</select>			</div>			<div class=\"row\">				<div class=\"col-sm-8 col-md-8\">					<label for=\"password\">Password</label>					<input name=\"p\" type=\"password\" id=\"password\" placeholder=\"Password\" />				</div>			</div>			<button class=\"submit\">Connect</button>			</div>		</fieldset>	</form></body></html>";

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    delay(1000);

    Serial.begin(115200);
    Serial.println();
    Serial.println("Reading settings from EEPROM");
    EEPROM.begin(EEPROM_SIZE);

    savedSSID = readStringEEPROM(EEPROM_SSID);
    savedPASS = readStringEEPROM(EEPROM_PASS);
    Serial.println(savedSSID);
    Serial.println(savedPASS);

    // Try to guess if we got saved data or random unitialized
    if (savedSSID.length() == MAX_EEPROM_LEN || savedPASS.length() == MAX_EEPROM_LEN)
    {
        Serial.println("Unitialized data from EEPROM");
        Serial.println("Setting up AP");
        connected = false;
    }
    else
    {
        Serial.println("Trying to connect to saved WiFi");
        connected = connectWifi(savedSSID, savedPASS);
    }

    if (!connected)
    {
        // Start Access Point
        Serial.println("Setting up access point");

        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        networks = scanNetworks();
        setupAP();
    }

    digitalWrite(LED_BUILTIN, LOW); // Low lights the led

    server.begin();
    Serial.println("Server started");
}

void loop()
{
    if(!connected) 
    {
        // Captive portal. Give our IP to everything
        dnsServer.processNextRequest();
    }
    WiFiClient client = server.available();

    if (client)
    {
        String currentLine = "";
        while (client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                if (c == '\n')
                {
                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0)
                    {
                        if (connected)
                        {
                            // Connected to WiFi. Respond with main page
                            writeMainResponse(client);
                        }
                        else
                        {
                            // Access point. Respond with wifi-select page
                            writeAPResponse(client);
                        }
                        break;
                    }
                    else
                    {
                        if (currentLine.startsWith("GET"))
                        {
                            // Trim to make cleaner switch
                            // GET /xxxx HTTP/1.1
                            String path = currentLine.substring(5, currentLine.length() - 9);
                            switch (path[0])
                            {
                            case 'A':
                                Serial.println("I'm doing a thing");
                                blinkenLight();
                                break;
                            case 'B':
                                Serial.println("I'm doing some other thing");
                                break;
                            case 'C':
                                if (connectionRequest(path))
                                {
                                    // Redirect for convinience
                                    writeRedirectHeader(client, "http://" + String(mdnsname) + ".local");
                                }
                                break;
                            default:
                                break;
                            }
                        }
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {
                    currentLine += c;
                }
            }
        }
        client.stop();
    }
}

bool connectionRequest(String params)
{
    Serial.println(params);
    // Parse parameters by finding =
    // Format is: C?ssid=SSID&p=PASSWORD
    int i = params.indexOf('=', 0);
    int i2 = params.indexOf('&', 0);
    String ssid = params.substring(i + 1, i2);
    ssid.replace('+', ' '); // remove html encoding

    i = params.indexOf('=', i2);
    String pass = params.substring(i + 1);

    Serial.println(ssid);
    Serial.println(pass);

    connected = connectWifi(ssid, pass); // Globals. Me hates it
    return connected;
}

void blinkenLight()
{
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
}

void writeMainResponse(WiFiClient client)
{
    writeOkHeader(client);
    String htmlParsed = mainHtmlOutput;
    htmlParsed.replace("%WIFI%", ipaddress);
    client.println(htmlParsed);
    client.println();
    Serial.println("Wrote main response to client");
}

void writeAPResponse(WiFiClient client)
{
    writeOkHeader(client);
    String htmlParsed = apHtmlOutput;
    htmlParsed.replace("%SSIDLIST%", networks);
    client.println(htmlParsed);
    client.println();
    Serial.println("Wrote AP response to client");
}

void writeOkHeader(WiFiClient client)
{
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();
}

void writeRedirectHeader(WiFiClient client, String redirectUrl)
{
    Serial.println("Redirecting to " + redirectUrl);
    client.println("HTTP/1.1 301 Moved Permanently");
    client.println("Content-type:text/html");
    client.print("Location: ");
    client.println(redirectUrl);
    client.println();
    client.println();
}

void setupAP()
{
    const byte DNS_PORT = 53;

    IPAddress local_IP(10, 0, 1, 1);
    IPAddress subnet(255, 255, 0, 0);

    Serial.println("Configuring access point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_IP, local_IP, subnet);
    //    WiFi.softAP(ssid, password); // Using pwd
    WiFi.softAP(ssid); // Not using pwd

    dnsServer.start(DNS_PORT, "*", local_IP);

    IPAddress myIP = WiFi.softAPIP();
    ipaddress = myIP.toString();
    Serial.println("AP ssid:" + String(ssid));
    Serial.println("AP IP address: " + ipaddress);
}

bool connectWifi(String ssid, String pass)
{
    char ssidA[100];
    char passA[100];
    int i = 5;

    WiFi.disconnect();

    Serial.println("Trying to connect to " + ssid + " with passwd:" + pass);
    // WiFi.begin needs char*
    ssid.toCharArray(ssidA, 99);
    pass.toCharArray(passA, 99);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssidA, passA);

    while (i-- > 0 && WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Connection failed");
        return false;
    }
    ipaddress = WiFi.localIP().toString();
    Serial.println("WiFi connected, IP address: " + ipaddress);

    if (!MDNS.begin(mdnsname))
    {
        Serial.println("Error setting up MDNS responder!");
    }
    else
    {
        Serial.println("mDNS responder started, name:" + String(mdnsname) + ".local");
        MDNS.addService("http", "tcp", 80);
    }
    ipaddress += " (" + String(mdnsname) + ".local)";

    if (ssid != savedSSID || pass != savedPASS)
    {
        // Wifi config has changed, write working to EEPROM
        Serial.println("Writing Wifi config to EEPROM");
        writeStringEEPROM(EEPROM_SSID, ssidA);
        writeStringEEPROM(EEPROM_PASS, passA);
    }

    return true;
}

String scanNetworks()
{
    Serial.println("scan start");

    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0)
    {
        Serial.println("no networks found");
    }
    else
    {
        Serial.print(n);
        Serial.println(" networks found");
        String htmloptions = "";
        for (int i = 0; i < n; ++i)
        {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            htmloptions += "<option>" + String(WiFi.SSID(i)) + "</option>";
            delay(10);
        }
        Serial.println(htmloptions);
        return htmloptions;
    }
    Serial.println("");
    return "";
}

void writeStringEEPROM(char add, String data)
{
    int _size = data.length();
    if (_size > MAX_EEPROM_LEN)
        return;
    int i;
    for (i = 0; i < _size; i++)
    {
        EEPROM.write(add + i, data[i]);
    }
    EEPROM.write(add + _size, '\0'); //Add termination null character for String Data
    EEPROM.commit();
}

String readStringEEPROM(char add)
{
    int i;
    char data[MAX_EEPROM_LEN + 1];
    int len = 0;
    unsigned char k;
    k = EEPROM.read(add);
    while (k != '\0' && len < MAX_EEPROM_LEN) //Read until null character
    {
        k = EEPROM.read(add + len);
        data[len] = k;
        len++;
    }
    data[len] = '\0';
    return String(data);
}
