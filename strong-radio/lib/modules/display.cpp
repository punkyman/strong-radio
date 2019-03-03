
//******************************************************************************************
//                              D I S P L A Y V O L U M E                                  *
//******************************************************************************************
// Show the current volume as an indicator on the screen.                                  *
//******************************************************************************************
void displayvolume()
{
#if defined ( USETFT )
  static uint8_t oldvol = 0 ;                        // Previous volume
  uint8_t        pos ;                               // Positon of volume indicator

  if ( vs1053player.getVolume() != oldvol )
  {
    pos = map ( vs1053player.getVolume(), 0, 100, 0, 160 ) ;
  }
  tft.fillRect ( 0, 126, pos, 2, RED ) ;             // Paint red part
  tft.fillRect ( pos, 126, 160 - pos, 2, GREEN ) ;   // Paint green part
#endif
}


//******************************************************************************************
//                              D I S P L A Y I N F O                                      *
//******************************************************************************************
// Show a string on the LCD at a specified y-position in a specified color                 *
//******************************************************************************************
#if defined ( USETFT )
void displayinfo ( const char* str, uint16_t pos, uint16_t height, uint16_t color )
{
  char buf [ strlen ( str ) + 1 ] ;             // Need some buffer space
  
  strcpy ( buf, str ) ;                         // Make a local copy of the string
  utf8ascii ( buf ) ;                           // Convert possible UTF8
  tft.fillRect ( 0, pos, 160, height, BLACK ) ; // Clear the space for new info
  tft.setTextColor ( color ) ;                  // Set the requested color
  tft.setCursor ( 0, pos ) ;                    // Prepare to show the info
  tft.println ( buf ) ;                         // Show the string
}
#else
#define displayinfo(a,b,c,d)                    // Empty declaration
#endif


//******************************************************************************************
//                        S H O W S T R E A M T I T L E                                    *
//******************************************************************************************
// Show artist and songtitle if present in metadata.                                       *
// Show always if full=true.                                                               *
//******************************************************************************************
void showstreamtitle ( const char *ml, bool full )
{
  char*             p1 ;
  char*             p2 ;
  char              streamtitle[150] ;           // Streamtitle from metadata

  if ( strstr ( ml, "StreamTitle=" ) )
  {
    dbgprint ( "Streamtitle found, %d bytes", strlen ( ml ) ) ;
    dbgprint ( ml ) ;
    p1 = (char*)ml + 12 ;                       // Begin of artist and title
    if ( ( p2 = strstr ( ml, ";" ) ) )          // Search for end of title
    {
      if ( *p1 == '\'' )                        // Surrounded by quotes?
      {
        p1++ ;
        p2-- ;
      }
      *p2 = '\0' ;                              // Strip the rest of the line
    }
    // Save last part of string as streamtitle.  Protect against buffer overflow
    strncpy ( streamtitle, p1, sizeof ( streamtitle ) ) ;
    streamtitle[sizeof ( streamtitle ) - 1] = '\0' ;
  }
  else if ( full )
  {
    // Info probably from playlist
    strncpy ( streamtitle, ml, sizeof ( streamtitle ) ) ;
    streamtitle[sizeof ( streamtitle ) - 1] = '\0' ;
  }
  else
  {
    icystreamtitle = "" ;                       // Unknown type
    return ;                                    // Do not show
  }
  // Save for status request from browser ;
  icystreamtitle = streamtitle ;
  if ( ( p1 = strstr ( streamtitle, " - " ) ) ) // look for artist/title separator
  {
    *p1++ = '\n' ;                              // Found: replace 3 characters by newline
    p2 = p1 + 2 ;
    if ( *p2 == ' ' )                           // Leading space in title?
    {
      p2++ ;
    }
    strcpy ( p1, p2 ) ;                         // Shift 2nd part of title 2 or 3 places
  }
  displayinfo ( streamtitle, 20, 40, CYAN ) ;   // Show title at position 20
}
