#pragma once

#include "Vector.h"
#include "VirtualMethod.h"

class ViewVectors
{
public:
	Vector view;

	Vector hullMin;
	Vector hullMax;

	Vector duckHullMin;
	Vector duckHullMax;
	Vector duckView;

	Vector obsHullMin;
	Vector obsHullMax;

	Vector deadViewHeight;
};

class GameRules
{
public:
	VIRTUAL_METHOD(const ViewVectors*, getViewVectors, 31, (), (this))
};