/*
 * Router.h
 *
 *  Created on: Feb 3, 2016
 *      Author: lieven
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include <Cbor.h>

class Subscriber {
public:
	virtual Erc handle(Cbor& msg)=0;
};

class Router {
	static Router* _first;
	Router* _next;
	Subscriber*  _subscriber;
	const char* _pattern;
public:
	Router(const char* pattern,Subscriber* subscriber);
	virtual ~Router();
//	void subscribe(const char* pattern,Subscriber* subscriber);
	static void publish(const char* dest,Cbor msg);
	static Router* first();
	Router*next();
	void reg();

};

#endif /* ROUTER_H_ */
