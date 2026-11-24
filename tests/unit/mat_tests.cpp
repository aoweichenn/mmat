// Copyright 2026 mcnn authors
// SPDX-License-Identifier: BSD-3-Clause

#include <cstddef>
#include <cstdint>

#include <vector>

#include <gtest/gtest.h>
#include <mmat/mat.hpp>
#include <mmemory/allocator.hpp>

namespace {

mmat::Mat make_1d(std::int32_t w) {
    mmat::Mat mat;
    mat.create(w, 4U, static_cast<mmemory::Allocator*>(nullptr));
    return mat;
}

mmat::Mat make_3d() {
    return mmat::Mat(4, 3, 2, 4U, static_cast<mmemory::Allocator*>(nullptr));
}

mmat::Mat make_4d() {
    return mmat::Mat(4, 3, 2, 2, 4U, static_cast<mmemory::Allocator*>(nullptr));
}

}  // namespace

TEST(Mat, DefaultIsEmpty) {
    const mmat::Mat mat;
    EXPECT_TRUE(mat.empty());
    EXPECT_EQ(mat.dims, 0);
    EXPECT_EQ(mat.w, 0);
    EXPECT_EQ(mat.elemsize, 0U);
    EXPECT_EQ(mat.elempack, 0);
    EXPECT_EQ(mat.data, nullptr);
    EXPECT_EQ(mat.refcount, nullptr);
    EXPECT_EQ(mat.allocator, nullptr);
}

TEST(Mat, OneTwoThreeFourDimensions) {
    const mmat::Mat one = make_1d(4);
    EXPECT_EQ(one.dims, 1);
    EXPECT_EQ(one.w, 4);
    EXPECT_EQ(one.total(), 4U);

    mmat::Mat two;
    two.create(4, 3, static_cast<std::size_t>(4U), static_cast<mmemory::Allocator*>(nullptr));
    EXPECT_EQ(two.dims, 2);
    EXPECT_EQ(two.h, 3);
    EXPECT_EQ(two.total(), 12U);

    const mmat::Mat three = make_3d();
    EXPECT_EQ(three.dims, 3);
    EXPECT_EQ(three.c, 2);
    EXPECT_EQ(three.total(), 24U);

    const mmat::Mat four = make_4d();
    EXPECT_EQ(four.dims, 4);
    EXPECT_EQ(four.d, 2);
    EXPECT_EQ(four.c, 2);
    EXPECT_EQ(four.total(), 48U);
}

TEST(Mat, CopySharesStorage) {
    mmat::Mat a = make_1d(16);
    ASSERT_FALSE(a.empty());
    ASSERT_NE(a.refcount, nullptr);
    EXPECT_EQ(*a.refcount, 1);

    mmat::Mat b(a);
    EXPECT_EQ(b.data, a.data);
    EXPECT_EQ(b.refcount, a.refcount);
    EXPECT_EQ(*a.refcount, 2);

    b.release();
    EXPECT_EQ(*a.refcount, 1);
    a.release();
    EXPECT_TRUE(a.empty());
}

TEST(Mat, CloneIsDeepCopy) {
    mmat::Mat a = make_1d(16);
    ASSERT_FALSE(a.empty());
    for (std::size_t index = 0; index < 16; ++index) {
        a[index] = static_cast<float>(index);
    }

    const mmat::Mat b = a.clone();
    EXPECT_NE(b.data, a.data);
    EXPECT_EQ(b.total(), a.total());
    for (std::size_t index = 0; index < 16; ++index) {
        EXPECT_EQ(b[index], static_cast<float>(index));
    }
}

TEST(Mat, CloneFromReplacesContents) {
    const mmat::Mat source = make_1d(4);
    mmat::Mat target;
    target.clone_from(source);
    EXPECT_FALSE(target.empty());
    EXPECT_EQ(target.total(), 4U);
    EXPECT_NE(target.data, source.data);
}

TEST(Mat, CreateLikeCopiesShapeAndType) {
    const mmat::Mat shape_source = make_3d();
    mmat::Mat target;
    target.create_like(shape_source);
    ASSERT_FALSE(target.empty());
    EXPECT_EQ(target.dims, shape_source.dims);
    EXPECT_EQ(target.w, shape_source.w);
    EXPECT_EQ(target.h, shape_source.h);
    EXPECT_EQ(target.c, shape_source.c);
    EXPECT_EQ(target.elemsize, shape_source.elemsize);
    EXPECT_EQ(target.elempack, shape_source.elempack);
    EXPECT_EQ(target.total(), shape_source.total());
    EXPECT_NE(target.data, shape_source.data);
}

TEST(Mat, ElemsizeBitsPerElement) {
    const mmat::Mat packed(8, static_cast<std::size_t>(4U), 4, static_cast<mmemory::Allocator*>(nullptr));
    EXPECT_EQ(packed.elembits(), 8);

    mmat::Mat half;
    half.create(8, static_cast<std::size_t>(2U), 1, static_cast<mmemory::Allocator*>(nullptr));
    EXPECT_EQ(half.elembits(), 16);
}

