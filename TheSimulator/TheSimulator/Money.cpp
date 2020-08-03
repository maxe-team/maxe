#include "Money.h"

const std::string Money::postfixes[] = { "", "K", "M", "B", "T" };

void Money::setCents(unsigned int cents) {
	auto w = whole();
	this->setInternalValue((w >= 0 ? 1 : -1) * ((w >= 0 ? w : -w) * 100 + cents) * CENT_OFFSET);
}

std::string Money::toCentString() const {
	std::string fullString = toFullString();
	return fullString.substr(0, fullString.length() - 3);
}

std::string Money::toPostfixedString(unsigned int digitsBeforePostfix) const {
	signed long long int whole = (signed long long int)abs();
	/* the first step is to find postfix with which the number has no more than digitsBeforePostfix digits */
	long long int digitCap = 1;
	for (unsigned int i = 0; i < digitsBeforePostfix; i++)
		digitCap *= 10;

	long long int postfixIndex = 0;
	while (whole > digitCap) {
		postfixIndex++;
		long long int newWhole = whole / 1000;
		int rem = whole % 1000;
		if (rem >= 500)
			newWhole += 1;

		whole = newWhole;
	}

	std::string baseString = std::to_string(whole);
	/* insert separators of thousands: , */
	for (long long int i = baseString.length() - 3; i > 0; i -= 3) {
		baseString.insert(i, 1, ',');
	}
	/*append apropriate postfix*/
	baseString += postfixes[postfixIndex];

	return *this > 0 ? baseString : "-" + baseString;
}
