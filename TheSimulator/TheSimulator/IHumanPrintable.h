#pragma once

#include "IPrintable.h"

class IHumanPrintable :	public virtual IPrintable {
public:
	IHumanPrintable() = default;

	virtual void printHuman() const = 0;
	void print() const override { printHuman(); };
};

