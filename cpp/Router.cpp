/*
 * Router.cpp
 *
 *  Created on: Feb 3, 2016
 *      Author: lieven
 */

#include <Router.h>
#include <Wildcard.h>

Router::Router(const char* pattern, Subscriber* subscriber) {
	_next = 0;
	_subscriber = subscriber;
	_pattern = pattern;
	INFO(" new router %s",pattern);
	reg();
}

Router* Router::_first = 0;

Router::~Router() {
}

Router* Router::first() {
	return _first;
}

Router* Router::next() {
	return _next;
}

void Router::reg() {
	if (_first == 0) {
		_first = this;
	} else {
		Router* cursor = _first;
		while (cursor->_next != 0) {
			cursor = cursor->_next;
		}
		cursor->_next = this;
	}
}

void Router::publish(const char* dest,Cbor msg){
	Router* cursor = _first;
			while (cursor != 0) {
				INFO(" compare %s : %s ",dest,cursor->_pattern);
				if ( wildcardMatch(dest,cursor->_pattern,true,'\0')) {
					cursor->_subscriber->handle(msg);
					INFO(" match ");
				}
				cursor = cursor->_next;
			}
}



