//******************************************************************************************
//*  Esp_radio -- Webradio receiver for ESP8266, (color) display and VS1053 MP3 module,    *
//*               by Ed Smallenburg (ed@smallenburg.nl)                                    *
//*  With ESP8266 running at 80 MHz, it is capable of handling up to 256 kb bitrate.       *
//*  With ESP8266 running at 160 MHz, it is capable of handling up to 320 kb bitrate.      *
//******************************************************************************************
// ESP8266 libraries used:
//  - ESP8266WiFi       - Part of ESP8266 Arduino default libraries.
//  - SPI               - Part of Arduino default libraries.
//  - Adafruit_GFX      - https://github.com/adafruit/Adafruit-GFX-Library
//  - TFT_ILI9163C      - https://github.com/sumotoy/TFT_ILI9163C
//  - ESPAsyncTCP       - https://github.com/me-no-dev/ESPAsyncTCP
//  - ESPAsyncWebServer - https://github.com/me-no-dev/ESPAsyncWebServer
//  - FS - https://github.com/esp8266/arduino-esp8266fs-plugin/releases/download/0.2.0/ESP8266FS-0.2.0.zip
//  - ArduinoOTA        - Part of ESP8266 Arduino default libraries.
//  - AsyncMqttClient   - https://github.com/marvinroger/async-mqtt-client
//  - TinyXML           - Fork https://github.com/adafruit/TinyXML
//
// A library for the VS1053 (for ESP8266) is not available (or not easy to find).  Therefore
// a class for this module is derived from the maniacbug library and integrated in this sketch.
//
// See http://www.internet-radio.com for suitable stations.  Add the stations of your choice
// to the .ini-file.
//
// Brief description of the program:
// First a suitable WiFi network is found and a connection is made.
// Then a connection will be made to a shoutcast server.  The server starts with some
// info in the header in readable ascii, ending with a double CRLF, like:
//  icy-name:Classic Rock Florida - SHE Radio
//  icy-genre:Classic Rock 60s 70s 80s Oldies Miami South Florida
//  icy-url:http://www.ClassicRockFLorida.com
//  content-type:audio/mpeg
//  icy-pub:1
//  icy-metaint:32768          - Metadata after 32768 bytes of MP3-data
//  icy-br:128                 - in kb/sec (for Ogg this is like "icy-br=Quality 2"
//
// After de double CRLF is received, the server starts sending mp3- or Ogg-data.  For mp3, this
// data may contain metadata (non mp3) after every "metaint" mp3 bytes.
// The metadata is empty in most cases, but if any is available the content will be presented on the TFT.
// Pushing the input button causes the player to select the next preset station present in the .ini file.
//
// The display used is a Chinese 1.8 color TFT module 128 x 160 pixels.  The TFT_ILI9163C.h
// file has been changed to reflect this particular module.  TFT_ILI9163C.cpp has been
// changed to use the full screenwidth if rotated to mode "3".  Now there is room for 26
// characters per line and 16 lines.  Software will work without installing the display.
// If no TFT is used, you may use GPIO2 and GPIO15 as control buttons.  See definition of "USETFT" below.
// Switches are than programmed as:
// GPIO2 : "Goto station 1"
// GPIO0 : "Next station"
// GPIO15: "Previous station".  Note that GPIO15 has to be LOW when starting the ESP8266.
//         The button for GPIO15 must therefore be connected to VCC (3.3V) instead of GND.

