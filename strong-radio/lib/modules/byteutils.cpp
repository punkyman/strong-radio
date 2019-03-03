//******************************************************************************************
//                            C H K H D R L I N E                                          *
//******************************************************************************************
// Check if a line in the header is a reasonable headerline.                               *
// Normally it should contain something like "icy-xxxx:abcdef".                            *
//******************************************************************************************
bool chkhdrline ( const char* str )
{
  char    b ;                                         // Byte examined
  int     len = 0 ;                                   // Lengte van de string

  while ( ( b = *str++ ) )                            // Search to end of string
  {
    len++ ;                                           // Update string length
    if ( ! isalpha ( b ) )                            // Alpha (a-z, A-Z)
    {
      if ( b != '-' )                                 // Minus sign is allowed
      {
        if ( b == ':' )                               // Found a colon?
        {
          return ( ( len > 5 ) && ( len < 50 ) ) ;    // Yes, okay if length is okay
        }
        else
        {
          return false ;                              // Not a legal character
        }
      }
    }
  }
  return false ;                                      // End of string without colon
}


//******************************************************************************************
//                           H A N D L E B Y T E _ C H                                     *
//******************************************************************************************
// Handle the next byte of data from server.                                               *
// Chunked transfer encoding aware. Chunk extensions are not supported.                    *
//******************************************************************************************
void handlebyte_ch ( uint8_t b, bool force )
{
  static int  chunksize = 0 ;                         // Chunkcount read from stream

  if ( chunked && !force && 
       ( datamode & ( DATA |                          // Test op DATA handling
                      METADATA |
                      PLAYLISTDATA ) ) )
  {
    if ( chunkcount == 0 )                            // Expecting a new chunkcount?
    {
       if ( b == '\r' )                               // Skip CR
       {
         return ;      
       }
       else if ( b == '\n' )                          // LF ?
       {
         chunkcount = chunksize ;                     // Yes, set new count
         chunksize = 0 ;                              // For next decode
         return ;
       }
       // We have received a hexadecimal character.  Decode it and add to the result.
       b = toupper ( b ) - '0' ;                      // Be sure we have uppercase
       if ( b > 9 )
       {
         b = b - 7 ;                                  // Translate A..F to 10..15
       }
       chunksize = ( chunksize << 4 ) + b ;
    }
    else
    {
      handlebyte ( b, force ) ;                       // Normal data byte
      chunkcount-- ;                                  // Update count to next chunksize block
    }
  }
  else
  {
    handlebyte ( b, force ) ;                         // Normal handling of this byte
  }
}


