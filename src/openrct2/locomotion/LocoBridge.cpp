/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers / NeXTycoon
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "LocoBridge.h"
#include "../core/Path.hpp"
#include "../platform/Platform.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

namespace OpenRCT2::Locomotion
{
    namespace fs = std::filesystem;

    uint8_t MapLocoVehicleToRideType(LocoTransportType type, uint16_t maxSpeedMph)
    {
        switch (type)
        {
            case LocoTransportType::train:
                if (maxSpeedMph >= 120)
                {
                    return RIDE_TYPE_MONORAIL;
                }
                return RIDE_TYPE_MINIATURE_RAILWAY;

            case LocoTransportType::bus:
            case LocoTransportType::truck:
                return RIDE_TYPE_CAR_RIDE;

            case LocoTransportType::tram:
                return RIDE_TYPE_MINIATURE_RAILWAY;

            case LocoTransportType::aircraft:
                return RIDE_TYPE_MINI_HELICOPTERS;

            case LocoTransportType::ship:
                return RIDE_TYPE_BOAT_HIRE;

            default:
                return RIDE_TYPE_MINIATURE_RAILWAY;
        }
    }

    std::string_view GetLocoCargoName(LocoCargoType cargo)
    {
        switch (cargo)
        {
            case LocoCargoType::passengers:
                return "Passengers";
            case LocoCargoType::mail:
                return "Mail";
            case LocoCargoType::coal:
                return "Coal";
            case LocoCargoType::ironOre:
                return "Iron Ore";
            case LocoCargoType::steel:
                return "Steel";
            case LocoCargoType::wood:
                return "Wood";
            case LocoCargoType::goods:
                return "Goods";
            case LocoCargoType::grain:
                return "Grain";
            case LocoCargoType::livestock:
                return "Livestock";
            case LocoCargoType::oil:
                return "Oil";
            case LocoCargoType::chemicals:
                return "Chemicals";
            case LocoCargoType::food:
                return "Food";
            case LocoCargoType::custom:
                return "Custom Cargo";
            case LocoCargoType::none:
                return "None";
            default:
                return "Cargo";
        }
    }

    float CalculateConsistAcceleration(const LocoConsist& consist, float currentSpeedMph, int32_t gradePercent)
    {
        const uint32_t totalPower = consist.GetTotalPower();
        const uint32_t totalWeight = consist.GetTotalWeight();
        const uint16_t maxSpeed = consist.GetMaxSpeed();

        if (totalWeight == 0)
        {
            return 0.0f;
        }

        // If at or above max speed, cannot accelerate forward
        if (currentSpeedMph >= static_cast<float>(maxSpeed) && maxSpeed > 0)
        {
            return 0.0f;
        }

        const float speedClamped = (std::max)(1.0f, currentSpeedMph);

        // Tractive Force in lbs: P (HP) * 375 * transmission_efficiency / V (mph)
        // Chris Sawyer diesel/steam/electric efficiency approx 0.85
        constexpr float efficiency = 0.85f;
        float tractiveForceLbf = (static_cast<float>(totalPower) * 375.0f * efficiency) / speedClamped;

        // Adhesion limit: typical steel-on-steel adhesion ~ 0.25 * powered weight in lbs
        // Assuming ~70% of consist weight is on driven axles
        const float adhesionWeightLbs = static_cast<float>(totalWeight) * 2000.0f * 0.70f;
        const float maxAdhesionForceLbf = adhesionWeightLbs * 0.25f;
        if (tractiveForceLbf > maxAdhesionForceLbf)
        {
            tractiveForceLbf = maxAdhesionForceLbf;
        }

        // Train Resistance (Davis formula approximation):
        // Rolling resistance: ~4 lbs per short ton
        const float weightTons = static_cast<float>(totalWeight);
        const float rollingResistanceLbf = weightTons * 4.0f;

        // Flange / Curve / Air resistance
        const float airResistanceLbf = 0.0025f * 110.0f * speedClamped * speedClamped;

        // Grade resistance: 20 lbs per ton per 1% grade slope
        const float gradeResistanceLbf = weightTons * 20.0f * static_cast<float>(gradePercent);

        const float totalResistanceLbf = rollingResistanceLbf + airResistanceLbf + gradeResistanceLbf;
        const float netForceLbf = tractiveForceLbf - totalResistanceLbf;

        // a (mph/s) = NetForce (lbf) / (91.1 * Weight in tons)
        // (91.1 comes from 2000 lbs/ton / 32.174 ft/s^2 * 1.4667 ft/s per mph)
        const float accelMphPerSec = netForceLbf / (91.1f * weightTons);

        return accelMphPerSec;
    }

    LocoBridgeManager::LocoBridgeManager()
    {
        InitialiseDefaultIndustries();
    }

