
//******************************************************************************************
//                                  X M L  C A L L B A C K                                 *
//******************************************************************************************
// Process XML tags into variables.                                                        *
//******************************************************************************************
void XML_callback ( uint8_t statusflags, char* tagName, uint16_t tagNameLen,
                    char* data,  uint16_t dataLen )
{
  if ( statusflags & STATUS_START_TAG )
  {
    if ( tagNameLen )
    {
      xmlOpen = String ( tagName ) ;
      //dbgprint ( "Start tag %s",tagName ) ;
    }
  }
  else if ( statusflags & STATUS_END_TAG )
  {
    //dbgprint ( "End tag %s", tagName ) ;
  }
  else if ( statusflags & STATUS_TAG_TEXT )
  {
    xmlTag = String( tagName ) ;
    xmlData = String( data ) ;
    //dbgprint ( Serial.print( "Tag: %s, text: %s", tagName, data ) ;
  }
  else if ( statusflags & STATUS_ATTR_TEXT )
  {
    //dbgprint ( "Attribute: %s, text: %s", tagName, data ) ;
  }
  else if  ( statusflags & STATUS_ERROR )
  {
    //dbgprint ( "XML Parsing error  Tag: %s, text: %s", tagName, data ) ;
  }
}


//******************************************************************************************
//                                  X M L  P A R S E                                       *
//******************************************************************************************
// Parses streams from XML data.                                                           *
//******************************************************************************************
String xmlparse ( String mount )
{
  // Example URL for XML Data Stream:
  // http://playerservices.streamtheworld.com/api/livestream?version=1.5&mount=IHR_TRANAAC&lang=en
  // Clear all variables for use.
  char   tmpstr[200] ;                              // Full GET command, later stream URL
  char   c ;                                        // Next input character from reply
  String urlout ;                                   // Result URL
  bool   urlfound = false ;                         // Result found

  stationServer = "" ;
  stationPort = "" ;
  stationMount = "" ;
  xmlTag = "" ;
  xmlData = "" ;
  stop_mp3client() ; // Stop any current wificlient connections.
  dbgprint ( "Connect to new iHeartRadio host: %s", mount.c_str() ) ;
  datamode = INIT ;                                 // Start default in metamode
  chunked = false ;                                 // Assume not chunked
  // Create a GET commmand for the request.
  sprintf ( tmpstr, xmlget, mount.c_str() ) ;
  dbgprint ( "%s", tmpstr ) ;
  // Connect to XML stream.
  mp3client = new WiFiClient() ;
  if ( mp3client->connect ( xmlhost, xmlport ) ) {
    dbgprint ( "Connected!" ) ;
    mp3client->print ( String ( tmpstr ) + " HTTP/1.1\r\n"
                       "Host: " + xmlhost + "\r\n"
                       "User-Agent: Mozilla/5.0\r\n"
                       "Connection: close\r\n\r\n" ) ;
    // Check for XML Data.
    while ( true )
    {
      if ( mp3client->available() )
      {
        char c = mp3client->read() ;
        if ( c == '<' )
        {
          c = mp3client->read() ;
          if ( c == '?' )
          {
            xml.processChar ( '<' ) ;
            xml.processChar ( '?' ) ;
            break ;
          }
        }
      }
      yield() ;
    }
    dbgprint ( "XML parser processing..." ) ;
    // Process XML Data.
    while (true) 
    {
      if ( mp3client->available() )
      {
        c = mp3client->read() ;
        xml.processChar ( c ) ;
        if ( xmlTag != "" )
        {
          if ( xmlTag.endsWith ( "/status-code" ) )   // Status code seen?
          {
            if ( xmlData != "200" )                   // Good result?
            {
              dbgprint ( "Bad xml status-code %s",    // No, show and stop interpreting
                          xmlData.c_str() ) ;
              break ;
            }
          }
          if ( xmlTag.endsWith ( "/ip" ) )
          {
            stationServer = xmlData ;
          }
          else if ( xmlTag.endsWith ( "/port" ) )
          {
            stationPort = xmlData ;
          }
          else if ( xmlTag.endsWith ( "/mount"  ) )
          {
            stationMount = xmlData ;
          }
        }
      }
      // Check if all the station values are stored.
      urlfound = ( stationServer != "" && stationPort != "" && stationMount != "" ) ;
      if ( urlfound )
      {
        xml.reset() ;
        break ;
      }
      yield() ;
    }
    tmpstr[0] = '\0' ;                            
    if ( urlfound )
    {
      sprintf ( tmpstr, "%s:%s/%s_SC",                   // Build URL for ESP-Radio to stream.
                        stationServer.c_str(),
                        stationPort.c_str(),
                        stationMount.c_str() ) ;
      dbgprint ( "Found: %s", tmpstr ) ;
    }
    dbgprint ( "Closing XML connection." ) ;
    stop_mp3client () ;
  }
  else
  {
    dbgprint ( "Can't connect to XML host!" ) ;
    tmpstr[0] = '\0' ;                            
  }
  return String ( tmpstr ) ;                           // Return final streaming URL.
}
