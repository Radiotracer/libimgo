/*
	$Id: imgio.h 52 2009-04-04 14:10:07Z frey $
*/

#ifndef IMGIO_H
#define IMGIO_H

#include <mip/image.h>

#define IMAGE_EXTENSION ".im"

#ifdef __cplusplus
extern "C" {
#endif
void imgio_aflag(int *flagvalue, int *wflag);
void imgio_verbose(int flagvalue);
void imgio_err(char *errbuf, int abortflag);
IMAGE *imgio_createshortimage(char *filename, int xdim, int ydim, int zdim);
IMAGE *imgio_openimage(char *filename, char status, int *xdim, int *ydim, int *zdim);
void imgio_closeimage(IMAGE *image);
void imgio_readimage(IMAGE *image, float *ppix, int pixstart, int pixend);
void imgio_readbyteimage(IMAGE *image, BYTETYPE *ppix, int pixstart, int pixend);
void imgio_readgreyimage(IMAGE *image, short *ppix, int pixstart, int pixend);
void imgio_writeimage(IMAGE *image, float *ppix, int pixstart, int pixend);
void imgio_writeshortimage(IMAGE *image, float *ppix, float fOffset, float fScaleFac, int pixstart, int pixend);
void pixstats(float *pixels, int num);
void imgio_readslices(IMAGE *image, int start, int end, float *pixels);
void imgio_writeslices(IMAGE *image, int start, int end, float *pixels);
void imgio_readline(IMAGE *image, int slice, int pos, int direction, float *pixels);
void imgio_writeline(IMAGE *image, int slice, int pos, int direction, float *pixels);
void imgio_putinfo(IMAGE *image, char *id, char *val);
void imgio_getinfo(IMAGE *image, char *id, char *val);
void imgio_gettitle(IMAGE *image, char *title);
void imgio_puttitle(IMAGE *image, char *title);
void imgio_copyinfo(IMAGE *image1, IMAGE *image2);
void imgio_dim(IMAGE *image, int *pixformat, int *dimc);
void imgio_bounds(IMAGE *image, int *dimv);
float *readimage2d(char *fname, int *piXdim, int *piYdim);
unsigned char *readbyteimage3d(char *fname, int *piXdim, int *piYdim, int *piZdim);
short *readgreyimage3d(char *fname, int *piXdim, int *piYdim, int *piZdim);
float *readimage3d(char *fname, int *piXdim, int *piYdim, int *piZdim);
void writeimage(char *fname, int xdim, int ydim, int zdim, float *pixels);
void writeshortimage(char *fname, int xdim, int ydim, int zdim, float *pixels, float fOffset, float fScaleFac);
void irl_imheader(char *imagename, int *pixcnt, int *dimc, int *dimv);
char *imgio_errstr(void);
void imgio_set_abort_flag(int iAbortFlag);
int imgio_errflag(int iReset);
#ifdef __cplusplus
}
#endif

#endif
