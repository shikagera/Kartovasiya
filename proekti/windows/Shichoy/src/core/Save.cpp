#include "core/Save.h"
#include <cstring>

namespace shichoy {

void Blob::u32(uint32_t v) {
    for (int i = 0; i < 4; ++i) data_.push_back(uint8_t((v >> (i * 8)) & 0xFF));
}
void Blob::i32(int32_t v) { u32(uint32_t(v)); }
void Blob::f32(float v) { uint32_t b; std::memcpy(&b, &v, 4); u32(b); }
void Blob::u8(uint8_t v) { data_.push_back(v); }
void Blob::str(const std::string& s) {
    u32(uint32_t(s.size()));
    data_.insert(data_.end(), s.begin(), s.end());
}

bool Blob::need(size_t n) {
    if (!ok_ || pos_ + n > data_.size()) { ok_ = false; return false; }
    return true;
}
uint32_t Blob::ru32() {
    if (!need(4)) return 0;
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v |= uint32_t(data_[pos_ + i]) << (i * 8);
    pos_ += 4;
    return v;
}
int32_t Blob::ri32() { return int32_t(ru32()); }
float Blob::rf32() { uint32_t b = ru32(); float v; std::memcpy(&v, &b, 4); return v; }
uint8_t Blob::ru8() { if (!need(1)) return 0; return data_[pos_++]; }
std::string Blob::rstr() {
    uint32_t n = ru32();
    if (n > 1u << 20) { ok_ = false; return {}; }     // защита от мусора в файле
    if (!need(n)) return {};
    std::string s(reinterpret_cast<const char*>(data_.data() + pos_), n);
    pos_ += n;
    return s;
}

} // namespace shichoy
