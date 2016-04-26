/*
 * DWM1000_Tag.h
 *
 *  Created on: Feb 12, 2016
 *      Author: lieven
 */

#ifndef DWM1000_Tag_H_
#define DWM1000_Tag_H_

#include <Handler.h>

class DWM1000_Tag: public Handler {
	uint32_t _count;
public:
	DWM1000_Tag();
	virtual ~DWM1000_Tag();
	void mode(uint32_t m);
	void init();
	void resetChip();
	void initSpi();
	static bool interrupt_detected;
	static void my_dwt_isr();
	bool isInterruptDetected();
	bool clearInterrupt();
	void enableIsr();
	bool dispatch(Msg& msg);
};

#endif /* DWM1000_Tag_H_ */
