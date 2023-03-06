#pragma once
#include "MaterialRenderContext.h"

enum StencilOperation
{
	STENCILOPERATION_KEEP = 1,
	STENCILOPERATION_ZERO = 2,
	STENCILOPERATION_REPLACE = 3,
	STENCILOPERATION_INCRSAT = 4,
	STENCILOPERATION_DECRSAT = 5,
	STENCILOPERATION_INVERT = 6,
	STENCILOPERATION_INCR = 7,
	STENCILOPERATION_DECR = 8,
	STENCILOPERATION_FORCE_DWORD = 0x7fffffff
};

enum StencilComparisonFunction
{
	STENCILCOMPARISONFUNCTION_NEVER = 1,
	STENCILCOMPARISONFUNCTION_LESS = 2,
	STENCILCOMPARISONFUNCTION_EQUAL = 3,
	STENCILCOMPARISONFUNCTION_LESSEQUAL = 4,
	STENCILCOMPARISONFUNCTION_GREATER = 5,
	STENCILCOMPARISONFUNCTION_NOTEQUAL = 6,
	STENCILCOMPARISONFUNCTION_GREATEREQUAL = 7,
	STENCILCOMPARISONFUNCTION_ALWAYS = 8,
	STENCILCOMPARISONFUNCTION_FORCE_DWORD = 0x7fffffff
};

class ShaderStencilState
{
public:
	bool enabled;
	StencilOperation failOp;
	StencilOperation ZfailOp;
	StencilOperation passOp;
	StencilComparisonFunction compareFunc;
	int referenceValue;
	unsigned int testMask;
	unsigned int writeMask;

	ShaderStencilState() noexcept
	{
		enabled = false;
		passOp = failOp = ZfailOp = STENCILOPERATION_KEEP;
		compareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
		referenceValue = 0;
		testMask = writeMask = 0xFFFFFFFF;
	}

	void setStencilState(MaterialRenderContext* renderContext) noexcept
	{
		renderContext->setStencilEnable(enabled);
		renderContext->setStencilFailOperation(failOp);
		renderContext->setStencilZFailOperation(ZfailOp);
		renderContext->setStencilPassOperation(passOp);
		renderContext->setStencilCompareFunction(compareFunc);
		renderContext->setStencilReferenceValue(referenceValue);
		renderContext->setStencilTestMask(testMask);
		renderContext->setStencilWriteMask(writeMask);
	}
};