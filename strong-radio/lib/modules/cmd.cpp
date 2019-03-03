//******************************************************************************************
//                             S C A N S E R I A L                                         *
//******************************************************************************************
// Listen to commands on the Serial inputline.                                             *
//******************************************************************************************
void scanserial()
{
  static String serialcmd ;                      // Command from Serial input
  char          c ;                              // Input character
  char*         reply ;                          // Reply string froma analyzeCmd
  uint16_t      len ;                            // Length of input string

  while ( Serial.available() )                   // Any input seen?
  {
    c =  (char)Serial.read() ;                   // Yes, read the next input character
    //Serial.write ( c ) ;                       // Echo
    len = serialcmd.length() ;                   // Get the length of the current string
    if ( ( c == '\n' ) || ( c == '\r' ) )
    {
      if ( len )
      {
        strncpy ( cmd, serialcmd.c_str(), sizeof(cmd) ) ;
        reply = analyzeCmd ( cmd) ;              // Analyze command and handle it
        dbgprint ( reply ) ;                     // Result for debugging
        serialcmd = "" ;                         // Prepare for new command
      }
    }
    if ( c >= ' ' )                              // Only accept useful characters
    {
      serialcmd += c ;                           // Add to the command
    }
    if ( len >= ( sizeof(cmd) - 2 )  )           // Check for excessive length
    {
      serialcmd = "" ;                           // Too long, reset
    }
  }
}

//******************************************************************************************
//                             A N A L Y Z E C M D                                         *
//******************************************************************************************
// Handling of the various commands from remote webclient, Serial or MQTT.                 *
// Version for handling string with: <parameter>=<value>                                   *
//******************************************************************************************
char* analyzeCmd ( const char* str )
{
  char*  value ;                                 // Points to value after equalsign in command

  value = strstr ( str, "=" ) ;                  // See if command contains a "="
  if ( value )
  {
    *value = '\0' ;                              // Separate command from value
    value++ ;                                    // Points to value after "="
  }
  else
  {
    value = (char*) "0" ;                        // No value, assume zero
  }
  return  analyzeCmd ( str, value ) ;            // Analyze command and handle it
}

