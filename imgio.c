/*
	imgio: an i/o interface to /usr/image for irl
	
	currently all errors are caused by calling imgio_err() which, by
	default, displays a message and then terminates. 
	
	$Id: imgio.c 52 2009-04-04 14:10:07Z frey $ 
	
 	Copyright 2002-2005 The Johns Hopkins University. ALL RIGHTS RESERVED.
*/

#ifdef WIN32
#include <io.h>
#define unlink _unlink
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>
#include <mip/image.h>
#include <mip/miputil.h>
#include "imgio.h"

#define X2d 1
#define Y2d 0
#define X3d 2
#define Y3d 1
#define Z3d 0

static int errflag=0;
							/* is set to non-zero value by imgio_err function if   */
							/* it is called with abort flag as zero.It can be read */
							/* and reset by programmer using the function          */
							/* imgio_aflag                                         */

static int verbose=0;		/* setting this to 1 prints out lots of messages*/

static int isgAbortFlag = 1; /* if nonzero, imgio errors result in abort.
											if zero, errors only set errflag */

static char pchsgImgioErrString[257];

void imgio_set_abort_flag(int iAbortFlag)
/*
	Purpose: sets the imgio global abort flag. When this flag is nonzero,
		calls to imgio_err will result in the program exiting. If it is
		zero, these errors only set the errflag and return. The user
		can check the status of error flag using the imgio_errflag call.
*/
{
	isgAbortFlag = iAbortFlag;
}
int imgio_errflag(int iReset)
{
   int iReturnVal=errflag;/* save the error flag value for return*/

   if (iReset != 0)
      errflag=0;
   return(iReturnVal);
}


/*--------------------------------------------------------------------------*/
/* Purpose:When called with wflag as false simply puts the current errflag	 */
/*     value in flagvalue and if wflag is true puts the current errflag	 	 */
/*     value in flagvalue and then resets it to zero i.e. noerror condition */
/*--------------------------------------------------------------------------*/
void imgio_aflag(int *flagvalue, int *wflag)
{
   if(*wflag) {
     *flagvalue=errflag;
     errflag=0;
   }
   else
		*flagvalue = errflag;
}

void imgio_verbose(int flagvalue)
{
	verbose= flagvalue;
}
   
/* August 1,1990 : modified to make it return to caller or abort depending on */
/*                 boolean value of abortflag.                                */
void imgio_err(char *errbuf, int abortflag)
{
   if (abortflag || verbose) fprintf(stderr,"imgio error: %s\n",errbuf);
   if(abortflag && isgAbortFlag != 0)
		exit(-1);
   else
		errflag=1;
		strncpy(pchsgImgioErrString, errbuf, 256);
		pchsgImgioErrString[256] = '\0';
}

char *imgio_errstr()
{
	return pchsgImgioErrString;
}

IMAGE *imgio_createshortimage(char *filename, int xdim, int ydim, int zdim)
{
	char errbuf[256];
	int dimc;
	int dimv[nDIMV];
	IMAGE *image;

	if (zdim == 0 || zdim == 1) {
		dimc = 2;
		dimv[Y2d]= ydim;
		dimv[X2d]= xdim;
	} 
	else {
		dimc=3;
		dimv[Z3d]= zdim;
		dimv[Y3d]= ydim;
		dimv[X3d]= xdim;
	}
	if (verbose) 
		fprintf(stderr,"unlink:%d\n",unlink(filename));
	else
		unlink(filename);

	if ((image = imcreat(filename,DEFAULT,GREY,dimc,dimv)) == INVALID) { 
		sprintf(errbuf,"can't create image %s: %s", filename, imerror());
		imgio_err(errbuf, 1);
		return (NULL);
	}
	return image;
}

