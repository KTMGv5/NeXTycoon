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
#include "../core/IStream.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace OpenRCT2::Locomotion
{
    /**
     * Determines whether the given raw data buffer represents a Chris Sawyer's
     * Locomotion file (.SV5 saved game, .SC5 scenario, or .LC5 landscape).
     */
    [[nodiscard]] bool IsLocoFile(const uint8_t* data, size_t length);

    /**
     * Detects the specific Locomotion file type from buffer content and extension hints.
     */
    [[nodiscard]] LocoFileType DetectLocoFileType(const uint8_t* data, size_t length, std::string_view extension = {});

    /**
     * Reads and unpacks the header metadata of a Locomotion .SV5/.SC5 save stream.
     */
    [[nodiscard]] std::optional<LocoSaveHeader> ReadLocoSaveHeader(IStream& stream);

    /**
     * Extracts list of required Locomotion object DAT names from a save/scenario stream.
     */
    [[nodiscard]] std::vector<std::string> GetLocoRequiredObjects(IStream& stream);

} // namespace OpenRCT2::Locomotion
