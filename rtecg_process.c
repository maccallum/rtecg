#include <math.h>
#include "rtecg.h"
#include "rtecg_process.h"

rtecg_float rtecg_pt_preprocess(rtecg_float *buf, rtecg_ctr bufpos_r, rtecg_ctr buflen, int mwilen)
{
	int n = 5 + mwilen - 2;
	if(bufpos_r < n){
		return 0;
	}
	rtecg_float sum = 0;
	for(int i = (bufpos_r - n) + 4; i <= bufpos_r; i++){
		// this is from the paper:
		//sum += pow(0.125 * (-1 * buf[(i - 4) % buflen] - 2 * buf[(i - 3) % buflen] + 2 * buf[(i - 1) % buflen] + buf[i % buflen]), 2.);
	
		// this is from the errata sheet:
		sum += pow(0.1 * (-2 * buf[(i - 4) % buflen] - buf[(i - 3) % buflen] + buf[(i - 1) % buflen] + 2. * buf[i % buflen]), 2.);
	}
	return sum / mwilen;
}
