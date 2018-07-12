// digital filter functions

#include "PsnAdSend.h"

/* Filter the data in a long array */
void CFilter::FilterData( LONG *data, int count )
{
	double *cp, *m1, *m2, w, o;
	int i, j;
	
	for(i = 0; i < count ;i++)  { 
	    m1 = m_pM;
    	m2 = m_pM + m_NumQuads;
    	cp = m_pC;
    	o = (double)*data * *cp++;
	    for(j = 0; j < m_NumQuads ;j++)  { 
			w = o - *m1 * *cp++;
      		w -= *m2 * *cp++;
      		o = w + *m1 * *cp++;
      		o += *m2 * *cp++;
      		*m2++ = *m1;
      		*m1++ = w;
    	}
    	*data++ = (int)o;
	}
}

/* Called to set up the filter */
BOOL CFilter::Setup( char fType, double freq, int poles, int spsRate )
{
	double *cc;
	int i, j;
				
	CleanUp();
	
	m_Type = fType;
	m_Freq = freq;
	m_Poles = poles;
	m_SampRate = spsRate;
	m_Apass1 = -1.0;
			
	if(fType == 'L')  {
		m_Wpass1 = m_Freq;
		m_Wstop1 = m_Freq * 2;
	}
	else  {
		m_Wpass1 = m_Freq;
		m_Wstop1 = m_Freq / 2;
	}
	if(poles <= 0 || poles >= 33)  {
		m_FilterError = TRUE;
		return FALSE;
	}
	m_Astop1 = -((double)poles * 6.0);
	if( !TestData() )  {
		m_FilterError = TRUE;
		return FALSE;
	}
	m_Wpass1 = PI2 * m_Wpass1;		/*  Convert all edge frequencies to rad/sec */
	m_Wstop1 = PI2 * m_Wstop1;
	if( !CalcFilterCoefs() )  {
		m_FilterError = TRUE;
		return FALSE;
	}
	m_NumQuads = (m_Order + 1) / 2;
 	if( !(m_pC = (double *)malloc( (m_NumQuads * 4 + 1) * sizeof(double)) ) )  {
		m_FilterError = TRUE;
		return FALSE;
	}
    if( !(m_pM = (double *)malloc( m_NumQuads * 2 * sizeof(double)) ) )  {
		m_FilterError = TRUE;
		return FALSE;
	}
   	cc = m_pC;
   	*cc++ = m_Gain;
   	for(i = 0; i < m_NumQuads; i++)  { 
		j = i * 3;
   		*cc++ = m_pBcoefs[j+1];
   		*cc++ = m_pBcoefs[j+2];
   		*cc++ = m_pAcoefs[j+1];
   		*cc++ = m_pAcoefs[j+2];
  	}
	memset( m_pM, 0, m_NumQuads * 2 * sizeof(double) );
	return TRUE;
}

void CFilter::Reset()
{
	int i, j;
	double *cc = m_pC;
	
   	*cc++ = m_Gain;
   	for(i = 0; i < m_NumQuads; i++)  { 
		j = i * 3;
   		*cc++ = m_pBcoefs[j+1];
   		*cc++ = m_pBcoefs[j+2];
   		*cc++ = m_pAcoefs[j+1];
   		*cc++ = m_pAcoefs[j+2];
  	}
	memset( m_pM, 0, m_NumQuads * 2 * sizeof(double) );
}

BOOL CFilter::Check( char fType, double freq, int poles, int spsRate )
{
	CleanUp();
	
	m_Type = fType;
	m_Freq = freq;
	m_SampRate = spsRate;
	m_Apass1 = -1.0;
		
	if(fType == 'L')  {
		m_Wpass1 = m_Freq;
		m_Wstop1 = m_Freq * 2;
	}
	else  {
		m_Wpass1 = m_Freq;
		m_Wstop1 = m_Freq / 2;
	}
	if(poles <= 0 || poles >= 33)  {
		m_FilterError = TRUE;
		return FALSE;
	}
	m_Astop1 = -((double)poles * 6.0);
	if( !TestData() )  {
		m_FilterError = TRUE;
		return FALSE;
	}
	m_Wpass1 = PI2 * m_Wpass1;		/*  Convert all edge frequencies to rad/sec */
	m_Wstop1 = PI2 * m_Wstop1;
	if( !CalcFilterCoefs() )  {
		m_FilterError = TRUE;
		return FALSE;
	}
	return TRUE;
}

