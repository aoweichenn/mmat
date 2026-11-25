// Copyright 2017 Tencent
// Copyright 2026 mcnn authors
// SPDX-License-Identifier: BSD-3-Clause

// ncnn-compatible Mat core: storage, reference counting, dims/strides/packing.
// Image, layer-based normalize and Vulkan extensions live in the consumer.

#ifndef MMAT_MAT_HPP
#define MMAT_MAT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#if defined(_MSC_VER)
#include <atomic>
#endif

#include <mmat/config.hpp>
#include <mmemory/allocator.hpp>

#include <mplatform/cpu.hpp>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif
#if defined(__SSE2__)
#include <emmintrin.h>
#if defined(__AVX__)
#include <immintrin.h>
#endif
#endif
#if defined(__mips_msa)
#include <msa.h>
#endif
#if defined(__loongarch_sx)
#include <lsxintrin.h>
#endif
#if defined(__riscv_vector)
#include <riscv_vector.h>
#endif

namespace mmat {

namespace detail {

inline std::int32_t xadd(std::int32_t* addr, std::int32_t delta) noexcept {
    // Compiler intrinsics keep this usable with older libc++ (e.g. NDK r27,
    // which predates std::atomic_ref).
#if defined(_MSC_VER)
    return std::atomic_ref<std::int32_t>(*addr).fetch_add(delta, std::memory_order_acq_rel);
#else
    return __atomic_fetch_add(addr, delta, __ATOMIC_ACQ_REL);
#endif
}

// SIMD fill lane counts. The overload definitions live in this header because
// the available overload set depends on the translation unit's ISA flags.
[[maybe_unused]] constexpr std::int32_t kVectorLanes4 = 4;
[[maybe_unused]] constexpr std::int32_t kVectorLanes8 = 8;
[[maybe_unused]] constexpr std::int32_t kVectorLanes16 = 16;
[[maybe_unused]] constexpr std::int32_t kBytesPerWord = 4;

}  // namespace detail

class MMAT_EXPORT Mat {
public:
    // empty
    Mat();
    // vec
    Mat(std::int32_t w, std::size_t elemsize = 4u, mmemory::Allocator* allocator = nullptr);
    // image
    Mat(std::int32_t w, std::int32_t h, std::size_t elemsize = 4u, mmemory::Allocator* allocator = nullptr);
    // dim
    Mat(std::int32_t w, std::int32_t h, std::int32_t c, std::size_t elemsize = 4u,
        mmemory::Allocator* allocator = nullptr);
    // cube
    Mat(std::int32_t w, std::int32_t h, std::int32_t d, std::int32_t c, std::size_t elemsize = 4u,
        mmemory::Allocator* allocator = nullptr);
    // packed vec
    Mat(std::int32_t w, std::size_t elemsize, std::int32_t elempack, mmemory::Allocator* allocator = nullptr);
    // packed image
    Mat(std::int32_t w, std::int32_t h, std::size_t elemsize, std::int32_t elempack,
        mmemory::Allocator* allocator = nullptr);
    // packed dim
    Mat(std::int32_t w, std::int32_t h, std::int32_t c, std::size_t elemsize, std::int32_t elempack,
        mmemory::Allocator* allocator = nullptr);
    // packed cube
    Mat(std::int32_t w, std::int32_t h, std::int32_t d, std::int32_t c, std::size_t elemsize, std::int32_t elempack,
        mmemory::Allocator* allocator = nullptr);
    // packed vec with batch count
    Mat(std::int32_t w, std::size_t elemsize, std::int32_t elempack, std::int32_t n,
        mmemory::Allocator* allocator = nullptr);
    // packed image with batch count
    Mat(std::int32_t w, std::int32_t h, std::size_t elemsize, std::int32_t elempack, std::int32_t n,
        mmemory::Allocator* allocator = nullptr);
    // packed dim with batch count
    Mat(std::int32_t w, std::int32_t h, std::int32_t c, std::size_t elemsize, std::int32_t elempack, std::int32_t n,
        mmemory::Allocator* allocator = nullptr);
    // packed cube with batch count
    Mat(std::int32_t w, std::int32_t h, std::int32_t d, std::int32_t c, std::size_t elemsize, std::int32_t elempack,
        std::int32_t n, mmemory::Allocator* allocator = nullptr);
    // copy
    Mat(const Mat& m);
    // external vec
    Mat(std::int32_t w, void* data, std::size_t elemsize = 4u, mmemory::Allocator* allocator = nullptr);
    // external image
    Mat(std::int32_t w, std::int32_t h, void* data, std::size_t elemsize = 4u, mmemory::Allocator* allocator = nullptr);
    // external dim
    Mat(std::int32_t w, std::int32_t h, std::int32_t c, void* data, std::size_t elemsize = 4u,
        mmemory::Allocator* allocator = nullptr);
    // external cube
    Mat(std::int32_t w, std::int32_t h, std::int32_t d, std::int32_t c, void* data, std::size_t elemsize = 4u,
        mmemory::Allocator* allocator = nullptr);
    // external packed vec
    Mat(std::int32_t w, void* data, std::size_t elemsize, std::int32_t elempack,
        mmemory::Allocator* allocator = nullptr);
    // external packed image
    Mat(std::int32_t w, std::int32_t h, void* data, std::size_t elemsize, std::int32_t elempack,
        mmemory::Allocator* allocator = nullptr);
    // external packed dim
    Mat(std::int32_t w, std::int32_t h, std::int32_t c, void* data, std::size_t elemsize, std::int32_t elempack,
        mmemory::Allocator* allocator = nullptr);
    // external packed cube
    Mat(std::int32_t w, std::int32_t h, std::int32_t d, std::int32_t c, void* data, std::size_t elemsize,
        std::int32_t elempack, mmemory::Allocator* allocator = nullptr);
    // external packed vec with batch count
    Mat(std::int32_t w, void* data, std::size_t elemsize, std::int32_t elempack, std::int32_t n,
        mmemory::Allocator* allocator = nullptr);
    // external packed image with batch count
    Mat(std::int32_t w, std::int32_t h, void* data, std::size_t elemsize, std::int32_t elempack, std::int32_t n,
        mmemory::Allocator* allocator = nullptr);
    // external packed dim with batch count
    Mat(std::int32_t w, std::int32_t h, std::int32_t c, void* data, std::size_t elemsize, std::int32_t elempack,
        std::int32_t n, mmemory::Allocator* allocator = nullptr);
    // external packed cube with batch count
    Mat(std::int32_t w, std::int32_t h, std::int32_t d, std::int32_t c, void* data, std::size_t elemsize,
        std::int32_t elempack, std::int32_t n, mmemory::Allocator* allocator = nullptr);
    // release
    ~Mat();
    // assign
    Mat& operator=(const Mat& m);
    // set all
    void fill(float v);
    void fill(std::int32_t v);
#if defined(__ARM_NEON)
    void fill(float32x4_t _v);
    void fill(uint16x4_t _v);
#if !defined(_MSC_VER)
    void fill(int32x4_t _v);
#endif
    void fill(int32x4_t _v0, int32x4_t _v1);
#if defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
#if !defined(_MSC_VER)
    void fill(float16x4_t _v);
    void fill(float16x8_t _v);
#endif
#endif  // __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
#endif  // __ARM_NEON
#if defined(__SSE2__)
#if defined(__AVX__)
#if defined(__AVX512F__)
    void fill(const __m512& _v);
#endif  // __AVX512F__
    void fill(const __m256& _v, std::int32_t i = 0);
#endif  // __AVX__
    void fill(const __m128& _v);
    void fill(const __m128i& _v);
#endif  // __SSE2__
#if defined(__mips_msa)
    void fill(v4f32 _v);
#endif  // __mips_msa
#if defined(__loongarch_sx)
    void fill(__m128 _v);
#endif  //__loongarch_sx
#if defined(__riscv_vector)
    void fill(vfloat32m1_t _v);
    void fill(vuint16m1_t _v);
    void fill(vint8m1_t _v);
#if defined(__riscv_zvfh)
    void fill(vfloat16m1_t _v);
#endif  // __riscv_zvfh
#endif  // __riscv_vector
    template<typename T>
    void fill(T v);
    // deep copy
    Mat clone(mmemory::Allocator* allocator = nullptr) const;
    // deep copy from other mat, inplace
    void clone_from(const mmat::Mat& mat, mmemory::Allocator* allocator = nullptr);
    // reshape vec
    Mat reshape(std::int32_t w, mmemory::Allocator* allocator = nullptr) const;
    // reshape image
    Mat reshape(std::int32_t w, std::int32_t h, mmemory::Allocator* allocator = nullptr) const;
    // reshape dim
    Mat reshape(std::int32_t w, std::int32_t h, std::int32_t c, mmemory::Allocator* allocator = nullptr) const;
    // reshape cube
    Mat reshape(std::int32_t w, std::int32_t h, std::int32_t d, std::int32_t c,
                mmemory::Allocator* allocator = nullptr) const;
    // allocate vec
    void create(std::int32_t w, std::size_t elemsize = 4u, mmemory::Allocator* allocator = nullptr);
    // allocate image
    void create(std::int32_t w, std::int32_t h, std::size_t elemsize = 4u, mmemory::Allocator* allocator = nullptr);
    // allocate dim
    void create(std::int32_t w, std::int32_t h, std::int32_t c, std::size_t elemsize = 4u,
                mmemory::Allocator* allocator = nullptr);
    // allocate cube
    void create(std::int32_t w, std::int32_t h, std::int32_t d, std::int32_t c, std::size_t elemsize = 4u,
                mmemory::Allocator* allocator = nullptr);
    // allocate packed vec
    void create(std::int32_t w, std::size_t elemsize, std::int32_t elempack, mmemory::Allocator* allocator = nullptr);
    // allocate packed image
    void create(std::int32_t w, std::int32_t h, std::size_t elemsize, std::int32_t elempack,
                mmemory::Allocator* allocator = nullptr);
    // allocate packed dim
    void create(std::int32_t w, std::int32_t h, std::int32_t c, std::size_t elemsize, std::int32_t elempack,
                mmemory::Allocator* allocator = nullptr);
    // allocate packed cube
    void create(std::int32_t w, std::int32_t h, std::int32_t d, std::int32_t c, std::size_t elemsize,
                std::int32_t elempack, mmemory::Allocator* allocator = nullptr);
    // allocate like
    void create_like(const Mat& m, mmemory::Allocator* allocator = nullptr);
    // allocate like with batch count
    void create_like(const Mat& m, std::int32_t n, mmemory::Allocator* allocator = nullptr);
    // allocate packed vec with batch count
    void create(std::int32_t w, std::size_t elemsize, std::int32_t elempack, std::int32_t n,
                mmemory::Allocator* allocator = nullptr);
    // allocate packed image with batch count
    void create(std::int32_t w, std::int32_t h, std::size_t elemsize, std::int32_t elempack, std::int32_t n,
                mmemory::Allocator* allocator = nullptr);
    // allocate packed dim with batch count
    void create(std::int32_t w, std::int32_t h, std::int32_t c, std::size_t elemsize, std::int32_t elempack,
                std::int32_t n, mmemory::Allocator* allocator = nullptr);
    // allocate packed cube with batch count
    void create(std::int32_t w, std::int32_t h, std::int32_t d, std::int32_t c, std::size_t elemsize,
                std::int32_t elempack, std::int32_t n, mmemory::Allocator* allocator = nullptr);
    // refcount++
    void addref();
    // refcount--
    void release();

