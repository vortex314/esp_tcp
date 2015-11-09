/*
 * MqttMqttTree.h
 *
 *  Created on: Nov 8, 2015
 *      Author: lieven
 */

#ifndef MQTTTREE_H_
#define MQTTTREE_H_

//============================================================================
// Name        : MqttTreeTest.cpp
// Author      : Lieven Merckx
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "Sys.h"
#include "Erc.h"
#include "Bytes.h"
#include "Uart.h"
#include "Str.h"
#include "stdint.h"

typedef Erc (*Xdr)(void* instance, Bytes& bytes);

class MqttTree {
private:
	static MqttTree _root;
	MqttTree* _next;
	MqttTree* _parent;
	MqttTree* _firstChild;
	const char* _name;
	union {
		void* _instance;
		const char* _szValue;
		uint32_t* _ui32;
		struct {
			Xdr _putter;
			Xdr _getter;
		};
	};
	enum {
		T_INSTANCE, T_STRING, T_UINT32, T_PUTGET, T_NODE
	} _type;

public:
	static MqttTree& root() ;
	MqttTree(const char* name) ;
	const char *getName();
	MqttTree& add(const char* s);
	MqttTree& add(const char* s, void* pv) ;
	MqttTree& add(const char* s, Xdr putter, Xdr getter = 0);
	MqttTree& add(const char* name, const char* value);
	MqttTree* get(const char* s) ;
	static MqttTree* find(char* name) ;
	MqttTree& parent();
	MqttTree& firstChild();
	MqttTree& next();
	MqttTree* walkNext() ;
	Erc getter(Bytes& bytes);
	Erc putter(Bytes& bytes) ;
	bool hasData(Bytes& bytes);
	Erc getFullName(Str& str);
};

#endif /* MQTTTREE_H_ */