BOOL CFilter::TestData()
{
	double freqMax = m_SampRate / 2;
		
	if(m_Type == 'L')  {
		if( !TestDouble( m_Apass1, GAIN_TRAN, GAIN_PASS) )
			return FALSE;
		if( !TestDouble( m_Astop1, GAIN_STOP, GAIN_TRAN) )
			return FALSE;
		if( !TestDouble( m_Wpass1, FREQ_MIN, freqMax) )
			return FALSE;
		if( !TestDouble( m_Wstop1, m_Wpass1+IOTA, freqMax) )
			return FALSE;
	}
	else  {
		if(!TestDouble( m_Apass1, GAIN_TRAN, GAIN_PASS) )
			return FALSE;
		if(!TestDouble( m_Astop1, GAIN_STOP, GAIN_TRAN) )
			return FALSE;
		if(!TestDouble( m_Wpass1, FREQ_MIN, freqMax) )
			return FALSE;
		if(!TestDouble( m_Wstop1, FREQ_MIN, m_Wpass1-IOTA) )
			return FALSE;
	}
	return TRUE;
}

BOOL CFilter::CalcFilterCoefs()
{ 
	if( !WarpFreqs() )
		return FALSE;;
	if( !CalcFilterOrder() )
		return FALSE;;
	if( !CalcNormalCoefs() )
		return FALSE;;
	if( !UnNormalizeCoefs() )
		return FALSE;;
	if(!BilinearTransform())
		return FALSE;;
	if(!UnWarpFreqs())
		return FALSE;;
	return TRUE;
}

void CFilter::CleanUp()
{

	m_Apass1 = m_Apass2 = m_Astop1 = m_Astop2 = m_Wpass1 = m_Wpass2 = m_Wstop1 = m_Wstop2 = 0.0;
	m_SampRate = m_Gain = m_Freq = 0.0;
	
	m_Order = m_NumQuads = m_FilterError = 0;
	m_Type = 0;

	if(m_pAcoefs)  {
		free(m_pAcoefs);
		m_pAcoefs = 0;
	}
    if(m_pBcoefs)  {
		free(m_pBcoefs);
		m_pBcoefs = 0;
	}
	if(m_pC)  {
		free(m_pC);
		m_pC = 0;
	}
	if(m_pM)  {
		free(m_pM);
		m_pM = 0;
	}
}

CFilter::CFilter()
{
	m_pAcoefs = m_pBcoefs = m_pC = m_pM = NULL;
	CleanUp();	
}

CFilter::~CFilter()
{
	CleanUp();
}

BOOL CFilter::WarpFreqs()
{
	switch(m_Type)  { 
		case 'L':
    	case 'H':
      		m_Wpass1 = 2 * m_SampRate * tan(m_Wpass1/(2 * m_SampRate));
			m_Wstop1 = 2 * m_SampRate * tan(m_Wstop1/(2 * m_SampRate));
      		break;
    	case 'P':
    	case 'S':
      		m_Wpass1 = 2 * m_SampRate * tan(m_Wpass1/(2 * m_SampRate));
      		m_Wpass2 = 2 * m_SampRate * tan(m_Wpass2/(2 * m_SampRate));
			m_Wstop1 = 2 * m_SampRate * tan(m_Wstop1/(2 * m_SampRate));
			m_Wstop2 = 2 * m_SampRate * tan(m_Wstop2/(2 * m_SampRate));
      		break;
    	default:
			return FALSE;;
	}
	return TRUE;
}

int CFilter::UnWarpFreqs()
{
	switch(m_Type)  { 
		case 'L':
		case 'H':
      		m_Wpass1 = 2 * m_SampRate * atan(m_Wpass1/(2 * m_SampRate));
      		m_Wstop1 = 2 * m_SampRate * atan(m_Wstop1/(2 * m_SampRate));
		    break;
		case 'P':
 		case 'S':
      		m_Wpass1 = 2 * m_SampRate * atan(m_Wpass1/(2 * m_SampRate));
			m_Wpass2 = 2 * m_SampRate * atan(m_Wpass2/(2 * m_SampRate));
			m_Wstop1 = 2 * m_SampRate * atan(m_Wstop1/(2 * m_SampRate));
			m_Wstop2 = 2 * m_SampRate * atan(m_Wstop2/(2 * m_SampRate));
		    break;
		default:
			return FALSE;;
	}
	return TRUE;
}

