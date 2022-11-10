#ifndef lightIndImageGross_H_
#define lightIndImageGross_H_

// Copyright (C) 2007 Todd R. Kueny, Inc.  All Rights Reserved.
//
// Change Log:
//
//	  09-15-2007 - TRK - Created.
//
//

#include "lightUtilities.h"
#include "genericUtilities.h"

typedef pdfXArray<pdfXString, pdfXString> DumperElementPathT, * pDumperElementPathT;

class indImageGrossC;

typedef struct {
  char * rawData;
  int oHeight;
  int endRowExclusive;
  int startColumnInclusive;
  int endColumnExclusive;
  int currentRow;
  int currentColumn;  // for (currentColumn = startColumnInclusive; currentColumn++; currentColumn < endColumnExclusive) ...
  int bytesPerStep;
  int totalBytesWritten;
  int bytesWritten;
} indJLTCodecState, * pindJLTCodecState;

#define MIN_PREVIEW_PIXELS (50)

class indImagePreviewBaseC {
protected:
  bool deleteBuffer;
  unsigned char * imageBuffer;
  int colorsPerEntry;
  int height;
  int width;
  pdfXArray<pdfXString, pdfXString&> filters;
  
public:
  indImagePreviewBaseC() {
    imageBuffer = NULL;
    deleteBuffer = false;
    colorsPerEntry = -1;
    height = -1;
    width = -1;
  }
    
  indImagePreviewBaseC(unsigned char * ib, int colsEnt, int h, int w) {
    imageBuffer = ib;
    deleteBuffer = false;
    colorsPerEntry = colsEnt;
    height = h;
    width = w;
  }
  
  void initializeCodec(pindJLTCodecState state);

  bool isInitialized(void) {
    return imageBuffer != NULL;
  }
  
  void getInfo(int& h, int& w) {
    h = height;
    w = width;
  }
  
  void init(int colsEnt, int h, int w) {
    int wx;
    int hx;
    
    wx = 4 * ((w + 3) / 4);
    hx = 4 * ((h + 3) / 4);
    imageBuffer = new unsigned char[colsEnt * wx * hx];
    deleteBuffer = true;
    colorsPerEntry = colsEnt;
    height = hx;
    width = wx;
  }
  
  void applyWaterMark(void);
  
  void addFilter(pdfXString filter) {
    filters.SetAtGrow(filters.GetSize(), filter);
  }
  
  virtual bool dumpFilteredPreviewImage(pdfXString somePath) {
    return false;
  }
  
  bool deriveFromBaseImageFiltered(unsigned char * baseIb, 
                                   int pixelHeight, 
                                   int paddedHeightBytes,
                                   int pixelWidth);
  
  ~indImagePreviewBaseC() {
    if (deleteBuffer) {
      delete imageBuffer;
    }
  }
};

typedef struct {
  char iid[4]; // 'I' 'C' '9' '6'
  short int height;
  short int width;
  short int q1;
  short int q2;
  short int q3;
  short int q4;
} IC96T;

#define indIMAGE_INPUT_BUFFER_MAX (30000000)
#define indOUTPUT_BUFFER_MAX (600000000)

class indImageDumperC {
protected:
  bool isPre;
  bool isPlane;
  pdfXMap<int, int, bool, bool> toImage;
  bool imageAll;
  int virtualCurrentPage;
  int totalPageCount;
  bool skipWritingFile;
  pdfXString fileRoot, fileExt, filePath;
  int groupNo;
  
public:
  indImageGrossC * destination;
  
  indImageDumperC(indImageGrossC * parent) {
    destination = parent;
    isPre = true;
    imageAll = true;
    isPlane = true;
    virtualCurrentPage = 1;
    totalPageCount = 1;
    skipWritingFile = false;
  }
  
  ~indImageDumperC() {
  };
  
  virtual void setOutputMetaData(pdfXString source) {
    groupNo = 0;
    splitFilePath(source, filePath, fileRoot, fileExt, true);
  }
  
  virtual void setVirtualOutputPageToFirst(void) {
    virtualCurrentPage = 1;
  }
  
  virtual void setSkipWritingFile(bool skip) {
    skipWritingFile = skip;
  }
  
  virtual int getVirtualOutputPage(void) {
    return virtualCurrentPage;
  }
  
  virtual int getActualOutputPage(void) {
    return totalPageCount;
  }
  