IMAGE *imgio_openimage (char *filename, char status,
								int *xdim, int *ydim, int *zdim)
{
	int dimc;
	int dimv[nDIMV];
	int PixFormat;
	IMAGE *image = NULL;
	char errbuf[256];

	if (verbose)
		fprintf(stderr,"openimage: filename=%s, status=%c\n", filename, status);

	switch (status) {
		case 'n':
		case 'N':
			if (verbose) 
				fprintf(stderr,"opening new\n");

			if (*zdim == 0 || *zdim == 1) {
				dimc = 2;
				dimv[Y2d]= *ydim;
				dimv[X2d]= *xdim;
			} 
			else {
				dimc=3;
				dimv[Z3d]= *zdim;
				dimv[Y3d]= *ydim;
				dimv[X3d]= *xdim;
			}

			/* make sure file doesn't exist */	
			if (verbose) 
				fprintf(stderr,"unlink:%d\n",unlink(filename));
			else
				unlink(filename);

			if ((image = imcreat(filename,DEFAULT,REAL,dimc,dimv)) == INVALID) { 
				sprintf(errbuf,"can't create image %s: %s", filename, imerror());
				imgio_err(errbuf, 1);
				return (NULL);
			}
			break;
		case 'o':
		case 'O':
			if (verbose) 
				fprintf(stderr, "opening old\n");

			if ((image = imopen(filename,UPDATE)) == INVALID) {
				if (verbose)
					fprintf(stderr,"can't open image with write permission\n");

				if ((image = imopen(filename,READ)) == INVALID) {
					sprintf(errbuf,"can't open image %s: %s with READ permission",
								filename,imerror());
					imgio_err(errbuf,1);
					return (NULL);
				} 
				else if (verbose) 
					fprintf(stderr,"opened with read permission\n");
			}

			imdim(image, &PixFormat, &dimc);

			imbounds(image, dimv);

			if (dimc == 1) {
				*zdim = 1;
				*ydim = 1;
				*xdim=dimv[0];
			}
			if (dimc == 2) {
				*zdim = 1;
				*xdim = dimv[X2d];
				*ydim = dimv[Y2d];
			} 
			else if (dimc == 3) {
				*zdim = dimv[Z3d];
				*xdim = dimv[X3d];
				*ydim = dimv[Y3d];
			} 
			else 
   			sprintf(errbuf,"image %s is %dD. only 2&3d images are allowed", 
							filename, dimc);

			if (image->SwapNeeded)
				fprintf(stderr, "Pixels swapped from nonnative format\n");

			if (PixFormat != REAL && PixFormat != GREY && PixFormat != BYTE) {
				sprintf(errbuf,"%s does not have pixels in format REAL or GREY or BYTE", 
							filename);
				imgio_err(errbuf,1);
				return NULL;
			}

			break;
		default:
			imgio_err("imgio_open status must be either 'o' or 'n'", 1);
			return NULL;
			break;
	}

	return (image);
}

void imgio_closeimage(IMAGE *image)
{
	if (imclose((IMAGE *)image) == INVALID)
		imgio_err(imerror(), 1);
}

/*---------------------------------------------------------------------------*/
/* Purpose : reads the pixels of a image specified by image into the array   */
/*           pix. The pixels  are specified  by giving the starting pixel    */
/*           and the ending pixel .														  */
/*---------------------------------------------------------------------------*/
void imgio_readimage(IMAGE *image, float *ppix, int pixstart, int pixend)
{
   char eb[400];
   int pixformat;
   int dimc;
   int dimv[nDIMV];
   int numpixels, i;
	GREYTYPE *pgTmp;

   strcpy(eb,"Error in imgio routine \"imgio_readimage\":");

   imdim(image,&pixformat,&dimc);

   imbounds(image,dimv);

   if(pixformat!=REAL && pixformat != GREY){
		imgio_err(strcat(eb,"can read only realtype or greytype images"),1);
		return;
	}

   if(dimc < 1 || dimc > 3){
		imgio_err(strcat(eb,"can use only 1, 2 or 3-d images"),1);
		return;
	}

   numpixels = pixend-pixstart+1;

   if(imread(image,pixstart,pixend,(GREYTYPE*)ppix)==INVALID){
   	imgio_err(strcat(eb,imerror()),1);
		return;
	}

	if (pixformat == GREY){
		fprintf(stderr,"converting grey image to real\n");
		/*convert to real*/
		pgTmp = (GREYTYPE *)ppix;
		for(i=numpixels-1; i >= 0; --i)
			ppix[i] = (REALTYPE)pgTmp[i];
	}
}