int CFilter::CalcFilterOrder()
{ 
	double kernel, ratio, n, wp1, wp2, ws1, ws2;

	wp1 = m_Wpass1;  wp2 = m_Wpass2;
	ws1 = m_Wstop1;  ws2 = m_Wstop2;
	switch (m_Type)  { 
		case 'L':
      		ratio = ws1 / wp1;
      		break;
    	case 'H':
      		ratio = wp1 / ws1;
      		break;
		case 'P':
      		if(ws1 > (wp1 * wp2) / ws2)  {
      			ws2 = (wp1 * wp2) / ws1;
        		m_Wstop2 = ws2;
      		}
      		else  { 
				ws1 = (wp1 * wp2) / ws2;
				m_Wstop1 = ws1;
			}
      		ratio = (ws2 - ws1) / (wp2 - wp1);
      		break;
    	case 'S':
			if(wp1 > (ws1 * ws2) / wp2)  {
				wp2 = (ws1 * ws2) / wp1;
        		m_Wpass2 = wp2;
      		}
      		else  {
      			wp1 = (ws1 * ws2) / wp2;
        		m_Wpass1 = wp1;
      		}
      		ratio = (wp2 - wp1) / (ws2 - ws1);
      		break;
		default:	
			return FALSE;;
  	}
	kernel = ((pow(10.0,-0.1*m_Astop1) - 1) / (pow(10.0,-0.1*m_Apass1) - 1));
	
	// for butterworth
	n = log10(kernel) / (2 * log10(ratio));
	if(n > 200)
		return FALSE;;
	m_Order = (int)ceil(n);
	return TRUE;
}

int CFilter::CalcNormalCoefs()
{ 
	int numCoefs;

	numCoefs = 3 * ((m_Order + 1) / 2);
	if(!(m_pAcoefs = (double *)malloc(numCoefs * sizeof(double))))
		return FALSE;;
	if(!(m_pBcoefs = (double *)malloc(numCoefs * sizeof(double))))
		return FALSE;;
	if( !CalcButterCoefs() )
		return FALSE;;
	return TRUE;
}

int CFilter::CalcButterCoefs()
{ 
	double R, epsilon, theta, sigma, omega;
	int m, a, b;

	if(!m_pAcoefs)
		return FALSE;;
	if(!m_pBcoefs)
		return FALSE;;
	if(m_Order <= 0)
  		return FALSE;;
	epsilon = sqrt( pow(10.0,-0.1*m_Apass1) - 1.0 );
	R = pow(epsilon,-1.0/m_Order);
	m_Gain = 1.0;
	a = 0; b = 0;
	if(m_Order % 2)  { 
		m_pAcoefs[a++] = 0.0;
    	m_pAcoefs[a++] = 0.0;
    	m_pAcoefs[a++] = R;
    	m_pBcoefs[b++] = 0.0;
    	m_pBcoefs[b++] = 1.0;
    	m_pBcoefs[b++] = R;
  	}
	for(m = 0;m < m_Order/2;m++)  {
		theta = PI*(2*m + m_Order +1) / (2 * m_Order);
    	sigma = R * cos(theta);
    	omega = R * sin(theta);
		m_pAcoefs[a++] = 0.0;
    	m_pAcoefs[a++] = 0.0;
    	m_pAcoefs[a++] = sigma*sigma+omega*omega;
    	m_pBcoefs[b++] = 1.0;
    	m_pBcoefs[b++] = -2 * sigma;
    	m_pBcoefs[b++] = sigma*sigma+omega*omega;
  	}
	return TRUE;
}

int CFilter::UnNormalizeCoefs()
{ 
	double freq, BW, Wo;
	int error;

	freq = m_Wpass1;
	Wo = sqrt(m_Wpass1 * m_Wpass2);
	BW = m_Wpass2 - m_Wpass1;
	switch(m_Type)  { 
		case 'L':
      		error = UnNormLPCoefs( freq );
      		break;
		case 'H':
			error = UnNormHPCoefs( freq );
			break;
    	case 'P':
      		error = UnNormBPCoefs( BW, Wo );
			break;
    	case 'S':
      		error = UnNormBSCoefs( BW, Wo);
			break;
    	default:
      		return FALSE;;
	}
	if(error)
		return FALSE;;
	return TRUE;
}

