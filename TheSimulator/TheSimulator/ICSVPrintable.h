#pragma once

#include "IPrintable.h"

class ICSVPrintable : public virtual IPrintable {
public:
	ICSVPrintable() = default;

	virtual void printCSV() const = 0;
};