//
// For configuration of the WiFi network(s): see the global data section further on.
//
// The SPI interface for VS1053 and TFT uses hardware SPI.
//
// Wiring:
// NodeMCU  GPIO    Pin to program  Wired to LCD        Wired to VS1053      Wired to rest
// -------  ------  --------------  ---------------     -------------------  ---------------------
// D0       GPIO16  16              -                   pin 1 DCS            -
// D1       GPIO5    5              -                   pin 2 CS             LED on nodeMCU
// D2       GPIO4    4              -                   pin 4 DREQ           -
// D3       GPIO0    0 FLASH        -                   -                    Control button "Next station"
// D4       GPIO2    2              pin 3 (D/C)         -                    (OR)Control button "Station 1"
// D5       GPIO14  14 SCLK         pin 5 (CLK)         pin 5 SCK            -
// D6       GPIO12  12 MISO         -                   pin 7 MISO           -
// D7       GPIO13  13 MOSI         pin 4 (DIN)         pin 6 MOSI           -
// D8       GPIO15  15              pin 2 (CS)          -                    (OR)Control button "Previous station"
// D9       GPI03    3 RXD0         -                   -                    Reserved serial input
// D10      GPIO1    1 TXD0         -                   -                    Reserved serial output
// -------  ------  --------------  ---------------     -------------------  ---------------------
// GND      -        -              pin 8 (GND)         pin 8 GND            Power supply
// VCC 3.3  -        -              pin 6 (VCC)         -                    LDO 3.3 Volt
// VCC 5 V  -        -              pin 7 (BL)          pin 9 5V             Power supply
// RST      -        -              pin 1 (RST)         pin 3 RESET          Reset circuit
//
// The reset circuit is a circuit with 2 diodes to GPIO5 and GPIO16 and a resistor to ground
// (wired OR gate) because there was not a free GPIO output available for this function.
// This circuit is included in the documentation.
// Issues:
// Webserver produces error "LmacRxBlk:1" after some time.  After that it will work very slow.
// The program will reset the ESP8266 in such a case.  Now we have switched to async webserver,
// the problem still exists, but the program will not crash anymore.
// Upload to ESP8266 not reliable.
//
// 31-03-2016, ES: First set-up.
// 01-04-2016, ES: Detect missing VS1053 at start-up.
// 05-04-2016, ES: Added commands through http server on port 80.
// 14-04-2016, ES: Added icon and switch preset on stream error.
// 18-04-2016, ES: Added SPIFFS for webserver.
// 19-04-2016, ES: Added ringbuffer.
// 20-04-2016, ES: WiFi Passwords through SPIFFS files, enable OTA.
// 21-04-2016, ES: Switch to Async Webserver.
// 27-04-2016, ES: Save settings, so same volume and preset will be used after restart.
// 03-05-2016, ES: Add bass/treble settings (see also new index.html).
// 04-05-2016, ES: Allow stations like "skonto.ls.lv:8002/mp3".
// 06-05-2016, ES: Allow hiddens WiFi station if this is the only .pw file.
// 07-05-2016, ES: Added preset selection in webserver.
// 12-05-2016, ES: Added support for Ogg-encoder.
// 13-05-2016, ES: Better Ogg detection.
// 17-05-2016, ES: Analog input for commands, extra buttons if no TFT required.
// 26-05-2016, ES: Fixed BUTTON3 bug (no TFT).
// 27-05-2016, ES: Fixed restore station at restart.
// 04-07-2016, ES: WiFi.disconnect clears old connection now (thanks to Juppit).
// 23-09-2016, ES: Added commands via MQTT and Serial input, Wifi set-up in AP mode.
// 04-10-2016, ES: Configuration in .ini file. No more use of EEPROM and .pw files.
// 11-10-2016, ES: Allow stations that have no bitrate in header like icecast.err.ee/raadio2.mp3.
// 14-10-2016, ES: Updated for async-mqtt-client-master 0.5.0
// 22-10-2016, ES: Correction mute/unmute.
// 15-11-2016, ES: Support for .m3u playlists.
// 22-12-2016, ES: Support for localhost (play from SPIFFS).
// 28-12-2016, ES: Implement "Resume" request.
// 31-12-2016, ES: Allow ContentType "text/css".
// 02-01-2017, ES: Webinterface in PROGMEM.
// 16-01-2017, ES: Correction playlists.
// 17-01-2017, ES: Bugfix config page and playlist.
// 23-01-2017, ES: Bugfix playlist.
// 26-01-2017, ES: Check on wrong icy-metaint.
// 30-01-2017, ES: Allow chunked transfer encoding.
// 01-02-2017, ES: Bugfix file upload.
// 26-04-2017, ES: Better output webinterface on preset change.
// 03-05-2017, ES: Prevent to start inputstream if no network.
// 04-05-2017, ES: Integrate iHeartRadio, thanks to NonaSuomy.
// 09-05-2017, ES: Fixed abs problem.
// 11-05-2017, ES: Convert UTF8 characters before display, thanks to everyb313.
// 24-05-2017, ES: Correction. Do not skip first part of .mp3 file.
// 26-05-2017, ES: Correction playing from .m3u playlist and LC/UC problem.
// 31-05-2017, ES: Volume indicator on TFT.
// 02-02-2018, ES: Force 802.11N connection.
// 18-04-2018, ES: Workaround for not working wifi.connected().
// 05-10-2018, ES: Fixed exception if no network was found.
//
// Define the version number, also used for webserver as Last-Modified header:
#define VERSION "Fri, 05 Oct 2018 09:30:00 GMT"
// TFT.  Define USETFT if required.
//#define USETFT
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncMqttClient.h>
#include <SPI.h>
#if defined ( USETFT )
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>
#endif
#include <Ticker.h>
#include <stdio.h>
#include <string.h>
#include <FS.h>
#include <ArduinoOTA.h>
#include <TinyXML.h>
#include <SPIFFS.h>

