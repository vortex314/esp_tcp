/*
 * DWM1000.h
 *
 *  Created on: Feb 12, 2016
 *      Author: lieven
 */

#ifndef DWM1000_H_
#define DWM1000_H_

#include <Handler.h>

class DWM1000: public Handler {
	uint32_t _count;
public:
	DWM1000();
	virtual ~DWM1000();
	void mode(uint32_t m);
	void init();
	bool dispatch(Msg& msg);
};

#endif /* DWM1000_H_ */
