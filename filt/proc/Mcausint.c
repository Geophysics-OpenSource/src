/* Causal integration on the first axis. 

Takes: < in.rsf > out.rsf
*/

#include <rsf.h>
#include "causint.h"

int main(int argc, char* argv[])
{
    int n1, n2, i2;
    bool adj;
    float *pp, *qq;
    sf_file in, out;

    sf_init(argc,argv);

    in = sf_input("in");
    out = sf_output("out");

    if (!sf_histint(in,"n1",&n1)) sf_error("No n1= in input");
    n2 = sf_leftsize(in,1);

    pp = sf_floatalloc(n1);
    qq = sf_floatalloc(n1);

    if (!sf_getbool("adj",&adj)) adj=false;
    /* if y, do adjoint integration */

    for (i2=0; i2 < n2; i2++) {
	sf_read(pp,sizeof(float),n1,in);
	if (adj) {
	    causint_lop (true,false,n1,n1,qq,pp);
	} else {
	    causint_lop (false,false,n1,n1,pp,qq);
	}
	sf_write(qq,sizeof(float),n1,out);
    }

    sf_close();
    exit(0);
}

/* 	$Id: Mcausint.c,v 1.1 2004/03/26 03:32:17 fomels Exp $	 */



