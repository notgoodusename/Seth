#include "Auto.h"

#include "AutoDetonate.h"
#include "AutoVaccinator.h"

#include "../../SDK/Entity.h"
#include "../../SDK/LocalPlayer.h"

void Auto::run(UserCmd* cmd) noexcept
{
	AutoDetonate::run(cmd);
	AutoVaccinator::run(cmd);
}

void Auto::updateInput() noexcept
{
	config->autoKey.handleToggle();
}

void Auto::draw(ImDrawList* drawList) noexcept
{
	AutoVaccinator::draw(drawList);
}

void Auto::reset() noexcept
{
	AutoVaccinator::reset();
}