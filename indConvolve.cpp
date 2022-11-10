#include "stdafx.h"

// Copyright (C) 2007 Todd R. Kueny, Inc.  All Rights Reserved.
//
// Change Log:
//
//	  10-07-2007 - TRK - Created.
//
//

#ifdef __PORTABLE
#include "Support/pdfXPortability.h"
#endif // __PORTABLE

#include "indConvolve.h"
#include "lightIndImageGross.h"

#define CONVOLUTION_MAX_KERNEL (9)

int convolve1(unsigned char * pixBuffer, 
              int colorsPerEntry,
              int bufScale,
              int oH, 
              int width, 
              float inBias,
              unsigned char * currentColorMax,
              double amount) {
  //
  //  [ -1,1   0,1   1,1  ]
  //  [ -1,0   0,0   1,0  ]
  //  [ -1,-1  0,-1  1,-1 ]
  //    =>
  //  [ -1,-1  -1,0  -1,1  0,-1,  0,0  0,1  1,-1  1,0  1,1 ]
  //     0       1     2    3      4    5    6     7    8
  //
  //     ^
  //  up |  across ->
  //
 
  //  [ -2, 2  -1, 2  0, 2  1, 2  2, 2  ]
  //  [ -2, 1  -1, 1  0, 1  1, 1  2, 1  ]
  //  [ -2, 0  -1, 0  0, 0  1, 0  2, 0  ]
  //  [ -2,-1  -1,-1  0,-1  1,-1  2,-1  ]
  //  [ -2,-2  -1,-2  0,-2  1,-2  2,-2  ]
  //    =>
  //  [ -1,-1  -1,0  -1,1  0,-1,  0,0  0,1  1,-1  1,0  1,1 ]
  //     0       1     2    3      4    5    6     7    8
  //
  //     ^
  //  up |  across ->
  //
  float * kernel, kernelV, sums[4], kSum;
  unsigned char * pixBase, * outBase, * backScans[(CONVOLUTION_MAX_KERNEL + 2) / 2], * backPixelBases[(CONVOLUTION_MAX_KERNEL + 2) / 2];
  int i, j, kStepX, kStepY, kernelWidth, pixelStep, kCenterX, kCenterY, oHeight;

  oHeight = oH * colorsPerEntry;
  kernelWidth = 3;
  kernel = new float[kernelWidth * kernelWidth];
  kernel[0] = -1.0;
  kernel[1] = -1.0;
  kernel[2] = -1.0;
  kernel[3] = -1.0;
  kernel[4] = 9.0;
  kernel[5] = -1.0;
  kernel[6] = -1.0;
  kernel[7] = -1.0;
  kernel[8] = -1.0;
  
  kernelWidth = 3;
  pixelStep = oHeight; // step to the adjacent pixel
  kCenterX = (kernelWidth - 1) / 2;
  kCenterY = (kernelWidth - 1) / 2;
  
  for (i = 0; i <= kCenterX; i++) {
    backScans[i] = new unsigned char[pixelStep];
    backPixelBases[i] = NULL;
  }

  for (i = kernelWidth / 2; i <  (width * bufScale) - (kernelWidth / 2); i++ ) {
    outBase = pixBuffer + (i * pixelStep);
    
    if (backPixelBases[kCenterX] != NULL) {
      unsigned char * a, * b;
      
      a = backPixelBases[kCenterX];
      b = backScans[kCenterX];
      for (j = 0; j < pixelStep; j++) {
        a[j] = (((1.0 - amount) * a[j]) + (amount * b[j]));
        if (a[j] > currentColorMax[0]) {
          currentColorMax[0] = a[j];
        }
      }
      //memcpy(backPixelBases[kCenterX], backScans[kCenterX], pixelStep);
      backPixelBases[kCenterX] = NULL;
    }
    
    {
      unsigned char * lastScan, * lastBase;
      
      lastScan = backScans[kCenterX];
      lastBase = backPixelBases[kCenterX];
      
      for (j = kCenterX; j > 0; j--) {
        backScans[j] = backScans[j - 1];
        backPixelBases[j] = backPixelBases[j - 1];
      }
      
      backScans[0] = lastScan;
      backPixelBases[0] = lastBase;
    }
    
    backPixelBases[0] = outBase;
    memcpy(backScans[0], outBase, pixelStep);

    for (j = colorsPerEntry * bufScale * (kernelWidth / 2); j < oHeight - (colorsPerEntry * bufScale * (kernelWidth / 2)); j += colorsPerEntry) {

      sums[indColorC] = 0.0;
      sums[indColorM] = 0.0;
      sums[indColorY] = 0.0;
      sums[indColorK] = 0.0;
      kSum = 0.0;
    
      for (kStepX = -(kernelWidth - 1) / 2; kStepX <= (kernelWidth - 1) / 2; kStepX += 1) {
        for (kStepY = -(kernelWidth - 1) / 2; kStepY <= (kernelWidth - 1) / 2; kStepY += 1) {
          int ki, px;
          
          ki = ((kCenterX + kStepX) * kernelWidth) + (kCenterY + kStepY);
        
          kernelV = kernel[ ki ];
        
          px = (kStepX * pixelStep) + (kStepY * colorsPerEntry);
          
          pixBase = pixBuffer + (i * pixelStep) + j + px;
                  
          sums[indColorC] += ((int)(pixBase[indColorC] & 0xFF)) * kernelV;
          sums[indColorM] += ((int)(pixBase[indColorM] & 0xFF)) * kernelV;
          sums[indColorY] += ((int)(pixBase[indColorY] & 0xFF)) * kernelV;
          sums[indColorK] += ((int)(pixBase[indColorK] & 0xFF)) * kernelV;
          kSum += kernelV;
        }
      }
      
      if (kSum <= 0.0) {
        kSum = 1.0;
      }
      
      sums[indColorC] = inBias + (sums[indColorC] / kSum);
      sums[indColorM] = inBias + (sums[indColorM] / kSum);
      sums[indColorY] = inBias + (sums[indColorY] / kSum);
      sums[indColorK] = inBias + (sums[indColorK] / kSum);
      
      if (sums[indColorC] < 0.0) {
        sums[indColorC] = 0.0;
      } else if (sums[indColorC] > 255.0) {
        sums[indColorC] = 255.0;
      }
      
      if (sums[indColorM] < 0.0) {
        sums[indColorM] = 0.0;
      } else if (sums[indColorM] > 255.0) {
        sums[indColorM] = 255.0;
      }

      if (sums[indColorY] < 0.0) {
        sums[indColorY] = 0.0;
      } else if (sums[indColorY] > 255.0) {
        sums[indColorY] = 255.0;
      }

      if (sums[indColorK] < 0.0) {
        sums[indColorK] = 0.0;
      } else if (sums[indColorK] > 255.0) {
        sums[indColorK] = 255.0;
      }
      
      backScans[0][j + indColorC] = (unsigned char)sums[indColorC];
      backScans[0][j + indColorM] = (unsigned char)sums[indColorM];
      backScans[0][j + indColorY] = (unsigned char)sums[indColorY];
      backScans[0][j + indColorK] = (unsigned char)sums[indColorK];
    }
  }

  for (i = 0; i <= kCenterX; i++) {
    delete backScans[i];
  }
  
  return 0;
}

