#pragma once

#include "Engine.h"
#include "Vector.h"

class ViewSetup
{
public:
	int x;
	int unscaledX;
	int y;
	int unscaledY;
	int width;
	int unscaledWidth;
	int height;
	int stereoEye;
	int unscaledHeight;
	bool ortho;
	float orthoLeft;
	float orthoTop;
	float orthoRight;
	float orthoBottom;
	float fov;
	float viewmodelFov;
	Vector origin;
	Vector angles;
	float zNear;
	float zFar;
	float viewmodelZNear;
	float viewmodelZFar;
	bool renderToSubrectOfLargerScreen;
	float aspectRatio;
	bool offCenter;
	float offCenterTop;
	float offCenterBottom;
	float offCenterLeft;
	float offCenterRight;
	bool doBloomAndToneMapping;
	bool cacheFullSceneState;
	bool viewToProjectionOverride;
	Matrix4x4 viewToProjection;
};