void imgio_readbyteimage(IMAGE *image, BYTETYPE *ppix, int pixstart, int pixend)
{
   char eb[400];
   int pixformat;
   int dimc;
   int dimv[nDIMV];
   int numpixels;

   strcpy(eb,"Error in imgio routine \"imgio_readbyteimage\":");

   imdim(image,&pixformat,&dimc);

   imbounds(image,dimv);

   if (pixformat != BYTE){
		imgio_err(strcat(eb,"can only read byte images"),1);
		return;
	}

   if(dimc < 1 || dimc > 3){
		imgio_err(strcat(eb,"can use only 1, 2 or 3-d images"),1);
		return;
	}

   numpixels = pixend-pixstart+1;

	if (verbose)
		fprintf(stderr,"num=%d, start=%d, end=%d\n",numpixels, pixstart, pixend);
   if(imread(image,pixstart,pixend,(GREYTYPE*)ppix)==INVALID){
   	imgio_err(strcat(eb,imerror()),1);
		return;
	}
}

void imgio_readgreyimage(IMAGE *image, short *ppix, int pixstart, int pixend)
{
   char eb[400];
   int pixformat;
   int dimc;
   int dimv[nDIMV];
   int numpixels, i;
	BYTETYPE *pbTmp;

   strcpy(eb,"Error in imgio routine \"imgio_readgreyimage\":");

   imdim(image,&pixformat,&dimc);

   imbounds(image,dimv);

   if(pixformat != GREY && pixformat != BYTE){
		imgio_err(strcat(eb,"can read only greytype or bytetype images"),1);
		return;
	}

   if(dimc < 1 || dimc > 3){
		imgio_err(strcat(eb,"can use only 1, 2 or 3-d images"),1);
		return;
	}

   numpixels = pixend-pixstart+1;

   if(imread(image,pixstart,pixend,ppix)==INVALID){
   	imgio_err(strcat(eb,imerror()),1);
		return;
	}

	if (pixformat == BYTE){
		if (verbose)
			fprintf(stderr,"converting byte to real image\n");
		/* convert to GREY */
		pbTmp = (BYTETYPE *)ppix;
		for(i=numpixels-1; i >= 0; --i)
			ppix[i] = (GREYTYPE)pbTmp[i];
	}
}


/*---------------------------------------------------------------------------*/
/* Purpose : writes the  pixels  in the  array  ppix								  */
/*           in the image specified by image starting at pixstart  			  */
/*           and ending at pixend (including both).If sufficient number of   */
/*           pixel values are not present in ppix error occurs.              */
/*---------------------------------------------------------------------------*/
void imgio_writeimage(IMAGE *image, float *ppix, int pixstart, int pixend)
{
   char eb[400];
   int pixformat;
   int dimc;
   int dimv[nDIMV];
   int numpixels;

   strcpy(eb,"Error in imgio routine \"imgio_writeimage\":");

   imdim(image,&pixformat,&dimc);

   imbounds(image,dimv);

   if(pixformat!=REAL){
		imgio_err(strcat(eb,"can write only realtype images"),1);
		return;
	}

   if((dimc!=2)&&(dimc!=3)){
		imgio_err(strcat(eb,"can use only 2 or 3 d images"),1);
		return;
	}

   numpixels = pixend-pixstart+1;
   if(imwrite(image,pixstart,pixend,(GREYTYPE*)ppix)==INVALID){
		imgio_err(strcat(eb,imerror()),1);
		return;
	}
}

void imgio_writeshortimage(IMAGE *image, float *ppix, 
		float fOffset, float fScaleFac, 
		int pixstart, int pixend)
/* writes array of floating point pixels to greytype image. The pixels
 * are converted to short via the transformation:
 * greypixel = (floatpixel - fOffset)*fScaleFac;
 * the original floating point pixels are not changed by the transformation
*/
{
   char eb[400];
   int pixformat;
   int dimc;
   int dimv[nDIMV];
   int numpixels;
	GREYTYPE *greypixels;

   strcpy(eb,"Error in imgio routine \"imgio_writeshortimage\":");

   imdim(image,&pixformat,&dimc);

   imbounds(image,dimv);

   if(pixformat!=GREY){
		imgio_err(strcat(eb,"writeshortimage can write only greytype images"),1);
		return;
	}

   if((dimc!=2)&&(dimc!=3)){
		imgio_err(strcat(eb,"can use only 2 or 3 d images"),1);
		return;
	}
	
   numpixels = pixend-pixstart+1;
	greypixels = RealToShort(ppix, numpixels, fOffset, fScaleFac);


   if(imwrite(image,pixstart,pixend,greypixels)==INVALID){
		imgio_err(strcat(eb,imerror()),1);
		return;
	}
	IrlFree(greypixels);
}


