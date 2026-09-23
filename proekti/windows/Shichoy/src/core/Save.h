// Save / Сохранение: мета-прогресс и полный снимок текущего забега.
// Забег восстанавливается целиком — колода, реликвии, инвентарь, регион.
#pragma once
#include "core/Types.h"
#include <string>
#include <vector>

namespace shichoy {

// Простейший бинарный поток: длина + данные, без выравнивания и указателей.
class Blob {
public:
    // запись
    void u32(uint32_t v);
    void i32(int32_t v);
    void f32(float v);
    void u8(uint8_t v);
    void str(const std::string& s);

    // чтение
    uint32_t ru32();
    int32_t  ri32();
    float    rf32();
    uint8_t  ru8();
    std::string rstr();

    bool ok() const { return ok_; }
    void rewind() { pos_ = 0; ok_ = true; }
    const std::vector<uint8_t>& bytes() const { return data_; }
    void adopt(const uint8_t* p, size_t n) { data_.assign(p, p + n); pos_ = 0; ok_ = true; }

private:
    bool need(size_t n);
    std::vector<uint8_t> data_;
    size_t pos_ = 0;
    bool   ok_ = true;
};

} // namespace shichoy
