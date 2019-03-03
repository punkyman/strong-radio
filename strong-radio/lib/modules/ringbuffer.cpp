//******************************************************************************************
// Ringbuffer (fifo) routines.                                                             *
//******************************************************************************************
//******************************************************************************************
//                              R I N G S P A C E                                          *
//******************************************************************************************
inline bool ringspace()
{
  return ( rcount < RINGBFSIZ ) ;     // True is at least one byte of free space is available
}


//******************************************************************************************
//                              R I N G A V A I L                                          *
//******************************************************************************************
inline uint16_t ringavail()
{
  return rcount ;                     // Return number of bytes available
}


//******************************************************************************************
//                                P U T R I N G                                            *
//******************************************************************************************
void putring ( uint8_t b )                 // Put one byte in the ringbuffer
{
  // No check on available space.  See ringspace()
  *(ringbuf + rbwindex) = b ;         // Put byte in ringbuffer
  if ( ++rbwindex == RINGBFSIZ )      // Increment pointer and
  {
    rbwindex = 0 ;                    // wrap at end
  }
  rcount++ ;                          // Count number of bytes in the
}


//******************************************************************************************
//                                G E T R I N G                                            *
//******************************************************************************************
uint8_t getring()
{
  // Assume there is always something in the bufferpace.  See ringavail()
  if ( ++rbrindex == RINGBFSIZ )      // Increment pointer and
  {
    rbrindex = 0 ;                    // wrap at end
  }
  rcount-- ;                          // Count is now one less
  return *(ringbuf + rbrindex) ;      // return the oldest byte
}

//******************************************************************************************
//                               E M P T Y R I N G                                         *
//******************************************************************************************
void emptyring()
{
  rbwindex = 0 ;                      // Reset ringbuffer administration
  rbrindex = RINGBFSIZ - 1 ;
  rcount = 0 ;
}

//******************************************************************************************
//                             G E T E N C R Y P T I O N T Y P E                           *
//******************************************************************************************
// Read the encryption type of the network and return as a 4 byte name                     *
//*********************4********************************************************************
const char* getEncryptionType ( int thisType )
{
  switch (thisType)
  {
    case ENC_TYPE_WEP:
      return "WEP " ;
    case ENC_TYPE_TKIP:
      return "WPA " ;
    case ENC_TYPE_CCMP:
      return "WPA2" ;
    case ENC_TYPE_NONE:
      return "None" ;
    case ENC_TYPE_AUTO:
      return "Auto" ;
  }
  return "????" ;
}