extern "C"
{
//#include "user_interface.h"
}

// Definitions for 3 control switches on analog input
// You can test the analog input values by holding down the switch and select /?analog=1
// in the web interface. See schematics in the documentation.
// Switches are programmed as "Goto station 1", "Next station" and "Previous station" respectively.
// Set these values to 2000 if not used or tie analog input to ground.
#define NUMANA  3
//#define asw1    252
//#define asw2    334
//#define asw3    499
#define asw1    2000
#define asw2    2000
#define asw3    2000
//
// Color definitions for the TFT screen (if used)
#define BLACK   0x0000
#define BLUE    0xF800
#define RED     0x001F
#define GREEN   0x07E0
#define CYAN    GREEN | BLUE
#define MAGENTA RED | BLUE
#define YELLOW  RED | GREEN
// Digital I/O used
// Pins for VS1053 module
#define VS1053_CS     5
#define VS1053_DCS    16
#define VS1053_DREQ   4
// Pins CS and DC for TFT module (if used, see definition of "USETFT")
#define TFT_CS 15
#define TFT_DC 2
// Control button (GPIO) for controlling station
#define BUTTON1 2
#define BUTTON2 0
#define BUTTON3 15
// Ringbuffer for smooth playing. 20000 bytes is 160 Kbits, about 1.5 seconds at 128kb bitrate.
#define RINGBFSIZ 20000
// Debug buffer size
#define DEBUG_BUFFER_SIZE 100
// Name of the ini file
#define INIFILENAME "/radio.ini"
// Access point name if connection to WiFi network fails.  Also the hostname for WiFi and OTA.
// Not that the password of an AP must be at least as long as 8 characters.
// Also used for other naming.
#define NAME "Esp-radio"
// Maximum number of MQTT reconnects before give-up
#define MAXMQTTCONNECTS 20
//
//******************************************************************************************
// Forward declaration of various functions                                                *
//******************************************************************************************
//void   displayinfo ( const char* str, uint16_t pos, uint16_t height, uint16_t color ) ;
void   showstreamtitle ( const char* ml, bool full = false ) ;
void   handlebyte ( uint8_t b, bool force = false ) ;
void   handlebyte_ch ( uint8_t b, bool force = false ) ;
void   handleFS ( AsyncWebServerRequest* request ) ;
void   handleFSf ( AsyncWebServerRequest* request, const String& filename ) ;
void   handleCmd ( AsyncWebServerRequest* request )  ;
void   handleFileUpload ( AsyncWebServerRequest* request, String filename,
                          size_t index, uint8_t* data, size_t len, bool final ) ;