//******************************************************************************************
//                           H A N D L E B Y T E                                           *
//******************************************************************************************
// Handle the next byte of data from server.                                               *
// This byte will be send to the VS1053 most of the time.                                  *
// Note that the buffer the data chunk must start at an address that is a muttiple of 4.   *
// Set force to true if chunkbuffer must be flushed.                                       *
//******************************************************************************************
void handlebyte ( uint8_t b, bool force )
{
  static uint16_t  playlistcnt ;                       // Counter to find right entry in playlist
  static bool      firstmetabyte ;                     // True if first metabyte (counter)
  static int       LFcount ;                           // Detection of end of header
  static __attribute__((aligned(4))) uint8_t buf[32] ; // Buffer for chunk
  static int       bufcnt = 0 ;                        // Data in chunk
  static bool      firstchunk = true ;                 // First chunk as input
  String           lcml ;                              // Lower case metaline
  String           ct ;                                // Contents type
  static bool      ctseen = false ;                    // First line of header seen or not
  int              inx ;                               // Pointer in metaline
  int              i ;                                 // Loop control

  if ( datamode == INIT )                              // Initialize for header receive
  {
    ctseen = false ;                                   // Contents type not seen yet
    metaint = 0 ;                                      // No metaint found
    LFcount = 0 ;                                      // For detection end of header
    bitrate = 0 ;                                      // Bitrate still unknown
    dbgprint ( "Switch to HEADER" ) ;
    datamode = HEADER ;                                // Handle header
    totalcount = 0 ;                                   // Reset totalcount
    metaline = "" ;                                    // No metadata yet
    firstchunk = true ;                                // First chunk expected
  }
  if ( datamode == DATA )                              // Handle next byte of MP3/Ogg data
  {
    buf[bufcnt++] = b ;                                // Save byte in chunkbuffer
    if ( bufcnt == sizeof(buf) || force )              // Buffer full?
    {
      if ( firstchunk )
      {
        firstchunk = false ;
        dbgprint ( "First chunk:" ) ;                  // Header for printout of first chunk
        for ( i = 0 ; i < 32 ; i += 8 )                // Print 4 lines
        {
          dbgprint ( "%02X %02X %02X %02X %02X %02X %02X %02X",
                     buf[i],   buf[i + 1], buf[i + 2], buf[i + 3],
                     buf[i + 4], buf[i + 5], buf[i + 6], buf[i + 7] ) ;
        }
      }
      vs1053player.playChunk ( buf, bufcnt ) ;         // Yes, send to player
      bufcnt = 0 ;                                     // Reset count
    }
    totalcount++ ;                                     // Count number of bytes, ignore overflow
    if ( metaint != 0 )                                // No METADATA on Ogg streams or mp3 files
    {
      if ( --datacount == 0 )                          // End of datablock?
      {
        if ( bufcnt )                                  // Yes, still data in buffer?
        {
          vs1053player.playChunk ( buf, bufcnt ) ;     // Yes, send to player
          bufcnt = 0 ;                                 // Reset count
        }
        datamode = METADATA ;
        firstmetabyte = true ;                         // Expecting first metabyte (counter)
      }
    }
    return ;
  }
  if ( datamode == HEADER )                            // Handle next byte of MP3 header
  {
    if ( ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      // Yes, ignore
    }
    else if ( b == '\n' )                              // Linefeed ?
    {
      LFcount++ ;                                      // Count linefeeds
      if ( chkhdrline ( metaline.c_str() ) )           // Reasonable input?
      {
        lcml = metaline ;                              // Use lower case for compare
        lcml.toLowerCase() ;
        dbgprint ( metaline.c_str() ) ;                // Yes, Show it
        if ( lcml.indexOf ( "content-type" ) >= 0 )    // Line with "Content-Type: xxxx/yyy"
        {
          ctseen = true ;                              // Yes, remember seeing this
          ct = metaline.substring ( 14 ) ;             // Set contentstype. Not used yet
          dbgprint ( "%s seen.", ct.c_str() ) ;
        }
        if ( lcml.startsWith ( "icy-br:" ) )
        {
          bitrate = metaline.substring(7).toInt() ;    // Found bitrate tag, read the bitrate
          if ( bitrate == 0 )                          // For Ogg br is like "Quality 2"
          {
            bitrate = 87 ;                             // Dummy bitrate
          }
        }
        else if ( lcml.startsWith ( "icy-metaint:" ) )
        {
          metaint = metaline.substring(12).toInt() ;   // Found metaint tag, read the value
        }
        else if ( lcml.startsWith ( "icy-name:" ) )
        {
          icyname = metaline.substring(9) ;            // Get station name
          icyname.trim() ;                             // Remove leading and trailing spaces
          displayinfo ( icyname.c_str(), 60, 68,
                        YELLOW ) ;                     // Show station name at position 60
        }
        else if ( lcml.startsWith ( "transfer-encoding:" ) )
        {
          // Station provides chunked transfer
          if ( lcml.endsWith ( "chunked" ) )
          {
            chunked = true ;                           // Remember chunked transfer mode
            chunkcount = 0 ;                           // Expect chunkcount in DATA
          }
        }
      }
      metaline = "" ;                                  // Reset this line
      if ( ( LFcount == 2 ) && ctseen )                // Some data seen and a double LF?
      {
        dbgprint ( "Switch to DATA, bitrate is %d"     // Show bitrate
                   ", metaint is %d",                  // and metaint
                   bitrate, metaint ) ;
        datamode = DATA ;                              // Expecting data now
        datacount = metaint ;                          // Number of bytes before first metadata
        bufcnt = 0 ;                                   // Reset buffer count
        vs1053player.startSong() ;                     // Start a new song
      }
    }
    else
    {
      metaline += (char)b ;                            // Normal character, put new char in metaline
      LFcount = 0 ;                                    // Reset double CRLF detection
    }
    return ;
  }
  if ( datamode == METADATA )                          // Handle next byte of metadata
  {
    if ( firstmetabyte )                               // First byte of metadata?
    {
      firstmetabyte = false ;                          // Not the first anymore
      metacount = b * 16 + 1 ;                         // New count for metadata including length byte
      if ( metacount > 1 )
      {
        dbgprint ( "Metadata block %d bytes",
                   metacount - 1 ) ;                   // Most of the time there are zero bytes of metadata
      }
      metaline = "" ;                                  // Set to empty
    }
    else
    {
      metaline += (char)b ;                            // Normal character, put new char in metaline
    }
    if ( --metacount == 0 )
    {
      if ( metaline.length() )                         // Any info present?
      {
        // metaline contains artist and song name.  For example:
        // "StreamTitle='Don McLean - American Pie';StreamUrl='';"
        // Sometimes it is just other info like:
        // "StreamTitle='60s 03 05 Magic60s';StreamUrl='';"
        // Isolate the StreamTitle, remove leading and trailing quotes if present.
        showstreamtitle ( metaline.c_str() ) ;         // Show artist and title if present in metadata
      }
      if ( metaline.length() > 1500 )                  // Unlikely metaline length?
      {
        dbgprint ( "Metadata block too long! Skipping all Metadata from now on." ) ;
        metaint = 0 ;                                  // Probably no metadata
        metaline = "" ;                                // Do not waste memory on this
      }
      datacount = metaint ;                            // Reset data count
      bufcnt = 0 ;                                     // Reset buffer count
      datamode = DATA ;                                // Expecting data
    }
  }
  if ( datamode == PLAYLISTINIT )                      // Initialize for receive .m3u file
  {
    // We are going to use metadata to read the lines from the .m3u file
    metaline = "" ;                                    // Prepare for new line
    LFcount = 0 ;                                      // For detection end of header
    datamode = PLAYLISTHEADER ;                        // Handle playlist data
    playlistcnt = 1 ;                                  // Reset for compare
    totalcount = 0 ;                                   // Reset totalcount
    dbgprint ( "Read from playlist" ) ;
  }
  if ( datamode == PLAYLISTHEADER )                    // Read header
  {
    if ( ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      // Yes, ignore
    }
    else if ( b == '\n' )                              // Linefeed ?
    {
      LFcount++ ;                                      // Count linefeeds
      dbgprint ( "Playlistheader: %s",                 // Show playlistheader
                 metaline.c_str() ) ;
      metaline = "" ;                                  // Ready for next line
      if ( LFcount == 2 )
      {
        dbgprint ( "Switch to PLAYLISTDATA" ) ;
        datamode = PLAYLISTDATA ;                      // Expecting data now
        return ;
      }
    }
    else
    {
      metaline += (char)b ;                            // Normal character, put new char in metaline
      LFcount = 0 ;                                    // Reset double CRLF detection
    }
  }
  if ( datamode == PLAYLISTDATA )                      // Read next byte of .m3u file data
  {
    if ( ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      // Yes, ignore
    }
    else if ( b == '\n' )                              // Linefeed ?
    {
      dbgprint ( "Playlistdata: %s",                   // Show playlistheader
                 metaline.c_str() ) ;
      if ( metaline.length() < 5 )                     // Skip short lines
      {
        return ;
      }
      if ( metaline.indexOf ( "#EXTINF:" ) >= 0 )      // Info?
      {
        if ( playlist_num == playlistcnt )             // Info for this entry?
        {
          inx = metaline.indexOf ( "," ) ;             // Comma in this line?
          if ( inx > 0 )
          {
            // Show artist and title if present in metadata
            showstreamtitle ( metaline.substring ( inx + 1 ).c_str(), true ) ;
          }
        }
      }
      if ( metaline.startsWith ( "#" ) )               // Commentline?
      {
        metaline = "" ;
        return ;                                       // Ignore commentlines
      }
      // Now we have an URL for a .mp3 file or stream.  Is it the rigth one?
      dbgprint ( "Entry %d in playlist found: %s", playlistcnt, metaline.c_str() ) ;
      if ( playlist_num == playlistcnt  )
      {
        inx = metaline.indexOf ( "http://" ) ;         // Search for "http://"
        if ( inx >= 0 )                                // Does URL contain "http://"?
        {
          host = metaline.substring ( inx + 7 ) ;      // Yes, remove it and set host
        }
        else
        {
          host = metaline ;                            // Yes, set new host
        }
        connecttohost() ;                              // Connect to it
      }
      metaline = "" ;
      host = playlist ;                                // Back to the .m3u host
      playlistcnt++ ;                                  // Next entry in playlist
    }
    else
    {
      metaline += (char)b ;                            // Normal character, add it to metaline
    }
    return ;
  }
}
