//******************************************************************************************
//                         H A N D L E F I L E U P L O A D                                 *
//******************************************************************************************
// Handling of upload request.  Write file to SPIFFS.                                      *
//******************************************************************************************
void handleFileUpload ( AsyncWebServerRequest *request, String filename,
                        size_t index, uint8_t *data, size_t len, bool final )
{
  String          path ;                              // Filename including "/"
  static File     f ;                                 // File handle output file
  char*           reply ;                             // Reply for webserver
  static uint32_t t ;                                 // Timer for progress messages
  uint32_t        t1 ;                                // For compare
  static uint32_t totallength ;                       // Total file length
  static size_t   lastindex ;                         // To test same index

  if ( index == 0 )
  {
    path = String ( "/" ) + filename ;                // Form SPIFFS filename
    SPIFFS.remove ( path ) ;                          // Remove old file
    f = SPIFFS.open ( path, "w" ) ;                   // Create new file
    t = millis() ;                                    // Start time
    totallength = 0 ;                                 // Total file lengt still zero
    lastindex = 0 ;                                   // Prepare test
  }
  t1 = millis() ;                                     // Current timestamp
  // Yes, print progress
  dbgprint ( "File upload %s, t = %d msec, len %d, index %d",
               filename.c_str(), t1 - t, len, index ) ;
  if ( len )                                          // Something to write?
  {
    if ( ( index != lastindex ) || ( index == 0 ) )   // New chunk?
    {
      f.write ( data, len ) ;                         // Yes, transfer to SPIFFS
      totallength += len ;                            // Update stored length
      lastindex = index ;                             // Remenber this part
    }
  }
  if ( final )                                        // Was this last chunk?
  {
    f.close() ;                                       // Yes, clode the file
    reply = dbgprint ( "File upload %s, %d bytes finished",
                       filename.c_str(), totallength ) ;
    request->send ( 200, "", reply ) ;
  }
}


//******************************************************************************************
//                                H A N D L E F S F                                        *
//******************************************************************************************
// Handling of requesting files from the SPIFFS/PROGMEM. Example: /favicon.ico             *
//******************************************************************************************
void handleFSf ( AsyncWebServerRequest* request, const String& filename )
{
  static String          ct ;                           // Content type
  AsyncWebServerResponse *response ;                    // For extra headers

  dbgprint ( "FileRequest received %s", filename.c_str() ) ;
  ct = getContentType ( filename ) ;                    // Get content type
  if ( ( ct == "" ) || ( filename == "" ) )             // Empty is illegal
  {
    request->send ( 404, "text/plain", "File not found" ) ;
  }
  else
  {
    if ( filename.indexOf ( "index.html" ) >= 0 )       // Index page is in PROGMEM
    {
      response = request->beginResponse_P ( 200, ct, index_html ) ;
    }
    else if ( filename.indexOf ( "radio.css" ) >= 0 )   // CSS file is in PROGMEM
    {
      response = request->beginResponse_P ( 200, ct, radio_css ) ;
    }
    else if ( filename.indexOf ( "config.html" ) >= 0 ) // Config page is in PROGMEM
    {
      response = request->beginResponse_P ( 200, ct, config_html ) ;
    }
    else if ( filename.indexOf ( "about.html" ) >= 0 )  // About page is in PROGMEM
    {
      response = request->beginResponse_P ( 200, ct, about_html ) ;
    }
    else if ( filename.indexOf ( "favicon.ico" ) >= 0 ) // Favicon icon is in PROGMEM
    {
      response = request->beginResponse_P ( 200, ct, favicon_ico, sizeof ( favicon_ico ) ) ;
    }
    else
    {
      response = request->beginResponse ( SPIFFS, filename, ct ) ;
    }
    // Add extra headers
    response->addHeader ( "Server", NAME ) ;
    response->addHeader ( "Cache-Control", "max-age=3600" ) ;
    response->addHeader ( "Last-Modified", VERSION ) ;
    request->send ( response ) ;
  }
  dbgprint ( "Response sent" ) ;
}


//******************************************************************************************
//                                H A N D L E F S                                          *
//******************************************************************************************
// Handling of requesting files from the SPIFFS. Example: /favicon.ico                     *
//******************************************************************************************
void handleFS ( AsyncWebServerRequest* request )
{
  handleFSf ( request, request->url() ) ;               // Rest of handling
}