void pixstats(float *pixels, int num)
{
	register float max, sum, *pf;

	max = sum = 0.0;
	for(pf = pixels; num > 0; --num, ++pf){
		sum += *pf;
		max = max > *pf ? max : *pf;
	}
	fprintf(stderr,"sum=%.5g, max=%.5g\n",sum,max);
}

/*------------------------------------------------------------------------*/
/*	reads slices at z=start to end in the image file specified by image.
	inputs
		IMAGE *image
		int start,end starting and ending slice number
	outputs
		float *pixels xdim * ydim * (start - end + 1) array in which the
			image is returned. the image is returned so that z
                        varies most slowly, then y, then x.					  */
/*------------------------------------------------------------------------*/
void imgio_readslices(IMAGE *image, int start, int end, float *pixels)
{
	int dimc, dimv[nDIMV], PixFormat;
	int i;
	long pixperslice;
	char errbuf[256];
	GREYTYPE *ppix, *pgrey;
	float *pfloat;

	imdim(image, &PixFormat, &dimc);
	imbounds(image, dimv);
	if (verbose)
		fprintf(stderr,"readslices from %d to %d",start,end);

	if (dimc == 2) {
		if (start != end || start != 0){
			imgio_err("can only read slice 0 from a 2 d image\n",1);
			return;
		}
		pixperslice = dimv[X2d] * dimv[Y2d];
	}
	else {
		if (start > end || start < 0 || end > dimv[Z3d]-1) {
         sprintf(errbuf,"slices %d to %d are not in the image",start,end);
			imgio_err(errbuf,1);
			return;
		}
		pixperslice = dimv[X3d] * dimv[Y3d];
	}

	/* the following code assumes that the pixels are stored with the
			Z dimension varying slowest */
	
	if (PixFormat == GREY) {
		ppix = (GREYTYPE*)pvIrlMalloc(sizeof(GREYTYPE)*pixperslice*(end-start+1),
							"imgio_readslices:pix"); 
	}
	else 
		ppix = (GREYTYPE *)pixels;

	if (imread(image,pixperslice*start,pixperslice*(end+1)-1,ppix) == INVALID) {
		sprintf(errbuf,"readslices: %s",imerror());
		imgio_err(errbuf,1);
		return;
	}

	if (PixFormat == GREY) { /*convert pixels from grey to float*/
/* a bug here */
	/*	for(i=pixperslice, pfloat=pixels, pgrey=ppix; i > 0; --i) */
		for(i=pixperslice*(end-start+1), pfloat=pixels, pgrey=ppix; i > 0; --i) 
			*pfloat++ = (float)*pgrey++;
		IrlFree((char *)ppix);
	}

	if (verbose)
		pixstats(pixels, pixperslice * (end - start + 1));
}

/*-------------------------------------------------------------------------*/
/*
	writes slices at z=start,end in the image file specified by image. only 
	images of type real can be written
	inputs
		IMAGE *image
		int start,end starting and ending slice number
		float image xdim * ydim * (start - end + 1) array from which the
		image is written. the image should be organized so that z varies
  	        most slowly, then y, then x.
																								  */
