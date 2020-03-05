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

#ifndef TRINITY_FORMATIONMOVEMENTGENERATOR_H
#define TRINITY_FORMATIONMOVEMENTGENERATOR_H

#include "AbstractFollower.h"
#include "MovementGenerator.h"
#include "Optional.h"

class Unit;

#define FORMATION_MOVE_INTERVAL 1200 // sniffed
#define FORMATION_SPLINE_DURATION 1650 // sniffed

class FormationMovementGenerator : public MovementGenerator, public AbstractFollower
{
    public:
        MovementGeneratorType GetMovementGeneratorType() const override { return FORMATION_MOTION_TYPE; }

        FormationMovementGenerator(Unit* formationLeader, float angle, float distance);
        ~FormationMovementGenerator();

        void Initialize(Unit* owner) override;
        void Finalize(Unit* owner) override;
        void Reset(Unit* owner) override;
        bool Update(Unit* owner, uint32 diff) override;

    private:
        void MovementInform(Unit* owner);
        void LaunchSplineMovement(Unit* owner);

        bool _interrupt;
        int32 _formationMoveInterval;
        float _formationAngle;
        float _formationDistance;
        Optional<Position> _lastLeaderPosition;
};

#endif // TRINITY_FORMATIONMOVEMENTGENERATOR_H
