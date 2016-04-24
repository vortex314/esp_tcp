/*
 * DWM1000_Anchor_Tag.h
 *
 *  Created on: Feb 12, 2016
 *      Author: lieven
 */

#ifndef DWM1000_Anchor_H_
#define DWM1000_Anchor_H_

#include <Handler.h>

class DWM1000_Anchor: public Handler {
	uint32_t _count;
public:
	DWM1000_Anchor();
	virtual ~DWM1000_Anchor();
	void mode(uint32_t m);
	void init();
	void resetChip();
	void initSpi();
	bool dispatch(Msg& msg);
};

#endif /* DWM1000_Anchor_Tag_H_ */
