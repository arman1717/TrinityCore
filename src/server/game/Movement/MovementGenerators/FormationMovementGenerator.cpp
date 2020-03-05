/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Creature.h"
#include "CreatureAI.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "FormationMovementGenerator.h"
#include "G3DPosition.hpp"

FormationMovementGenerator::FormationMovementGenerator(Unit* formationLeader, float angle, float distance)
    : AbstractFollower(ASSERT_NOTNULL(formationLeader)), _interrupt(false), _formationMoveInterval(100), _formationAngle(angle), _formationDistance(distance) { }

FormationMovementGenerator::~FormationMovementGenerator() = default;

void FormationMovementGenerator::Initialize(Unit* owner)
{
    owner->AddUnitState(UNIT_STATE_ROAMING);
    _lastLeaderPosition.reset();

    if (owner->HasUnitState(UNIT_STATE_NOT_MOVE) || owner->IsMovementPreventedByCasting())
    {
        _interrupt = true;
        owner->StopMoving();
        return;
    }

    owner->AddUnitState(UNIT_STATE_ROAMING_MOVE);
}

bool FormationMovementGenerator::Update(Unit* owner, uint32 diff)
{
    // nullptr protects
    if (!owner)
        return false;

    // we are in combat, we will re-initialize a new formation movement when we are done
    if (owner->IsInCombat())
        return false;

    // formation target might have gone
    if (!GetTarget() || !GetTarget()->IsInWorld())
        return false;

    // our formation member is unable to move
    if (owner->HasUnitState(UNIT_STATE_NOT_MOVE) || owner->IsMovementPreventedByCasting())
    {
        _interrupt = true;
        owner->StopMoving();
        return true;
    }

    // update interrupt state
    if ((_interrupt && owner->movespline->Finalized()) || !owner->movespline->Finalized())
    {
        _interrupt = false;
        owner->AddUnitState(UNIT_STATE_ROAMING_MOVE);
    }

    // trigger next formation follow movement
    _formationMoveInterval -= diff;
    if (!_interrupt && _formationMoveInterval <= 0)
        if (GetTarget()->GetPosition() != _lastLeaderPosition.get())
            LaunchSplineMovement(owner);

    _lastLeaderPosition = GetTarget()->GetPosition();

    if (_formationMoveInterval <= 0)
        _formationMoveInterval = FORMATION_MOVE_INTERVAL;

    return true;
}

void FormationMovementGenerator::Finalize(Unit* owner)
{
    owner->ClearUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
}

void FormationMovementGenerator::Reset(Unit* owner)
{
    owner->StopMoving();
    Initialize(owner);
}

void FormationMovementGenerator::MovementInform(Unit* owner)
{
}

void FormationMovementGenerator::LaunchSplineMovement(Unit* owner)
{
    Unit* leader = GetTarget();

    // First we calculate our destination

    Position dest = leader->GetPosition();
    if (!leader->movespline->Finalized())
        dest = Vector3ToPosition(leader->movespline->FinalDestination());

    float angle = leader->GetAngle(dest);
    dest.m_positionX += std::cos(angle + _formationAngle) * _formationDistance;
    dest.m_positionY += std::sin(angle + _formationAngle) * _formationDistance;

    // Normalize offsets
    Trinity::NormalizeMapCoord(dest.m_positionX);
    Trinity::NormalizeMapCoord(dest.m_positionY);
    if (!owner->IsFlying())
        owner->UpdateGroundPositionZ(dest.GetPositionX(), dest.GetPositionY(), dest.m_positionZ);

    // Now we update our home position
    if (owner->IsCreature())
        owner->ToCreature()->SetHomePosition(dest);

    // Preparing spline
    Movement::MoveSplineInit init(owner);
    init.MoveTo(dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ());

    // Determining our spline duration
    if (leader->movespline->Finalized())
        init.SetVelocity(owner->GetSpeed(MOVE_WALK));
    else if (owner->GetExactDist(dest) > 1.f)
    {
        uint32 length = std::min<uint32>(leader->movespline->RemainingTime(), FORMATION_SPLINE_DURATION);
        init.SetEnforcedLength(length);
    }

    init.Launch();
}