char*  dbgprint( const char* format, ... ) ;
char*  analyzeCmd ( const char* str ) ;
char*  analyzeCmd ( const char* par, const char* val ) ;
String chomp ( String str ) ;
void   publishIP() ;
String xmlparse ( String mount ) ;
void   put_eeprom_station ( int index, const char *entry ) ;
char*  get_eeprom_station ( int index ) ;
int    find_eeprom_station ( const char *search_entry ) ;
int    find_free_eeprom_entry() ;
bool   connecttohost() ;


//
//******************************************************************************************
// Global data section.                                                                    *
//******************************************************************************************
// There is a block ini-data that contains some configuration.  Configuration data is      *
// saved in the SPIFFS file radio.ini by the webinterface.  On restart the new data will   *
// be read from this file.                                                                 *
// Items in ini_block can be changed by commands from webserver/MQTT/Serial.               *
//******************************************************************************************
struct ini_struct
{
  String         mqttbroker ;                              // The name of the MQTT broker server
  uint16_t       mqttport ;                                // Port, default 1883
  String         mqttuser ;                                // User for MQTT authentication
  String         mqttpasswd ;                              // Password for MQTT authentication
  String         mqtttopic ;                               // Topic to suscribe to
  String         mqttpubtopic ;                            // Topic to pubtop (IP will be published)
  uint8_t        reqvol ;                                  // Requested volume
  uint8_t        rtone[4] ;                                // Requested bass/treble settings
  int8_t         newpreset ;                               // Requested preset
  String         ssid ;                                    // SSID of WiFi network to connect to
  String         passwd ;                                  // Password for WiFi network
} ;

enum datamode_t { INIT = 1, HEADER = 2, DATA = 4,
                  METADATA = 8, PLAYLISTINIT = 16,
                  PLAYLISTHEADER = 32, PLAYLISTDATA = 64,
                  STOPREQD = 128, STOPPED = 256
                } ;        // State for datastream

// Global variables
int              DEBUG = 1 ;
ini_struct       ini_block ;                               // Holds configurable data
WiFiClient       *mp3client = NULL ;                       // An instance of the mp3 client
AsyncWebServer   cmdserver ( 80 ) ;                        // Instance of embedded webserver on port 80
AsyncMqttClient  mqttclient ;                              // Client for MQTT subscriber
IPAddress        mqtt_server_IP ;                          // IP address of MQTT broker
char             cmd[130] ;                                // Command from MQTT or Serial
#if defined ( USETFT )
TFT_ILI9163C     tft = TFT_ILI9163C ( TFT_CS, TFT_DC ) ;
#endif
Ticker           tckr ;                                    // For timing 100 msec
TinyXML          xml;                                      // For XML parser.
uint32_t         totalcount = 0 ;                          // Counter mp3 data
datamode_t       datamode ;                                // State of datastream
int              metacount ;                               // Number of bytes in metadata
int              datacount ;                               // Counter databytes before metadata
String           metaline ;                                // Readable line in metadata
String           icystreamtitle ;                          // Streamtitle from metadata
String           icyname ;                                 // Icecast station name
int              bitrate ;                                 // Bitrate in kb/sec
int              metaint = 0 ;                             // Number of databytes between metadata
int8_t           currentpreset = -1 ;                      // Preset station playing
String           host ;                                    // The URL to connect to or file to play
String           playlist ;                                // The URL of the specified playlist
bool             xmlreq = false ;                          // Request for XML parse.
bool             hostreq = false ;                         // Request for new host
bool             reqtone = false ;                         // New tone setting requested
bool             muteflag = false ;                        // Mute output
uint8_t*         ringbuf ;                                 // Ringbuffer for VS1053
uint16_t         rbwindex = 0 ;                            // Fill pointer in ringbuffer
uint16_t         rbrindex = RINGBFSIZ - 1 ;                // Emptypointer in ringbuffer
uint16_t         rcount = 0 ;                              // Number of bytes in ringbuffer
uint16_t         analogsw[NUMANA] = { asw1, asw2, asw3 } ; // 3 levels of analog input
uint16_t         analogrest ;                              // Rest value of analog input
bool             resetreq = false ;                        // Request to reset the ESP8266
bool             NetworkFound ;                            // True if WiFi network connected
String           networks ;                                // Found networks
String           anetworks ;                               // Aceptable networks (present in .ini file)
String           presetlist ;                              // List for webserver
uint8_t          num_an ;                                  // Number of acceptable networks in .ini file
String           testfilename = "" ;                       // File to test (SPIFFS speed)
uint16_t         mqttcount = 0 ;                           // Counter MAXMQTTCONNECTS
int8_t           playlist_num = 0 ;                        // Nonzero for selection from playlist
File             mp3file  ;                                // File containing mp3 on SPIFFS
bool             localfile = false ;                       // Play from local mp3-file or not
bool             chunked = false ;                         // Station provides chunked transfer
int              chunkcount = 0 ;                          // Counter for chunked transfer

