#ifndef _p3_cmn_h_
#define _p3_cmn_h_

typedef struct {
	char	working_hour[16];
	char	extra_hour[16];
}P3_TIM_CAL;

extern void p3_cal_working_tim(const char* const, const void* const, P3_TIM_CAL*);

#endif
