// Copyright 2017 Tencent
// Copyright 2026 mcnn authors
// SPDX-License-Identifier: BSD-3-Clause

#include <mmat/mat.hpp>

#include <cstring>

namespace mmat {

namespace {

[[maybe_unused]] constexpr std::int32_t kDims3 = 3;
[[maybe_unused]] constexpr std::int32_t kDims4 = 4;
[[maybe_unused]] constexpr std::size_t kCstepAlignment = 16;
[[maybe_unused]] constexpr std::size_t kRefcountAlignment = 4;
[[maybe_unused]] constexpr std::size_t kBatchStrideAlignment = 4096;
[[maybe_unused]] constexpr std::int32_t kBitsPerByte = 8;

}  // namespace

#if MMAT_BATCH
static Mat reshape_batch(const Mat& bottom_blob, std::int32_t dims, std::int32_t w, std::int32_t h, std::int32_t d,
                         std::int32_t c, mmemory::Allocator* allocator) {
    Mat top_blob;
    for (std::int32_t b = 0; b < bottom_blob.n; b++) {
        Mat bottom_blob_b = bottom_blob.batch(b);
        Mat top_blob_b;
        if (dims == 1) {
            top_blob_b = bottom_blob_b.reshape(w, allocator);
        } else if (dims == 2) {
            top_blob_b = bottom_blob_b.reshape(w, h, allocator);
        } else if (dims == kDims3) {
            top_blob_b = bottom_blob_b.reshape(w, h, c, allocator);
        } else {  // if (dims == kDims4)
            top_blob_b = bottom_blob_b.reshape(w, h, d, c, allocator);
        }

        if (top_blob_b.empty()) {
            return Mat();
        }

        if (b == 0) {
            top_blob.create_like(top_blob_b, bottom_blob.n, allocator);
            if (top_blob.empty()) {
                return top_blob;
            }
        }

        memcpy(top_blob.batch(b), top_blob_b, top_blob_b.total() * top_blob_b.elemsize);
    }

    return top_blob;
}
#endif  // MMAT_BATCH

Mat Mat::clone(mmemory::Allocator* _allocator) const {
    if (empty()) {
        return Mat();
    }

    Mat m;
#if MMAT_BATCH
    if (n > 1) {
        m.create_like(*this, n, _allocator);

        if (m.empty()) {
            return m;
        }

        // copy batch by batch (nstep may include 4K padding)
        std::size_t single_batch_size = total() * elemsize;
        for (std::int32_t b = 0; b < n; b++) {
            memcpy(m.batch(b), batch(b), single_batch_size);
        }

        return m;
    }
#endif  // MMAT_BATCH

    if (dims == 1) {
        m.create(w, elemsize, elempack, _allocator);
    } else if (dims == 2) {
        m.create(w, h, elemsize, elempack, _allocator);
    } else if (dims == kDims3) {
        m.create(w, h, c, elemsize, elempack, _allocator);
    } else if (dims == kDims4) {
        m.create(w, h, d, c, elemsize, elempack, _allocator);
    }

    if (m.empty()) {
        return m;
    }

    if (total() > 0) {
        if (cstep == m.cstep) {
            memcpy(m.data, data, total() * elemsize);
        } else {
            // copy by channel for differnet cstep
            std::size_t size = (std::size_t)w * h * d * elemsize;
            for (std::int32_t i = 0; i < c; i++) {
                memcpy(m.channel(i), channel(i), size);
            }
        }
    }

    return m;
}

void Mat::clone_from(const mmat::Mat& mat, mmemory::Allocator* allocator) {
    *this = mat.clone(allocator);
}

Mat Mat::reshape(std::int32_t _w, mmemory::Allocator* _allocator) const {
    if (w * h * d * c != _w) {
        return Mat();
    }

    if (dims >= kDims3 && cstep != (std::size_t)w * h * d) {
#if MMAT_BATCH
        if (n > 1) {
            return reshape_batch(*this, 1, _w, 0, 0, 0, _allocator);
        }
#endif

        Mat m;
        m.create(_w, elemsize, elempack, _allocator);
        if (m.empty()) {
            return m;
        }

        // flatten
        for (std::int32_t i = 0; i < c; i++) {
            const void* ptr = (std::uint8_t*)data + i * cstep * elemsize;
            void* mptr = (std::uint8_t*)m.data + i * (std::size_t)w * h * d * elemsize;
            memcpy(mptr, ptr, (std::size_t)w * h * d * elemsize);
        }

        return m;
    }

    Mat m = *this;

    m.dims = 1;
    m.w = _w;
    m.h = 1;
    m.d = 1;
    m.c = 1;

    m.cstep = mmemory::align_size(_w * elemsize, kCstepAlignment) / elemsize;

    return m;
}

Mat Mat::reshape(std::int32_t _w, std::int32_t _h, mmemory::Allocator* _allocator) const {
    if (w * h * d * c != _w * _h) {
        return Mat();
    }

    if (dims >= kDims3 && cstep != (std::size_t)w * h * d) {
#if MMAT_BATCH
        if (n > 1) {
            return reshape_batch(*this, 2, _w, _h, 0, 0, _allocator);
        }
#endif

        Mat m;
        m.create(_w, _h, elemsize, elempack, _allocator);
        if (m.empty()) {
            return m;
        }

        // flatten
        for (std::int32_t i = 0; i < c; i++) {
            const void* ptr = (std::uint8_t*)data + i * cstep * elemsize;
            void* mptr = (std::uint8_t*)m.data + i * (std::size_t)w * h * d * elemsize;
            memcpy(mptr, ptr, (std::size_t)w * h * d * elemsize);
        }

        return m;
    }

    Mat m = *this;

    m.dims = 2;
    m.w = _w;
    m.h = _h;
    m.d = 1;
    m.c = 1;

    m.cstep = mmemory::align_size((std::size_t)_w * _h * elemsize, kCstepAlignment) / elemsize;

    return m;
}

Mat Mat::reshape(std::int32_t _w, std::int32_t _h, std::int32_t _c, mmemory::Allocator* _allocator) const {
    if (w * h * d * c != _w * _h * _c) {
        return Mat();
    }

    if (dims < kDims3) {
        if ((std::size_t)_w * _h != mmemory::align_size((std::size_t)_w * _h * elemsize, kCstepAlignment) / elemsize) {
#if MMAT_BATCH
            if (n > 1) {
                return reshape_batch(*this, kDims3, _w, _h, 0, _c, _allocator);
            }
#endif

            Mat m;
            m.create(_w, _h, _c, elemsize, elempack, _allocator);
            if (m.empty()) {
                return m;
            }

            // align channel
            for (std::int32_t i = 0; i < _c; i++) {
                const void* ptr = (std::uint8_t*)data + i * (std::size_t)_w * _h * elemsize;
                void* mptr = (std::uint8_t*)m.data + i * m.cstep * m.elemsize;
                memcpy(mptr, ptr, (std::size_t)_w * _h * elemsize);
            }

            return m;
        }
    } else if (c != _c) {
        // flatten and then align
        Mat tmp = reshape(_w * _h * _c, _allocator);
        if (tmp.empty()) {
            return tmp;
        }
        return tmp.reshape(_w, _h, _c, _allocator);
    }

    Mat m = *this;

    m.dims = kDims3;
    m.w = _w;
    m.h = _h;
    m.d = 1;
    m.c = _c;

    m.cstep = mmemory::align_size((std::size_t)_w * _h * elemsize, kCstepAlignment) / elemsize;

    return m;
}

Mat Mat::reshape(std::int32_t _w, std::int32_t _h, std::int32_t _d, std::int32_t _c,
                 mmemory::Allocator* _allocator) const {
    if (w * h * d * c != _w * _h * _d * _c) {
        return Mat();
    }

    if (dims < kDims3) {
        if ((std::size_t)_w * _h * _d !=
            mmemory::align_size((std::size_t)_w * _h * _d * elemsize, kCstepAlignment) / elemsize) {
#if MMAT_BATCH
            if (n > 1) {
                return reshape_batch(*this, kDims4, _w, _h, _d, _c, _allocator);
            }
#endif

            Mat m;
            m.create(_w, _h, _d, _c, elemsize, elempack, _allocator);
            if (m.empty()) {
                return m;
            }

            // align channel
            for (std::int32_t i = 0; i < _c; i++) {
                const void* ptr = (std::uint8_t*)data + i * (std::size_t)_w * _h * _d * elemsize;
                void* mptr = (std::uint8_t*)m.data + i * m.cstep * m.elemsize;
                memcpy(mptr, ptr, (std::size_t)_w * _h * _d * elemsize);
            }

            return m;
        }
    } else if (c != _c) {
        // flatten and then align
        Mat tmp = reshape(_w * _h * _d * _c, _allocator);
        if (tmp.empty()) {
            return tmp;
        }
        return tmp.reshape(_w, _h, _d, _c, _allocator);
    }

    Mat m = *this;

    m.dims = kDims4;
    m.w = _w;
    m.h = _h;
    m.d = _d;
    m.c = _c;

    m.cstep = mmemory::align_size((std::size_t)_w * _h * _d * elemsize, kCstepAlignment) / elemsize;

    return m;
}

void Mat::create(std::int32_t _w, std::size_t _elemsize, mmemory::Allocator* _allocator) {
#if MMAT_BATCH
    if (dims == 1 && w == _w && elemsize == _elemsize && elempack == 1 && allocator == _allocator && n == 1)
#else
    if (dims == 1 && w == _w && elemsize == _elemsize && elempack == 1 && allocator == _allocator)
#endif
        return;

    release();

    elemsize = _elemsize;
    elempack = 1;
    allocator = _allocator;

    dims = 1;
    w = _w;
    h = 1;
    d = 1;
    c = 1;

    cstep = mmemory::align_size(w * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif

    std::size_t totalsize = mmemory::align_size(total() * elemsize, kRefcountAlignment);
    if (totalsize > 0) {
        if (allocator) {
            data = allocator->allocate(totalsize + (std::int32_t)sizeof(*refcount));
        } else {
            data = mmemory::fast_malloc(totalsize + (std::int32_t)sizeof(*refcount));
        }
    }

    if (data) {
        refcount = (std::int32_t*)(((std::uint8_t*)data) + totalsize);
        *refcount = 1;
    }
}

void Mat::create(std::int32_t _w, std::int32_t _h, std::size_t _elemsize, mmemory::Allocator* _allocator) {
#if MMAT_BATCH
    if (dims == 2 && w == _w && h == _h && elemsize == _elemsize && elempack == 1 && allocator == _allocator && n == 1)
#else
    if (dims == 2 && w == _w && h == _h && elemsize == _elemsize && elempack == 1 && allocator == _allocator)
#endif
        return;

    release();

    elemsize = _elemsize;
    elempack = 1;
    allocator = _allocator;

    dims = 2;
    w = _w;
    h = _h;
    d = 1;
    c = 1;

    cstep = mmemory::align_size((std::size_t)w * h * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif

    std::size_t totalsize = mmemory::align_size(total() * elemsize, kRefcountAlignment);
    if (totalsize > 0) {
        if (allocator) {
            data = allocator->allocate(totalsize + (std::int32_t)sizeof(*refcount));
        } else {
            data = mmemory::fast_malloc(totalsize + (std::int32_t)sizeof(*refcount));
        }
    }

    if (data) {
        refcount = (std::int32_t*)(((std::uint8_t*)data) + totalsize);
        *refcount = 1;
    }
}

void Mat::create(std::int32_t _w, std::int32_t _h, std::int32_t _c, std::size_t _elemsize,
                 mmemory::Allocator* _allocator) {
#if MMAT_BATCH
    if (dims == kDims3 && w == _w && h == _h && c == _c && elemsize == _elemsize && elempack == 1 &&
        allocator == _allocator && n == 1)
#else
    if (dims == kDims3 && w == _w && h == _h && c == _c && elemsize == _elemsize && elempack == 1 &&
        allocator == _allocator)
#endif
        return;

    release();

    elemsize = _elemsize;
    elempack = 1;
    allocator = _allocator;

    dims = kDims3;
    w = _w;
    h = _h;
    d = 1;
    c = _c;

    cstep = mmemory::align_size((std::size_t)w * h * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif

    std::size_t totalsize = mmemory::align_size(total() * elemsize, kRefcountAlignment);
    if (totalsize > 0) {
        if (allocator) {
            data = allocator->allocate(totalsize + (std::int32_t)sizeof(*refcount));
        } else {
            data = mmemory::fast_malloc(totalsize + (std::int32_t)sizeof(*refcount));
        }
    }

    if (data) {
        refcount = (std::int32_t*)(((std::uint8_t*)data) + totalsize);
        *refcount = 1;
    }
}

void Mat::create(std::int32_t _w, std::int32_t _h, std::int32_t _d, std::int32_t _c, std::size_t _elemsize,
                 mmemory::Allocator* _allocator) {
#if MMAT_BATCH
    if (dims == kDims4 && w == _w && h == _h && d == _d && c == _c && elemsize == _elemsize && elempack == 1 &&
        allocator == _allocator && n == 1)
#else
    if (dims == kDims4 && w == _w && h == _h && d == _d && c == _c && elemsize == _elemsize && elempack == 1 &&
        allocator == _allocator)
#endif
        return;

    release();

    elemsize = _elemsize;
    elempack = 1;
    allocator = _allocator;

    dims = kDims4;
    w = _w;
    h = _h;
    d = _d;
    c = _c;

    cstep = mmemory::align_size((std::size_t)w * h * d * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif

    std::size_t totalsize = mmemory::align_size(total() * elemsize, kRefcountAlignment);
    if (totalsize > 0) {
        if (allocator) {
            data = allocator->allocate(totalsize + (std::int32_t)sizeof(*refcount));
        } else {
            data = mmemory::fast_malloc(totalsize + (std::int32_t)sizeof(*refcount));
        }
    }

    if (data) {
        refcount = (std::int32_t*)(((std::uint8_t*)data) + totalsize);
        *refcount = 1;
    }
}

void Mat::create(std::int32_t _w, std::size_t _elemsize, std::int32_t _elempack, mmemory::Allocator* _allocator) {
#if MMAT_BATCH
    if (dims == 1 && w == _w && elemsize == _elemsize && elempack == _elempack && allocator == _allocator && n == 1)
#else
    if (dims == 1 && w == _w && elemsize == _elemsize && elempack == _elempack && allocator == _allocator)
#endif
        return;

    release();

    elemsize = _elemsize;
    elempack = _elempack;
    allocator = _allocator;

    dims = 1;
    w = _w;
    h = 1;
    d = 1;
    c = 1;

    cstep = mmemory::align_size(w * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif

    std::size_t totalsize = mmemory::align_size(total() * elemsize, kRefcountAlignment);
    if (totalsize > 0) {
        if (allocator) {
            data = allocator->allocate(totalsize + (std::int32_t)sizeof(*refcount));
        } else {
            data = mmemory::fast_malloc(totalsize + (std::int32_t)sizeof(*refcount));
        }
    }

    if (data) {
        refcount = (std::int32_t*)(((std::uint8_t*)data) + totalsize);
        *refcount = 1;
    }
}

void Mat::create(std::int32_t _w, std::int32_t _h, std::size_t _elemsize, std::int32_t _elempack,
                 mmemory::Allocator* _allocator) {
#if MMAT_BATCH
    if (dims == 2 && w == _w && h == _h && elemsize == _elemsize && elempack == _elempack && allocator == _allocator &&
        n == 1)
#else
    if (dims == 2 && w == _w && h == _h && elemsize == _elemsize && elempack == _elempack && allocator == _allocator)
#endif
        return;

    release();

    elemsize = _elemsize;
    elempack = _elempack;
    allocator = _allocator;

    dims = 2;
    w = _w;
    h = _h;
    d = 1;
    c = 1;

    cstep = mmemory::align_size((std::size_t)w * h * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif

    std::size_t totalsize = mmemory::align_size(total() * elemsize, kRefcountAlignment);
    if (totalsize > 0) {
        if (allocator) {
            data = allocator->allocate(totalsize + (std::int32_t)sizeof(*refcount));
        } else {
            data = mmemory::fast_malloc(totalsize + (std::int32_t)sizeof(*refcount));
        }
    }

    if (data) {
        refcount = (std::int32_t*)(((std::uint8_t*)data) + totalsize);
        *refcount = 1;
    }
}

void Mat::create(std::int32_t _w, std::int32_t _h, std::int32_t _c, std::size_t _elemsize, std::int32_t _elempack,
                 mmemory::Allocator* _allocator) {
#if MMAT_BATCH
    if (dims == kDims3 && w == _w && h == _h && c == _c && elemsize == _elemsize && elempack == _elempack &&
        allocator == _allocator && n == 1)
#else
    if (dims == kDims3 && w == _w && h == _h && c == _c && elemsize == _elemsize && elempack == _elempack &&
        allocator == _allocator)
#endif
        return;

    release();

    elemsize = _elemsize;
    elempack = _elempack;
    allocator = _allocator;

    dims = kDims3;
    w = _w;
    h = _h;
    d = 1;
    c = _c;

    cstep = mmemory::align_size((std::size_t)w * h * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif

    std::size_t totalsize = mmemory::align_size(total() * elemsize, kRefcountAlignment);
    if (totalsize > 0) {
        if (allocator) {
            data = allocator->allocate(totalsize + (std::int32_t)sizeof(*refcount));
        } else {
            data = mmemory::fast_malloc(totalsize + (std::int32_t)sizeof(*refcount));
        }
    }

    if (data) {
        refcount = (std::int32_t*)(((std::uint8_t*)data) + totalsize);
        *refcount = 1;
    }
}

void Mat::create(std::int32_t _w, std::int32_t _h, std::int32_t _d, std::int32_t _c, std::size_t _elemsize,
                 std::int32_t _elempack, mmemory::Allocator* _allocator) {
#if MMAT_BATCH
    if (dims == kDims4 && w == _w && h == _h && d == _d && c == _c && elemsize == _elemsize && elempack == _elempack &&
        allocator == _allocator && n == 1)
#else
    if (dims == kDims4 && w == _w && h == _h && d == _d && c == _c && elemsize == _elemsize && elempack == _elempack &&
        allocator == _allocator)
#endif
        return;

    release();

    elemsize = _elemsize;
    elempack = _elempack;
    allocator = _allocator;

    dims = kDims4;
    w = _w;
    h = _h;
    d = _d;
    c = _c;

    cstep = mmemory::align_size((std::size_t)w * h * d * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif

    std::size_t totalsize = mmemory::align_size(total() * elemsize, kRefcountAlignment);
    if (totalsize > 0) {
        if (allocator) {
            data = allocator->allocate(totalsize + (std::int32_t)sizeof(*refcount));
        } else {
            data = mmemory::fast_malloc(totalsize + (std::int32_t)sizeof(*refcount));
        }
    }

    if (data) {
        refcount = (std::int32_t*)(((std::uint8_t*)data) + totalsize);
        *refcount = 1;
    }
}

void Mat::create_like(const Mat& m, mmemory::Allocator* _allocator) {
#if MMAT_BATCH
    if (m.n > 1) {
        create_like(m, m.n, _allocator);
        return;
    }
#endif

    std::int32_t _dims = m.dims;
    if (_dims == 1) {
        create(m.w, m.elemsize, m.elempack, _allocator);
    }
    if (_dims == 2) {
        create(m.w, m.h, m.elemsize, m.elempack, _allocator);
    }
    if (_dims == kDims3) {
        create(m.w, m.h, m.c, m.elemsize, m.elempack, _allocator);
    }
    if (_dims == kDims4) {
        create(m.w, m.h, m.d, m.c, m.elemsize, m.elempack, _allocator);
    }
}

#if MMAT_BATCH
void Mat::create_like(const Mat& m, std::int32_t _n, mmemory::Allocator* _allocator) {
    if (m.dims == 1) {
        create(m.w, m.elemsize, m.elempack, _n, _allocator);
    } else if (m.dims == 2) {
        create(m.w, m.h, m.elemsize, m.elempack, _n, _allocator);
    } else if (m.dims == kDims3) {
        create(m.w, m.h, m.c, m.elemsize, m.elempack, _n, _allocator);
    } else if (m.dims == kDims4) {
        create(m.w, m.h, m.d, m.c, m.elemsize, m.elempack, _n, _allocator);
    }
}

void Mat::create(std::int32_t _w, std::size_t _elemsize, std::int32_t _elempack, std::int32_t _n,
                 mmemory::Allocator* _allocator) {
    if (_n <= 1) {
        create(_w, _elemsize, _elempack, _allocator);
        return;
    }

    if (dims == 1 && w == _w && elemsize == _elemsize && elempack == _elempack && allocator == _allocator && n == _n) {
        return;
    }

    release();

    elemsize = _elemsize;
    elempack = _elempack;
    allocator = _allocator;

    dims = 1;
    w = _w;
    h = 1;
    d = 1;
    c = 1;
    n = _n;

    cstep = mmemory::align_size((std::size_t)w * elemsize, kCstepAlignment) / elemsize;
    nstep = mmemory::align_size(cstep * elemsize, kBatchStrideAlignment) / elemsize;

    std::size_t totalsize = mmemory::align_size(nstep * n * elemsize, kRefcountAlignment);
    if (totalsize > 0) {
        if (allocator) {
            data = allocator->allocate(totalsize + (std::int32_t)sizeof(*refcount));
        } else {
            data = mmemory::fast_malloc(totalsize + (std::int32_t)sizeof(*refcount));
        }
    }

    if (data) {
        refcount = (std::int32_t*)(((std::uint8_t*)data) + totalsize);
        *refcount = 1;
    }
}

void Mat::create(std::int32_t _w, std::int32_t _h, std::size_t _elemsize, std::int32_t _elempack, std::int32_t _n,
                 mmemory::Allocator* _allocator) {
    if (_n <= 1) {
        create(_w, _h, _elemsize, _elempack, _allocator);
        return;
    }

    if (dims == 2 && w == _w && h == _h && elemsize == _elemsize && elempack == _elempack && allocator == _allocator &&
        n == _n) {
        return;
    }

    release();

    elemsize = _elemsize;
    elempack = _elempack;
    allocator = _allocator;

    dims = 2;
    w = _w;
    h = _h;
    d = 1;
    c = 1;
    n = _n;

    cstep = mmemory::align_size((std::size_t)w * h * elemsize, kCstepAlignment) / elemsize;
    nstep = mmemory::align_size(cstep * elemsize, kBatchStrideAlignment) / elemsize;

    std::size_t totalsize = mmemory::align_size(nstep * n * elemsize, kRefcountAlignment);
    if (totalsize > 0) {
        if (allocator) {
            data = allocator->allocate(totalsize + (std::int32_t)sizeof(*refcount));
        } else {
            data = mmemory::fast_malloc(totalsize + (std::int32_t)sizeof(*refcount));
        }
    }

    if (data) {
        refcount = (std::int32_t*)(((std::uint8_t*)data) + totalsize);
        *refcount = 1;
    }
}

void Mat::create(std::int32_t _w, std::int32_t _h, std::int32_t _c, std::size_t _elemsize, std::int32_t _elempack,
                 std::int32_t _n, mmemory::Allocator* _allocator) {
    if (_n <= 1) {
        create(_w, _h, _c, _elemsize, _elempack, _allocator);
        return;
    }

    if (dims == kDims3 && w == _w && h == _h && c == _c && elemsize == _elemsize && elempack == _elempack &&
        allocator == _allocator && n == _n) {
        return;
    }

    release();

    elemsize = _elemsize;
    elempack = _elempack;
    allocator = _allocator;

    dims = kDims3;
    w = _w;
    h = _h;
    d = 1;
    c = _c;
    n = _n;

    cstep = mmemory::align_size((std::size_t)w * h * elemsize, kCstepAlignment) / elemsize;
    nstep = mmemory::align_size(cstep * c * elemsize, kBatchStrideAlignment) / elemsize;

    std::size_t totalsize = mmemory::align_size(nstep * n * elemsize, kRefcountAlignment);
    if (totalsize > 0) {
        if (allocator) {
            data = allocator->allocate(totalsize + (std::int32_t)sizeof(*refcount));
        } else {
            data = mmemory::fast_malloc(totalsize + (std::int32_t)sizeof(*refcount));
        }
    }

    if (data) {
        refcount = (std::int32_t*)(((std::uint8_t*)data) + totalsize);
        *refcount = 1;
    }
}

void Mat::create(std::int32_t _w, std::int32_t _h, std::int32_t _d, std::int32_t _c, std::size_t _elemsize,
                 std::int32_t _elempack, std::int32_t _n, mmemory::Allocator* _allocator) {
    if (_n <= 1) {
        create(_w, _h, _d, _c, _elemsize, _elempack, _allocator);
        return;
    }

    if (dims == kDims4 && w == _w && h == _h && d == _d && c == _c && elemsize == _elemsize && elempack == _elempack &&
        allocator == _allocator && n == _n) {
        return;
    }

    release();

    elemsize = _elemsize;
    elempack = _elempack;
    allocator = _allocator;

    dims = kDims4;
    w = _w;
    h = _h;
    d = _d;
    c = _c;
    n = _n;

    cstep = mmemory::align_size((std::size_t)w * h * d * elemsize, kCstepAlignment) / elemsize;
    nstep = mmemory::align_size(cstep * c * elemsize, kBatchStrideAlignment) / elemsize;

    std::size_t totalsize = mmemory::align_size(nstep * n * elemsize, kRefcountAlignment);
    if (totalsize > 0) {
        if (allocator) {
            data = allocator->allocate(totalsize + (std::int32_t)sizeof(*refcount));
        } else {
            data = mmemory::fast_malloc(totalsize + (std::int32_t)sizeof(*refcount));
        }
    }

    if (data) {
        refcount = (std::int32_t*)(((std::uint8_t*)data) + totalsize);
        *refcount = 1;
    }
}
#else
void Mat::create_like(const Mat& m, std::int32_t, mmemory::Allocator* _allocator) {
    create_like(m, _allocator);
}

void Mat::create(std::int32_t _w, std::size_t _elemsize, std::int32_t _elempack, std::int32_t,
                 mmemory::Allocator* _allocator) {
    create(_w, _elemsize, _elempack, _allocator);
}

void Mat::create(std::int32_t _w, std::int32_t _h, std::size_t _elemsize, std::int32_t _elempack, std::int32_t,
                 mmemory::Allocator* _allocator) {
    create(_w, _h, _elemsize, _elempack, _allocator);
}

void Mat::create(std::int32_t _w, std::int32_t _h, std::int32_t _c, std::size_t _elemsize, std::int32_t _elempack,
                 std::int32_t, mmemory::Allocator* _allocator) {
    create(_w, _h, _c, _elemsize, _elempack, _allocator);
}

void Mat::create(std::int32_t _w, std::int32_t _h, std::int32_t _d, std::int32_t _c, std::size_t _elemsize,
                 std::int32_t _elempack, std::int32_t, mmemory::Allocator* _allocator) {
    create(_w, _h, _d, _c, _elemsize, _elempack, _allocator);
}
#endif  // MMAT_BATCH

Mat::Mat()
        : data(nullptr),
          refcount(nullptr),
          elemsize(0),
          elempack(0),
          allocator(nullptr),
          dims(0),
          w(0),
          h(0),
          d(0),
          c(0),
          cstep(0)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
}

Mat::Mat(std::int32_t _w, std::size_t _elemsize, mmemory::Allocator* _allocator)
        : data(nullptr),
          refcount(nullptr),
          elemsize(0),
          elempack(0),
          allocator(nullptr),
          dims(0),
          w(0),
          h(0),
          d(0),
          c(0),
          cstep(0)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    create(_w, _elemsize, _allocator);
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::size_t _elemsize, mmemory::Allocator* _allocator)
        : data(nullptr),
          refcount(nullptr),
          elemsize(0),
          elempack(0),
          allocator(nullptr),
          dims(0),
          w(0),
          h(0),
          d(0),
          c(0),
          cstep(0)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    create(_w, _h, _elemsize, _allocator);
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::int32_t _c, std::size_t _elemsize, mmemory::Allocator* _allocator)
        : data(nullptr),
          refcount(nullptr),
          elemsize(0),
          elempack(0),
          allocator(nullptr),
          dims(0),
          w(0),
          h(0),
          d(0),
          c(0),
          cstep(0)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    create(_w, _h, _c, _elemsize, _allocator);
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::int32_t _d, std::int32_t _c, std::size_t _elemsize,
         mmemory::Allocator* _allocator)
        : data(nullptr),
          refcount(nullptr),
          elemsize(0),
          elempack(0),
          allocator(nullptr),
          dims(0),
          w(0),
          h(0),
          d(0),
          c(0),
          cstep(0)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    create(_w, _h, _d, _c, _elemsize, _allocator);
}

Mat::Mat(std::int32_t _w, std::size_t _elemsize, std::int32_t _elempack, mmemory::Allocator* _allocator)
        : data(nullptr),
          refcount(nullptr),
          elemsize(0),
          elempack(0),
          allocator(nullptr),
          dims(0),
          w(0),
          h(0),
          d(0),
          c(0),
          cstep(0)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    create(_w, _elemsize, _elempack, _allocator);
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::size_t _elemsize, std::int32_t _elempack,
         mmemory::Allocator* _allocator)
        : data(nullptr),
          refcount(nullptr),
          elemsize(0),
          elempack(0),
          allocator(nullptr),
          dims(0),
          w(0),
          h(0),
          d(0),
          c(0),
          cstep(0)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    create(_w, _h, _elemsize, _elempack, _allocator);
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::int32_t _c, std::size_t _elemsize, std::int32_t _elempack,
         mmemory::Allocator* _allocator)
        : data(nullptr),
          refcount(nullptr),
          elemsize(0),
          elempack(0),
          allocator(nullptr),
          dims(0),
          w(0),
          h(0),
          d(0),
          c(0),
          cstep(0)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    create(_w, _h, _c, _elemsize, _elempack, _allocator);
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::int32_t _d, std::int32_t _c, std::size_t _elemsize,
         std::int32_t _elempack, mmemory::Allocator* _allocator)
        : data(nullptr),
          refcount(nullptr),
          elemsize(0),
          elempack(0),
          allocator(nullptr),
          dims(0),
          w(0),
          h(0),
          d(0),
          c(0),
          cstep(0)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    create(_w, _h, _d, _c, _elemsize, _elempack, _allocator);
}