// XML parse globals.
const char* xmlhost = "playerservices.streamtheworld.com" ;// XML data source
const char* xmlget =  "GET /api/livestream"                // XML get parameters
                      "?version=1.5"                       // API Version of IHeartRadio
                      "&mount=%sAAC"                       // MountPoint with Station Callsign
                      "&lang=en" ;                         // Language
int         xmlport = 80 ;                                 // XML Port
uint8_t     xmlbuffer[150] ;                               // For XML decoding
String      xmlOpen ;                                      // Opening XML tag
String      xmlTag ;                                       // Current XML tag
String      xmlData ;                                      // Data inside tag
String      stationServer( "" ) ;                          // Radio stream server
String      stationPort( "" ) ;                            // Radio stream port
String      stationMount( "" ) ;                           // Radio stream Callsign

//******************************************************************************************
// End of global data section.                                                             *
//******************************************************************************************

//******************************************************************************************
// Pages and CSS for the webinterface.                                                     *
//******************************************************************************************
#include "about_html.h"
#include "config_html.h"
#include "index_html.h"
#include "radio_css.h"
#include "favicon_ico.h"
//

//******************************************************************************************
//                                   O T A S T A R T                                       *
//******************************************************************************************
// Update via WiFi has been started by Arduino IDE.                                        *
//******************************************************************************************
void otastart()
{
  dbgprint ( "OTA Started" ) ;
}

