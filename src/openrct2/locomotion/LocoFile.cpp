/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers / NeXTycoon
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "LocoFile.h"

#include "../core/FileStream.h"
#include "../core/MemoryStream.h"
#include "../core/String.hpp"
#include "../sawyer_coding/SawyerChunkReader.h"

#include <algorithm>
#include <cstring>

namespace OpenRCT2::Locomotion
{
    using namespace OpenRCT2::SawyerCoding;

    bool IsLocoFile(const uint8_t* data, size_t length)
    {
        if (data == nullptr || length < sizeof(ChunkHeader) + 16)
        {
            return false;
        }

        // Locomotion saves begin with standard Sawyer chunks (ChunkEncoding::rle or ChunkEncoding::rleCompressed)
        const auto* header = reinterpret_cast<const ChunkHeader*>(data);
        if (header->encoding != ChunkEncoding::none &&
            header->encoding != ChunkEncoding::rle &&
            header->encoding != ChunkEncoding::rleCompressed &&
            header->encoding != ChunkEncoding::rotate)
        {
            return false;
        }

        // Validate reasonable uncompressed chunk length
        if (header->length == 0 || header->length > 64 * 1024 * 1024)
        {
            return false;
        }

        return true;
    }

    LocoFileType DetectLocoFileType(const uint8_t* data, size_t length, std::string_view extension)
    {
        if (!extension.empty())
        {
            if (String::iequals(extension, ".sv5"))
                return LocoFileType::savedGame;
            if (String::iequals(extension, ".sc5"))
                return LocoFileType::scenario;
            if (String::iequals(extension, ".lc5"))
                return LocoFileType::landscape;
            if (String::iequals(extension, ".dat"))
                return LocoFileType::objectData;
        }

        if (!IsLocoFile(data, length))
        {
            return LocoFileType::unknown;
        }

        // Default to saved game if format passes Sawyer validation
        return LocoFileType::savedGame;
    }

    std::optional<LocoSaveHeader> ReadLocoSaveHeader(IStream& stream)
    {
        try
        {
            SawyerChunkReader reader(&stream);
            auto chunk = reader.ReadChunk();
            if (chunk == nullptr || chunk->GetLength() < sizeof(LocoSaveHeader))
            {
                return std::nullopt;
            }

            LocoSaveHeader hdr = {};
            const auto* src = static_cast<const uint8_t*>(chunk->GetData());
            std::memcpy(&hdr, src, (std::min)(sizeof(LocoSaveHeader), chunk->GetLength()));

            // Ensure null termination of strings
            hdr.scenarioName[sizeof(hdr.scenarioName) - 1] = '\0';
            hdr.scenarioDetails[sizeof(hdr.scenarioDetails) - 1] = '\0';

            return hdr;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::vector<std::string> GetLocoRequiredObjects(IStream& stream)
    {
        std::vector<std::string> objectNames;
        try
        {
            SawyerChunkReader reader(&stream);

            // Chunk 0: Header
            auto headerChunk = reader.ReadChunk();
            if (headerChunk == nullptr)
                return objectNames;

            // Chunk 1: Packed Objects table in Locomotion saves
            auto objectsChunk = reader.ReadChunk();
            if (objectsChunk != nullptr && objectsChunk->GetLength() >= sizeof(LocoObjectHeader))
            {
                const auto* ptr = static_cast<const uint8_t*>(objectsChunk->GetData());
                size_t remaining = objectsChunk->GetLength();

                while (remaining >= sizeof(LocoObjectHeader))
                {
                    const auto* objHeader = reinterpret_cast<const LocoObjectHeader*>(ptr);
                    char nameBuf[9] = {};
                    std::memcpy(nameBuf, objHeader->name, 8);
                    nameBuf[8] = '\0';

                    // Trim trailing whitespace
                    std::string trimmed = nameBuf;
                    while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\0'))
                    {
                        trimmed.pop_back();
                    }

                    if (!trimmed.empty())
                    {
                        objectNames.push_back(trimmed);
                    }

                    ptr += sizeof(LocoObjectHeader);
                    remaining -= sizeof(LocoObjectHeader);
                }
            }
        }
        catch (...)
        {
            // Return any objects extracted before EOF/error
        }
        return objectNames;
    }

} // namespace OpenRCT2::Locomotion