  virtual void advanceVirtualOutputPage(void) {
    virtualCurrentPage += 1;
    totalPageCount += 1;
  }
  
  // one-based...
  //
  virtual void setPageToImage(int page) {
    imageAll = false;
    toImage[page] = true;
  }
  
  virtual bool canImagePage(int page) {
    if (imageAll) {
      return true;
    } else {
      bool ix;
      
      if (toImage.Lookup(page, ix)) {
        return ix;
      } else {
        return false;
      }
    }
  }
  
  virtual bool isPreImageDumper(void) {
    return isPre;
  }
  
  virtual bool isPlaneImageDumper(void) {
    return isPlane;
  }
  
  virtual bool dumpImage(void);
  
  virtual bool dumpMasterFile(int groupSize,
                              pDumperElementPathT dmp, 
                              pDumperElementPathT pageControlInfo, 
                              pdfXString path, 
                              pdfXString otemplate,
                              bool breakOnGroup,
                              bool aggregate,
                              bool cleanup);
  
  virtual pdfXString stringPerformSubstitutions(pdfXString input);
};

class indImageLoaderC {
public:
  indImageGrossC * destination;
  pdfXString inputId;

  indImageLoaderC(indImageGrossC * parent);  
  ~indImageLoaderC() {
  };
  
  virtual bool loadImages(int pageLimit) {
    return true;
  }
};

#define MAX_IMAGE_DUMPERS (3)
#define MAX_OUTPUT_COLORANTS (4)

enum {
  indColorR = 0,
  indColorG = 1,
  indColorB = 2,
  
  indColorFirst = 0,
  indColorC = 0,
  indColorM = 1,
  indColorY = 2,
  indColorK = 3,
  indColorMax = 4,
  indColorMask = 0xF,
  indLastPlane = 0x80,

  indPressSimNone= 0x0000,
  indPressSimDropStrokeWhiteBlack = 0x00001,
  
  indIC96FileFormat = 1,
  indIC05FileFormat = 2,
  
  indColorStyleAdobeCMYK2RGB = 1,
  indColorStyleGhostscriptCMYK2RGB = 2
};

extern void indRGB_2_CMYK(double * RGB, double * CMYK, int styleCMYKRGB = indColorStyleAdobeCMYK2RGB);
extern void indCMYK_2_RGB(double * CMYK, double * RGB, int styleCMYKRGB = indColorStyleAdobeCMYK2RGB);

#define indALIGNFORPRESS(x) (16 * (((x) + 15) / 16))

class indImageGrossC {

protected:
  
  indImageLoaderC * loader;
  indImageDumperC * dumpers[MAX_IMAGE_DUMPERS];
  int dumperCount;
  int subImageCounter;
  int subImageGroupCounter;
  int subWithinImageCounter;

  pdfXMap<int, int, bool, bool> toExtract;
  pdfXMap<int, int, int, int> imageRes;
  pdfXMap<int, int, int, int> pressSims;
  
  int defaultRes;
  int defaultPressSim;
  int pageModulator;
  
public:
  
  indImageGrossC();
  ~indImageGrossC();
  
  void setPageModulator(int pm) {
    pageModulator = pm;
  }
  
  int modulatePage(int page) {
    if (pageModulator != -1) {
      return (page % pageModulator) == 0 ? 28 : page % pageModulator;
    } else {
      return page;
    }
  }
  
  void setPagePressSim(int page, int s) {
    pressSims[page] = s;
  }
  
  void setDefaultPressSim(int s) {
    defaultPressSim = s;
  }
  
  int getPagePressSim(int page) {
    int ix;
    
    if (pressSims.Lookup(modulatePage(page), ix)) {
      return ix;
    } else {
      return defaultPressSim;
    }
  }
  
  void setPageResolution(int page, int s) {
    imageRes[page] = s;
  }
  
  void setDefaultResolution(int s) {
    defaultRes = s;
  }
  
  int getPageResolution(int page) {
    int ix;
    
    if (imageRes.Lookup(modulatePage(page), ix)) {
      return ix;
    } else {
      return defaultRes;
    }
  }
  
  void setPageToExtract(int page, bool extract = true) {
    toExtract[page] = extract;
  }
  
  bool allPagesExtracted(void) {
    POSITION p;
    p = toExtract.GetStartPosition();
    while (p != NULL) {
      bool v;
      int key;
      
      toExtract.GetNextAssoc(p, key, v);
      if (v) {
        return false;
      }
    }
    
    if (pageModulator == -1) {
      p = toExtract.GetStartPosition();
      while (p != NULL) {
        bool v;
        int key;
        
        toExtract.GetNextAssoc(p, key, v);
        toExtract[key] = true;
      }
    }
    
    return true;
  }
  
