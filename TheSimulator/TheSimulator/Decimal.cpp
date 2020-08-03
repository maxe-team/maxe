#include "Decimal.h"

Decimal::Decimal()
	: m_internalValue(0) {
}

Decimal::Decimal(int val) {
	m_internalValue = (decltype(m_internalValue))val * WHOLE_OFFSET;
}

Decimal::Decimal(signed long long int whole) {
	m_internalValue = whole * WHOLE_OFFSET;
}

Decimal::Decimal(float val) {
	m_internalValue = (decltype(m_internalValue))(std::floor(val * WHOLE_OFFSET));
}

Decimal::Decimal(double val) {
	m_internalValue = decltype(m_internalValue)(std::floor(val * WHOLE_OFFSET));
}

Decimal::Decimal(const Decimal& cpy)
	: Decimal() {
	setInternalValue(cpy.internalValue());
}

Decimal::~Decimal() {}

long long int Decimal::whole() const {
	return this->internalValue() / WHOLE_OFFSET;
}

unsigned int Decimal::fraction() const {
	return (unsigned int)(std::abs(this->internalValue()) % WHOLE_OFFSET);
}

Decimal Decimal::floor() const {
	return Decimal(this->whole());
}

Decimal Decimal::round() const {
	return Decimal(whole() + (fraction() >= WHOLE_OFFSET / 2 ? 1 : 0));
}

Decimal Decimal::ceil() const {
	return Decimal(whole() + (fraction() > 0 ? 1 : 0));
}

std::string Decimal::toFullString() const {
	std::string base_string = std::to_string(this->m_internalValue);
	if (base_string.length() < 6) {
		std::string padded(6 - base_string.length(), '0');
		base_string = padded + base_string;
	}
	base_string.insert(base_string.end() - 5, '.');

	return base_string;
}

#include <algorithm>
std::string Decimal::toDigits(unsigned int start, unsigned int num) const {
	std::string full = toFullString();
	int iend = (int)full.length() - start;
	int istart = std::max(0, iend - (int)num);
	std::string result = full.substr(istart, (size_t)iend - istart + 1);
	return result;
}

Decimal Decimal::fromInternalValue(signed long long int internalValue) {
	Decimal ret;

	ret.setInternalValue(internalValue);

	return ret;
}

Decimal Decimal::fromInternalValue(double internalValue) {
	Decimal ret;

	ret.setInternalValue((signed long long int)internalValue);

	return ret;
}

signed long long int Decimal::internalValue() const {
	return this->m_internalValue;
}

void Decimal::setInternalValue(signed long long int val) {
	this->m_internalValue = val;
}