Mat::Mat(std::int32_t _w, std::size_t _elemsize, std::int32_t _elempack, std::int32_t _n,
         mmemory::Allocator* _allocator)
        : data(nullptr),
          refcount(nullptr),
          elemsize(0),
          elempack(0),
          allocator(nullptr),
          dims(0),
          w(0),
          h(0),
          d(0),
          c(0),
          cstep(0)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    create(_w, _elemsize, _elempack, _n, _allocator);
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::size_t _elemsize, std::int32_t _elempack, std::int32_t _n,
         mmemory::Allocator* _allocator)
        : data(nullptr),
          refcount(nullptr),
          elemsize(0),
          elempack(0),
          allocator(nullptr),
          dims(0),
          w(0),
          h(0),
          d(0),
          c(0),
          cstep(0)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    create(_w, _h, _elemsize, _elempack, _n, _allocator);
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::int32_t _c, std::size_t _elemsize, std::int32_t _elempack,
         std::int32_t _n, mmemory::Allocator* _allocator)
        : data(nullptr),
          refcount(nullptr),
          elemsize(0),
          elempack(0),
          allocator(nullptr),
          dims(0),
          w(0),
          h(0),
          d(0),
          c(0),
          cstep(0)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    create(_w, _h, _c, _elemsize, _elempack, _n, _allocator);
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::int32_t _d, std::int32_t _c, std::size_t _elemsize,
         std::int32_t _elempack, std::int32_t _n, mmemory::Allocator* _allocator)
        : data(nullptr),
          refcount(nullptr),
          elemsize(0),
          elempack(0),
          allocator(nullptr),
          dims(0),
          w(0),
          h(0),
          d(0),
          c(0),
          cstep(0)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    create(_w, _h, _d, _c, _elemsize, _elempack, _n, _allocator);
}

