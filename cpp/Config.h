/*
 * Config.h
 *
 *  Created on: Oct 1, 2015
 *      Author: lieven
 */

#ifndef CONFIG_H_
#define CONFIG_H_


class Config {
public:
	virtual ~Config(){};
	IROM virtual bool set(const char* key, const char*s)=0;
	IROM virtual bool set(const char* key, int value)=0;
	IROM virtual void get(int& value, const char* key, int dflt)=0;
	IROM virtual void get(char* value, int length, const char* key,
			const char* dflt)=0;
};

#endif /* CONFIG_H_ */
