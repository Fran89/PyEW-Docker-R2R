// IReport.h

#ifndef _IREPORT_H_
#define _IREPORT_H_

#include "ISiam.h"

struct IReport : public ISiam {
public:
	virtual void FrameGraph() = 0;
	virtual void FrameImage() = 0;
	virtual void BlitImage() = 0;
	virtual void Frame() = 0;
	virtual void Text(const char *txt) = 0;
	virtual void Text(int index, const char *txt) = 0;
	virtual void Title(char *title) = 0;
	virtual void XAxis(double xmin, double xdel, double xmax, char *fmt) = 0;
	virtual void YAxis(double ymin, double ydel, double ymax, char *fmt) = 0;
	virtual void Graph(double w, double h) = 0;
	virtual void Image(int w, int h) = 0;
	virtual void PenUp() = 0;
	virtual int  H(double y) = 0;
	virtual int  W(double x) = 0;
	virtual void Pen() = 0;
	virtual void Pen(int ix) = 0;
	virtual void Pen(int ix, int w) = 0;
	virtual void Pen(int r, int g, int b, int w) = 0;
	virtual void Plot(double x, double y) = 0;
	virtual void Symbol(double x, double y) = 0;
	virtual void Pixel(int w, int h, int r, int g, int b) = 0;
	virtual void Page() = 0;
	// More detailed control
	virtual int getPageW() = 0;
	virtual int getPageH() = 0;
	virtual int getScaleW() = 0;
	virtual int getScaleH() = 0;
	virtual void setOrigin(int w, int h) = 0;
	virtual HDC getImageDC()  = 0;
};

#endif