/*------------------------------------------------------------------------*/
void imgio_writeslices(IMAGE *image, int start, int end, float *pixels)
{
	int dimc, dimv[nDIMV], PixFormat;
	long pixperslice;
	char errbuf[256];

	if (verbose)
		fprintf(stderr,"writeslices %d to %d\n",start,end);

	imdim(image,&PixFormat, &dimc);

	if (PixFormat != REAL){
		imgio_err("writeslices: Only real images can be written to",1);
		return;
	}

	imbounds(image, dimv);

	if (dimc == 2) {
		if (start != end || start != 0){
			imgio_err("writeslices: can only write slice 0 for a 2-d image\n",1);
			return;
		}
		pixperslice = dimv[X2d] * dimv[Y2d];
	} 
	else {
		if (start > end || start < 0 || end > dimv[Z3d]-1) {
			sprintf(errbuf,"slices %d to %d are not in the image",start,end);
			imgio_err(errbuf,1);
			return;
		}
		pixperslice = dimv[X3d] * dimv[Y3d];
	}

	/* the following code assumes that the pixels are stored with the
			Z dimension varying slowest */
	
	if (imwrite(image,pixperslice * start, pixperslice * (end+1) - 1,
			(GREYTYPE *)pixels) == INVALID) {
		sprintf(errbuf,"writeslices: %s",imerror());
		imgio_err(errbuf,1);
		return;
	}

	if (verbose)pixstats(pixels, pixperslice * (end - start + 1));
}


static int SetupEndpoints(int dimc, int *dimv, int slice, int pos,
	int direction, int endpoints[][2])
{
	char errbuf[256];
	int x,y;

/* compute the endpoints array needed for the call to imgetpix array
	so that we can read the data in the x or y = pos line (depending on
	direction). to do this we
	1 compute which element of endpoints corresponds to x and which
			  to y depending on the value dimension of the image
	2 assign values to the elements to read the appropriate line
			  of pixels in the appropriate direction
	3 return the number of pixels to be read
*/
	if (dimc == 3) {
		if (slice < 0 || slice > dimv[Z3d]-1){
			sprintf(errbuf,"slice %d is not in the image",slice);
			imgio_err(errbuf,1);
			return -1;
		}
		endpoints[Z3d][0] = endpoints[Z3d][1] = slice;
		x = X3d;
		y = Y3d;
	}
	else {/*2d image*/
		if (slice != 0) {
			sprintf(errbuf,"slice must be 0, not %d, for a 2d image",slice);
			imgio_err(errbuf,1);
			return -1;
		}
		x = X2d;
		y = Y2d;
	}	

	/* now set up endpoints array*/
	if (direction == 0) {/* x=constant*/		
		if (pos < 0 || pos > dimv[x]-1) {
			sprintf(errbuf,"line io: illegal value for pos: %d",pos);
			imgio_err(errbuf,1);
			return -1;
		}
		endpoints[x][0] = endpoints[x][1] = pos;
		endpoints[y][0] = 0;
		endpoints[y][1] = dimv[y]-1;
	}
	else {/* y=constant*/		
		if (pos < 0 || pos > dimv[y]-1) {
			sprintf(errbuf,"line io: illegal value for pos=%d",pos);
			imgio_err(errbuf,1);
			return -1;
		}
		endpoints[y][0] = endpoints[y][1] = pos;
		endpoints[x][0] = 0;
		endpoints[x][1] = dimv[x]-1;
	}

#ifdef DEBUG
	for (i=0; i<dimc; ++i)
		printf("%d:(%d-%d)\n",i,endpoints[i][0], endpoints[i][1]);
#endif

	/* return the number of pixels to be read. this is ydim for
		reading with constant x (direction==0) and xdim for reading
		with constant y.
	*/
	return (direction==0 ? dimv[y] : dimv[x]);
}

/*------------------------------------------------------------------------*/
/*	reads the pixels from row or colum number pos with z=slice into line   
	inputs																					  
		IMAGE *image;																		  
		int slice; the data is from z=slice
		int pos; defines the constant x or y position
		int direction; 0 to read along x = pos, 1 for y=pos
	outputs
		float *pixels; array of ydim or xdim in length where the pixels
                are placed																  */