Mat::Mat(const Mat& m)
        : data(m.data),
          refcount(m.refcount),
          elemsize(m.elemsize),
          elempack(m.elempack),
          allocator(m.allocator),
          dims(m.dims),
          w(m.w),
          h(m.h),
          d(m.d),
          c(m.c),
          cstep(m.cstep)
#if MMAT_BATCH
          ,
          n(m.n),
          nstep(m.nstep)
#endif
{
    addref();
}

Mat::Mat(std::int32_t _w, void* _data, std::size_t _elemsize, mmemory::Allocator* _allocator)
        : data(_data),
          refcount(nullptr),
          elemsize(_elemsize),
          elempack(1),
          allocator(_allocator),
          dims(1),
          w(_w),
          h(1),
          d(1),
          c(1)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    cstep = mmemory::align_size(w * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif
}

Mat::Mat(std::int32_t _w, std::int32_t _h, void* _data, std::size_t _elemsize, mmemory::Allocator* _allocator)
        : data(_data),
          refcount(nullptr),
          elemsize(_elemsize),
          elempack(1),
          allocator(_allocator),
          dims(2),
          w(_w),
          h(_h),
          d(1),
          c(1)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    cstep = mmemory::align_size((std::size_t)w * h * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::int32_t _c, void* _data, std::size_t _elemsize,
         mmemory::Allocator* _allocator)
        : data(_data),
          refcount(nullptr),
          elemsize(_elemsize),
          elempack(1),
          allocator(_allocator),
          dims(kDims3),
          w(_w),
          h(_h),
          d(1),
          c(_c)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    cstep = mmemory::align_size((std::size_t)w * h * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::int32_t _d, std::int32_t _c, void* _data, std::size_t _elemsize,
         mmemory::Allocator* _allocator)
        : data(_data),
          refcount(nullptr),
          elemsize(_elemsize),
          elempack(1),
          allocator(_allocator),
          dims(kDims4),
          w(_w),
          h(_h),
          d(_d),
          c(_c)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    cstep = mmemory::align_size((std::size_t)w * h * d * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif
}

