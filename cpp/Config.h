/*
 * Config.h
 *
 *  Created on: Oct 1, 2015
 *      Author: lieven
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "Sys.h"
class Config {
public:
	virtual ~Config() {
	}

	virtual bool set(const char* key, const char*s)=0;
	virtual Erc get(char* value, int length, const char* key,
			const char* dflt)=0;
};

#endif /* CONFIG_H_ */
