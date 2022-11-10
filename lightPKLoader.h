#ifndef lightPKLoader_H_
#define lightPKLoader_H_

// Copyright (C) 2007 Todd R. Kueny, Inc.  All Rights Reserved.
//
// Change Log:
//
//	  09-15-2007 - TRK - Created.
//
//

#ifdef __PORTABLE
#include "Support/pdfXPortability.h"
#endif // __PORTABLE

#include "Common/pdfdatatypes.h"

#include "lightIndImageGross.h"

typedef pdfXMap<int, int, bool, bool> PageMaskT;

typedef struct {
  int page;
  int sub;
  int image;
} StreamContextT;

typedef struct {
  double height;
  double width;
} JLYTSheetInfoT;

enum {
  lightJLYTSimplex = 1,
  lightJLYTDuplex = 2,
  lightJLYTUnknown = 9999
};

class lightJLYTInfoC {
public:
  pdfXString pdfVersion;
  pdfXString jlytVersion;
  pdfXString version;
  pdfXString JTM;
  pdfXString timeStamp;
  pdfXString jobName;
  pdfXString ripInfo;
  JLYTSheetInfoT frontSheetSize;
  JLYTSheetInfoT backSheetSize;
  int outputType;
  int cyclesPerSide;
  pdfXString paperInfo;
  bool tumble;
  
  int pixelHeight;
  int pixelWidth;
};

class lightPKLoaderC : public indImageLoaderC {

protected:
  FILE * input;
  int state;
  int length;
  bool produceImages;
  bool traceStates;
  
  StreamContextT thisState, lastState;
  
public:
    
  lightJLYTInfoC header;

  lightPKLoaderC(indImageGrossC * parent);
  ~lightPKLoaderC();
  
  void clear(void);
  void advanceState(void);
  
  bool init(pdfXString imagePath);
  bool loadImages(int pageLimit);
};

#endif // lightPKLoader_H_