Mat::Mat(std::int32_t _w, void* _data, std::size_t _elemsize, std::int32_t _elempack, mmemory::Allocator* _allocator)
        : data(_data),
          refcount(nullptr),
          elemsize(_elemsize),
          elempack(_elempack),
          allocator(_allocator),
          dims(1),
          w(_w),
          h(1),
          d(1),
          c(1)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    cstep = mmemory::align_size(w * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif
}

Mat::Mat(std::int32_t _w, std::int32_t _h, void* _data, std::size_t _elemsize, std::int32_t _elempack,
         mmemory::Allocator* _allocator)
        : data(_data),
          refcount(nullptr),
          elemsize(_elemsize),
          elempack(_elempack),
          allocator(_allocator),
          dims(2),
          w(_w),
          h(_h),
          d(1),
          c(1)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    cstep = mmemory::align_size((std::size_t)w * h * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::int32_t _c, void* _data, std::size_t _elemsize, std::int32_t _elempack,
         mmemory::Allocator* _allocator)
        : data(_data),
          refcount(nullptr),
          elemsize(_elemsize),
          elempack(_elempack),
          allocator(_allocator),
          dims(kDims3),
          w(_w),
          h(_h),
          d(1),
          c(_c)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    cstep = mmemory::align_size((std::size_t)w * h * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::int32_t _d, std::int32_t _c, void* _data, std::size_t _elemsize,
         std::int32_t _elempack, mmemory::Allocator* _allocator)
        : data(_data),
          refcount(nullptr),
          elemsize(_elemsize),
          elempack(_elempack),
          allocator(_allocator),
          dims(kDims4),
          w(_w),
          h(_h),
          d(_d),
          c(_c)
