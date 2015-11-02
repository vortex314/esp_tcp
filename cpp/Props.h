/*
 * Props.h
 *
 *  Created on: Nov 2, 2015
 *      Author: lieven
 */

#ifndef PROPS_H_
#define PROPS_H_

#include <Handler.h>
#include "Mqtt.h"

class Props: public Handler {
private:
	Mqtt* _mqtt;
public:
	Props(Mqtt* mqtt);
	virtual ~Props();
	bool dispatch(Msg& msg);
};

#endif /* PROPS_H_ */