    bool empty() const;
    std::size_t total() const;

    // bits per element
    std::int32_t elembits() const;

    // shape only
    Mat shape() const;

    // data reference
    Mat channel(std::int32_t c);
    const Mat channel(std::int32_t c) const;
    Mat depth(std::int32_t z);
    const Mat depth(std::int32_t z) const;
    float* row(std::int32_t y);
    const float* row(std::int32_t y) const;
    template<typename T>
    T* row(std::int32_t y);
    template<typename T>
    const T* row(std::int32_t y) const;

    // range reference
    Mat channel_range(std::int32_t c, std::int32_t channels);
    const Mat channel_range(std::int32_t c, std::int32_t channels) const;
    Mat depth_range(std::int32_t z, std::int32_t depths);
    const Mat depth_range(std::int32_t z, std::int32_t depths) const;
    Mat row_range(std::int32_t y, std::int32_t rows);
    const Mat row_range(std::int32_t y, std::int32_t rows) const;
    Mat range(std::int32_t x, std::int32_t n);
    const Mat range(std::int32_t x, std::int32_t n) const;

    // batch reference
    Mat batch(std::int32_t b);
    const Mat batch(std::int32_t b) const;
    Mat batch_range(std::int32_t b, std::int32_t batches);
    const Mat batch_range(std::int32_t b, std::int32_t batches) const;

