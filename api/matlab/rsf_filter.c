/* Run an RSF program on MATLAB data.
 *
 * MATLAB usage: rsf_filter(filter,input,output)
 *
 */
/*
  Copyright (C) 2013 University of Texas at Austin

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <string.h>
#include <stdio.h>

#include <mex.h>
#include <rsf.h>

#define DUP(a) if (dup(a) < 0) sf_error("dup error:")
#define MAXARG 256

void mexFunction(int nlhs, mxArray *plhs[], 
		 int nrhs, const mxArray *prhs[])
{
    int cmdlen, status, argc=2, i, ndim, esize, dims[SF_MAX_DIM];
    const int *dim=NULL;
    size_t nbuf = BUFSIZ, nd, j;
    char *cmd=NULL, *argv[] = {"matlab","-"};
    double *dr=NULL, *di=NULL;
    float *p=NULL;
    sf_complex *c=NULL;
    char buf[BUFSIZ], key[5], *iname, *oname, cmdline[SF_CMDLEN];
    sf_file inp;
    sf_datatype type;
    FILE *ifile, *ofile;

    /* Check for proper number of arguments. */
    if (nrhs < 2) mexErrMsgTxt("At least two inputs are required.");

    /* initialize Madagascar */
    sf_init(argc,argv);

    /* Input 2 must be a number. */
    if (!mxIsDouble(prhs[1])) mexErrMsgTxt("Input 2 must be double.");
    
    /* get data dimensions */
    ndim=mxGetNumberOfDimensions(prhs[1]);
    dim=mxGetDimensions(prhs[1]);
    
    /* get data size */
    nd = mxGetNumberOfElements(prhs[1]);

    ifile = sf_tempfile(&iname,"w+b");
    ofile = sf_tempfile(&oname,"w+b");
    fclose(ofile);

    inp = sf_output(iname);
    fclose(ifile);

    sf_setformat(inp,mxIsComplex(prhs[1])?"native_complex":"native_float");
    
    /* Output */
    for (i=0; i < ndim; i++) {
	sprintf(key,"n%d",i+1);
	sf_putint(inp,key,dim[i]);
    }
    
    if (mxIsComplex(prhs[1])) {
	/* complex data */
	c = (sf_complex*) buf;
	
	dr = mxGetPr(prhs[1]);
	
	/* pointer to imaginary part */
	di = mxGetPi(prhs[1]);
	
	for (j=0, nbuf /= sizeof(sf_complex); nd > 0; nd -= nbuf) {
	    if (nbuf > nd) nbuf=nd;
	    
	    for (i=0; i < nbuf; i++, j++) {
		c[i] = sf_cmplx((float) dr[j],(float) di[j]);
	    }
	    
	    sf_complexwrite(c,nbuf,inp);
	}
    } else { 
	/* real data */
	p = (float*) buf;
	
	dr = mxGetPr(prhs[1]);
	
	for (j=0, nbuf /= sizeof(float); nd > 0; nd -= nbuf) {
	    if (nbuf > nd) nbuf=nd;
	    
	    for (i=0; i < nbuf; i++, j++) {
		p[i] = (float) dr[j];
	    }
	    
	    sf_floatwrite(p,nbuf,inp);
	}
    } 

    sf_fileclose(inp);

    /* First input must be a string. */
    if (!mxIsChar(prhs[0]))
	mexErrMsgTxt("Second input must be a string.");
    
    /* First input must be a row vector. */
    if (mxGetM(prhs[0]) != 1)
	mexErrMsgTxt("Second input must be a row vector.");
    
    /* Get the length of the input string. */
    cmdlen = mxGetN(prhs[0]) + 1;
    
    /* Allocate memory for input string. */
    cmd = mxCalloc(cmdlen, sizeof(char));
    
    /* Copy the filter name into a C string. */
    status = mxGetString(prhs[0], cmd, cmdlen);
    if (status != 0) 
	mexWarnMsgTxt("Not enough space. String is truncated.");
    
    snprintf(cmdline,SF_CMDLEN,"%s < %s > %s",cmd,iname,oname);
/*    mexWarnMsgTxt(cmdline); */
    sf_system(cmdline);
 
    inp = sf_input(oname);

    type = sf_gettype (inp);
    ndim = sf_filedims(inp,dims);
    esize = sf_esize(inp);
    
    plhs[0] = mxCreateNumericArray(ndim,dims,mxDOUBLE_CLASS,
				   type==SF_COMPLEX?mxCOMPLEX:mxREAL);
    dr = mxGetPr(plhs[0]);
    if (type==SF_COMPLEX) di = mxGetPi(plhs[0]);
    
    /* get data size */
    nd = mxGetNumberOfElements(plhs[0]);
    
    /* Read data */
    for (j=0, nbuf /= esize; nd > 0; nd -= nbuf) {
	if (nbuf > nd) nbuf=nd;
	
	switch(type) {
	    case SF_FLOAT:
		
		sf_floatread((float*) buf,nbuf,inp);
		for (i=0; i < nbuf; i++, j++) {
		    dr[j] = (double) ((float*) buf)[i];
		}
		break;
	    case SF_INT:
		
		sf_intread((int*) buf,nbuf,inp);
		for (i=0; i < nbuf; i++, j++) {
		    dr[j] = (double) ((int*) buf)[i];
		}
		break;
	    case SF_COMPLEX:
		
		sf_complexread((sf_complex*) buf,nbuf,inp);
		for (i=0; i < nbuf; i++, j++) {
		    dr[j] = (double) crealf(((sf_complex*) buf)[i]);
		    di[j] = (double) cimagf(((sf_complex*) buf)[i]);
		}
		break;
	    default:
		mexErrMsgTxt("Unsupported file type.");
		break;
	}
    }

    sf_fileclose(inp);

    sf_rm(iname,true,false,false);
    sf_rm(oname,true,false,false);
}