#if MMAT_BATCH
          ,
          n(1),
          nstep(0)
#endif
{
    cstep = mmemory::align_size((std::size_t)w * h * d * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = total();
#endif
}

Mat::Mat(std::int32_t _w, void* _data, std::size_t _elemsize, std::int32_t _elempack, std::int32_t _n,
         mmemory::Allocator* _allocator)
        : data(_data),
          refcount(nullptr),
          elemsize(_elemsize),
          elempack(_elempack),
          allocator(_allocator),
          dims(1),
          w(_w),
          h(1),
          d(1),
          c(1)
#if MMAT_BATCH
          ,
          n(_n > 1 ? _n : 1),
          nstep(0)
#endif
{
    cstep = mmemory::align_size(w * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = n > 1 ? mmemory::align_size(total() * elemsize, kBatchStrideAlignment) / elemsize : total();
#else
    (void)_n;
#endif
}

Mat::Mat(std::int32_t _w, std::int32_t _h, void* _data, std::size_t _elemsize, std::int32_t _elempack, std::int32_t _n,
         mmemory::Allocator* _allocator)
        : data(_data),
          refcount(nullptr),
          elemsize(_elemsize),
          elempack(_elempack),
          allocator(_allocator),
          dims(2),
          w(_w),
          h(_h),
          d(1),
          c(1)
#if MMAT_BATCH
          ,
          n(_n > 1 ? _n : 1),
          nstep(0)
#endif
{
    cstep = mmemory::align_size((std::size_t)w * h * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = n > 1 ? mmemory::align_size(total() * elemsize, kBatchStrideAlignment) / elemsize : total();
#else
    (void)_n;
#endif
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::int32_t _c, void* _data, std::size_t _elemsize, std::int32_t _elempack,
         std::int32_t _n, mmemory::Allocator* _allocator)
        : data(_data),
          refcount(nullptr),
          elemsize(_elemsize),
          elempack(_elempack),
          allocator(_allocator),
          dims(kDims3),
          w(_w),
          h(_h),
          d(1),
          c(_c)
#if MMAT_BATCH
          ,
          n(_n > 1 ? _n : 1),
          nstep(0)
#endif
{
    cstep = mmemory::align_size((std::size_t)w * h * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = n > 1 ? mmemory::align_size(total() * elemsize, kBatchStrideAlignment) / elemsize : total();
#else
    (void)_n;
#endif
}

Mat::Mat(std::int32_t _w, std::int32_t _h, std::int32_t _d, std::int32_t _c, void* _data, std::size_t _elemsize,
         std::int32_t _elempack, std::int32_t _n, mmemory::Allocator* _allocator)
        : data(_data),
          refcount(nullptr),
          elemsize(_elemsize),
          elempack(_elempack),
          allocator(_allocator),
          dims(kDims4),
          w(_w),
          h(_h),
          d(_d),
          c(_c)
#if MMAT_BATCH
          ,
          n(_n > 1 ? _n : 1),
          nstep(0)
#endif
{
    cstep = mmemory::align_size((std::size_t)w * h * d * elemsize, kCstepAlignment) / elemsize;
#if MMAT_BATCH
    nstep = n > 1 ? mmemory::align_size(total() * elemsize, kBatchStrideAlignment) / elemsize : total();
#else
    (void)_n;
#endif
}

Mat::~Mat() {
    release();
}

void Mat::fill(float _v) {
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<float*>(data);

    std::size_t i = 0;
#if defined(__ARM_NEON)
    float32x4_t _c = vdupq_n_f32(_v);
    for (; i + (detail::kVectorLanes4 - 1) < size; i += detail::kVectorLanes4) {
        vst1q_f32(ptr, _c);
        ptr += detail::kVectorLanes4;
    }
#endif  // __ARM_NEON
    for (; i < size; i++) {
        *ptr++ = _v;
    }
}

Mat& Mat::operator=(const Mat& m) {
    if (this == &m) {
        return *this;
    }

    if (m.refcount) {
        detail::xadd(m.refcount, 1);
    }

    release();

    data = m.data;
    refcount = m.refcount;
    elemsize = m.elemsize;
    elempack = m.elempack;
    allocator = m.allocator;

    dims = m.dims;
    w = m.w;
    h = m.h;
    d = m.d;
    c = m.c;

    cstep = m.cstep;

#if MMAT_BATCH
    n = m.n;
    nstep = m.nstep;
#endif

    return *this;
}

void Mat::addref() {
    if (refcount) {
        detail::xadd(refcount, 1);
    }
}

void Mat::release() {
    if (refcount && detail::xadd(refcount, -1) == 1) {
        if (allocator) {
            allocator->deallocate(data);
        } else {
            mmemory::fast_free(data);
        }
    }

    data = nullptr;

    elemsize = 0;
    elempack = 0;

    dims = 0;
    w = 0;
    h = 0;
    d = 0;
    c = 0;

    cstep = 0;

#if MMAT_BATCH
    n = 1;
    nstep = 0;
#endif

    refcount = nullptr;
}

bool Mat::empty() const {
    return data == nullptr || total() == 0;
}

std::size_t Mat::total() const {
    return cstep * c;
}

std::int32_t Mat::elembits() const {
    return elempack ? static_cast<std::int32_t>(elemsize * kBitsPerByte) / elempack : 0;
}

Mat Mat::shape() const {
    if (dims == 1) {
        return Mat(w * elempack, nullptr, 4u, 1, n);
    }
    if (dims == 2) {
        return Mat(w, h * elempack, nullptr, 4u, 1, n);
    }
    if (dims == kDims3) {
        return Mat(w, h, c * elempack, nullptr, 4u, 1, n);
    }
    if (dims == kDims4) {
        return Mat(w, h, d, c * elempack, nullptr, 4u, 1, n);
    }

    return Mat();
}

Mat Mat::channel(std::int32_t _c) {
    Mat m(w, h, d, (std::uint8_t*)data + cstep * _c * elemsize, elemsize, elempack, allocator);
    m.dims = dims - 1;
    if (dims == kDims4) {
        m.cstep = (std::size_t)w * h;
#if MMAT_BATCH
        m.nstep = m.total();
#endif
    }
    return m;
}

const Mat Mat::channel(std::int32_t _c) const {
    Mat m(w, h, d, (std::uint8_t*)data + cstep * _c * elemsize, elemsize, elempack, allocator);
    m.dims = dims - 1;
    if (dims == kDims4) {
        m.cstep = (std::size_t)w * h;
#if MMAT_BATCH
        m.nstep = m.total();
#endif
    }
    return m;
}

Mat Mat::depth(std::int32_t z) {
    Mat m(w, h, (std::uint8_t*)data + (std::size_t)w * h * z * elemsize, elemsize, elempack, allocator);
    m.cstep = (std::size_t)w * h;
#if MMAT_BATCH
    m.nstep = m.total();
#endif
    return m;
}

const Mat Mat::depth(std::int32_t z) const {
    Mat m(w, h, (std::uint8_t*)data + (std::size_t)w * h * z * elemsize, elemsize, elempack, allocator);
    m.cstep = (std::size_t)w * h;
#if MMAT_BATCH
    m.nstep = m.total();
#endif
    return m;
}

float* Mat::row(std::int32_t y) {
    return (float*)((std::uint8_t*)data + (std::size_t)w * y * elemsize);
}

const float* Mat::row(std::int32_t y) const {
    return (const float*)((std::uint8_t*)data + (std::size_t)w * y * elemsize);
}

Mat Mat::channel_range(std::int32_t _c, std::int32_t channels) {
    Mat m(w, h, d, channels, (std::uint8_t*)data + cstep * _c * elemsize, elemsize, elempack, allocator);
    m.dims = dims;
    return m;
}

const Mat Mat::channel_range(std::int32_t _c, std::int32_t channels) const {
    Mat m(w, h, d, channels, (std::uint8_t*)data + cstep * _c * elemsize, elemsize, elempack, allocator);
    m.dims = dims;
    return m;
}

Mat Mat::depth_range(std::int32_t z, std::int32_t depths) {
    Mat m(w, h, depths, (std::uint8_t*)data + (std::size_t)w * h * z * elemsize, elemsize, elempack, allocator);
    m.cstep = (std::size_t)w * h;
#if MMAT_BATCH
    m.nstep = m.total();
#endif
    return m;
}

const Mat Mat::depth_range(std::int32_t z, std::int32_t depths) const {
    Mat m(w, h, depths, (std::uint8_t*)data + (std::size_t)w * h * z * elemsize, elemsize, elempack, allocator);
    m.cstep = (std::size_t)w * h;
#if MMAT_BATCH
    m.nstep = m.total();
#endif
    return m;
}

Mat Mat::row_range(std::int32_t y, std::int32_t rows) {
    Mat m(w, rows, (std::uint8_t*)data + (std::size_t)w * y * elemsize, elemsize, elempack, allocator);
    m.cstep = (std::size_t)w * rows;
#if MMAT_BATCH
    m.nstep = m.total();
#endif
    return m;
}

const Mat Mat::row_range(std::int32_t y, std::int32_t rows) const {
    Mat m(w, rows, (std::uint8_t*)data + (std::size_t)w * y * elemsize, elemsize, elempack, allocator);
    m.cstep = (std::size_t)w * rows;
#if MMAT_BATCH
    m.nstep = m.total();
#endif
    return m;
}

Mat Mat::range(std::int32_t x, std::int32_t n) {
    Mat m(n, (std::uint8_t*)data + x * elemsize, elemsize, elempack, allocator);
    m.cstep = (std::size_t)n;
#if MMAT_BATCH
    m.nstep = m.total();
#endif
    return m;
}

const Mat Mat::range(std::int32_t x, std::int32_t n) const {
    Mat m(n, (std::uint8_t*)data + x * elemsize, elemsize, elempack, allocator);
    m.cstep = (std::size_t)n;
#if MMAT_BATCH
    m.nstep = m.total();
#endif
    return m;
}

#if MMAT_BATCH
Mat Mat::batch(std::int32_t b) {
    Mat m(w, h, d, c, (std::uint8_t*)data + nstep * b * elemsize, elemsize, elempack, allocator);
    m.dims = dims;
    m.cstep = cstep;
    m.nstep = nstep;
    return m;
}

const Mat Mat::batch(std::int32_t b) const {
    Mat m(w, h, d, c, (std::uint8_t*)data + nstep * b * elemsize, elemsize, elempack, allocator);
    m.dims = dims;
    m.cstep = cstep;
    m.nstep = nstep;
    return m;
}

Mat Mat::batch_range(std::int32_t b, std::int32_t batches) {
    Mat m(w, h, d, c, (std::uint8_t*)data + nstep * b * elemsize, elemsize, elempack, batches, allocator);
    m.dims = dims;
    m.cstep = cstep;
    m.nstep = nstep;
    return m;
}

const Mat Mat::batch_range(std::int32_t b, std::int32_t batches) const {
    Mat m(w, h, d, c, (std::uint8_t*)data + nstep * b * elemsize, elemsize, elempack, batches, allocator);
    m.dims = dims;
    m.cstep = cstep;
    m.nstep = nstep;
    return m;
}
#else
Mat Mat::batch(std::int32_t) {
    Mat m(w, h, d, c, data, elemsize, elempack, allocator);
    m.dims = dims;
    m.cstep = cstep;
    return m;
}

const Mat Mat::batch(std::int32_t) const {
    Mat m(w, h, d, c, data, elemsize, elempack, allocator);
    m.dims = dims;
    m.cstep = cstep;
    return m;
}

Mat Mat::batch_range(std::int32_t, std::int32_t) {
    Mat m(w, h, d, c, data, elemsize, elempack, allocator);
    m.dims = dims;
    m.cstep = cstep;
    return m;
}

const Mat Mat::batch_range(std::int32_t, std::int32_t) const {
    Mat m(w, h, d, c, data, elemsize, elempack, allocator);
    m.dims = dims;
    m.cstep = cstep;
    return m;
}
#endif  // MMAT_BATCH

float& Mat::operator[](std::size_t i) {
    return ((float*)data)[i];
}

const float& Mat::operator[](std::size_t i) const {
    return ((const float*)data)[i];
}

}  // namespace mmat