  bool extractAll(void) {
    return toExtract.GetCount() == 0;
  }
  
  bool canExtractPage(int page) {
    if (extractAll()) {
      return true;
    } else {
      bool ix;
      
      if (toExtract.Lookup(modulatePage(page), ix)) {
        return ix;
      } else {
        return false;
      }
    }
  }
  
  virtual void initialize(int inputSize, int outputSize);
  virtual void clear(void); 
  virtual bool loadImages(int pageLimit);
  
  virtual bool buildIndicies(bool& handlesFullImage, int s, int presSim, bool skipLoad);
  
  virtual int getSubImageId(void) {
    return subImageCounter;
  }
  
  virtual indImageLoaderC * getLoader(void) {
    return loader;
  }
  
  virtual void advanceSubImageCount(void) {
    subImageCounter += 1;
    subWithinImageCounter += 1;
    if (pageModulator != -1 && subWithinImageCounter >= pageModulator) {
      pdfXString tmp;
      
      subWithinImageCounter = 0;
      subImageGroupCounter += 1;
      tmp.Format("%d", subImageGroupCounter);
      addDumpElement(NULL, "GROUPSEPERATOR", tmp);
    }
  }
  
  virtual void setImageLoader(indImageLoaderC * l) {
    loader = l;
  }
  
  virtual void addImageDumper(indImageDumperC * d) {
    if (dumperCount < MAX_IMAGE_DUMPERS) {
      dumpers[dumperCount] = d;
      dumperCount += 1;
    } else {
      fprintf(stderr, "Fatal - too many image dumpers!\n");
      exit( 1 );
    }
  }
  
  virtual bool processImage(int s, int presSim, bool skipLoad);
  
  virtual void imageProcessSequence(int s, int presSim, bool skipLoad) { 
    bool didEntireImage;
    
    didEntireImage = false;
    if (buildIndicies(didEntireImage, s, presSim, skipLoad)) {
      if (!didEntireImage)
      {
        dumpImage(true);
        processImage(s, presSim, skipLoad);
        if (!skipLoad) {
          dumpImage(false);
        }
      }
    }
  }
  
  bool addDumpElement(void * me, pdfXString key, pdfXString someFilePath) {
    int i;
    
    for (i = 0; i < dumperCount; i++) {
      if (me == dumpers[i] || me == NULL) {
        if (dumperPaths[i] == NULL && me != NULL) {
          dumperPaths[i] = new DumperElementPathT;
        }
        if (dumperPaths[i] != NULL) {
          dumperPaths[i]->SetAtGrow(dumperPaths[i]->GetSize(), key + "#$:" + someFilePath);
          return true;
        }
        break;
      }
    }
    
    return false;
  }
                      
  void dumpImage(bool pre) {
    int i;
    
    for (i = 0; i < dumperCount; i++) {
      if (pre) {
        if (dumpers[i]->isPreImageDumper()) {
          if (dumpers[i]->canImagePage(subImageCounter + 1)) {
            dumpers[i]->dumpImage();          
          }
         }
      } else {
        if (!dumpers[i]->isPreImageDumper()) {
          if (dumpers[i]->canImagePage(subImageCounter + 1)) {
            if (dumpers[i]->isPlaneImageDumper()) {
              dumpers[i]->dumpImage();          
            } else {
              if (lastPlane) {
                dumpers[i]->dumpImage();          
              }
            }
          }
        }
      }
    }
  }
  
  virtual inline bool imageBufferHasRoom(int count = 1) {
    return (imageBufferCurrent - imageBuffer) + count < imageBufferSize - 1;
  }
  
  virtual inline int getImageBufferSize(void) {
    return imageBufferCurrent - imageBuffer;
  }
  
  virtual inline void appendToImageBuffer(char c) {
    *imageBufferCurrent = c;
    imageBufferCurrent += 1;
  }
  
  virtual inline void advanceImageBuffer(int len) {
    imageBufferCurrent += len;
  }
  
  virtual inline void resetImageBuffer(void) {
    imageBufferCurrent = imageBuffer;
  }
  
  virtual inline char * getImageBuffer(void) {
    return imageBuffer;
  }
  
