#pragma once

class IPrintable {
public:
	virtual void print() const = 0;
protected:
	IPrintable() = default;
};

