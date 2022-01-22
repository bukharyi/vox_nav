/*
 *    This file was auto-generated using the ACADO Toolkit.
 *    
 *    While ACADO Toolkit is free software released under the terms of
 *    the GNU Lesser General Public License (LGPL), the generated code
 *    as such remains the property of the user who used ACADO Toolkit
 *    to generate this code. In particular, user dependent data of the code
 *    do not inherit the GNU LGPL license. On the other hand, parts of the
 *    generated code that are a direct copy of source code from the
 *    ACADO Toolkit or the software tools it is based on, remain, as derived
 *    work, automatically covered by the LGPL license.
 *    
 *    ACADO Toolkit is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *    
 */


#include "acado_common.h"


/** Column vector of size: 16 */
real_t odeAuxVar[ 16 ];

real_t rk_ttt;

/** Row vector of size: 78 */
real_t rk_xxx[ 78 ];

/** Row vector of size: 28 */
real_t rk_kkk[ 28 ];

#pragma omp threadprivate( odeAuxVar, rk_xxx, rk_ttt, rk_kkk )

void acado_rhs_forw(const real_t* in, real_t* out)
{
const real_t* xd = in;
const real_t* u = in + 28;
/* Vector of auxiliary variables; number of elements: 16. */
real_t* a = odeAuxVar;

/* Compute intermediate quantities: */
a[0] = (cos(xd[2]));
a[1] = (sin(xd[2]));
a[2] = ((real_t)(-1.0000000000000000e+00)*(sin(xd[2])));
a[3] = (xd[12]*a[2]);
a[4] = (xd[13]*a[2]);
a[5] = (xd[14]*a[2]);
a[6] = (xd[15]*a[2]);
a[7] = (cos(xd[2]));
a[8] = (xd[12]*a[7]);
a[9] = (xd[13]*a[7]);
a[10] = (xd[14]*a[7]);
a[11] = (xd[15]*a[7]);
a[12] = (xd[24]*a[2]);
a[13] = (xd[25]*a[2]);
a[14] = (xd[24]*a[7]);
a[15] = (xd[25]*a[7]);

/* Compute outputs: */
out[0] = (xd[3]*a[0]);
out[1] = (xd[3]*a[1]);
out[2] = u[1];
out[3] = u[0];
out[4] = ((xd[16]*a[0])+(xd[3]*a[3]));
out[5] = ((xd[17]*a[0])+(xd[3]*a[4]));
out[6] = ((xd[18]*a[0])+(xd[3]*a[5]));
out[7] = ((xd[19]*a[0])+(xd[3]*a[6]));
out[8] = ((xd[16]*a[1])+(xd[3]*a[8]));
out[9] = ((xd[17]*a[1])+(xd[3]*a[9]));
out[10] = ((xd[18]*a[1])+(xd[3]*a[10]));
out[11] = ((xd[19]*a[1])+(xd[3]*a[11]));
out[12] = (real_t)(0.0000000000000000e+00);
out[13] = (real_t)(0.0000000000000000e+00);
out[14] = (real_t)(0.0000000000000000e+00);
out[15] = (real_t)(0.0000000000000000e+00);
out[16] = (real_t)(0.0000000000000000e+00);
out[17] = (real_t)(0.0000000000000000e+00);
out[18] = (real_t)(0.0000000000000000e+00);
out[19] = (real_t)(0.0000000000000000e+00);
out[20] = ((xd[26]*a[0])+(xd[3]*a[12]));
out[21] = ((xd[27]*a[0])+(xd[3]*a[13]));
out[22] = ((xd[26]*a[1])+(xd[3]*a[14]));
out[23] = ((xd[27]*a[1])+(xd[3]*a[15]));
out[24] = (real_t)(0.0000000000000000e+00);
out[25] = (real_t)(1.0000000000000000e+00);
out[26] = (real_t)(1.0000000000000000e+00);
out[27] = (real_t)(0.0000000000000000e+00);
}