int CFilter::UnNormBPCoefs( double BW, double Wo )
{ 
	int qd, ocf, ncf, qd_start, numb_coefs, org_quads, org_order;
	double *org_num, *org_den, *new_num, *new_den;
	complex A,B,C,D,E;          /* temp cmplx vars    */

	org_order = m_Order;
	org_quads = (org_order + 1)/2;
	m_Order = org_order * 2;
	org_num = m_pAcoefs;
	org_den = m_pBcoefs;
	numb_coefs = 3 * org_order;
	if(!(new_num = (double *)malloc(numb_coefs*sizeof(double))))
		return TRUE;
	if(!(new_den = (double *)malloc(numb_coefs*sizeof(double))))
		return TRUE;
	if(org_order % 2)  {
		new_num[0] = org_num[1];
    	new_num[1] = BW * org_num[2];
    	new_num[2] = org_num[1] * Wo * Wo;
    	new_den[0] = org_den[1];
    	new_den[1] = BW * org_den[2];
    	new_den[2] = org_den[1] * Wo * Wo;
    	qd_start = 1;
  	}
  	else
		qd_start = 0;
	for(qd = qd_start;qd < org_quads;qd++)  {
		ocf = qd * 3;
		ncf = qd * 6 - qd_start * 3;
		if(org_num[ocf] == 0.0)  { 
			new_num[ncf] = 0.0;
      		new_num[ncf+1] = sqrt(org_num[ocf+2]) * BW;
      		new_num[ncf+2] = 0.0;
      		new_num[ncf+3] = 0.0;
      		new_num[ncf+4] = sqrt(org_num[ocf+2]) * BW;
      		new_num[ncf+5] = 0.0;
		}
    	else  {
      		A = cmplx(org_num[ocf],0);
      		B = cmplx(org_num[ocf+1],0);
      		C = cmplx(org_num[ocf+2],0);
		    cQuadratic(A,B,C,&D,&E);
      		A = cmplx(1,0);
      		B = cmul(cneg(D),cmplx(BW,0));
      		C = cmplx(Wo*Wo,0);
      		cQuadratic(A,B,C,&D,&E);
      		new_num[ncf] = 1.0;
      		new_num[ncf+1] = -2.0 * creal(D);
      		new_num[ncf+2] = creal(cmul(D,cconj(D)));
      		new_num[ncf+3] = 1.0;
      		new_num[ncf+4] = -2.0 * creal(E);
			new_num[ncf+5] = creal(cmul(E,cconj(E)));
	    }
    	A = cmplx(org_den[ocf],0);
    	B = cmplx(org_den[ocf+1],0);
    	C = cmplx(org_den[ocf+2],0);
    	cQuadratic(A,B,C,&D,&E);
    	A = cmplx(1,0);
    	B = cmul(cneg(D),cmplx(BW,0));
    	C = cmplx(Wo*Wo,0);
		cQuadratic(A,B,C,&D,&E);
    	new_den[ncf] = 1.0;
    	new_den[ncf+1] = -2.0 * creal(D);
    	new_den[ncf+2] = creal(cmul(D,cconj(D)));
    	new_den[ncf+3] = 1.0;
    	new_den[ncf+4] = -2.0 * creal(E);
    	new_den[ncf+5] = creal(cmul(E,cconj(E)));
	}
	free(m_pAcoefs);
	free(m_pBcoefs);
	m_pAcoefs = new_num;
	m_pBcoefs = new_den;
	return FALSE;;
}