//******************************************************************************************
//                                   S E T U P                                             *
//******************************************************************************************
// Setup for the program.                                                                  *
//******************************************************************************************
void setup()
{
  FSInfo      fs_info ;                                // Info about SPIFFS
  Dir         dir ;                                    // Directory struct for SPIFFS
  File        f ;                                      // Filehandle
  String      filename ;                               // Name of file found in SPIFFS

  Serial.begin ( 115200 ) ;                            // For debug
  Serial.println() ;
  system_update_cpu_freq ( 160 ) ;                     // Set to 80/160 MHz
  ringbuf = (uint8_t *) malloc ( RINGBFSIZ ) ;         // Create ring buffer
  xml.init ( xmlbuffer, sizeof(xmlbuffer),             // Initilize XML stream.
             &XML_callback ) ;
  memset ( &ini_block, 0, sizeof(ini_block) ) ;        // Init ini_block
  ini_block.mqttport = 1883 ;                          // Default port for MQTT
  SPIFFS.begin() ;                                     // Enable file system
  // Show some info about the SPIFFS
  SPIFFS.info ( fs_info ) ;
  dbgprint ( "FS Total %d, used %d", fs_info.totalBytes, fs_info.usedBytes ) ;
  if ( fs_info.totalBytes == 0 )
  {
    dbgprint ( "No SPIFFS found!  See documentation." ) ;
  }
  dir = SPIFFS.openDir("/") ;                          // Show files in FS
  while ( dir.next() )                                 // All files
  {
    f = dir.openFile ( "r" ) ;
    filename = dir.fileName() ;
    dbgprint ( "%-32s - %7d",                          // Show name and size
               filename.c_str(), f.size() ) ;
  }
  mk_lsan() ;                                          // Make a list of acceptable networks in ini file.
  listNetworks() ;                                     // Search for WiFi networks
  readinifile() ;                                      // Read .ini file
  getpresets() ;                                       // Get the presets from .ini-file
  WiFi.setPhyMode ( WIFI_PHY_MODE_11N ) ;              // Force 802.11N connection
  WiFi.persistent ( false ) ;                          // Do not save SSID and password
  WiFi.disconnect() ;                                  // The router may keep the old connection
  WiFi.mode ( WIFI_STA ) ;                             // This ESP is a station
  wifi_station_set_hostname ( (char*)NAME ) ;
  SPI.begin() ;                                        // Init SPI bus
  // Print some memory and sketch info
  dbgprint ( "Starting ESP Version %s...  Free memory %d",
             VERSION,
             system_get_free_heap_size() ) ;
  dbgprint ( "Sketch size %d, free size %d",
             ESP.getSketchSize(),
             ESP.getFreeSketchSpace() ) ;
  pinMode ( BUTTON2, INPUT_PULLUP ) ;                  // Input for control button 2
  vs1053player.begin() ;                               // Initialize VS1053 player
#if defined ( USETFT )
  tft.begin() ;                                        // Init TFT interface
  tft.fillRect ( 0, 0, 160, 128, BLACK ) ;             // Clear screen does not work when rotated
  tft.setRotation ( 3 ) ;                              // Use landscape format
  tft.clearScreen() ;                                  // Clear screen
  tft.setTextSize ( 1 ) ;                              // Small character font
  tft.setTextColor ( WHITE ) ;                         // Info in white
  tft.println ( "Starting" ) ;
  tft.println ( "Version:" ) ;
  tft.println ( VERSION ) ;
#else
  pinMode ( BUTTON1, INPUT_PULLUP ) ;                  // Input for control button 1
  pinMode ( BUTTON3, INPUT_PULLUP ) ;                  // Input for control button 3
#endif
  delay(10);
  analogrest = ( analogRead ( A0 ) + asw1 ) / 2  ;     // Assumed inactive analog input
  tckr.attach ( 0.100, timer100 ) ;                    // Every 100 msec
  dbgprint ( "Selected network: %-25s", ini_block.ssid.c_str() ) ;
  NetworkFound = connectwifi() ;                       // Connect to WiFi network
  //NetworkFound = false ;                             // TEST, uncomment for no network test
  dbgprint ( "Start server for commands" ) ;
  cmdserver.on ( "/", handleCmd ) ;                    // Handle startpage
  cmdserver.onNotFound ( handleFS ) ;                  // Handle file from FS
  cmdserver.onFileUpload ( handleFileUpload ) ;        // Handle file uploads
  cmdserver.begin() ;
  if ( NetworkFound )                                  // OTA and MQTT only if Wifi network found
  {
    ArduinoOTA.setHostname ( NAME ) ;                  // Set the hostname
    ArduinoOTA.onStart ( otastart ) ;
    ArduinoOTA.begin() ;                               // Allow update over the air
    if ( ini_block.mqttbroker.length() )               // Broker specified?
    {
      // Initialize the MQTT client
      WiFi.hostByName ( ini_block.mqttbroker.c_str(),
                        mqtt_server_IP ) ;             // Lookup IP of MQTT server
      mqttclient.onConnect ( onMqttConnect ) ;
      mqttclient.onDisconnect ( onMqttDisconnect ) ;
      mqttclient.onSubscribe ( onMqttSubscribe ) ;
      mqttclient.onUnsubscribe ( onMqttUnsubscribe ) ;
      mqttclient.onMessage ( onMqttMessage ) ;
      mqttclient.onPublish ( onMqttPublish ) ;
      mqttclient.setServer ( mqtt_server_IP,           // Specify the broker
                             ini_block.mqttport ) ;    // And the port
      mqttclient.setCredentials ( ini_block.mqttuser.c_str(),
                                  ini_block.mqttpasswd.c_str() ) ;
      mqttclient.setClientId ( NAME ) ;
      dbgprint ( "Connecting to MQTT %s, port %d, user %s, password %s...",
                 ini_block.mqttbroker.c_str(),
                 ini_block.mqttport,
                 ini_block.mqttuser.c_str(),
                 ini_block.mqttpasswd.c_str() ) ;
      mqttclient.connect();
    }
  }
  else
  {
    currentpreset = ini_block.newpreset ;              // No network: do not start radio
  }
  delay ( 1000 ) ;                                     // Show IP for a wile
  analogrest = ( analogRead ( A0 ) + asw1 ) / 2  ;     // Assumed inactive analog input
}