/*------------------------------------------------------------------------*/
void imgio_readline (IMAGE *image, int slice, int pos, int direction,
	float *pixels)
{
	int endpoints[3][2], Coarseness[3];
	int dimc, dimv[nDIMV], PixFormat,i,npix;
	char errbuf[256], *msg;
	GREYTYPE *ppix, *pgrey;
	float *pfloat;

	/* 
		the easiest way to read the pixels is to use imgetpix.
		this requires that we set up the array of endpoints
		for each dimension. So we must:
		1 set the coarseness to 1 for each dimension so that we read every pixel
		2 call SetupEndpoints to calculate the endpoints array
		3 call imgetpix to read the pixels
	*/

	msg= direction == 0 ? "x = " : "y = ";

	if (verbose)
		fprintf(stderr,"readline from slice %d with %s%d\n",slice,msg,pos);

	imdim(image,&PixFormat, &dimc);
	imbounds(image, dimv);

	/* want to read every pixel so set coarseness to all 1s*/
	for(i=0; i<dimc; ++i)
		Coarseness[i] = 1;

/* compute which indices into endpoints. these differ for 3d and 2d images*/
	npix=SetupEndpoints(dimc, dimv, slice, pos, direction, endpoints);

	if (PixFormat == GREY) {
		ppix = (GREYTYPE*)pvIrlMalloc(sizeof(GREYTYPE)*npix,"imgio_readline:pix");
	}
	else {
		ppix = (GREYTYPE *)pixels;
	}

	/* now we can read in the pixels */
	if (imgetpix(image, endpoints, Coarseness, ppix) == INVALID){
		sprintf(errbuf,"readslices: %s",imerror());
		imgio_err(errbuf,1);
		return;
	}

	if (PixFormat == GREY) {
		/*convert pixels from grey to float*/
		if (verbose)
			fprintf(stderr,"converting %d pixels to float\n",npix);

		for(i=npix ,pfloat=pixels, pgrey=ppix; i > 0; --i)
			*pfloat++ = (float)*pgrey++;

		IrlFree((char *)ppix);
	}
}

/*----------------------------------------------------------------------*/
/*
	writes the pixels in line into row or colum number pos with z=slice
	inputs
		IMAGE *image;
		int slice; the data is from z=slice
		int pos defines; the constant x or y position
		int direction; 0 to read along x = pos, 1 for y=pos
	outputs
		float *pixels; array of ydim or xdim in length where the pixels
                are placed
																							  */
/*---------------------------------------------------------------------*/
void imgio_writeline(IMAGE *image, int slice, int pos, int direction,
							float *pixels)
{
	int endpoints[3][2], Coarseness[3];
	int dimc, dimv[nDIMV], PixFormat,i;
	char errbuf[256], *msg;

	/* 
		the easiest way to write the pixels is to use imputpix.
		this requires that we set up the array of endpoints
		for each dimension. So we must:
		1 set the coarseness to 1 for each dimension so that we read every pixel
		2 call SetupEndpoints to calculate the endpoints array
		3 call imgetpix to read the pixels
	*/

	msg= direction == 0 ? "x = " : "y = ";

	if (verbose)
		fprintf(stderr,"writeline from slice %d with %s%d\n",slice,msg,pos);

	imdim(image,&PixFormat, &dimc);

	if (PixFormat != REAL){
		imgio_err("Only REAL images may be written to",1);
		return;
	}

	imbounds(image, dimv);

	/* want to read every pixel so set coarseness to all 1s*/
	for(i=0; i<dimc;++i)
		Coarseness[i] = 1;

/* compute which indices into endpoints. these differ for 3d and 2d images*/
	SetupEndpoints(dimc, dimv, slice, pos, direction, endpoints);

	/* now we can write out the pixels */
	if (imputpix(image, endpoints, Coarseness, (GREYTYPE*)pixels) == INVALID) {
		sprintf(errbuf,"readslices: %s",imerror());
		imgio_err(errbuf,1);
		return;
	}
}

/*---------------------------------------------------------------------------*/
/* Purpose:puts information specified by string pointer val of length vallen */
/*         in the field specified by string id of length idlen in the image  */
/*         specified by image.leading and Trailing blanks in val and id are   */
/*         deleted before being considered.                                  */
/*                                                                           */
/*---------------------------------------------------------------------------*/ 
void imgio_putinfo(IMAGE *image, char *id, char *val)
{
  char eb[400];

  if(imputinfo(image,id,val)==INVALID) {
    strcpy(eb,"Error in imgio_routine \"imgio_putinfo\":");
    imgio_err(strcat(eb,imerror()),0);
	 return;
  }
}


