/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers / NeXTycoon
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace OpenRCT2::Locomotion
{
    /**
     * File signatures and formats for Chris Sawyer's Locomotion.
     */
    enum class LocoFileType : uint8_t
    {
        savedGame,   // .SV5
        scenario,    // .SC5
        landscape,   // .LC5
        objectData,  // .DAT
        unknown = 255,
    };

    /**
     * Locomotion Object Types (23 distinct object classes in Chris Sawyer's Locomotion).
     */
    enum class LocoObjectType : uint8_t
    {
        interfaceSkin = 0,
        sound         = 1,
        currency      = 2,
        steam         = 3,
        rock          = 4,
        water         = 5,
        surface       = 6,
        townNames     = 7,
        cargo         = 8,
        wall          = 9,
        track         = 10,
        trackExtra    = 11,
        road          = 12,
        roadExtra     = 13,
        airport       = 14,
        dock          = 15,
        vehicle       = 16,
        tree          = 17,
        snow          = 18,
        climate       = 19,
        hillShapes    = 20,
        building      = 21,
        industry      = 22,

        count         = 23,
        none          = 255
    };

    /**
     * Locomotion Transport Modes and Vehicle Categories.
     */
    enum class LocoTransportType : uint8_t
    {
        train,
        bus,
        truck,
        tram,
        aircraft,
        ship,
    };

    /**
     * Locomotion Cargo Categories (Towns and Freight Industries).
     */
    enum class LocoCargoType : uint8_t
    {
        passengers = 0,
        mail       = 1,
        coal       = 2,
        ironOre    = 3,
        steel      = 4,
        wood       = 5,
        goods      = 6,
        grain      = 7,
        livestock  = 8,
        oil        = 9,
        chemicals  = 10,
        food       = 11,
        custom     = 254,
        none       = 255,
    };

    /**
     * Locomotion Track & Road Gauge Types.
     */
    enum class LocoTrackGauge : uint8_t
    {
        standardGaugeRail,
        narrowGaugeRail,
        monorail,
        maglev,
        standardRoad,
        tramway,
    };

    /**
     * Locomotion Signal Types.
     */
    enum class LocoSignalType : uint8_t
    {
        blockTwoAspect,
        blockThreeAspect,
        blockFourAspect,
        preSignal,
        exitSignal,
        oneWaySignal,
    };

#pragma pack(push, 1)
    /**
     * Chris Sawyer's Locomotion 16-byte object descriptor header.
     * Compatible with the Sawyer chunk encoding format.
     */
    struct LocoObjectHeader
    {
        uint8_t flags;
        char name[8];
        uint32_t checksum;
    };
    static_assert(sizeof(LocoObjectHeader) == 13 || sizeof(LocoObjectHeader) == 16 || sizeof(LocoObjectHeader) <= 16);

    /**
     * Header info stored at the start of Locomotion .SV5/.SC5 save archives.
     */
    struct LocoSaveHeader
    {
        uint32_t type;            // File type indicator (0 = saved game, 1 = scenario)
        uint32_t subType;
        uint32_t flags;
        uint16_t currentDay;
        uint16_t currentMonth;
        uint16_t currentYear;     // 1900 - 2100
        uint8_t numCompanies;     // Up to 15 competing transport companies
        uint8_t activeCompany;
        uint16_t mapWidth;        // Typically 384x384 tiles
        uint16_t mapHeight;
        char scenarioName[64];
        char scenarioDetails[256];
    };
#pragma pack(pop)

    /**
     * Represents a single vehicle unit in a Locomotion train or road consist.
     */
    struct LocoVehicleUnit
    {
        std::string name;
        LocoTransportType transportType = LocoTransportType::train;
        uint16_t powerHorsepower = 0;
        uint16_t weightTons = 0;
        uint16_t maxSpeedMph = 0;
        uint8_t lengthUnits = 4;
        LocoCargoType cargoCapacityType = LocoCargoType::passengers;
        uint16_t cargoCapacityAmount = 0;
        bool isPowered = true;
    };

    /**
     * Represents a full transport consist (Locomotives + Wagons / Multiple Units).
     */
    struct LocoConsist
    {
        std::string consistName;
        std::vector<LocoVehicleUnit> units;
        uint8_t ownerCompanyId = 0;

        [[nodiscard]] uint32_t GetTotalPower() const
        {
            uint32_t total = 0;
            for (const auto& u : units)
            {
                total += u.powerHorsepower;
            }
            return total;
        }

        [[nodiscard]] uint32_t GetTotalWeight() const
        {
            uint32_t total = 0;
            for (const auto& u : units)
            {
                total += u.weightTons;
            }
            return total;
        }

        [[nodiscard]] uint16_t GetMaxSpeed() const
        {
            if (units.empty())
                return 0;

            uint16_t speed = units[0].maxSpeedMph;
            for (const auto& u : units)
            {
                if (u.maxSpeedMph > 0 && u.maxSpeedMph < speed)
                {
                    speed = u.maxSpeedMph;
                }
            }
            return speed;
        }

        [[nodiscard]] size_t GetTotalLength() const
        {
            size_t len = 0;
            for (const auto& u : units)
            {
                len += u.lengthUnits;
            }
            return len;
        }
    };

    /**
     * Locomotion Industrial Producer / Consumer Specification.
     */
    struct LocoIndustryDef
    {
        std::string name;
        std::vector<LocoCargoType> producedCargos;
        std::vector<LocoCargoType> requiredCargos;
        uint32_t monthlyProductionRate = 0;
        bool isExtractive = false; // Primary resource (coal mine, forest, oil well)
    };
} // namespace OpenRCT2::Locomotion
