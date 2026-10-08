/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers / NeXTycoon
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/locomotion/LocoBridge.h>
#include <openrct2/locomotion/LocoFile.h>
#include <openrct2/locomotion/Locomotion.h>
#include <openrct2/ride/Ride.h>

using namespace OpenRCT2;
using namespace OpenRCT2::Locomotion;

TEST(LocomotionTests, FileTypeDetection)
{
    EXPECT_EQ(DetectLocoFileType(nullptr, 0, ".sv5"), LocoFileType::savedGame);
    EXPECT_EQ(DetectLocoFileType(nullptr, 0, ".sc5"), LocoFileType::scenario);
    EXPECT_EQ(DetectLocoFileType(nullptr, 0, ".lc5"), LocoFileType::landscape);
    EXPECT_EQ(DetectLocoFileType(nullptr, 0, ".dat"), LocoFileType::objectData);
    EXPECT_EQ(DetectLocoFileType(nullptr, 0, ".xyz"), LocoFileType::unknown);
}

TEST(LocomotionTests, VehicleRideTypeMapping)
{
    // Train mapping
    EXPECT_EQ(MapLocoVehicleToRideType(LocoTransportType::train, 60), RIDE_TYPE_MINIATURE_RAILWAY);
    EXPECT_EQ(MapLocoVehicleToRideType(LocoTransportType::train, 125), RIDE_TYPE_MONORAIL);

    // Road vehicle mapping
    EXPECT_EQ(MapLocoVehicleToRideType(LocoTransportType::bus, 50), RIDE_TYPE_CAR_RIDE);
    EXPECT_EQ(MapLocoVehicleToRideType(LocoTransportType::truck, 45), RIDE_TYPE_CAR_RIDE);

    // Tram, aircraft, ship mapping
    EXPECT_EQ(MapLocoVehicleToRideType(LocoTransportType::tram, 30), RIDE_TYPE_MINIATURE_RAILWAY);
    EXPECT_EQ(MapLocoVehicleToRideType(LocoTransportType::aircraft, 200), RIDE_TYPE_MINI_HELICOPTERS);
    EXPECT_EQ(MapLocoVehicleToRideType(LocoTransportType::ship, 25), RIDE_TYPE_BOAT_HIRE);
}

TEST(LocomotionTests, CargoNames)
{
    EXPECT_EQ(GetLocoCargoName(LocoCargoType::passengers), "Passengers");
    EXPECT_EQ(GetLocoCargoName(LocoCargoType::coal), "Coal");
    EXPECT_EQ(GetLocoCargoName(LocoCargoType::ironOre), "Iron Ore");
    EXPECT_EQ(GetLocoCargoName(LocoCargoType::steel), "Steel");
    EXPECT_EQ(GetLocoCargoName(LocoCargoType::wood), "Wood");
    EXPECT_EQ(GetLocoCargoName(LocoCargoType::goods), "Goods");
    EXPECT_EQ(GetLocoCargoName(LocoCargoType::grain), "Grain");
    EXPECT_EQ(GetLocoCargoName(LocoCargoType::food), "Food");
    EXPECT_EQ(GetLocoCargoName(LocoCargoType::oil), "Oil");
    EXPECT_EQ(GetLocoCargoName(LocoCargoType::chemicals), "Chemicals");
}

TEST(LocomotionTests, ConsistPhysics)
{
    LocoConsist train;
    train.consistName = "Class 37 Freight";

    // Add diesel locomotive
    LocoVehicleUnit loco;
    loco.name = "Class 37";
    loco.powerHorsepower = 1750;
    loco.weightTons = 100;
    loco.maxSpeedMph = 90;
    loco.isPowered = true;
    train.units.push_back(loco);

    // Add 4 freight hoppers
    for (int i = 0; i < 4; ++i)
    {
        LocoVehicleUnit wagon;
        wagon.name = "Mineral Wagon";
        wagon.powerHorsepower = 0;
        wagon.weightTons = 30;
        wagon.maxSpeedMph = 90;
        wagon.isPowered = false;
        train.units.push_back(wagon);
    }

    EXPECT_EQ(train.GetTotalPower(), 1750u);
    EXPECT_EQ(train.GetTotalWeight(), 220u); // 100 + 4 * 30
    EXPECT_EQ(train.GetMaxSpeed(), 90u);

    // Starting acceleration on level grade (0%) at 10 mph
    float accelLevel = CalculateConsistAcceleration(train, 10.0f, 0);
    EXPECT_GT(accelLevel, 0.0f);

    // Acceleration on steep uphill grade (+4%) should be noticeably less
    float accelUphill = CalculateConsistAcceleration(train, 10.0f, 4);
    EXPECT_LT(accelUphill, accelLevel);

    // Acceleration at or beyond max speed should be 0
    float accelAtTopSpeed = CalculateConsistAcceleration(train, 90.0f, 0);
    EXPECT_LE(accelAtTopSpeed, 0.0f);
}

TEST(LocomotionTests, StandardIndustriesChain)
{
    const auto& industries = GetLocoBridge().GetIndustries();
    EXPECT_GE(industries.size(), 10u);

    // Find Coal Mine and Steel Mill
    bool foundCoalMine = false;
    bool foundSteelMill = false;

    for (const auto& ind : industries)
    {
        if (ind.name == "Coal Mine")
        {
            foundCoalMine = true;
            EXPECT_TRUE(ind.isExtractive);
            EXPECT_EQ(ind.producedCargos.size(), 1u);
            EXPECT_EQ(ind.producedCargos[0], LocoCargoType::coal);
        }
        else if (ind.name == "Steel Mill")
        {
            foundSteelMill = true;
            EXPECT_FALSE(ind.isExtractive);
            EXPECT_EQ(ind.producedCargos.size(), 1u);
            EXPECT_EQ(ind.producedCargos[0], LocoCargoType::steel);
        }
    }

    EXPECT_TRUE(foundCoalMine);
    EXPECT_TRUE(foundSteelMill);
}