    // access raw data
    template<typename T>
    operator T*();
    template<typename T>
    operator const T*() const;

    // convenient access float vec element
    float& operator[](std::size_t i);
    const float& operator[](std::size_t i) const;

    // pointer to the data
    void* data;

    // pointer to the reference counter
    // when points to user-allocated data, the pointer is NULL
    std::int32_t* refcount;

    // element size in bytes
    // 4 = float32/int32
    // 2 = float16/bfloat16
    // 1 = int8/uint8
    // 0 = empty
    std::size_t elemsize;

    // packed count inside element
    // c/1-d-h-w-1  c/1-h-w-1  h/1-w-1  w/1-1  scalar
    // c/4-d-h-w-4  c/4-h-w-4  h/4-w-4  w/4-4  sse/neon
    // c/8-d-h-w-8  c/8-h-w-8  h/8-w-8  w/8-8  avx/fp16
    std::int32_t elempack;

    // the allocator
    mmemory::Allocator* allocator;

    // the dimension rank
    std::int32_t dims;

    std::int32_t w;
    std::int32_t h;
    std::int32_t d;
    std::int32_t c;

    std::size_t cstep;

#if MMAT_BATCH
    // batch count, default 1
    std::int32_t n;
    // element step from one batch to the next
    // for non-batch Mat, equals total()
    std::size_t nstep;
#else
    static const std::int32_t n = 1;
#endif  // MMAT_BATCH
};

template<typename T>
MMAT_FORCEINLINE void Mat::fill(T _v) {
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<T*>(data);
    for (std::size_t i = 0; i < size; i++) {
        ptr[i] = _v;
    }
}

template<typename T>
MMAT_FORCEINLINE T* Mat::row(std::int32_t y) {
    return reinterpret_cast<T*>(static_cast<std::uint8_t*>(data) +
                                static_cast<std::size_t>(w) * static_cast<std::size_t>(y) * elemsize);
}

template<typename T>
MMAT_FORCEINLINE const T* Mat::row(std::int32_t y) const {
    return reinterpret_cast<const T*>(static_cast<const std::uint8_t*>(data) +
                                      static_cast<std::size_t>(w) * static_cast<std::size_t>(y) * elemsize);
}

template<typename T>
MMAT_FORCEINLINE Mat::operator T*() {
    return reinterpret_cast<T*>(data);
}

template<typename T>
MMAT_FORCEINLINE Mat::operator const T*() const {
    return reinterpret_cast<const T*>(data);
}

MMAT_FORCEINLINE void Mat::fill(std::int32_t _v) {
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<std::int32_t*>(data);

    std::size_t i = 0;
#if defined(__ARM_NEON)
    int32x4_t _c = vdupq_n_s32(_v);
    for (; i + (detail::kVectorLanes4 - 1) < size; i += detail::kVectorLanes4) {
        vst1q_s32(ptr, _c);
        ptr += detail::kVectorLanes4;
    }
#endif  // __ARM_NEON
    for (; i < size; i++) {
        *ptr++ = _v;
    }
}

#if defined(__ARM_NEON)
MMAT_FORCEINLINE void Mat::fill(float32x4_t _v) {
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<float*>(data);
    for (std::size_t i = 0; i < size; i++) {
        vst1q_f32(ptr, _v);
        ptr += detail::kVectorLanes4;
    }
}

MMAT_FORCEINLINE void Mat::fill(uint16x4_t _v) {
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<std::uint16_t*>(data);
    for (std::size_t i = 0; i < size; i++) {
        vst1_u16(ptr, _v);
        ptr += detail::kVectorLanes4;
    }
}

#if !defined(_MSC_VER)
MMAT_FORCEINLINE void Mat::fill(int32x4_t _v) {
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<std::int32_t*>(data);
    for (std::size_t i = 0; i < size; i++) {
        vst1q_s32(ptr, _v);
        ptr += detail::kVectorLanes4;
    }
}
#endif

MMAT_FORCEINLINE void Mat::fill(int32x4_t _v0, int32x4_t _v1) {
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<std::int32_t*>(data);
    for (std::size_t i = 0; i < size; i++) {
        vst1q_s32(ptr, _v0);
        vst1q_s32(ptr + detail::kVectorLanes4, _v1);
        ptr += detail::kVectorLanes8;
    }
}
#if defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
#if !defined(_MSC_VER)
MMAT_FORCEINLINE void Mat::fill(float16x4_t _v) {
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<__fp16*>(data);
    for (std::size_t i = 0; i < size; i++) {
        vst1_f16(ptr, _v);
        ptr += detail::kVectorLanes4;
    }
}

MMAT_FORCEINLINE void Mat::fill(float16x8_t _v) {
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<__fp16*>(data);
    for (std::size_t i = 0; i < size; i++) {
        vst1q_f16(ptr, _v);
        ptr += detail::kVectorLanes8;
    }
}
#endif
#endif  // __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
#endif  // __ARM_NEON

#if defined(__SSE2__)
#if defined(__AVX__)
#if defined(__AVX512F__)
MMAT_FORCEINLINE void Mat::fill(const __m512& _v) {
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<float*>(data);
    for (std::size_t i = 0; i < size; i++) {
        _mm512_storeu_ps(ptr, _v);
        ptr += detail::kVectorLanes16;
    }
}
#endif  // __AVX512F__
MMAT_FORCEINLINE void Mat::fill(const __m256& _v, std::int32_t _i) {
    // old gcc cannot overload __m128 and __m256 type
    // add a dummy std::int32_t parameter for different mangled function symbol
    (void)_i;
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<float*>(data);
    for (std::size_t i = 0; i < size; i++) {
        _mm256_storeu_ps(ptr, _v);
        ptr += detail::kVectorLanes8;
    }
}
#endif  // __AVX__
MMAT_FORCEINLINE void Mat::fill(const __m128& _v) {
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<float*>(data);
    for (std::size_t i = 0; i < size; i++) {
        _mm_storeu_ps(ptr, _v);
        ptr += detail::kVectorLanes4;
    }
}
MMAT_FORCEINLINE void Mat::fill(const __m128i& _v) {
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<std::uint16_t*>(data);
    for (std::size_t i = 0; i < size; i++) {
        _mm_store_si128(reinterpret_cast<__m128i*>(ptr), _v);
        ptr += detail::kVectorLanes8;
    }
}
#endif  // __SSE2__

#if defined(__mips_msa)
MMAT_FORCEINLINE void Mat::fill(v4f32 _v) {
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<float*>(data);
    for (std::size_t i = 0; i < size; i++) {
        __msa_st_w((v4i32)_v, ptr, 0);
        ptr += detail::kVectorLanes4;
    }
}
#endif  // __mips_msa

#if defined(__loongarch_sx)
MMAT_FORCEINLINE void Mat::fill(__m128 _v) {
    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<float*>(data);
    for (std::size_t i = 0; i < size; i++) {
        __lsx_vst(_v, ptr, 0);
        ptr += detail::kVectorLanes4;
    }
}
#endif  // __loongarch_sx

#if defined(__riscv_vector)
MMAT_FORCEINLINE void Mat::fill(vfloat32m1_t _v) {
    const std::int32_t packn = mplatform::cpu_riscv_vlenb() / detail::kBytesPerWord;
    const std::size_t vl = __riscv_vsetvl_e32m1(packn);

    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<float*>(data);
    for (std::size_t i = 0; i < size; i++) {
        __riscv_vse32_v_f32m1(ptr, _v, vl);
        ptr += packn;
    }
}

MMAT_FORCEINLINE void Mat::fill(vuint16m1_t _v) {
    const std::int32_t packn = mplatform::cpu_riscv_vlenb() / 2;
    const std::size_t vl = __riscv_vsetvl_e16m1(packn);

    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<std::uint16_t*>(data);
    for (std::size_t i = 0; i < size; i++) {
        __riscv_vse16_v_u16m1(ptr, _v, vl);
        ptr += packn;
    }
}

MMAT_FORCEINLINE void Mat::fill(vint8m1_t _v) {
    const std::int32_t packn = mplatform::cpu_riscv_vlenb() / 1;
    const std::size_t vl = __riscv_vsetvl_e8m1(packn);

    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<std::int8_t*>(data);
    for (std::size_t i = 0; i < size; i++) {
        __riscv_vse8_v_i8m1(ptr, _v, vl);
        ptr += packn;
    }
}
#if defined(__riscv_zvfh)
MMAT_FORCEINLINE void Mat::fill(vfloat16m1_t _v) {
    const std::int32_t packn = mplatform::cpu_riscv_vlenb() / 2;
    const std::size_t vl = __riscv_vsetvl_e16m1(packn);

    std::size_t size = total();
#if MMAT_BATCH
    size = nstep * static_cast<std::size_t>(n);
#endif
    auto* ptr = static_cast<__fp16*>(data);
    for (std::size_t i = 0; i < size; i++) {
        __riscv_vse16_v_f16m1(ptr, _v, vl);
        ptr += packn;
    }
}
#endif  // __riscv_zvfh
#endif  // __riscv_vector

}  // namespace mmat

#endif  // MMAT_MAT_HPP
