/*
 * Wildcard.h
 *
 *  Created on: Feb 3, 2016
 *      Author: lieven
 */

#ifndef WILDCARD_H_
#define WILDCARD_H_

extern bool wildcardMatch(const char * pTameText,        // A string without wildcards
		const char * pWildText, // A (potentially) corresponding string with wildcards
		bool bCaseSensitive ,  // By default, match on 'X' vs 'x'
		char cAltTerminator  // For function names, for example, you can stop at the first '('
		);

class Wildcard {
public:
	Wildcard();
	virtual ~Wildcard();
};

#endif /* WILDCARD_H_ */