int CFilter::UnNormBSCoefs( double BW,double Wo )
{ 
	int qd,ocf,ncf,qd_start, numb_coefs, org_quads, org_order;
	double *org_num, *org_den, *new_num, *new_den;
	complex A,B,C,D,E;

	org_order = m_Order;
	org_quads = (org_order + 1)/2;
	m_Order = org_order * 2;
	org_num = m_pAcoefs;
	org_den = m_pBcoefs;
	numb_coefs = 3 * org_order;
	if(!(new_num = (double *)malloc(numb_coefs*sizeof(double))))
		return TRUE;
	if(!(new_den = (double *)malloc(numb_coefs*sizeof(double))))
		return TRUE;
	if(org_order % 2)  { 
		m_Gain *= (org_num[2] / org_den[2]);
		new_num[0] = 1.0;
    	new_num[1] = BW * org_num[1] / org_num[2];
    	new_num[2] = Wo * Wo;
    	new_den[0] = 1.0;
    	new_den[1] = BW * org_den[1] / org_den[2];
    	new_den[2] = Wo * Wo;
    	qd_start = 1;
  	}
  	else
		qd_start = 0;
	for(qd = qd_start;qd < org_quads;qd++)  { 
    	ocf = qd * 3;
    	ncf = qd * 6 - qd_start * 3;
		m_Gain *= (org_num[ocf+2] / org_den[ocf+2]);
		if(org_num[ocf] == 0)  { 
			new_num[ncf] = 1.0;
      		new_num[ncf+1] = 0.0;
      		new_num[ncf+2] = Wo * Wo;
      		new_num[ncf+3] = 1.0;
      		new_num[ncf+4] = 0.0;
      		new_num[ncf+5] = Wo * Wo;
    	}
		else  {
			A = cmplx(org_num[ocf],0);
      		B = cmplx(org_num[ocf+1],0);
      		C = cmplx(org_num[ocf+2],0);
      		cQuadratic(A,B,C,&D,&E);
      		A = cmplx(1,0);
      		B = cmul(cneg(cdiv(cmplx(1,0),D)),cmplx(BW,0));
      		C = cmplx(Wo*Wo,0);
      		cQuadratic(A,B,C,&D,&E);
			new_num[ncf] = 1.0;
      		new_num[ncf+1] = -2.0 * creal(D);
      		new_num[ncf+2] = creal(cmul(D,cconj(D)));
      		new_num[ncf+3] = 1.0;
      		new_num[ncf+4] = -2.0 * creal(E);
      		new_num[ncf+5] = creal(cmul(E,cconj(E)));
    	}
		A = cmplx(org_den[ocf],0);
    	B = cmplx(org_den[ocf+1],0);
    	C = cmplx(org_den[ocf+2],0);
    	cQuadratic(A,B,C,&D,&E);
    	A = cmplx(1,0);
    	B = cmul(cneg(cdiv(cmplx(1,0),D)),cmplx(BW,0));
    	C = cmplx(Wo*Wo,0);
    	cQuadratic(A,B,C,&D,&E);
		new_den[ncf] = 1.0;
    	new_den[ncf+1] = -2.0 * creal(D);
    	new_den[ncf+2] = creal(cmul(D,cconj(D)));
    	new_den[ncf+3] = 1.0;
    	new_den[ncf+4] = -2.0 * creal(E);
    	new_den[ncf+5] = creal(cmul(E,cconj(E)));
  	}
	free(m_pAcoefs);
	free(m_pBcoefs);
	m_pAcoefs = new_num;
	m_pBcoefs = new_den;
	return FALSE;
}

int CFilter::UnNormHPCoefs( double freq )
{ 
	int qd, cf, qd_start;
	if(m_Order % 2)  { 
		m_Gain *= (m_pAcoefs[2] / m_pBcoefs[2]);
    	m_pAcoefs[2] = freq*m_pAcoefs[1]/m_pAcoefs[2];
    	m_pAcoefs[1] = 1.0;
    	m_pBcoefs[2] = freq / m_pBcoefs[2];
    	qd_start = 1;
  	}
  	else
  		qd_start = 0;

	for(qd = qd_start; qd < (m_Order + 1)/2; qd++)  { 
		cf = qd * 3;
    	m_Gain *= (m_pAcoefs[cf+2] / m_pBcoefs[cf+2]);
    	m_pAcoefs[cf+1] *= (freq / m_pAcoefs[cf+2]);
    	m_pAcoefs[cf+2] = freq * freq * m_pAcoefs[cf] / m_pAcoefs[cf+2];
		m_pAcoefs[cf] = 1.0;
    	m_pBcoefs[cf+1] *= (freq / m_pBcoefs[cf+2]);
    	m_pBcoefs[cf+2] = freq * freq * m_pBcoefs[cf] / m_pBcoefs[cf+2];
		m_pBcoefs[cf] = 1.0;
  	}
  	return FALSE;;
}

int CFilter::UnNormLPCoefs( double freq )
{ 
	int qd,cf, qd_start;
	if(m_Order % 2)  {
		m_pAcoefs[2] *= freq;
    	m_pBcoefs[2] *= freq;
    	qd_start = 1;
  	}
  	else
		qd_start = 0;
	for(qd = qd_start; qd < (m_Order + 1)/2; qd++)  { 
		cf = qd * 3;
		m_pAcoefs[cf+1] *= freq;
    	m_pAcoefs[cf+2] *= (freq * freq);
    	m_pBcoefs[cf+1] *= freq;
    	m_pBcoefs[cf+2] *= (freq * freq);
  	}
	return FALSE;;
}

