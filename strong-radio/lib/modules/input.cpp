//******************************************************************************************
//                                  A N A G E T S W                                        *
//******************************************************************************************
// Translate analog input to switch number.  0 is inactive.                                *
// Note that it is adviced to avoid expressions as the argument for the abs function.      *
//******************************************************************************************
uint8_t anagetsw ( uint16_t v )
{
  int      i ;                                    // Loop control
  int      oldmindist = 1000 ;                    // Detection least difference
  int      newdist ;                              // New found difference
  uint8_t  sw = 0 ;                               // Number of switch detected (0 or 1..3)

  if ( v > analogrest )                           // Inactive level?
  {
    for ( i = 0 ; i < NUMANA ; i++ )
    {
      newdist = analogsw[i] - v ;                  // Compute difference
      newdist = abs ( newdist ) ;                  // Make it absolute
      if ( newdist < oldmindist )                  // New least difference?
      {
        oldmindist = newdist ;                     // Yes, remember
        sw = i + 1 ;                               // Remember switch
      }
    }
  }
  return sw ;                                      // Return active switch
}