TEST(Mat, ReshapePreservesStorage) {
    mmat::Mat a = make_1d(24);
    ASSERT_FALSE(a.empty());

    const mmat::Mat b = a.reshape(4, 6);
    EXPECT_EQ(b.dims, 2);
    EXPECT_EQ(b.w, 4);
    EXPECT_EQ(b.h, 6);
    EXPECT_EQ(b.data, a.data);
    EXPECT_EQ(b.refcount, a.refcount);

    const mmat::Mat bad = a.reshape(5, 5);
    EXPECT_TRUE(bad.empty());
}

TEST(Mat, ChannelRowDepthAndRangeViews) {
    mmat::Mat a = make_4d();
    ASSERT_FALSE(a.empty());

    EXPECT_EQ(a.channel(1).data, static_cast<std::uint8_t*>(a.data) + a.cstep * 4U);
    EXPECT_EQ(a.depth(1).d, 1);
    EXPECT_EQ(a.depth_range(1, 1).d, 1);
    EXPECT_EQ(a.row(1), static_cast<float*>(a.data) + 4);
    EXPECT_EQ(a.channel_range(1, 1).c, 1);
    EXPECT_EQ(a.row_range(1, 1).h, 1);
    EXPECT_EQ(a.range(1, 2).w, 2);
}

TEST(Mat, FillWritesEveryElement) {
    mmat::Mat a = make_1d(8);
    a.fill(3.5F);
    for (std::size_t index = 0; index < 8; ++index) {
        EXPECT_EQ(a[index], 3.5F);
    }

    mmat::Mat b = make_1d(8);
    b.fill(7);
    for (std::int32_t index = 0; index < 8; ++index) {
        EXPECT_EQ(reinterpret_cast<const std::int32_t*>(b.data)[index], 7);
    }

    mmat::Mat c = make_1d(8);
    c.fill<std::int32_t>(9);
    for (std::int32_t index = 0; index < 8; ++index) {
        EXPECT_EQ(reinterpret_cast<const std::int32_t*>(c.data)[index], 9);
    }
}

TEST(Mat, ExternalDataIsNotOwned) {
    std::vector<float> values(8, 1.0F);
    {
        mmat::Mat mat(8, values.data(), 4U, nullptr);
        EXPECT_EQ(mat.data, values.data());
        EXPECT_EQ(mat.refcount, nullptr);
        EXPECT_FALSE(mat.empty());
    }
    EXPECT_EQ(values[0], 1.0F);
}

TEST(Mat, ShapeReturnsMetadataOnly) {
    mmat::Mat a = make_3d();
    const mmat::Mat shape = a.shape();
    EXPECT_EQ(shape.dims, a.dims);
    EXPECT_EQ(shape.w, a.w);
    EXPECT_EQ(shape.h, a.h);
    EXPECT_EQ(shape.c, a.c);
    EXPECT_EQ(shape.elemsize, a.elemsize);
    EXPECT_EQ(shape.elempack, a.elempack);
    EXPECT_EQ(shape.data, nullptr);
    EXPECT_EQ(shape.refcount, nullptr);
}

TEST(Mat, PoolAllocatorBackedStorage) {
    mmemory::PoolAllocator pool;
    {
        mmat::Mat mat(32, 4U, &pool);
        ASSERT_FALSE(mat.empty());
        EXPECT_EQ(mat.allocator, &pool);
        EXPECT_EQ(pool.outstanding(), 1U);
    }
    EXPECT_EQ(pool.outstanding(), 0U);
}

TEST(Mat, AddrefIncrementsSharedCount) {
    mmat::Mat a = make_1d(4);
    ASSERT_NE(a.refcount, nullptr);
    EXPECT_EQ(*a.refcount, 1);

    // addref() is how legacy code records an extra owner that will later be
    // released through a borrowed Mat handle. Model that second handle here.
    a.addref();
    EXPECT_EQ(*a.refcount, 2);
    mmat::Mat borrowed;
    borrowed.data = a.data;
    borrowed.refcount = a.refcount;
    borrowed.allocator = a.allocator;
    borrowed.elemsize = a.elemsize;
    borrowed.elempack = a.elempack;
    borrowed.release();
    EXPECT_EQ(*a.refcount, 1);
    a.release();
    EXPECT_TRUE(a.empty());
}

#if MMAT_BATCH
TEST(Mat, BatchAndBatchRangeViews) {
    mmat::Mat a;
    a.create(4, static_cast<std::size_t>(4U), 1, 3, static_cast<mmemory::Allocator*>(nullptr));
    ASSERT_FALSE(a.empty());
    EXPECT_EQ(a.n, 3);
    EXPECT_EQ(a.batch(1).n, 1);
    EXPECT_EQ(a.batch(1).data, static_cast<std::uint8_t*>(a.data) + a.nstep * 4U);
    EXPECT_EQ(a.batch_range(1, 2).n, 2);
}
#endif
