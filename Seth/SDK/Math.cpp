#include "AttributeManager.h"
#include "Entity.h"
#include "Math.h"
#include "ModelInfo.h"
#include "LocalPlayer.h"
#include "Vector.h"

#include "../Helpers.h"

bool Math::canBackstab(Vector angles, Vector entityAngles, Vector entityWorldSpaceCenter) noexcept
{
    Vector vecToTarget;
    vecToTarget = entityWorldSpaceCenter - localPlayer->getWorldSpaceCenter();
    vecToTarget.z = 0.0f;
    vecToTarget.normalizeInPlace();

    // Get owner forward view vector
    Vector vecOwnerForward;
    Vector::fromAngleAll(angles, &vecOwnerForward, NULL, NULL);
    vecOwnerForward.z = 0.0f;
    vecOwnerForward.normalizeInPlace();

    // Get target forward view vector
    Vector vecTargetForward;
    Vector::fromAngleAll(entityAngles, &vecTargetForward, NULL, NULL);
    vecTargetForward.z = 0.0f;
    vecTargetForward.normalizeInPlace();

    // Make sure owner is behind, facing and aiming at target's back
    float posVsTargetViewDot = vecToTarget.dotProduct(vecTargetForward);
    float posVsOwnerViewDot = vecToTarget.dotProduct(vecOwnerForward);
    float viewAnglesDot = vecTargetForward.dotProduct(vecOwnerForward);

    return (posVsTargetViewDot > 0.f && posVsOwnerViewDot > 0.5f && viewAnglesDot > -0.3f);
}

bool Math::doesMeleeHit(Entity* activeWeapon, int index, const Vector angles) noexcept
{
    static Vector vecSwingMinsBase{ -18.0f, -18.0f, -18.0f };
    static Vector vecSwingMaxsBase{ 18.0f, 18.0f, 18.0f };

    if (!localPlayer)
        return false;

    float swingRange = activeWeapon->getSwingRange();
    if (swingRange <= 0.0f)
        return false;

    float boundsScale = AttributeManager::attributeHookFloat(1.0f, "melee_bounds_multiplier", activeWeapon);
    Vector vecSwingMins = vecSwingMinsBase * boundsScale;
    Vector vecSwingMaxs = vecSwingMaxsBase * boundsScale;

    float modelScale = localPlayer->modelScale();
    if (modelScale > 1.0f)
    {
        swingRange *= modelScale;
        vecSwingMins *= modelScale;
        vecSwingMaxs *= modelScale;
    }

    swingRange = AttributeManager::attributeHookFloat(swingRange, "melee_range_multiplier", activeWeapon);

    Vector vecForward = Vector::fromAngle(angles);
    Vector vecSwingStart = localPlayer->getEyePosition();
    Vector vecSwingEnd = vecSwingStart + vecForward * swingRange;

    Trace trace;
    interfaces->engineTrace->traceRay({ vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs }, MASK_SOLID, TraceFilterSimple{ localPlayer.get(), 0 }, trace);
    return trace.entity && trace.entity->index() == index;
}

Vector Math::getCenterOfHitbox(const matrix3x4 matrix[MAXSTUDIOBONES], StudioBbox* hitbox) noexcept
{
    auto VectorTransformWrapper = [](const Vector& in1, const matrix3x4 in2, Vector& out)
    {
        auto VectorTransform = [](const float* in1, const matrix3x4 in2, float* out)
        {
            auto dotProducts = [](const float* v1, const float* v2)
            {
                return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
            };
            out[0] = dotProducts(in1, in2[0]) + in2[0][3];
            out[1] = dotProducts(in1, in2[1]) + in2[1][3];
            out[2] = dotProducts(in1, in2[2]) + in2[2][3];
        };
        VectorTransform(&in1.x, in2, &out.x);
    };

    Vector min, max;
    VectorTransformWrapper(hitbox->bbMin, matrix[hitbox->bone], min);
    VectorTransformWrapper(hitbox->bbMax, matrix[hitbox->bone], max);
    return (min + max) * 0.5f;
}

Vector Math::getCenterOfHitbox(Entity* entity, const matrix3x4 matrix[MAXSTUDIOBONES], int hitboxNumber) noexcept
{
    auto VectorTransformWrapper = [](const Vector& in1, const matrix3x4 in2, Vector& out)
    {
        auto VectorTransform = [](const float* in1, const matrix3x4 in2, float* out)
        {
            auto dotProducts = [](const float* v1, const float* v2)
            {
                return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
            };
            out[0] = dotProducts(in1, in2[0]) + in2[0][3];
            out[1] = dotProducts(in1, in2[1]) + in2[1][3];
            out[2] = dotProducts(in1, in2[2]) + in2[2][3];
        };
        VectorTransform(&in1.x, in2, &out.x);
    };

    const Model* model = entity->getModel();
    if (!model)
        return Vector{ 0.0f, 0.0f, 0.0f };

    StudioHdr* hdr = interfaces->modelInfo->getStudioModel(model);
    if (!hdr)
        return Vector{ 0.0f, 0.0f, 0.0f };

    StudioHitboxSet* set = hdr->getHitboxSet(0);
    if (!set)
        return Vector{ 0.0f, 0.0f, 0.0f };

    StudioBbox* hitbox = set->getHitbox(hitboxNumber);
    if (!hitbox)
        return Vector{ 0.0f, 0.0f, 0.0f };

    Vector min, max;
    VectorTransformWrapper(hitbox->bbMin, matrix[hitbox->bone], min);
    VectorTransformWrapper(hitbox->bbMax, matrix[hitbox->bone], max);
    return (min + max) * 0.5f;
}

