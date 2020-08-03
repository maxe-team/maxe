#pragma once

#include <cmath>
#include <tuple>
#include <string>

class Decimal {
public:
	Decimal();
	explicit Decimal(int val);
	explicit Decimal(signed long long int whole);
	explicit Decimal(float val);
	explicit Decimal(double val);
	Decimal(const Decimal& cpy);
	~Decimal();

	inline Decimal& operator=(const Decimal& rhs) { this->m_internalValue = rhs.internalValue(); return *this; }
	inline Decimal& operator=(const int rhs) { this->m_internalValue = rhs; return *this; }
	inline Decimal& operator=(const float rhs) { this->m_internalValue = decltype(m_internalValue)((double)rhs * WHOLE_OFFSET); return *this; }
	inline Decimal& operator=(const double rhs) { this->m_internalValue = decltype(m_internalValue)(rhs * WHOLE_OFFSET); return *this; }

	inline Decimal& operator+=(const Decimal& rhs) { this->m_internalValue += rhs.internalValue(); return *this; }
	inline Decimal& operator+=(const int rhs) { this->m_internalValue += (decltype(m_internalValue))rhs * WHOLE_OFFSET; return *this; }
	inline Decimal& operator+=(const signed long long int rhs) { this->m_internalValue += (decltype(m_internalValue))rhs * WHOLE_OFFSET; return *this; }
	inline Decimal& operator+=(const float rhs) { this->m_internalValue += (decltype(m_internalValue))((double)rhs * WHOLE_OFFSET); return *this; }
	inline Decimal& operator+=(const double rhs) { this->m_internalValue += (decltype(m_internalValue))(rhs * WHOLE_OFFSET); return *this; }

	inline Decimal& operator-=(const Decimal& rhs) { this->m_internalValue -= rhs.internalValue(); return *this; }
	inline Decimal& operator-=(const int rhs) { this->m_internalValue -= (decltype(m_internalValue))rhs * WHOLE_OFFSET; return *this; }
	inline Decimal& operator-=(const signed long long int rhs) { this->m_internalValue -= (decltype(m_internalValue))rhs * WHOLE_OFFSET; return *this; }
	inline Decimal& operator-=(const float rhs) { this->m_internalValue -= (decltype(m_internalValue))((double)rhs * WHOLE_OFFSET); return *this; }
	inline Decimal& operator-=(const double rhs) { this->m_internalValue -= (decltype(m_internalValue))(rhs * WHOLE_OFFSET); return *this; }

	inline Decimal& operator*=(const Decimal& rhs) { this->m_internalValue *= rhs.internalValue(); return *this; }
	inline Decimal& operator*=(const int rhs) { this->m_internalValue *= rhs; return *this; }
	inline Decimal& operator*=(const signed long long int rhs) { this->m_internalValue *= rhs; return *this; }
	inline Decimal& operator*=(const float rhs) { this->m_internalValue = (decltype(m_internalValue))((double)this->m_internalValue * rhs); return *this; }
	inline Decimal& operator*=(const double rhs) { this->m_internalValue = (decltype(m_internalValue))(this->m_internalValue * rhs); return *this; }

	inline Decimal& operator/=(const Decimal& rhs) { this->m_internalValue /= rhs.internalValue(); return *this; }
	inline Decimal& operator/=(const int rhs) { this->m_internalValue /= rhs; return *this; }
	inline Decimal& operator/=(const signed long long int rhs) { this->m_internalValue /= rhs; return *this; }
	inline Decimal& operator/=(const float rhs) { this->m_internalValue = (decltype(m_internalValue))(this->m_internalValue / rhs); return *this; }
	inline Decimal& operator/=(const double rhs) { this->m_internalValue = (decltype(m_internalValue))(this->m_internalValue / rhs); return *this; }

	inline Decimal operator+(const Decimal& rhs) const { return Decimal::fromInternalValue(this->m_internalValue + rhs.m_internalValue); }
	inline Decimal operator+(const int rhs) const { return Decimal::fromInternalValue(this->m_internalValue + rhs * WHOLE_OFFSET); }
	inline Decimal operator+(const signed long long int rhs) const { return Decimal::fromInternalValue(this->m_internalValue + rhs * WHOLE_OFFSET); }
	inline Decimal operator+(const float rhs) const { return Decimal::fromInternalValue(this->m_internalValue + decltype(m_internalValue)((double)rhs * WHOLE_OFFSET)); }
	inline Decimal operator+(const double rhs) const { return Decimal::fromInternalValue(this->m_internalValue + decltype(m_internalValue)((double)rhs * WHOLE_OFFSET)); }

