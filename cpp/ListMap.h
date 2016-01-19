/*
 * ListMap.h
 *
 *  Created on: Jan 16, 2016
 *      Author: lieven
 */

#ifndef LISTMAP_H_
#define LISTMAP_H_
#include "Erc.h"
#include "Sys.h"

template<typename T>
class ListNode {
	ListNode* _next;
public:
	T _t;

	IROM ListNode(T t) {
		_t = t;
		_next = 0;
	}
	IROM
	~ListNode();
	ListNode* IROM next() {
		return _next;
	}
	ListNode* IROM next(ListNode* nxt) {
		_next = nxt;
		return this;
	}

	Erc IROM add(ListNode* l);

};

typedef Erc (*MatchFunction)(const char* name, void* args);

template<typename T>
class ListMap {
	typedef struct {
		const char* key;
		T value;
	} Element;
	ListNode<Element*>* _first;
	ListNode<Element*>* _cursor;
public:
	IROM
	ListMap();
	T* IROM findKey(const char* key);
	T* IROM findMatch(const char* keyFilter);
	T* IROM callOnMatch(const char* keyFilter, MatchFunction mf,
			void *instance);
	const char* IROM getKey();
	Erc IROM setKey(const char* key);
	Erc IROM add(const char* key, T data);
	T* IROM first();
	T* IROM next();

};

#endif /* LISTMAP_H_ */