//******************************************************************************************
//                                   L O O P                                               *
//******************************************************************************************
// Main loop of the program.  Minimal time is 20 usec.  Will take about 4 msec if VS1053   *
// needs data.                                                                             *
// Sometimes the loop is called after an interval of more than 100 msec.                   *
// In that case we will not be able to fill the internal VS1053-fifo in time (especially   *
// at high bitrate).                                                                       *
// A connection to an MP3 server is active and we are ready to receive data.               *
// Normally there is about 2 to 4 kB available in the data stream.  This depends on the    *
// sender.                                                                                 *
//******************************************************************************************
void loop()
{
  uint32_t    maxfilechunk  ;                           // Max number of bytes to read from
                                                        // stream or file
  // Try to keep the ringbuffer filled up by adding as much bytes as possible
  if ( datamode & ( INIT | HEADER | DATA |              // Test op playing
                    METADATA | PLAYLISTINIT |
                    PLAYLISTHEADER |
                    PLAYLISTDATA ) )
  {
    if ( localfile )
    {
      maxfilechunk = mp3file.available() ;              // Bytes left in file
      if ( maxfilechunk > 1024 )                        // Reduce byte count for this loop()
      {
        maxfilechunk = 1024 ;
      }
      while ( ringspace() && maxfilechunk-- )
      {
        putring ( mp3file.read() ) ;                    // Yes, store one byte in ringbuffer
        yield() ;
      }
    }
    else
    {
      maxfilechunk = mp3client->available() ;          // Bytes available from mp3 server
      if ( maxfilechunk > 1024 )                       // Reduce byte count for this loop()
      {
        maxfilechunk = 1024 ;
      }
      while ( ringspace() && maxfilechunk-- )
      {
        putring ( mp3client->read() ) ;                // Yes, store one byte in ringbuffer
        yield() ;
      }
    }
    yield() ;
  }
  while ( vs1053player.data_request() && ringavail() ) // Try to keep VS1053 filled
  {
    handlebyte_ch ( getring() ) ;                      // Yes, handle it
  }
  yield() ;
  if ( datamode == STOPREQD )                          // STOP requested?
  {
    dbgprint ( "STOP requested" ) ;
    if ( localfile )
    {
      mp3file.close() ;
    }
    else
    {
      stop_mp3client() ;                               // Disconnect if still connected
    }
    handlebyte_ch ( 0, true ) ;                        // Force flush of buffer
    vs1053player.setVolume ( 0 ) ;                     // Mute
    vs1053player.stopSong() ;                          // Stop playing
    emptyring() ;                                      // Empty the ringbuffer
    datamode = STOPPED ;                               // Yes, state becomes STOPPED
#if defined ( USETFT )
    tft.fillRect ( 0, 0, 160, 128, BLACK ) ;           // Clear screen does not work when rotated
#endif
    delay ( 500 ) ;
  }
  if ( localfile )
  {
    if ( datamode & ( INIT | HEADER | DATA |           // Test op playing
                      METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER |
                      PLAYLISTDATA ) )
    {
      if ( ( mp3file.available() == 0 ) && ( ringavail() == 0 ) )
      {
        datamode = STOPREQD ;                          // End of local mp3-file detected
      }
    }
  }
  if ( ini_block.newpreset != currentpreset )          // New station or next from playlist requested?
  {
    if ( datamode != STOPPED )                         // Yes, still busy?
    {
      datamode = STOPREQD ;                            // Yes, request STOP
    }
    else
    {
      if ( playlist_num )                               // Playing from playlist?
      { // Yes, retrieve URL of playlist
        playlist_num += ini_block.newpreset -
                        currentpreset ;                 // Next entry in playlist
        ini_block.newpreset = currentpreset ;           // Stay at current preset
      }
      else
      {
        host = readhostfrominifile(ini_block.newpreset) ; // Lookup preset in ini-file
      }
      dbgprint ( "New preset/file requested (%d/%d) from %s",
                 currentpreset, playlist_num, host.c_str() ) ;
      if ( host != ""  )                                // Preset in ini-file?
      {
        hostreq = true ;                                // Force this station as new preset
      }
      else
      {
        // This preset is not available, return to preset 0, will be handled in next loop()
        ini_block.newpreset = 0 ;                       // Wrap to first station
      }
    }
  }
  if ( hostreq )                                        // New preset or station?
  {
    hostreq = false ;
    currentpreset = ini_block.newpreset ;               // Remember current preset
    
    localfile = host.startsWith ( "localhost/" ) ;      // Find out if this URL is on localhost
    if ( localfile )                                    // Play file from localhost?
    {
      if ( connecttofile() )                            // Yes, open mp3-file
      {
        datamode = DATA ;                               // Start in DATA mode
      }
    }
    else
    {
      if ( host.startsWith ( "ihr/" ) )                 // iHeartRadio station requested?
      {
        host = host.substring ( 4 ) ;                   // Yes, remove "ihr/"
        host = xmlparse ( host ) ;                      // Parse the xml to get the host
      }
      connecttohost() ;                                 // Switch to new host
    }
  }
  if ( xmlreq )                                         // Directly xml requested?
  {
    xmlreq = false ;                                    // Yes, clear request flag
    host = xmlparse ( host ) ;                          // Parse the xml to get the host
    connecttohost() ;                                   // and connect to this host
  }
  if ( reqtone )                                        // Request to change tone?
  {
    reqtone = false ;
    vs1053player.setTone ( ini_block.rtone ) ;          // Set SCI_BASS to requested value
  }
  if ( resetreq )                                       // Reset requested?
  {
    delay ( 1000 ) ;                                    // Yes, wait some time
    ESP.restart() ;                                     // Reboot
  }
  if ( muteflag )
  {
    vs1053player.setVolume ( 0 ) ;                      // Mute
  }
  else
  {
    vs1053player.setVolume ( ini_block.reqvol ) ;       // Unmute
  }
  displayvolume() ;                                     // Show volume on display
  if ( testfilename.length() )                          // File to test?
  {
    testfile ( testfilename ) ;                         // Yes, do the test
    testfilename = "" ;                                 // Clear test request
  }
  scanserial() ;                                        // Handle serial input
  ArduinoOTA.handle() ;                                 // Check for OTA
}




