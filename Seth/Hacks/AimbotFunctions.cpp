#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"

#include "AimbotFunctions.h"
#include "Animations.h"

#include "../SDK/Angle.h"
#include "../SDK/ConVar.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"
#include "../SDK/WeaponId.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/PhysicsSurfaceProps.h"
#include "../SDK/WeaponData.h"
#include "../SDK/ModelInfo.h"

Vector AimbotFunction::calculateRelativeAngle(const Vector& source, const Vector& destination, const Vector& viewAngles) noexcept
{
    return ((destination - source).toAngle() - viewAngles).normalize();
}