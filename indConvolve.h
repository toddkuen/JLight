#ifndef indConvolve_H_
#define indConvolve_H_

// Copyright (C) 2007 Todd R. Kueny, Inc.  All Rights Reserved.
//
// Change Log:
//
//	  10-07-2007 - TRK - Created.
//
//

extern int convolve1(unsigned char * pixBuffer, 
                     int colorsPerEntry, 
                     int bufScale,
                     int oH, 
                     int width, 
                     float inBias,
                     unsigned char * currentColorMax,
                     double amount);

#endif // indConvolve_H_
