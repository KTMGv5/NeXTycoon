/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers / NeXTycoon
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Locomotion.h"
#include "../ride/Ride.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace OpenRCT2::Locomotion
{
    /**
     * Maps a Locomotion transport vehicle type to its closest corresponding
     * NeXTycoon / RCT2 ride simulation category.
     */
    [[nodiscard]] uint8_t MapLocoVehicleToRideType(LocoTransportType type, uint16_t maxSpeedMph);

    /**
     * Translates a Locomotion cargo identifier to a readable string name.
     */
    [[nodiscard]] std::string_view GetLocoCargoName(LocoCargoType cargo);

    /**
     * Calculates train consist acceleration (mph/s) given current grade and speed.
     */
    [[nodiscard]] float CalculateConsistAcceleration(const LocoConsist& consist, float currentSpeedMph, int32_t gradePercent);

    /**
     * Core Manager bridging Chris Sawyer's Locomotion transport networks,
     * vehicles, and cargo industries into NeXTycoon.
     */
    class LocoBridgeManager final
    {
    private:
        std::string _locomotionPath;
        bool _isAvailable = false;
        std::vector<LocoIndustryDef> _standardIndustries;
        std::vector<LocoConsist> _activeConsists;

    public:
        LocoBridgeManager();
        ~LocoBridgeManager() = default;

        /**
         * Scans system for Chris Sawyer's Locomotion installation (Steam, GOG, CD).
         */
        void Initialise();

        [[nodiscard]] bool IsAvailable() const
        {
            return _isAvailable;
        }

        [[nodiscard]] const std::string& GetLocomotionPath() const
        {
            return _locomotionPath;
        }

        void SetLocomotionPath(std::string_view path);

        [[nodiscard]] const std::vector<LocoIndustryDef>& GetIndustries() const
        {
            return _standardIndustries;
        }

        [[nodiscard]] const std::vector<LocoConsist>& GetActiveConsists() const
        {
            return _activeConsists;
        }

        void RegisterConsist(const LocoConsist& consist);
        void ClearConsists();

    private:
        void InitialiseDefaultIndustries();
        bool ValidateLocoDirectory(const std::string& path) const;
    };

    /**
     * Global accessor for the NeXTycoon Locomotion Bridge subsystem.
     */
    [[nodiscard]] LocoBridgeManager& GetLocoBridge();

} // namespace OpenRCT2::Locomotion
