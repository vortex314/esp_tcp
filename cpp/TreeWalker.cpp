/*
 * TreeWalker.cpp
 *
 *  Created on: Nov 9, 2015
 *      Author: lieven
 */
#include "Sys.h"
#include "MqttTree.h"
#include "Handler.h"
#include "Mqtt.h"
#include "Str.h"
#include "Bytes.h"

extern Mqtt* mqtt;

class TreeWalker: public Handler {
private:
	MqttTree* _pt;
	Str topic;
	Bytes value;
public:
	TreeWalker() :
			Handler("TreeWalker"), topic(40), value(200) {
		_pt = &MqttTree::root();
	}

	void next() {
		while (_pt) {
			_pt = _pt->walkNext();
		}
		if (_pt == 0)
			_pt = &MqttTree::root();
	}

	IROM bool dispatch(Msg& msg) {
		PT_BEGIN()
		;
		PT_WAIT_UNTIL(msg.is("123", SIG_INIT));
		while (true) {
			PT_YIELD_UNTIL(mqtt->isConnected());
			while (mqtt->isConnected()) {
				next();
				value.clear();
				if (_pt->hasData(value)) {
					topic.clear();
					_pt->getFullName(topic);
					mqtt->publish(topic, value,
					MQTT_QOS1_FLAG + MQTT_RETAIN_FLAG);
					PT_YIELD_UNTIL(mqtt->_mqttPublisher->isReady());
				}
			}

			PT_END()
		;
}
}
};

