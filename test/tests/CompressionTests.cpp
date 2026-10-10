/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers / NeXTycoon
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <cstring>
#include <gtest/gtest.h>
#include <openrct2/core/Compression.h>
#include <openrct2/core/MemoryStream.h>
#include <random>
#include <vector>

using namespace OpenRCT2;

namespace
{
    enum class DataKind
    {
        compressible,
        incompressible,
    };

    std::vector<uint8_t> MakeData(size_t size, DataKind kind)
    {
        std::vector<uint8_t> data(size);
        std::mt19937 rng(0xC0FFEEu + static_cast<uint32_t>(size));
        if (kind == DataKind::incompressible)
        {
            for (auto& b : data)
                b = static_cast<uint8_t>(rng());
        }
        else
        {
            // Runs of repeated bytes with occasional noise, similar to tile/entity data
            size_t i = 0;
            while (i < size)
            {
                const auto value = static_cast<uint8_t>(rng() % 16);
                const size_t run = 1 + (rng() % 64);
                for (size_t j = 0; j < run && i < size; j++, i++)
                    data[i] = value;
            }
        }
        return data;
    }

    struct CompressionCase
    {
        size_t size;
        DataKind kind;
    };

    // Sizes chosen around zstd's 128 KiB block size and buffer boundaries
    const CompressionCase kCases[] = {
        { 1, DataKind::compressible },
        { 1000, DataKind::compressible },
        { 128 * 1024 - 1, DataKind::compressible },
        { 128 * 1024, DataKind::compressible },
        { 128 * 1024 + 1, DataKind::compressible },
        { 128 * 1024 - 1, DataKind::incompressible },
        { 128 * 1024, DataKind::incompressible },
        { 128 * 1024 + 1, DataKind::incompressible },
        { 3 * 128 * 1024 + 17, DataKind::incompressible },
        { 1024 * 1024 + 7, DataKind::compressible },
        { 5 * 1024 * 1024 + 3, DataKind::compressible },
    };

    MemoryStream Compress(const std::vector<uint8_t>& data, bool useZstd, Compression::ZstdMetadata meta)
    {
        MemoryStream src(data.data(), data.size());
        MemoryStream dst;
        bool ok = useZstd ? Compression::zstdCompress(src, data.size(), dst, meta)
                          : Compression::zlibCompress(src, data.size(), dst, Compression::ZlibHeaderType::gzip);
        EXPECT_TRUE(ok);
        dst.SetPosition(0);
        return dst;
    }

    bool Decompress(MemoryStream& compressed, uint64_t expectedLength, bool useZstd, std::vector<uint8_t>& out)
    {
        compressed.SetPosition(0);
        MemoryStream dst;
        bool ok = useZstd
            ? Compression::zstdDecompress(compressed, compressed.GetLength(), dst, expectedLength)
            : Compression::zlibDecompress(
                  compressed, compressed.GetLength(), dst, expectedLength, Compression::ZlibHeaderType::gzip);
        out.assign(
            static_cast<const uint8_t*>(dst.GetData()), static_cast<const uint8_t*>(dst.GetData()) + dst.GetLength());
        return ok;
    }
} // namespace

TEST(CompressionTests, ZstdRoundTripWithoutMetadata)
{
    // This is exactly how OrcaStream (park files / network maps) uses zstd
    for (const auto& c : kCases)
    {
        SCOPED_TRACE(testing::Message() << "size=" << c.size << " kind=" << static_cast<int>(c.kind));
        auto data = MakeData(c.size, c.kind);
        auto compressed = Compress(data, true, Compression::ZstdMetadata::none);
        std::vector<uint8_t> out;
        ASSERT_TRUE(Decompress(compressed, data.size(), true, out));
        ASSERT_EQ(out.size(), data.size());
        EXPECT_EQ(0, std::memcmp(out.data(), data.data(), data.size()));
    }
}

TEST(CompressionTests, ZstdRoundTripWithMetadata)
{
    for (const auto& c : kCases)
    {
        SCOPED_TRACE(testing::Message() << "size=" << c.size << " kind=" << static_cast<int>(c.kind));
        auto data = MakeData(c.size, c.kind);
        auto compressed = Compress(data, true, Compression::ZstdMetadata::both);
        std::vector<uint8_t> out;
        ASSERT_TRUE(Decompress(compressed, data.size(), true, out));
        ASSERT_EQ(out.size(), data.size());
        EXPECT_EQ(0, std::memcmp(out.data(), data.data(), data.size()));
    }
}

TEST(CompressionTests, ZstdRejectsWrongExpectedLength)
{
    auto data = MakeData(300 * 1024, DataKind::compressible);
    auto compressed = Compress(data, true, Compression::ZstdMetadata::none);
    std::vector<uint8_t> out;
    EXPECT_FALSE(Decompress(compressed, data.size() - 1, true, out));
    EXPECT_FALSE(Decompress(compressed, data.size() + 1, true, out));
}

TEST(CompressionTests, GzipRoundTrip)
{
    for (const auto& c : kCases)
    {
        SCOPED_TRACE(testing::Message() << "size=" << c.size << " kind=" << static_cast<int>(c.kind));
        auto data = MakeData(c.size, c.kind);
        auto compressed = Compress(data, false, Compression::ZstdMetadata::none);
        std::vector<uint8_t> out;
        ASSERT_TRUE(Decompress(compressed, data.size(), false, out));
        ASSERT_EQ(out.size(), data.size());
        EXPECT_EQ(0, std::memcmp(out.data(), data.data(), data.size()));
    }
}

TEST(CompressionTests, GzipRejectsWrongExpectedLength)
{
    auto data = MakeData(300 * 1024, DataKind::compressible);
    auto compressed = Compress(data, false, Compression::ZstdMetadata::none);
    std::vector<uint8_t> out;
    EXPECT_FALSE(Decompress(compressed, data.size() - 1, false, out));
    EXPECT_FALSE(Decompress(compressed, data.size() + 1, false, out));
}
