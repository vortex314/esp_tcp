/*
 * gpio.h
 *
 *  Created on: Nov 17, 2015
 *      Author: lieven
 */

#ifndef INCLUDE_GPIO_C_H_
#define INCLUDE_GPIO_C_H_

void initPins();
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
int analogRead(uint8_t pin);
void analogReference(uint8_t mode);
void analogWrite(uint8_t pin, int val);
void analogWriteFreq(uint32_t freq);
void analogWriteRange(uint32_t range);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

#endif /* INCLUDE_GPIO_C_H_ */