Vector Math::calculateRelativeAngle(const Vector& source, const Vector& destination, const Vector& viewAngles) noexcept
{
    return ((destination - source).toAngle() - viewAngles).normalize();
}

bool intersectLineWithBb(Vector& start, Vector& end, Vector& min, Vector& max) noexcept
{
    float d1, d2, f;
    auto start_solid = true;
    auto t1 = -1.0f, t2 = 1.0f;

    const float s[3] = { start.x, start.y, start.z };
    const float e[3] = { end.x, end.y, end.z };
    const float mi[3] = { min.x, min.y, min.z };
    const float ma[3] = { max.x, max.y, max.z };

    for (auto i = 0; i < 6; i++) {
        if (i >= 3) {
            const auto j = i - 3;

            d1 = s[j] - ma[j];
            d2 = d1 + e[j];
        }
        else {
            d1 = -s[i] + mi[i];
            d2 = d1 - e[i];
        }

        if (d1 > 0.0f && d2 > 0.0f)
            return false;

        if (d1 <= 0.0f && d2 <= 0.0f)
            continue;

        if (d1 > 0)
            start_solid = false;

        if (d1 > d2) {
            f = d1;
            if (f < 0.0f)
                f = 0.0f;

            f /= d1 - d2;
            if (f > t1)
                t1 = f;
        }
        else {
            f = d1 / (d1 - d2);
            if (f < t2)
                t2 = f;
        }
    }

    return start_solid || (t1 < t2&& t1 >= 0.0f);
}

Vector vectorRotate(Vector& in1, Vector& in2) noexcept
{
    auto vector_rotate = [](const Vector& in1, const matrix3x4& in2)
    {
        return Vector(in1.dotProduct(in2[0]), in1.dotProduct(in2[1]), in1.dotProduct(in2[2]));
    };
    auto angleMatrix = [](const Vector& angles, matrix3x4& matrix)
    {
        float sr, sp, sy, cr, cp, cy;

        Helpers::sinCos(Helpers::deg2rad(angles[1]), &sy, &cy);
        Helpers::sinCos(Helpers::deg2rad(angles[0]), &sp, &cp);
        Helpers::sinCos(Helpers::deg2rad(angles[2]), &sr, &cr);

        // matrix = (YAW * PITCH) * ROLL
        matrix[0][0] = cp * cy;
        matrix[1][0] = cp * sy;
        matrix[2][0] = -sp;

        float crcy = cr * cy;
        float crsy = cr * sy;
        float srcy = sr * cy;
        float srsy = sr * sy;
        matrix[0][1] = sp * srcy - crsy;
        matrix[1][1] = sp * srsy + crcy;
        matrix[2][1] = sr * cp;

        matrix[0][2] = (sp * crcy + srsy);
        matrix[1][2] = (sp * crsy - srcy);
        matrix[2][2] = cr * cp;

        matrix[0][3] = 0.0f;
        matrix[1][3] = 0.0f;
        matrix[2][3] = 0.0f;
    };
    matrix3x4 m;
    angleMatrix(in2, m);
    return vector_rotate(in1, m);
}

void vectorITransform(const Vector& in1, const matrix3x4& in2, Vector& out) noexcept
{
    out.x = (in1.x - in2[0][3]) * in2[0][0] + (in1.y - in2[1][3]) * in2[1][0] + (in1.z - in2[2][3]) * in2[2][0];
    out.y = (in1.x - in2[0][3]) * in2[0][1] + (in1.y - in2[1][3]) * in2[1][1] + (in1.z - in2[2][3]) * in2[2][1];
    out.z = (in1.x - in2[0][3]) * in2[0][2] + (in1.y - in2[1][3]) * in2[1][2] + (in1.z - in2[2][3]) * in2[2][2];
}

bool Math::hitboxIntersection(const matrix3x4 matrix[MAXSTUDIOBONES], int _hitbox, StudioHitboxSet* set, const Vector& start, const Vector& end) noexcept
{
    auto VectorTransform_Wrapper = [](const Vector& in1, const matrix3x4 in2, Vector& out)
    {
        auto VectorTransform = [](const float* in1, const matrix3x4 in2, float* out)
        {
            auto DotProducts = [](const float* v1, const float* v2)
            {
                return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
            };
            out[0] = DotProducts(in1, in2[0]) + in2[0][3];
            out[1] = DotProducts(in1, in2[1]) + in2[1][3];
            out[2] = DotProducts(in1, in2[2]) + in2[2][3];
        };
        VectorTransform(&in1.x, in2, &out.x);
    };

    StudioBbox* hitbox = set->getHitbox(_hitbox);
    if (!hitbox)
        return false;

    Vector mins, maxs;
    VectorTransform_Wrapper(vectorRotate(hitbox->bbMin, hitbox->angle), matrix[hitbox->bone], mins);
    VectorTransform_Wrapper(vectorRotate(hitbox->bbMax, hitbox->angle), matrix[hitbox->bone], maxs);

    vectorITransform(start, matrix[hitbox->bone], mins);
    vectorITransform(end, matrix[hitbox->bone], maxs);

    if (intersectLineWithBb(mins, maxs, hitbox->bbMin, hitbox->bbMax))
        return true;
    return false;
}