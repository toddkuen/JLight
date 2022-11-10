#ifndef lightGenericDumpers_H_
#define lightGenericDumpers_H_

// Copyright (C) 2007 Todd R. Kueny, Inc.  All Rights Reserved.
//
// Change Log:
//
//	  09-15-2007 - TRK - Created.
//
//

#include "lightIndImageGross.h"

class indPDFPreviewDumperC : public indImagePreviewBaseC {
protected:
  double scale;
public:
  indPDFPreviewDumperC(double s) : indImagePreviewBaseC() {
    scale = s;
  }
  
  double getScale(void) {
    return scale;
  }
  
  bool dumpFilteredPreviewImage(pdfXString somePath);
};

class indHexDumperC : public indImageDumperC {
protected:
  
  pdfXString fileTemplate;
  
public:
  
  indHexDumperC(indImageGrossC * parent);
  void setFileTemplate(pdfXString templt);
  bool dumpImage(void);
};

class indMathematicaDumperC : public indImageDumperC {
protected:
  
  int width, height, xOffset, yOffset;
  pdfXString fileTemplate;
  
public:
  
  void setImageArea(int w, int h, int xOff, int yOff) {
    width = w;
    height = h;
    xOffset = xOff;
    yOffset = yOff;
  }
  
  indMathematicaDumperC(indImageGrossC * parent);
  void setFileTemplate(pdfXString templt);
  bool dumpImage(void);
};

class indPDF1DumperC : public indImageDumperC {
protected:
  
  int width, height, xOffset, yOffset;
  pdfXString fileTemplate, previewTemplate, previewPath;
  int cyclesPerSide;
  double sheetWidth;
  double sheetHeight;
  bool flateCompress;
  bool ascii85encode;
  pdfXArray<indPDFPreviewDumperC *, indPDFPreviewDumperC *> previews;
  
public:
    
  void setCyclesPerSize(int c)
  {
    cyclesPerSide = c;
  }
  
  void setImageArea(int w, int h, int xOff, int yOff) {
    width = w;
    height = h;
    xOffset = xOff;
    yOffset = yOff;
  }
  
  void setSheetSize(double h, double w) {
    sheetWidth = w;
    sheetHeight = h;
  }
  
  void setFlateCompress(bool b) {
    flateCompress = b;
  }
  
  void setAscii85Encode(bool b) {
    ascii85encode = b;
  }
  
  indPDF1DumperC(indImageGrossC * parent);
  ~indPDF1DumperC();
  
  void setFileTemplate(pdfXString templt);
  bool dumpImage(void);
  
  bool addPreview(double scale, int filterControl, pdfXString& error);
  
  void setPreviewTemplate(pdfXString pt) {
    previewTemplate = pt;
  }
  
  void setPreviewPath(pdfXString pp) {
    previewPath = pp;
  }
  
  bool dumpMasterFile(int groupSize,
                      pDumperElementPathT dmp, 
                      pDumperElementPathT pageControlInfo, 
                      pdfXString path, 
                      pdfXString otemplate,
                      bool breakOnGroup,
                      bool aggregate,
                      bool cleanup);
};

class indPS1DumperC : public indImageDumperC {
protected:
  
  int width, height, xOffset, yOffset;
  pdfXString fileTemplate;
  double sheetWidth;
  double sheetHeight;
  bool lzwCompress;
  bool ascii85encode;
  
public:
    
    void setImageArea(int w, int h, int xOff, int yOff) {
      width = w;
      height = h;
      xOffset = xOff;
      yOffset = yOff;
    }
  
  void setSheetSize(double h, double w) {
    sheetWidth = w;
    sheetHeight = h;
  }
  
  void setLZWCompress(bool b) {
    lzwCompress = b;
  }
  
  void setAscii85Encode(bool b) {
    ascii85encode = b;
  }
  
  indPS1DumperC(indImageGrossC * parent);
  void setFileTemplate(pdfXString templt);
  bool dumpImage(void);
  
  bool dumpMasterFile(int groupSize,
                      pDumperElementPathT dmp, 
                      pDumperElementPathT pageControlInfo, 
                      pdfXString path, 
                      pdfXString otemplate,
                      bool breakOnGroup,
                      bool aggregate, 
                      bool cleanup);
};

#endif // lightGenericDumpers_H_

