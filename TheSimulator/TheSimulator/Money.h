#pragma once

#include "Decimal.h"

class Money : public Decimal {
public:
	Money() : Decimal() {}
	Money(int wholes) : Decimal(wholes) {}
	Money(signed long long int wholes) : Decimal(wholes) {}
	Money(signed long long int wholes, unsigned int cents) : Money(wholes) { setCents(cents); }
	Money(float val) : Decimal(val) {}
	Money(double val) : Decimal(val) {}
	Money(const Money& cpy) : Decimal() { setInternalValue(cpy.internalValue()); }
	Money(const Decimal& cpy) : Decimal(cpy) {} //for amazing convenience

	void setCents(unsigned int cents);
	unsigned int cents() const { return (unsigned int)std::abs(fraction() / CENT_OFFSET); }
	unsigned int roundedCents() const { return cents() + (cents() >= 50 ? 1 : 0); }
	unsigned int ceiledCents() const { return cents() + (cents() > 0 ? 1 : 0); }
	Money roundToCents() const { return Money(this->whole(), this->roundedCents()); }
	Money floorToCents() const { return Money(this->whole(), this->cents()); }
	Money ceilToCents() const { return Money(this->whole(), this->ceiledCents()); }

	std::string toCentString() const;

	std::string toPostfixedString(unsigned int digitsBeforePostfix) const;

	static const long long int CENT_OFFSET = WHOLE_OFFSET / 100;
	static const std::string postfixes[];
};
using mny = Money;

/* HACK: THIS IS PROHIBITED BY STANDARD! OH, IM SUCH A REBEL!!! */
namespace std {
	template<> class numeric_limits<mny> {
	public:
		static mny lowest() { return mny(-500000000000LL); }; //FIFTY FREAKIN BILLION
		static mny max() { return mny(+500000000000LL); };
	};
}