    void LocoBridgeManager::Initialise()
    {
        // Check standard Locomotion install directories across common drive letters
        static const std::string searchPaths[] = {
            "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Locomotion",
            "C:\\Steam\\steamapps\\common\\Locomotion",
            "D:\\Steam\\steamapps\\common\\Locomotion",
            "E:\\Steam\\steamapps\\common\\Locomotion",
            "C:\\GOG Games\\Locomotion",
            "C:\\Program Files (x86)\\GOG Galaxy\\Games\\Locomotion",
            "C:\\Program Files\\Atari\\Locomotion",
            "C:\\Program Files (x86)\\Atari\\Locomotion",
        };

        for (const auto& path : searchPaths)
        {
            if (ValidateLocoDirectory(path))
            {
                _locomotionPath = path;
                _isAvailable = true;
                return;
            }
        }
    }

    void LocoBridgeManager::SetLocomotionPath(std::string_view path)
    {
        std::string p(path);
        if (ValidateLocoDirectory(p))
        {
            _locomotionPath = std::move(p);
            _isAvailable = true;
        }
        else
        {
            _locomotionPath = std::move(p);
            _isAvailable = false;
        }
    }

    void LocoBridgeManager::RegisterConsist(const LocoConsist& consist)
    {
        _activeConsists.push_back(consist);
    }

    void LocoBridgeManager::ClearConsists()
    {
        _activeConsists.clear();
    }

    bool LocoBridgeManager::ValidateLocoDirectory(const std::string& path) const
    {
        std::error_code ec;
        if (path.empty() || !fs::is_directory(path, ec))
        {
            return false;
        }

        // Chris Sawyer's Locomotion contains an ObjData folder and/or Data/g1.dat
        auto objDataPath = fs::path(path) / "ObjData";
        auto dataPath = fs::path(path) / "Data";
        auto locoExe = fs::path(path) / "Loco.exe";
        auto locoExeLower = fs::path(path) / "loco.exe";

        if (fs::is_directory(objDataPath, ec) ||
            fs::is_directory(dataPath, ec) ||
            fs::is_regular_file(locoExe, ec) ||
            fs::is_regular_file(locoExeLower, ec))
        {
            return true;
        }

        return false;
    }

    void LocoBridgeManager::InitialiseDefaultIndustries()
    {
        _standardIndustries.clear();

        // 1. Coal Mine
        _standardIndustries.push_back({
            "Coal Mine",
            { LocoCargoType::coal },
            {},
            800,
            true,
        });

        // 2. Power Station
        _standardIndustries.push_back({
            "Power Station",
            {},
            { LocoCargoType::coal },
            0,
            false,
        });

        // 3. Iron Ore Mine
        _standardIndustries.push_back({
            "Iron Ore Mine",
            { LocoCargoType::ironOre },
            {},
            600,
            true,
        });

        // 4. Steel Mill
        _standardIndustries.push_back({
            "Steel Mill",
            { LocoCargoType::steel },
            { LocoCargoType::ironOre, LocoCargoType::coal },
            500,
            false,
        });

        // 5. Forest
        _standardIndustries.push_back({
            "Forest",
            { LocoCargoType::wood },
            {},
            750,
            true,
        });

        // 6. Sawmill
        _standardIndustries.push_back({
            "Sawmill",
            { LocoCargoType::goods },
            { LocoCargoType::wood },
            400,
            false,
        });

        // 7. Oil Wells
        _standardIndustries.push_back({
            "Oil Wells",
            { LocoCargoType::oil },
            {},
            900,
            true,
        });

        // 8. Oil Refinery
        _standardIndustries.push_back({
            "Oil Refinery",
            { LocoCargoType::chemicals },
            { LocoCargoType::oil },
            600,
            false,
        });

        // 9. Chemical Plant
        _standardIndustries.push_back({
            "Chemical Plant",
            { LocoCargoType::goods },
            { LocoCargoType::chemicals },
            350,
            false,
        });

        // 10. Farm
        _standardIndustries.push_back({
            "Farm",
            { LocoCargoType::grain, LocoCargoType::livestock },
            {},
            700,
            true,
        });

        // 11. Food Processing Plant
        _standardIndustries.push_back({
            "Food Processing Plant",
            { LocoCargoType::food },
            { LocoCargoType::grain, LocoCargoType::livestock },
            500,
            false,
        });

        // 12. Factory
        _standardIndustries.push_back({
            "Factory",
            { LocoCargoType::goods },
            { LocoCargoType::steel, LocoCargoType::food },
            450,
            false,
        });
    }

    LocoBridgeManager& GetLocoBridge()
    {
        static LocoBridgeManager instance;
        return instance;
    }

} // namespace OpenRCT2::Locomotion