#if 0
COLORREF CImageConvolutionView::Convolve(CDC* pDC, int sourcex, 
    int sourcey, float kernel[3][3], int nBias,BOOL bGrayscale)
{
    float rSum = 0, gSum = 0, bSum = 0, kSum = 0;
    COLORREF clrReturn = RGB(0,0,0);
    for (int i=0; i <= 2; i++)//loop through rows
    {
        for (int j=0; j <= 2; j++)//loop through columns
        {
            //get pixel near source pixel
            /*
            if x,y is source pixel then we loop 
            through and get pixels at coordinates:
            x-1,y-1
            x-1,y
            x-1,y+1
            x,y-1
            x,y
            x,y+1
            x+1,y-1
            x+1,y
            x+1,y+1
            */
            COLORREF tmpPixel = pDC->GetPixel(sourcex+(i-(2>>1)),
                sourcey+(j-(2>>1)));
            //get kernel value
            float fKernel = kernel[i][j];
            //multiply each channel by kernel value, and add to sum
            //notice how each channel is treated separately
            rSum += (GetRValue(tmpPixel)*fKernel);
            gSum += (GetGValue(tmpPixel)*fKernel);
            bSum += (GetBValue(tmpPixel)*fKernel);
            //add the kernel value to the kernel sum
            kSum += fKernel;
        }
    }
    //if kernel sum is less than 0, reset to 1 to avoid divide by zero
    if (kSum <= 0)
        kSum = 1;
    //divide each channel by kernel sum
    rSum/=kSum;
    gSum/=kSum;
    bSum/=kSum;
    //add bias if desired
    rSum += nBias;
    gSum += nBias;
    bSum += nBias;
    //prevent channel overflow by clamping to 0..255
    if (rSum > 255)
        rSum = 255;
    else if (rSum < 0)
        rSum = 0;
    if (gSum > 255)
        gSum = 255;
    else if (gSum < 0)
        gSum = 0;
    if (bSum > 255)
        bSum = 255;
    else if (bSum < 0)
        bSum = 0;
    //return new pixel value
    if (bGrayscale)
    {
        int grayscale=0.299*rSum + 0.587*gSum + 0.114*bSum;
        rSum=grayscale;
        gSum=grayscale;
        bSum=grayscale;
    }

    clrReturn = RGB(rSum,gSum,bSum);
    return clrReturn;
}

#endif // 0