/* Fixed step size:0.1 */
int acado_integrate( real_t* const rk_eta, int resetIntegrator )
{
int error;

int run1;
rk_ttt = 0.0000000000000000e+00;
rk_eta[4] = 1.0000000000000000e+00;
rk_eta[5] = 0.0000000000000000e+00;
rk_eta[6] = 0.0000000000000000e+00;
rk_eta[7] = 0.0000000000000000e+00;
rk_eta[8] = 0.0000000000000000e+00;
rk_eta[9] = 1.0000000000000000e+00;
rk_eta[10] = 0.0000000000000000e+00;
rk_eta[11] = 0.0000000000000000e+00;
rk_eta[12] = 0.0000000000000000e+00;
rk_eta[13] = 0.0000000000000000e+00;
rk_eta[14] = 1.0000000000000000e+00;
rk_eta[15] = 0.0000000000000000e+00;
rk_eta[16] = 0.0000000000000000e+00;
rk_eta[17] = 0.0000000000000000e+00;
rk_eta[18] = 0.0000000000000000e+00;
rk_eta[19] = 1.0000000000000000e+00;
rk_eta[20] = 0.0000000000000000e+00;
rk_eta[21] = 0.0000000000000000e+00;
rk_eta[22] = 0.0000000000000000e+00;
rk_eta[23] = 0.0000000000000000e+00;
rk_eta[24] = 0.0000000000000000e+00;
rk_eta[25] = 0.0000000000000000e+00;
rk_eta[26] = 0.0000000000000000e+00;
rk_eta[27] = 0.0000000000000000e+00;
rk_xxx[28] = rk_eta[28];
rk_xxx[29] = rk_eta[29];
rk_xxx[30] = rk_eta[30];
rk_xxx[31] = rk_eta[31];
rk_xxx[32] = rk_eta[32];
rk_xxx[33] = rk_eta[33];
rk_xxx[34] = rk_eta[34];
rk_xxx[35] = rk_eta[35];
rk_xxx[36] = rk_eta[36];
rk_xxx[37] = rk_eta[37];
rk_xxx[38] = rk_eta[38];
rk_xxx[39] = rk_eta[39];
rk_xxx[40] = rk_eta[40];
rk_xxx[41] = rk_eta[41];
rk_xxx[42] = rk_eta[42];
rk_xxx[43] = rk_eta[43];
rk_xxx[44] = rk_eta[44];
rk_xxx[45] = rk_eta[45];
rk_xxx[46] = rk_eta[46];
rk_xxx[47] = rk_eta[47];
rk_xxx[48] = rk_eta[48];
rk_xxx[49] = rk_eta[49];
rk_xxx[50] = rk_eta[50];
rk_xxx[51] = rk_eta[51];
rk_xxx[52] = rk_eta[52];
rk_xxx[53] = rk_eta[53];
rk_xxx[54] = rk_eta[54];
rk_xxx[55] = rk_eta[55];
rk_xxx[56] = rk_eta[56];
rk_xxx[57] = rk_eta[57];
rk_xxx[58] = rk_eta[58];
rk_xxx[59] = rk_eta[59];
rk_xxx[60] = rk_eta[60];
rk_xxx[61] = rk_eta[61];
rk_xxx[62] = rk_eta[62];
rk_xxx[63] = rk_eta[63];
rk_xxx[64] = rk_eta[64];
rk_xxx[65] = rk_eta[65];
rk_xxx[66] = rk_eta[66];
rk_xxx[67] = rk_eta[67];
rk_xxx[68] = rk_eta[68];
rk_xxx[69] = rk_eta[69];
rk_xxx[70] = rk_eta[70];
rk_xxx[71] = rk_eta[71];
rk_xxx[72] = rk_eta[72];
rk_xxx[73] = rk_eta[73];
rk_xxx[74] = rk_eta[74];
rk_xxx[75] = rk_eta[75];
rk_xxx[76] = rk_eta[76];
rk_xxx[77] = rk_eta[77];

for (run1 = 0; run1 < 1; ++run1)
{
rk_xxx[0] = + rk_eta[0];
rk_xxx[1] = + rk_eta[1];
rk_xxx[2] = + rk_eta[2];
rk_xxx[3] = + rk_eta[3];
rk_xxx[4] = + rk_eta[4];
rk_xxx[5] = + rk_eta[5];
rk_xxx[6] = + rk_eta[6];
rk_xxx[7] = + rk_eta[7];
rk_xxx[8] = + rk_eta[8];
rk_xxx[9] = + rk_eta[9];
rk_xxx[10] = + rk_eta[10];
rk_xxx[11] = + rk_eta[11];
rk_xxx[12] = + rk_eta[12];
rk_xxx[13] = + rk_eta[13];
rk_xxx[14] = + rk_eta[14];
rk_xxx[15] = + rk_eta[15];
rk_xxx[16] = + rk_eta[16];
rk_xxx[17] = + rk_eta[17];
rk_xxx[18] = + rk_eta[18];
rk_xxx[19] = + rk_eta[19];
rk_xxx[20] = + rk_eta[20];
rk_xxx[21] = + rk_eta[21];
rk_xxx[22] = + rk_eta[22];
rk_xxx[23] = + rk_eta[23];
rk_xxx[24] = + rk_eta[24];
rk_xxx[25] = + rk_eta[25];
rk_xxx[26] = + rk_eta[26];
rk_xxx[27] = + rk_eta[27];
acado_rhs_forw( rk_xxx, rk_kkk );
rk_eta[0] += + (real_t)1.0000000000000001e-01*rk_kkk[0];
rk_eta[1] += + (real_t)1.0000000000000001e-01*rk_kkk[1];
rk_eta[2] += + (real_t)1.0000000000000001e-01*rk_kkk[2];
rk_eta[3] += + (real_t)1.0000000000000001e-01*rk_kkk[3];
rk_eta[4] += + (real_t)1.0000000000000001e-01*rk_kkk[4];
rk_eta[5] += + (real_t)1.0000000000000001e-01*rk_kkk[5];
rk_eta[6] += + (real_t)1.0000000000000001e-01*rk_kkk[6];
rk_eta[7] += + (real_t)1.0000000000000001e-01*rk_kkk[7];
rk_eta[8] += + (real_t)1.0000000000000001e-01*rk_kkk[8];
rk_eta[9] += + (real_t)1.0000000000000001e-01*rk_kkk[9];
rk_eta[10] += + (real_t)1.0000000000000001e-01*rk_kkk[10];
rk_eta[11] += + (real_t)1.0000000000000001e-01*rk_kkk[11];
rk_eta[12] += + (real_t)1.0000000000000001e-01*rk_kkk[12];
rk_eta[13] += + (real_t)1.0000000000000001e-01*rk_kkk[13];
rk_eta[14] += + (real_t)1.0000000000000001e-01*rk_kkk[14];
rk_eta[15] += + (real_t)1.0000000000000001e-01*rk_kkk[15];
rk_eta[16] += + (real_t)1.0000000000000001e-01*rk_kkk[16];
rk_eta[17] += + (real_t)1.0000000000000001e-01*rk_kkk[17];
rk_eta[18] += + (real_t)1.0000000000000001e-01*rk_kkk[18];
rk_eta[19] += + (real_t)1.0000000000000001e-01*rk_kkk[19];
rk_eta[20] += + (real_t)1.0000000000000001e-01*rk_kkk[20];
rk_eta[21] += + (real_t)1.0000000000000001e-01*rk_kkk[21];
rk_eta[22] += + (real_t)1.0000000000000001e-01*rk_kkk[22];
rk_eta[23] += + (real_t)1.0000000000000001e-01*rk_kkk[23];
rk_eta[24] += + (real_t)1.0000000000000001e-01*rk_kkk[24];
rk_eta[25] += + (real_t)1.0000000000000001e-01*rk_kkk[25];
rk_eta[26] += + (real_t)1.0000000000000001e-01*rk_kkk[26];
rk_eta[27] += + (real_t)1.0000000000000001e-01*rk_kkk[27];
rk_ttt += 1.0000000000000000e+00;
}
error = 0;
return error;
}