/*---------------------------------------------------------------------------*/
/* Purpose:gets information in field id specified by a string of length      */
/*         idlen from image specified by image and puts it in string val      */
/*         of length vallen.leading and trailing blanks in id are deleted    */
/*         before being considered.                                          */
/*                                                                           */
/*---------------------------------------------------------------------------*/ 
void imgio_getinfo(IMAGE *image,char *id, char *val) 
{
   char eb[400];

   if((val=imgetinfo(image,id))==NULL) { 
        strcpy(eb,"Error in imgio routine \"imgio_getinfo\":");
   	  imgio_err(strcat(eb,imerror()),0);
		  return;
   }
}

/*---------------------------------------------------------------------------*/
/* Purpose:gets title string from image specified by image and puts it in     */
/*         string title of length titlelen.title should be of length one     */
/*         more than maximum title string allowed in images.                 */
/*                                                                           */
/*---------------------------------------------------------------------------*/ 
void imgio_gettitle(IMAGE *image, char *title)
{
   char eb[400];

   if(imgettitle(image,title)==INVALID) {
     strcpy(eb,"Error in imgio routine \"imgio_gettitle\":");
     imgio_err(strcat(eb,imerror()),0);
	  return;
   }
}

/*---------------------------------------------------------------------------*/
/* Purpose:puts title specified by string title of length titlelen in the    */
/*         image specified by image.The leading and trailing blanks of title  */
/*         are deleted before being considered.                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/ 
void imgio_puttitle(IMAGE *image,char *title)
{
   char eb[400];

   if(imputtitle(image,title)==INVALID) {
     strcpy(eb,"Error in imgio routine \"imgio_puttitle\":");
     imgio_err(strcat(eb,imerror()),0);
	  return;
   }
}

/*---------------------------------------------------------------------------*/
/* Purpose:copies information from image specified by image1 into image      */
/*         specified by image2.                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/ 
void imgio_copyinfo(IMAGE *image1, IMAGE *image2)
{
   char eb[400];

   if(imcopyinfo(image1,image2)==INVALID) {
       strcpy(eb,"Error in imgio routine \"imgio_copyinfo\":");
       imgio_err(strcat(eb,imerror()),0);
		 return;
   }
}
 
/*---------------------------------------------------------------------------*/
/* Purpose : gets info ids from image specified by image and puts it in      */
/*           idarray(array of f77 character type.The length of each          */
/*           character being idlen,the dimension of idarray being            */
/*           maxids and also puts the actual number of ids in numids.        */
/*           Truncation is done w.r.t name of each id and number of ids.     */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/*
void imgio_infoids(IMAGE *image,int *idarray,int *idlen,int *maxids,int *numids)
{
    char **tidarray,*tid,**t2id;
    char *s;
    char eb[400];
    int i,j;
 
    i=0;
    j=0;
    *numids=0;

    if((tidarray=iminfoids(imgio_images[*image]))==INVALID) {
      strcpy(eb,"Error in imgio routine \"imgio_infoids\":");
      imgio_err(strcat(eb,imerror()),0);
      return;
    }

    tid=idarray;
    t2id=tidarray;

    while((s=(*tidarray))&&(*numids < *maxids)) {  
       for(i=0;(s[i]&&(i< *idlen));i++)
       tid[i]=s[i];
       for(j=i;j<*idlen;j++)
       tid[j]=' ';
       tid += *idlen;
       (*numids)++;
       tidarray++;
    }

    IrlFree((char *)t2id);
}
*/

