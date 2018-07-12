
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: nrutil.h 2 2000-02-14 16:16:56Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 16:08:53  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:07:49  lucky
 *     Initial revision
 *
 *
 */


float *vector( int, int );
float **matrix( int, int, int, int );
float **convert_matrix( float *, int, int, int, int );
double *dvector( int, int );
double **dmatrix( int, int, int, int );
int *ivector( int, int );
int **imatrix( int, int, int, int );
float **submatrix( float **, int, int, int, int, int, int );
void free_vector( float *, int, int );
void free_dvector( double *, int, int );
void free_ivector( int *, int, int );
void free_matrix( float **, int, int, int, int );
void free_dmatrix( double **, int, int, int, int );
void free_imatrix( int **, int, int, int, int );
void free_submatrix( float **, int, int, int, int );
void free_convert_matrix( float **, int, int, int, int );
void nrerror( char [] );