int CFilter::BilinearTransform()
{ 
	int i, j, start, num_quads;
	double f2, f4, N0, N1, N2, D0, D1, D2;
	if((!m_pAcoefs) || (!m_pBcoefs))
		return FALSE;;
		
	f2 = 2 * m_SampRate;
  	f4 = f2 * f2;
  	num_quads = (m_Order + 1)/2;
	start = 0;
  	if(m_Order % 2)  {
		N0 = m_pAcoefs[2] + m_pAcoefs[1] * f2;
    	N1 = m_pAcoefs[2] - m_pAcoefs[1] * f2;
    	D0 = m_pBcoefs[2] + m_pBcoefs[1] * f2;
    	D1 = m_pBcoefs[2] - m_pBcoefs[1] * f2;
    	m_pAcoefs[0] = 1.0;
    	m_pAcoefs[1] = N1 / N0;
    	m_pAcoefs[2] = 0.0;
    	m_pBcoefs[0] = 1.0;
    	m_pBcoefs[1] = D1 / D0;
    	m_pBcoefs[2] = 0.0;
    	m_Gain *= (N0 / D0);
    	start = 1;
  	}
	for(i = start; i < num_quads ;i++)  { 
		j = 3 * i;
    	N0 = m_pAcoefs[j]*f4 + m_pAcoefs[j+1]*f2 + m_pAcoefs[j+2];
    	N1 = 2 * (m_pAcoefs[j+2] - m_pAcoefs[j]*f4);
    	N2 = m_pAcoefs[j]*f4 - m_pAcoefs[j+1]*f2 + m_pAcoefs[j+2];
    	D0 = m_pBcoefs[j]*f4 + m_pBcoefs[j+1]*f2 + m_pBcoefs[j+2];
    	D1 = 2 * (m_pBcoefs[j+2] - m_pBcoefs[j]*f4);
    	D2 = m_pBcoefs[j]*f4 - m_pBcoefs[j+1]*f2 + m_pBcoefs[j+2];
    	m_pAcoefs[j] = 1.0;
    	m_pAcoefs[j+1] = N1 / N0;
    	m_pAcoefs[j+2] = N2 / N0;
    	m_pBcoefs[j] = 1.0;
    	m_pBcoefs[j+1] = D1 / D0;
    	m_pBcoefs[j+2] = D2 / D0;
    	m_Gain *= (N0 / D0);
  	}
	return TRUE;
}

BOOL CFilter::TestDouble(double dvalue, double min, double max)
{ 
	if((dvalue >= min) && (dvalue <= max))
		return TRUE;
	return FALSE;;
}

complex CFilter::cadd(complex a,complex b)
{ 
	complex x;
	x.re = a.re + b.re;
	x.im = a.im + b.im;
	return x;
}

double CFilter::cang(complex a)
{ 
	return atan2(a.im,a.re);
}

complex CFilter::cconj(complex a)
{ 
	complex x;
	x.re = a.re;
	x.im = -a.im;
	return x;
}

complex CFilter::cdiv(complex a,complex b)
{ 
	complex x;
	double m;
	m = b.re*b.re + b.im*b.im;
  	x.re = (a.re*b.re + a.im*b.im)/m;
  	x.im = (a.im*b.re - a.re*b.im)/m;
  	return x;
}

double CFilter::cimag(complex a)
{ 
	return a.im;
}

double CFilter::cmag(complex a)
{ 
	return sqrt(a.re*a.re + a.im*a.im);
}

complex CFilter::cmplx(double re,double im)
{ 
	complex x;
  	x.re = re;
  	x.im = im;
  	return x;
}

complex CFilter::cmul(complex a,complex b)
{ 
	complex x;
	x.re = a.re*b.re - a.im*b.im;
  	x.im = a.re*b.im + a.im*b.re;
  	return x;
}

complex CFilter::cneg(complex a)
{ 
	complex x;
  	x.re = -a.re;
  	x.im = -a.im;
  	return x;
}

