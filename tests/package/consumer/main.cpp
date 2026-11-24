#include <cstdio>

#include <mmat/mat.hpp>

int main() {
    mmat::Mat mat(8, 3, static_cast<std::size_t>(4U));
    if (mat.empty() || mat.total() != 24U) {
        return 1;
    }

    mat.fill(2.0F);
    const mmat::Mat copy = mat.clone();
    if (copy.empty() || copy.data == mat.data) {
        return 2;
    }
    if (copy[0] != 2.0F || copy[23] != 2.0F) {
        return 3;
    }

    const mmat::Mat reshaped = mat.reshape(4, 6);
    if (reshaped.dims != 2 || reshaped.w != 4 || reshaped.h != 6) {
        return 4;
    }
    if (mat.channel(1).data == nullptr || mat.row(1) == nullptr) {
        return 5;
    }

    std::puts("mmat consumer ok");
    return 0;
}