	inline Decimal operator-(const Decimal& rhs) const { return Decimal::fromInternalValue(this->m_internalValue - rhs.m_internalValue); }
	inline Decimal operator-(const int rhs) const { return Decimal::fromInternalValue(this->m_internalValue - rhs * WHOLE_OFFSET); }
	inline Decimal operator-(const signed long long int rhs) const { return Decimal::fromInternalValue(this->m_internalValue - rhs * WHOLE_OFFSET); }
	inline Decimal operator-(const float rhs) const { return Decimal::fromInternalValue(this->m_internalValue - decltype(m_internalValue)((double)rhs * WHOLE_OFFSET)); }
	inline Decimal operator-(const double rhs) const { return Decimal::fromInternalValue(this->m_internalValue - decltype(m_internalValue)(rhs * WHOLE_OFFSET)); }

	inline Decimal operator-() const { return Decimal::fromInternalValue(-this->m_internalValue); }

	inline Decimal operator*(const Decimal& rhs) const { return Decimal::fromInternalValue(this->m_internalValue * rhs.internalValue()); }
	inline Decimal operator*(const int rhs) const { return Decimal::fromInternalValue(this->m_internalValue * rhs); }
	inline Decimal operator*(const signed long long int rhs) const { return Decimal::fromInternalValue(this->m_internalValue * rhs); }
	inline Decimal operator*(const float rhs) const { return Decimal::fromInternalValue((double)this->m_internalValue * rhs); }
	inline Decimal operator*(const double rhs) const { return Decimal::fromInternalValue(this->m_internalValue * rhs); }

	inline Decimal operator/(const Decimal& rhs) const { return Decimal((double)this->m_internalValue / (double)rhs.m_internalValue); }
	inline Decimal operator/(const int rhs) const { return Decimal::fromInternalValue(this->m_internalValue / rhs); }
	inline Decimal operator/(const signed long long int rhs) const { return Decimal::fromInternalValue(this->m_internalValue / rhs); }
	inline Decimal operator/(const float rhs) const { return Decimal::fromInternalValue(this->m_internalValue / rhs); }
	inline Decimal operator/(const double rhs) const { return Decimal::fromInternalValue(this->m_internalValue / rhs); }

	inline bool operator==(const Decimal& rhs) const { return rhs.m_internalValue == this->m_internalValue; }
	inline bool operator!=(const Decimal& rhs) const { return rhs.m_internalValue != this->m_internalValue; }
	inline bool operator>(const int val) const { return this->m_internalValue > val; }
	inline bool operator>(const Decimal& rhs) const { return this->m_internalValue > rhs.m_internalValue; }
	inline bool operator<(const Decimal& rhs) const { return this->m_internalValue < rhs.m_internalValue; }
	inline bool operator>=(const Decimal& rhs) const { return this->m_internalValue >= rhs.m_internalValue; }
	inline bool operator<=(const Decimal& rhs) const { return this->m_internalValue <= rhs.m_internalValue; }

	inline explicit operator signed long long int() const { return this->whole(); }
	inline explicit operator int() const { return (int)this->whole(); }
	inline explicit operator double() const { return (double)internalValue() / WHOLE_OFFSET; }
	inline explicit operator float() const { return (float)internalValue() / WHOLE_OFFSET; }

	Decimal abs() const { return m_internalValue > 0 ? *this : -*this; }

	Decimal floor() const;
	Decimal round() const;
	Decimal ceil() const;

	std::string toFullString() const;
	std::string toDigits(unsigned int start, unsigned int num) const;
	std::string signString() const { return m_internalValue > 0 ? "+" : (m_internalValue == 0 ? "" : "-"); }

	explicit operator std::string() const { return this->toFullString(); }
protected:
	void setInternalValue(signed long long int internalValue);
	signed long long int internalValue() const;

	static const long long int WHOLE_OFFSET = 100000;
	signed long long int whole() const;
	unsigned int fraction() const;

	static Decimal fromInternalValue(signed long long int internalValue);
	static Decimal fromInternalValue(double internalValue); // convenience method to supress the warnings
private:
	signed long long int m_internalValue;
};

using decimal = Decimal;