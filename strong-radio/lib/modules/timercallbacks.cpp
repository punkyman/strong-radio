//******************************************************************************************
//                                  T I M E R 1 0 0                                        *
//******************************************************************************************
// Examine button every 100 msec.                                                          *
//******************************************************************************************
void timer100()
{
  static int     count10sec = 0 ;                 // Counter for activatie 10 seconds process
  static int     oldval2 = HIGH ;                 // Previous value of digital input button 2
#if ( not ( defined ( USETFT ) ) )
  static int     oldval1 = HIGH ;                 // Previous value of digital input button 1
  static int     oldval3 = HIGH ;                 // Previous value of digital input button 3
#endif
  int            newval ;                         // New value of digital input switch
  uint16_t       v ;                              // Analog input value 0..1023
  static uint8_t aoldval = 0 ;                    // Previous value of analog input switch
  uint8_t        anewval ;                        // New value of analog input switch (0..3)

  if ( ++count10sec == 100  )                     // 10 seconds passed?
  {
    timer10sec() ;                                // Yes, do 10 second procedure
    count10sec = 0 ;                              // Reset count
  }
  else
  {
    newval = digitalRead ( BUTTON2 ) ;            // Test if below certain level
    if ( newval != oldval2 )                      // Change?
    {
      oldval2 = newval ;                          // Yes, remember value
      if ( newval == LOW )                        // Button pushed?
      {
        ini_block.newpreset = currentpreset + 1 ; // Yes, goto next preset station
        //dbgprint ( "Digital button 2 pushed" ) ;
      }
      return ;
    }
#if ( not ( defined ( USETFT ) ) )
    newval = digitalRead ( BUTTON1 ) ;            // Test if below certain level
    if ( newval != oldval1 )                      // Change?
    {
      oldval1 = newval ;                          // Yes, remember value
      if ( newval == LOW )                        // Button pushed?
      {
        ini_block.newpreset = 0 ;                 // Yes, goto first preset station
        //dbgprint ( "Digital button 1 pushed" ) ;
      }
      return ;
    }
    // Note that BUTTON3 has inverted input
    newval = digitalRead ( BUTTON3 ) ;            // Test if below certain level
    newval = HIGH + LOW - newval ;                // Reverse polarity
    if ( newval != oldval3 )                      // Change?
    {
      oldval3 = newval ;                          // Yes, remember value
      if ( newval == LOW )                        // Button pushed?
      {
        ini_block.newpreset = currentpreset - 1 ; // Yes, goto previous preset station
        //dbgprint ( "Digital button 3 pushed" ) ;
      }
      return ;
    }
#endif
    v = analogRead ( A0 ) ;                       // Read analog value
    anewval = anagetsw ( v ) ;                    // Check analog value for program switches
    if ( anewval != aoldval )                     // Change?
    {
      aoldval = anewval ;                         // Remember value for change detection
      if ( anewval != 0 )                         // Button pushed?
      {
        //dbgprint ( "Analog button %d pushed, v = %d", anewval, v ) ;
        if ( anewval == 1 )                       // Button 1?
        {
          ini_block.newpreset = 0 ;               // Yes, goto first preset
        }
        else if ( anewval == 2 )                  // Button 2?
        {
          ini_block.newpreset = currentpreset + 1 ; // Yes, goto next preset
        }
        else if ( anewval == 3 )                  // Button 3?
        {
          ini_block.newpreset = currentpreset - 1 ; // Yes, goto previous preset
        }
      }
    }
  }
}

//******************************************************************************************
//                                  T I M E R 1 0 S E C                                    *
//******************************************************************************************
// Extra watchdog.  Called every 10 seconds.                                               *
// If totalcount has not been changed, there is a problem and playing will stop.           *
// Note that a "yield()" within this routine or in called functions will cause a crash!    *
//******************************************************************************************
void timer10sec()
{
  static uint32_t oldtotalcount = 7321 ;          // Needed foor change detection
  static uint8_t  morethanonce = 0 ;              // Counter for succesive fails
  static uint8_t  t600 = 0 ;                      // Counter for 10 minutes

  if ( datamode & ( INIT | HEADER | DATA |        // Test op playing
                    METADATA | PLAYLISTINIT |
                    PLAYLISTHEADER |
                    PLAYLISTDATA ) )
  {
    if ( totalcount == oldtotalcount )            // Still playing?
    {
      dbgprint ( "No data input" ) ;              // No data detected!
      if ( morethanonce > 10 )                    // Happened too many times?
      {
        dbgprint ( "Going to restart..." ) ;
        ESP.restart() ;                           // Reset the CPU, probably no return
      }
      if ( datamode & ( PLAYLISTDATA |            // In playlist mode?
                        PLAYLISTINIT |
                        PLAYLISTHEADER ) )
      {
        playlist_num = 0 ;                        // Yes, end of playlist
      }
      if ( ( morethanonce > 0 ) ||                // Happened more than once?
           ( playlist_num > 0 ) )                 // Or playlist active?
      {
        datamode = STOPREQD ;                     // Stop player
        ini_block.newpreset++ ;                   // Yes, try next channel
        dbgprint ( "Trying other station/file..." ) ;
      }
      morethanonce++ ;                            // Count the fails
    }
    else
    {
      if ( morethanonce )                         // Recovered from data loss?
      {
        dbgprint ( "Recovered from dataloss" ) ;
        morethanonce = 0 ;                        // Data see, reset failcounter
      }
      oldtotalcount = totalcount ;                // Save for comparison in next cycle
    }
    if ( t600++ == 60 )                           // 10 minutes over?
    {
      t600 = 0 ;                                  // Yes, reset counter
      dbgprint ( "10 minutes over" ) ;
      publishIP() ;                               // Re-publish IP
    }
  }
}