  virtual inline char * getOutputBuffer(void) {
    return outputBuffer;
  }
  
  virtual inline char * getImageBufferCurrent(void) {
    return imageBufferCurrent;
  }
  
  virtual inline int getOutputBufferScale(void) {
    return outputBufferScale;
  }
  
  virtual inline void setOutputBufferScale(int s) {
    if (s != 1 && s != 2 && s != 4) {
      fatalError(lightLOG_INTERNAL_ERROR, "Incorrect image resolution.");
    }
    outputBufferScale = s;
  }
  
  virtual inline int getWidth(void) {
    return width;
  }
  
  virtual inline int getHeight(void) {
    return height;
  }
  
  virtual inline int getPlane(void) {
    return plane;
  }
  
  virtual inline LPCTSTR getPlaneAsString(void) {
    switch( plane ) {
      case indColorC: return "C"; break;
      case indColorM: return "M"; break;
      case indColorY: return "Y"; break;
      case indColorK: return "K"; break;
    }
    return "";
  }
  
  virtual inline bool isLastPlane(void) {
    return lastPlane;
  }
  
  virtual inline int getOHeight(void) {
    return oHeight;
  }
  
  virtual inline unsigned int * getCounts(void) {
    return counts;
  }
  
  virtual inline unsigned char ** getStarts(void) {
    return starts;
  }
  
  virtual inline void setDumpTextRaster(bool dtr) {
    dumpTextRaster = dtr;
  }
  
  virtual inline int getColorsPerEntry(void) {
    return colorsPerEntry;
  }
  
  virtual inline unsigned char * getIColors(void) {
    return icolors[plane];
  }
  
  virtual inline int getIColorMax(void) {
    return iColorMax[plane];
  }
  
  virtual void setLevelMargin(double marg) {
    levelMargin = marg;
  }
  
  virtual void setPressSim(int ps) {
    pressSim = ps;
  }
  
  virtual void setPerformAutoLevel(bool pal) {
    performAutoLevel = pal;
  }
  
  virtual void disablePlaneRendering(bool * bp) {
    int i;
    
    for (i = 0; i < indColorMax + 1; i++) {
      if (bp[i]) {
        renderPlane[i] = false;
      } else {
        renderPlane[i] = true;
      }
    }
    
  }

  virtual void setMetaData(pdfXString source) {
    splitFilePath(source, filePath, fileRoot, fileExt, true);
  }
  
  void nextPlane(void);
  void nextImage(void);
  int writePixel(unsigned char * outBase, int& outP, int value, int s, int pixelStep, int rMask, bool isGB, int lastV);
  pdfXString stringPerformSubstitutions(pdfXString input);
  bool dumpFileList(void * me, int groupSize, pDumperElementPathT control, pdfXString path, pdfXString otemplate, bool breakOnGroup, bool aggregate, bool cleanup);
  
protected:
    
  int width;
  int oHeight;
  int height;
  
  int plane;
  bool lastPlane;
  int workingCol;
  bool colStarted;
  int pass;
    
  char * imageBuffer;
  char * imageBufferCurrent;
  int imageBufferSize;
  
  char * outputBuffer;
  char * outputBufferCurrent;
  int outputBufferSize;
  int outputBufferScale;
  int colorsPerEntry;
  
  int inputScanLineLength;
  char * inputScanLine;
  int maskScanLineLength;
  char * maskScanLine;
  int outputScanLineLength;
  char * outputScanLine;

  // Reset for each plane...
  unsigned int * counts;
  unsigned char ** starts;
  
  // Accumulated across planes...
  unsigned char * icolors[indColorMax];
  int iColorMax[indColorMax];
  int colorValueRange[indColorMax][256];
  int colorPlaneValueMin[indColorMax];
  int colorPlaneValueMax[indColorMax];
  int colorValueMin;
  int colorValueMax;
  int minRenderColumn;
  int maxRenderColumn;
  
  double levelMargin;
  bool performAutoLevel;
  bool dumpTextRaster;
  bool applyConvolution;
  double convolutionMixAmount;
  
  bool renderPlane[indColorMax];
  pdfXString textRenditionPath;
  pdfXString textRenditionTemplate;
  pdfXString fileRoot, fileExt, filePath;
  
  int pressSim;
  
  int indVersion;
  
  pDumperElementPathT dumperPaths[MAX_IMAGE_DUMPERS];
};


#endif // lightIndImageGross_H_