//******************************************************************************************
//                             A N A L Y Z E C M D                                         *
//******************************************************************************************
// Handling of the various commands from remote webclient, serial or MQTT.                 *
// par holds the parametername and val holds the value.                                    *
// "wifi_00" and "preset_00" may appear more than once, like wifi_01, wifi_02, etc.        *
// Examples with available parameters:                                                     *
//   preset     = 12                        // Select start preset to connect to           *
//   preset_00  = <mp3 stream>              // Specify station for a preset 00-99 *)       *
//   volume     = 95                        // Percentage between 0 and 100                *
//   upvolume   = 2                         // Add percentage to current volume            *
//   downvolume = 2                         // Subtract percentage from current volume     *
//   toneha     = <0..15>                   // Setting treble gain                         *
//   tonehf     = <0..15>                   // Setting treble frequency                    *
//   tonela     = <0..15>                   // Setting bass gain                           *
//   tonelf     = <0..15>                   // Setting treble frequency                    *
//   station    = <mp3 stream>              // Select new station (will not be saved)      *
//   station    = <URL>.mp3                 // Play standalone .mp3 file (not saved)       *
//   station    = <URL>.m3u                 // Select playlist (will not be saved)         *
//   stop                                   // Stop playing                                *
//   resume                                 // Resume playing                              *
//   mute                                   // Mute the music                              *
//   unmute                                 // Unmute the music                            *
//   wifi_00    = mySSID/mypassword         // Set WiFi SSID and password *)               *
//   mqttbroker = mybroker.com              // Set MQTT broker to use *)                   *
//   mqttport   = 1883                      // Set MQTT port to use, default 1883 *)       *
//   mqttuser   = myuser                    // Set MQTT user for authentication *)         *
//   mqttpasswd = mypassword                // Set MQTT password for authentication *)     *
//   mqtttopic  = mytopic                   // Set MQTT topic to subscribe to *)           *
//   mqttpubtopic = mypubtopic              // Set MQTT topic to publish to *)             *
//   status                                 // Show current URL to play                    *
//   testfile   = <file on SPIFFS>          // Test SPIFFS reads for debugging purpose     *
//   test                                   // For test purposes                           *
//   debug      = 0 or 1                    // Switch debugging on or off                  *
//   reset                                  // Restart the ESP8266                         *
//   analog                                 // Show current analog input                   *
// Commands marked with "*)" are sensible in ini-file only                                 *
// Note that it is adviced to avoid expressions as the argument for the abs function.      *
//******************************************************************************************
char* analyzeCmd ( const char* par, const char* val )
{
  String             argument ;                       // Argument as string
  String             value ;                          // Value of an argument as a string
  int                ivalue ;                         // Value of argument as an integer
  static char        reply[250] ;                     // Reply to client, will be returned
  uint8_t            oldvol ;                         // Current volume
  bool               relative ;                       // Relative argument (+ or -)
  int                inx ;                            // Index in string

  strcpy ( reply, "Command accepted" ) ;              // Default reply
  argument = chomp ( par ) ;                          // Get the argument
  if ( argument.length() == 0 )                       // Lege commandline (comment)?
  {
    return reply ;                                    // Ignore
  }
  argument.toLowerCase() ;                            // Force to lower case
  value = chomp ( val ) ;                             // Get the specified value
  ivalue = value.toInt() ;                            // Also as an integer
  ivalue = abs ( ivalue ) ;                           // Make it absolute
  relative = argument.indexOf ( "up" ) == 0 ;         // + relative setting?
  if ( argument.indexOf ( "down" ) == 0 )             // - relative setting?
  {
    relative = true ;                                 // It's relative
    ivalue = - ivalue ;                               // But with negative value
  }
  if ( value.startsWith ( "http://" ) )               // Does (possible) URL contain "http://"?
  {
    value.remove ( 0, 7 ) ;                           // Yes, remove it
  }
  if ( value.length() )
  {
    dbgprint ( "Command: %s with parameter %s",
               argument.c_str(), value.c_str() ) ;
  }
  else
  {
    dbgprint ( "Command: %s (without parameter)",
               argument.c_str() ) ;
  }
  if ( argument.indexOf ( "volume" ) >= 0 )           // Volume setting?
  {
    // Volume may be of the form "upvolume", "downvolume" or "volume" for relative or absolute setting
    oldvol = vs1053player.getVolume() ;               // Get current volume
    if ( relative )                                   // + relative setting?
    {
      ini_block.reqvol = oldvol + ivalue ;            // Up by 0.5 or more dB
    }
    else
    {
      ini_block.reqvol = ivalue ;                     // Absolue setting
    }
    if ( ini_block.reqvol > 100 )
    {
      ini_block.reqvol = 100 ;                        // Limit to normal values
    }
    sprintf ( reply, "Volume is now %d",              // Reply new volume
              ini_block.reqvol ) ;
  }
  else if ( argument == "mute" )                      // Mute request
  {
    muteflag = true ;                                 // Request volume to zero
  }
  else if ( argument == "unmute" )                    // Unmute request?
  {
    muteflag = false ;                                // Request normal volume
  }
  else if ( argument.indexOf ( "preset" ) >= 0 )      // Preset station?
  {
    if ( !argument.startsWith ( "preset_" ) )         // But not a station URL
    {
      if ( relative )                                 // Relative argument?
      {
        ini_block.newpreset += ivalue ;               // Yes, adjust currentpreset
      }
      else
      {
        ini_block.newpreset = ivalue ;                // Otherwise set preset station
      }
      sprintf ( reply, "Preset is now %d",            // Reply new preset
                ini_block.newpreset ) ;
      playlist_num = 0 ;
    }
  }
  else if ( argument == "stop" )                      // Stop requested?
  {
    if ( datamode & ( HEADER | DATA | METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER | PLAYLISTDATA ) )

    {
      datamode = STOPREQD ;                           // Request STOP
    }
    else
    {
      strcpy ( reply, "Command not accepted!" ) ;     // Error reply
    }
  }
  else if ( argument == "resume" )                    // Request to resume?
  {
    if ( datamode == STOPPED )                        // Yes, are we stopped?
    {
      hostreq = true ;                                // Yes, request restart
    }
  }
  else if ( argument == "station" )                   // Station in the form address:port
  {
    if ( datamode & ( HEADER | DATA | METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER | PLAYLISTDATA ) )
    {
      datamode = STOPREQD ;                           // Request STOP
    }
    host = value ;                                    // Save it for storage and selection later
    hostreq = true ;                                  // Force this station as new preset
    sprintf ( reply,
              "New preset station %s accepted",       // Format reply
              host.c_str() ) ;
  }
  else if ( argument == "xml" )
  {
    if ( datamode & ( HEADER | DATA | METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER | PLAYLISTDATA ) )
    {
      datamode = STOPREQD ;                           // Request STOP
    }
    host = value ;                                    // Save it for storage and selection later
    xmlreq = true ;                                   // Run XML parsing process.
    sprintf ( reply,
              "New xml preset station %s accepted",   // Format reply
              host.c_str() ) ;
  }
  else if ( argument == "status" )                    // Status request
  {
    if ( datamode == STOPPED )
    {
      sprintf ( reply, "Player stopped" ) ;           // Format reply
    }
    else
    {
      sprintf ( reply, "%s - %s", icyname.c_str(),
                icystreamtitle.c_str() ) ;            // Streamtitle from metadata
    }
  }
  else if ( argument.startsWith ( "reset" ) )         // Reset request
  {
    resetreq = true ;                                 // Reset all
  }
  else if ( argument == "testfile" )                  // Testfile command?
  {
    testfilename = value ;                            // Yes, set file to test accordingly
  }
  else if ( argument == "test" )                      // Test command
  {
    sprintf ( reply, "Free memory is %d, ringbuf %d, stream %d",
              system_get_free_heap_size(), rcount, mp3client->available() ) ;
  }
  // Commands for bass/treble control
  else if ( argument.startsWith ( "tone" ) )          // Tone command
  {
    if ( argument.indexOf ( "ha" ) > 0 )              // High amplitue? (for treble)
    {
      ini_block.rtone[0] = ivalue ;                   // Yes, prepare to set ST_AMPLITUDE
    }
    if ( argument.indexOf ( "hf" ) > 0 )              // High frequency? (for treble)
    {
      ini_block.rtone[1] = ivalue ;                   // Yes, prepare to set ST_FREQLIMIT
    }
    if ( argument.indexOf ( "la" ) > 0 )              // Low amplitue? (for bass)
    {
      ini_block.rtone[2] = ivalue ;                   // Yes, prepare to set SB_AMPLITUDE
    }
    if ( argument.indexOf ( "lf" ) > 0 )              // High frequency? (for bass)
    {
      ini_block.rtone[3] = ivalue ;                   // Yes, prepare to set SB_FREQLIMIT
    }
    reqtone = true ;                                  // Set change request
    sprintf ( reply, "Parameter for bass/treble %s set to %d",
              argument.c_str(), ivalue ) ;
  }
  else if ( argument.startsWith ( "mqtt" ) )          // Parameter fo MQTT?
  {
    strcpy ( reply, "MQTT broker parameter changed. Save and restart to have effect" ) ;
    if ( argument.indexOf ( "broker" ) > 0 )          // Broker specified?
    {
      ini_block.mqttbroker = value.c_str() ;          // Yes, set broker accordingly
    }
    else if ( argument.indexOf ( "port" ) > 0 )       // Port specified?
    {
      ini_block.mqttport = ivalue ;                   // Yes, set port user accordingly
    }
    else if ( argument.indexOf ( "user" ) > 0 )       // User specified?
    {
      ini_block.mqttuser = value ;                    // Yes, set user accordingly
    }
    else if ( argument.indexOf ( "passwd" ) > 0 )     // Password specified?
    {
      ini_block.mqttpasswd = value.c_str() ;          // Yes, set broker password accordingly
    }
    else if ( argument.indexOf ( "pubtopic" ) > 0 )   // Publish topic specified?
    {
      ini_block.mqttpubtopic = value.c_str() ;        // Yes, set broker password accordingly
    }
    else if ( argument.indexOf ( "topic" ) > 0 )      // Topic specified?
    {
      ini_block.mqtttopic = value.c_str() ;           // Yes, set broker topic accordingly
    }
  }
  else if ( argument == "debug" )                     // debug on/off request?
  {
    DEBUG = ivalue ;                                  // Yes, set flag accordingly
  }
  else if ( argument == "analog" )                    // Show analog request?
  {
    sprintf ( reply, "Analog input = %d units",       // Read the analog input for test
              analogRead ( A0 ) ) ;
  }
  else if ( argument.startsWith ( "wifi" ) )          // WiFi SSID and passwd?
  {
    inx = value.indexOf ( "/" ) ;                     // Find separator between ssid and password
    // Was this the strongest SSID or the only acceptable?
    if ( num_an == 1 )
    {
      ini_block.ssid = value.substring ( 0, inx ) ;   // Only one.  Set as the strongest
    }
    if ( value.substring ( 0, inx ) == ini_block.ssid )
    {
      ini_block.passwd = value.substring ( inx + 1 ) ; // Yes, set password
    }
  }
  else if ( argument == "getnetworks" )               // List all WiFi networks?
  {
    sprintf ( reply, networks.c_str() ) ;             // Reply is SSIDs
  }
  else
  {
    sprintf ( reply, "%s called with illegal parameter: %s",
              NAME, argument.c_str() ) ;
  }
  return reply ;                                      // Return reply to the caller
}