/*--------------------------------------------------------------------------*/
/* Purpose: puts the pixelformat and dimensionality of the image specified  */
/*          by image in pixformat and dimc respectively .                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/
void imgio_dim(IMAGE *image,int *pixformat,int *dimc)
{
    char eb[400];

    if (imdim(image,pixformat,dimc)==INVALID) { 
		strcpy(eb,"Error in imgio routine \"imgio_dim\":");
      imgio_err(strcat(eb,imerror()),0);
		return;
    }
}
    
/*--------------------------------------------------------------------------*/
/* Purpose: puts the dimension vector information in the array dimv of the  */
/*          image specified by image.                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/
void imgio_bounds(IMAGE *image,int *dimv)
{
    char eb[400];

    if(imbounds(image,dimv)==INVALID) {
      strcpy(eb,"Error in imgio routine \"imgio_bounds\":");
      imgio_err(strcat(eb,imerror()),0);
		return;
    } 
}

float *readimage2d(char *fname, int *piXdim, int *piYdim)
{
	IMAGE *inimage;
	float *pix;
	int nx, ny, nz;

	inimage=imgio_openimage(fname, 'o', &nx, &ny, &nz);
	if (nz != 1){
		fprintf(stderr, "2d image must have zdim=1\n");
		exit(1);
	}
	*piXdim=nx;
	*piYdim=ny;
	pix = (float *)vector(nx * ny,"readimage2d:pix");
	if (pix == NULL){
		fprintf(stderr, "can't allocate kernel memory\n");
		exit(1);
	}
	imgio_readimage(inimage, pix, 0, nx*ny-1);
	imgio_closeimage(inimage);
	return(pix);
}

unsigned char *readbyteimage3d(char *fname, int *piXdim, int *piYdim, int *piZdim)
{
	IMAGE *inimage;
	unsigned char *pix;
	int nx, ny, nz;

	inimage=imgio_openimage(fname, 'o', &nx, &ny, &nz);
	*piXdim=nx;
	*piYdim=ny;
	*piZdim=nz;
	pix = (unsigned char *)pvIrlMalloc(sizeof(unsigned char)*nx*ny*nz,"readbyteimage3d:pix");
	imgio_readbyteimage(inimage, pix, 0, nx*ny*nz-1);
	imgio_closeimage(inimage);
	return(pix);
}
	
short *readgreyimage3d(char *fname, int *piXdim, int *piYdim, int *piZdim)
{
	IMAGE *inimage;
	short *pix;
	int nx, ny, nz;

	inimage=imgio_openimage(fname, 'o', &nx, &ny, &nz);
	*piXdim=nx;
	*piYdim=ny;
	*piZdim=nz;
	pix = (short *)pvIrlMalloc(sizeof(short)*nx*ny*nz,"readgreymage3d:pix");
	imgio_readgreyimage(inimage, pix, 0, nx*ny*nz-1);
	imgio_closeimage(inimage);
	return(pix);
}
	
float *readimage3d(char *fname, int *piXdim, int *piYdim, int *piZdim)
{
	IMAGE *inimage;
	float *pix;
	int nx, ny, nz;

	inimage=imgio_openimage(fname, 'o', &nx, &ny, &nz);
	*piXdim=nx;
	*piYdim=ny;
	*piZdim=nz;
	pix = (float *)vector(nx * ny * nz,"readimage3d:pix");
	imgio_readimage(inimage, pix, 0, nx*ny*nz-1);
	imgio_closeimage(inimage);
	return(pix);
}

void writeimage(char *fname, int xdim, int ydim, int zdim, float *pixels)
{
	IMAGE *outimage;

	if (verbose)
		fprintf(stderr,"writeimage %s: %dx%dx%d\n",fname, xdim,ydim,zdim);
	outimage = imgio_openimage(fname, 'n', &xdim, &ydim, &zdim);
	imgio_writeimage(outimage, pixels, 0, xdim*ydim*zdim-1);
	imgio_closeimage(outimage);
}

void writeshortimage(char *fname, int xdim, int ydim, int zdim, float *pixels,
		float fOffset, float fScaleFac)
{
	IMAGE *outimage;

	if (verbose)
		fprintf(stderr,"writeimage %s: %dx%dx%d\n",fname, xdim,ydim,zdim);
	outimage = imgio_createshortimage(fname, xdim, ydim, zdim);
	imgio_writeshortimage(outimage, pixels, fOffset, fScaleFac, 
				0, xdim*ydim*zdim-1);
	imgio_closeimage(outimage);
}

void irl_imheader(char *imagename, int *pixcnt, int *dimc, int *dimv){
  IMAGE* image;
  int maxmin[2];
  int pixformat, pixsize;
  char error_str[100];
  if((image = imopen(imagename,READ)) == NULL){
    sprintf(error_str, "Cannot open input image %s", imagename);
    irl_abort("irl_imheader",error_str);
  }
  if(imheader(image, &pixformat, &pixsize, pixcnt, dimc, dimv, maxmin) == 
     INVALID)
    irl_abort("irl_util","Failed to read input image header");
  imclose(image);
}
