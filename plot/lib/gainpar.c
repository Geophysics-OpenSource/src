#include <math.h>

#include <rsf.h>

#include "gainpar.h"

static void gain (sf_file in, float **data, int n1, int n2,int step,
		  float pclip,float phalf,
		  float *clip, float *gpow, float bias, int nt, float* buf);
static float quantile (float  p, float* x, int n);

void gainpar (sf_file in, float **data, int n1, int n2,int step,
	      float pclip,float phalf,
	      float *clip, float *gpow, float bias, 
	      int n3, int panel)
{
    int nt, i3;
    float *buf, *clipnp, *gpownp;

    nt = n1 / step;
    buf = sf_floatalloc(nt*n2);
  
    if(panel >= 0) {
	if (panel > 0) sf_seek(in,panel*n1*n2*sizeof(float),SEEK_SET);
	gain (in, data, n1, n2, step, pclip, phalf,
	      clip, gpow, bias, nt, buf);
    } else { /* gainpanel=each */
	clipnp = sf_floatalloc(n3);
	gpownp = sf_floatalloc(n3);

	for (i3=0; i3<n3; i3++) {
	    clipnp[i3] = 0.;
	    gpownp[i3] = 0.;	    
	    gain (in, data, n1, n2, step, pclip, phalf,
		  clipnp+i3, gpownp+i3, bias, nt, buf);
	    if (NULL == in) data += n1*n2; /* next panel */
	}

	if (*clip==0.) *clip = quantile (pclip,clipnp,n3);
	if (*gpow==0.) *gpow = quantile (phalf,gpownp,n3);
        free(clipnp);
        free(gpownp);
    }	

    free (buf);
}
 
static void gain (sf_file in, float **data, int n1, int n2, int step,
		  float pclip,float phalf, float *clipp, float *gpowp, 
		  float bias, int nt, float *buf)
{
    int j, i2, it, ntest, n;
    float clip, gpow, half;

    n = n2*nt;

    if (NULL != in) sf_floatread (data[0],n1*n2,in);

    for (j=i2=0; i2<n2; i2++) {
	for (it=0; it<nt; it++, j++) {
	    buf[j] = fabsf(data[i2][it*step]- bias);
	}
    }
    
    /* determine gain factors */
    clip = *clipp;

    if (clip==0.) {
	ntest = n*pclip/100. + .5;
	if (ntest >= n) { /* 100% clip */
	    clip = buf[0];		
	    for (j=1; j < n; j++) {
		if(buf[j] > clip) clip=buf[j];
	    }	
	} else {
	    clip = quantile(pclip,buf,n);
	}
	*clipp = clip;
    }
    
    if (*gpowp==0.) {
	ntest = n*phalf/100. + .5;
	if (ntest >= n) { /* 100% half */
	    half = buf[0];
	    for (j=1; j < n; j++) {
		if(buf[j] > half) half=buf[j];
	    }	
	} else {
	    half = quantile (phalf,buf,n);
	}
	
	if (clip==0. || half == clip || half/clip < 0.001) {
	    gpow = 1.;
	} else {
	    gpow = logf (.5) / logf (half / clip);
	    if (gpow < 0.1) {
		gpow = 0.1;
	    } else if (gpow > 10.) {
		gpow = 10.;
	    }
	}
	*gpowp = gpow;
    }
}

static float quantile (float  p, float* x, int n)
{
  int q;
  float *i, *j, ak, *low, *hi, buf, *k;

  q = (p*n)/100.;
  if      (q < 0)  q=0;
  else if (q >= n) q=n-1;
  low=x;
  hi=x+n-1;
  k=x+q; 
  while (low<hi) {
      ak = *k;
      i = low; j = hi;
      do {
	  while (*i < ak) i++;     
	  while (*j > ak) j--;     
	  if (i<=j) {
	      buf = *i;
	      *i++ = *j;
	      *j-- = buf;
	  }
      } while (i<=j);
      if (j<k) low = i; 
      if (k<i) hi = j;
  }
  return (*k);
}
