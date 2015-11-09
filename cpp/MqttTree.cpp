/*
 * MqttMqttTree.cpp
 *
 *  Created on: Nov 8, 2015
 *      Author: lieven
 */

#include <MqttTree.h>

MqttTree MqttTree::_root("");

MqttTree& MqttTree::root() {
	return _root;
}
MqttTree::MqttTree(const char* name) {
	_next = 0;
	_parent = 0;
	_firstChild = 0;
	_name = name;
	_putter = 0;
	_getter = 0;
	_instance = 0;
	_type = T_NODE;
}

const char *MqttTree::getName() {
	return _name;
}

MqttTree& MqttTree::add(const char* s) {
	MqttTree* pt = new MqttTree(s);
	pt->_parent = this;
	if (_firstChild == 0) {
		_firstChild = pt;
	} else {
		MqttTree* cursor = _firstChild;
		while (cursor->_next)
			cursor = cursor->_next;
		cursor->_next = pt;
	}
	return *pt;
}
MqttTree& MqttTree::add(const char* s, void* pv) {
	MqttTree& t = add(s);
	t._instance = pv;
	t._type = T_INSTANCE;
	return t;
}

MqttTree& MqttTree::add(const char* s, Xdr putter, Xdr getter) {
	MqttTree& t = add(s);
	t._putter = putter;
	t._getter = getter;
	t._type = T_PUTGET;
	return t;
}
MqttTree& MqttTree::add(const char* name, const char* value) {
	MqttTree& t = add(name);
	t._szValue = value;
	t._type = T_STRING;
	return t;
}
MqttTree* MqttTree::get(const char* s) {
	MqttTree* cursor = _firstChild;
	while (cursor != 0) {
		if (strcmp(cursor->_name, s) == 0)
			return cursor;
		cursor = cursor->_next;
	}
	return 0;
}
MqttTree* MqttTree::find(char* name) {
	char* stringp = name;
	char *ps;
	MqttTree* tree = &MqttTree::root();
	while (stringp) {
		ps = strsep(&stringp, "/");
		tree = tree->get(ps);
		if (tree == 0)
			break;
		if (stringp == 0) {
			return tree;
		}
	}
	return 0;
}

MqttTree* MqttTree::walkNext() {
	if (_firstChild)
		return _firstChild;
	if (_next)
		return _next;
	if (_parent) {
		return _parent->_next;
	}
	return 0;
}

Erc MqttTree::getter(Bytes& bytes) {
	if (_getter)
		return _getter(_parent->_instance, bytes);
	else
		return ENOENT;
}

Erc MqttTree::putter(Bytes& bytes) {
	if (_putter)
		return _putter(_parent->_instance, bytes);
	else
		return ENOENT;
}

bool MqttTree::hasData(Bytes& bytes) {
	if (_type == T_PUTGET && _getter != 0) {
		if (getter(bytes) == E_OK)
			return true;
	}
	return false;
}

Erc MqttTree::getFullName(Str& str) {
	if (_parent) {
		_parent->getFullName(str);
		if ( _parent->_parent) str << "/";
		str << _name;
	}
	return E_OK;
}