//******************************************************************************************
//                             H A N D L E C M D                                           *
//******************************************************************************************
// Handling of the various commands from remote (case sensitive). All commands have the    *
// form "/?parameter[=value]".  Example: "/?volume=50".                                    *
// The startpage will be returned if no arguments are given.                               *
// Multiple parameters are ignored.  An extra parameter may be "version=<random number>"   *
// in order to prevent browsers like Edge and IE to use their cache.  This "version" is    *
// ignored.                                                                                *
// Example: "/?upvolume=5&version=0.9775479450590543"                                      *
// The save and the list commands are handled specially.                                   *
//******************************************************************************************
void handleCmd ( AsyncWebServerRequest* request )
{
  AsyncWebParameter* p ;                                // Points to parameter structure
  static String      argument ;                         // Next argument in command
  static String      value ;                            // Value of an argument
  const char*        reply ;                            // Reply to client
  //uint32_t         t ;                                // For time test
  int                params ;                           // Number of params
  static File        f ;                                // Handle for writing /radio.ini to SPIFFS

  //t = millis() ;                                      // Timestamp at start
  params = request->params() ;                          // Get number of arguments
  if ( params == 0 )                                    // Any arguments
  {
    if ( NetworkFound )
    {
      handleFSf ( request, String( "/index.html") ) ;   // No parameters, send the startpage
    }
    else
    {
      handleFSf ( request, String( "/config.html") ) ;  // Or the configuration page if in AP mode
    }
    return ;
  }
  p = request->getParam ( 0 ) ;                         // Get pointer to parameter structure
  argument = p->name() ;                                // Get the argument
  argument.toLowerCase() ;                              // Force to lower case
  value = p->value() ;                                  // Get the specified value
  // For the "save" command, the contents is the value of the next parameter
  if ( argument.startsWith ( "save" ) && ( params > 1 ) )
  {
    reply = "Error saving " INIFILENAME ;               // Default reply
    p = request->getParam ( 1 ) ;                       // Get pointer to next parameter structure
    if ( p->isPost() )                                  // Does it have a POST?
    {
      f = SPIFFS.open ( INIFILENAME, "w" ) ;            // Save to inifile
      if ( f )
      {
        f.print ( p->value() ) ;
        f.close() ;
        reply = dbgprint ( "%s saved", INIFILENAME ) ;
      }
    }
  }
  else if ( argument.startsWith ( "list" ) )            // List all presets?
  {
    dbgprint ( "list request from browser" ) ;
    request->send ( 200, "text/plain", presetlist ) ;   // Send the reply
    return ;
  }
  else
  {
    reply = analyzeCmd ( argument.c_str(),              // Analyze it
                         value.c_str() ) ;
  }
  request->send ( 200, "text/plain", reply ) ;          // Send the reply
  //t = millis() - t ;
  // If it takes too long to send a reply, we run into the "LmacRxBlk:1"-problem.
  // Reset the ESP8266.....
  //if ( t > 8000 )
  //{
  //  ESP.restart() ;                                   // Last resource
  //}
}