void CFilter::cQuadratic(complex a,complex b,complex c, complex *d,complex *e)
{ 
	complex a2,ac4,sq;  /*  intermediate values */
  	
	a2 = cmul(a,cmplx(2,0));          /*  2*a         */
  	ac4 = cmul(cmul(a,c),cmplx(4,0)); /*  4*a*c       */
  	sq = csqr(csub(cmul(b,b),ac4)); /* sqrt(b*b-4*a*c)*/
  	*d = cdiv(cadd(cneg(b),sq),a2);   /*  first root  */
  	*e = cdiv(csub(cneg(b),sq),a2);   /*  second root */
}

double CFilter::creal(complex a)
{   
	return a.re;
}

complex CFilter::csqr(complex a)
{ 
	complex x;
  	x.re = sqrt(cmag(a)) * cos(cang(a)/2.0);
  	x.im = sqrt(cmag(a)) * sin(cang(a)/2.0);
  	return x;
}

complex CFilter::csub(complex a,complex b)
{ 
	complex x;
  	x.re = a.re - b.re;
  	x.im = a.im - b.im;
  	return x;
}

/*  CPeriodExtend code below.... */

void CPeriodExtend::FilterData( LONG *data, int samples )
{
	while( samples-- )
		Filter( data++ );
}

void CPeriodExtend::Filter( LONG *sample )
{
	double d, sampleBlock, sampleBare = *sample;			// samplebare provides unfiltered feedthrough
	int n;

	// Remove bias with long time-constant filter
	// sampleblock provides  bias-filtered feedthrough
	m_biasRegister0 = m_biasRegister0 + sampleBare / m_tc0;
	sampleBlock = sampleBare - m_biasRegister0;
	m_biasRegister0 = m_biasRegister0 - m_biasRegister0 / m_tc0;
	for( n = 0; n != m_nRep; n++ )  {
   		d = sampleBlock + m_sum1 * m_b1 + m_oldsum2 * m_b2;
		if( d < 0.0 )  {
			*sample = (int)( d - 0.5 );
			if( *sample < m_minData )
				*sample = m_minData;
		}
		else  {
			*sample = (int)( d + 0.5 );
			if( *sample > m_maxData )
				*sample = m_maxData;
		}
		m_sum1 = sampleBlock + m_sum1 - m_sum1 * m_sigmaF - m_oldsum2 * m_omega2F;
		m_oldsum2 = m_sum2;
   		m_sum2 = m_sum2 + m_sum1;
	}
}

void CPeriodExtend::Reset()
{
	m_sum1 = m_sum2 = m_oldsum2 = m_biasRegister0 = 0.0;
}

void CPeriodExtend::Setup( double pendulumFreq, double pendulumQ, double hpFreq, double hpQ, double sampleRate )
{
	m_pendulumFreq = pendulumFreq;
	m_pendulumQ = pendulumQ;
	m_hpFilterFreq = hpFreq;
	m_hpFilterQ = hpQ;
	m_spsRate = sampleRate;
	
	// 24-Bits max
	m_maxData = 16777215;
	m_minData = -16777216;
	
	m_pp = 1.0 / m_pendulumFreq;		// Pendulum Period
	m_qp = m_pendulumQ;				// Pendulum Q
	m_pf = 1.0 / m_hpFilterFreq;		// Long filter period
	m_tc0 = m_pf * sampleRate; 		// DC blocking time constant=long filter period
	// Tc1 is number of iterations in one pendulum period. The filter should have 
	// at least 50, so we adjust the number of iterations in the filter loop to achieve this.
	m_tc1 = m_pp * sampleRate;		// Pendulum period in samples
	m_nRep = ( (int)( 50 / m_tc1 ) ) + 1; 
	m_qf = m_hpFilterQ;

	m_omegaF = 2 * PI / (m_nRep * m_pf * sampleRate); 	// 2*Pi*Fp/Fs
	m_omega2F = m_omegaF * m_omegaF;
	m_sigmaF = m_omegaF / m_qf;
	m_omegaP = 2.0 * PI / (m_nRep * m_pp * sampleRate);
	m_b2 = m_omegaP * m_omegaP - m_omega2F;
	m_b1 = m_omegaP / m_qp - m_sigmaF;
	
	Reset();
}

void CPeriodExtend::GetParams( double *pendulumFreq, double *pendulumQ, double *hpFreq, double *hpQ )
{
	*pendulumFreq = m_pendulumFreq;
	*pendulumQ = m_pendulumQ;
	*hpFreq = m_hpFilterFreq;
	*hpQ = m_hpFilterQ;
}
