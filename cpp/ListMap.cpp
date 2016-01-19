/*
 * ListMap.cpp
 *
 *  Created on: Jan 16, 2016
 *      Author: lieven
 */

#include <ListMap.h>


template<typename T> ListMap<T>::ListMap() {
	_first = 0;
	_cursor = 0;
}

template<typename T> Erc ListMap<T>::add(const char* key, T data) {
	Element* pEl = new Element;
	pEl->key = key;
	pEl->value = data;
	ListNode<Element*>* nw = new ListNode<Element*>(pEl);

	if (_first == 0)
		_first = nw;
	else {
		ListNode<Element*>* cursor = _first;
		while (cursor->next()) {
			cursor = cursor->next();
		}
		cursor->next(nw);
	}
	return 0;
}

template<typename T> T* ListMap<T>::first() {
	_cursor = _first;
	return &_cursor->_t->value;
}

template<typename T> T* ListMap<T>::next() {
	_cursor = _cursor->next();
	if (_cursor == 0)
		return 0;
	return &_cursor->_t->value;
}

#include "string.h"
template<typename T> T* ListMap<T>::findKey(const char* key) {
	ListNode<Element*>* cursor = _first;

	while (cursor) {
		if (strcmp(cursor->_t->key, key) == 0)
			return &cursor->_t->value;
		cursor = cursor->next();
	}
	return 0;
}

template<typename T> T* ListMap<T>::findMatch(const char* key) {
	ListNode<Element*>* cursor = _first;

	while (cursor) {
		if (strncmp(cursor->_t->key, key, strlen(cursor->_t->key)) == 0)
			return &cursor->_t->value;
		cursor = cursor->next();
	}
	return 0;
}

template<typename T> T* ListMap<T>::callOnMatch(const char* key,
		MatchFunction mf, void* instance) {
	ListNode<T>* cursor = _first;

	while (cursor) {
		if (strncmp(cursor->key(), key, strlen(cursor->key())) == 0)
			(*mf)(key, instance);
		cursor = cursor->next();
	}
	return 0;
}
