/**
 * @file font.cpp
 *
 * フォント描画のプログラムを集めたファイル.
 */

#include "font.hpp"

extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

const uint8_t* GetFont(char c) {
  auto index = 16 * static_cast<unsigned int>(c);
  if (index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)) {
    return nullptr;
  }
  return &_binary_hankaku_bin_start + index;
}

// #@@range_begin(global_var)
FT_Library ft_library;
std::vector<uint8_t>* nihongo_buf;
// #@@range_end(global_var)

void WriteAscii(PixelWriter& writer, Vector2D<int> pos, char c, const PixelColor& color) {
  const uint8_t* font = GetFont(c);
  if (font == nullptr) {
    return;
  }
  for (int dy = 0; dy < 16; ++dy) {
    for (int dx = 0; dx < 8; ++dx) {
      if ((font[dy] << dx) & 0x80u) {
        writer.Write(pos + Vector2D<int>{dx, dy}, color);
      }
    }
  }
}

// #@@range_begin(write_string)
void WriteString(PixelWriter& writer, Vector2D<int> pos, const char* s, const PixelColor& color) {
  int x = 0;
  while (*s) {
    const auto [ u32, bytes ] = ConvertUTF8To32(s);
    WriteUnicode(writer, pos + Vector2D<int>{8 * x, 0}, u32, color);
    s += bytes;
    x += IsHankaku(u32) ? 1 : 2;
  }
}
// #@@range_end(write_string)

// #@@range_begin(count_utf8size)
int CountUTF8Size(uint8_t c) {
  if (c < 0x80) {
    return 1;
  } else if (0xc0 <= c && c < 0xe0) {
    return 2;
  } else if (0xe0 <= c && c < 0xf0) {
    return 3;
  } else if (0xf0 <= c && c < 0xf8) {
    return 4;
  }
  return 0;
}
// #@@range_end(count_utf8size)

// #@@range_begin(conv_utf8to32)
std::pair<char32_t, int> ConvertUTF8To32(const char* u8) {
  switch (CountUTF8Size(u8[0])) {
  case 1:
    return {
      static_cast<char32_t>(u8[0]),
      1
    };
  case 2:
    return {
      (static_cast<char32_t>(u8[0]) & 0b0001'1111) << 6 |
      (static_cast<char32_t>(u8[1]) & 0b0011'1111) << 0,
      2
    };
  case 3:
    return {
      (static_cast<char32_t>(u8[0]) & 0b0000'1111) << 12 |
      (static_cast<char32_t>(u8[1]) & 0b0011'1111) << 6 |
      (static_cast<char32_t>(u8[2]) & 0b0011'1111) << 0,
      3
    };
  case 4:
    return {
      (static_cast<char32_t>(u8[0]) & 0b0000'0111) << 18 |
      (static_cast<char32_t>(u8[1]) & 0b0011'1111) << 12 |
      (static_cast<char32_t>(u8[2]) & 0b0011'1111) << 6 |
      (static_cast<char32_t>(u8[3]) & 0b0011'1111) << 0,
      4
    };
  default:
    return { 0, 0 };
  }
}
// #@@range_end(conv_utf8to32)

// #@@range_begin(is_hankaku)
bool IsHankaku(char32_t c) {
  return c <= 0x7f;
}
// #@@range_end(is_hankaku)

// #@@range_begin(new_ftface)
WithError<FT_Face> NewFTFace() {
  FT_Face face;
  if (int err = FT_New_Memory_Face(
        ft_library, nihongo_buf->data(), nihongo_buf->size(), 0, &face)) {
    return { face, MAKE_ERROR(Error::kFreeTypeError) };
  }
  if (int err = FT_Set_Pixel_Sizes(face, 16, 16)) {
    return { face, MAKE_ERROR(Error::kFreeTypeError) };
  }
  return { face, MAKE_ERROR(Error::kSuccess) };
}
// #@@range_end(new_ftface)

// #@@range_begin(write_unicode)
void WriteUnicode(PixelWriter& writer, Vector2D<int> pos,
                  char32_t c, const PixelColor& color) {
  if (c <= 0x7f) {
    WriteAscii(writer, pos, c, color);
    return;
  }

  WriteAscii(writer, pos, '?', color);
  WriteAscii(writer, pos + Vector2D<int>{8, 0}, '?', color);
}
// #@@range_end(write_unicode)

// #@@range_begin(init_font)
void InitializeFont() {
  if (int err = FT_Init_FreeType(&ft_library)) {
    exit(1);
  }

  auto [ entry, pos_slash ] = fat::FindFile("/nihongo.ttf");
  if (entry == nullptr || pos_slash) {
    exit(1);
  }

  const size_t size = entry->file_size;
  nihongo_buf = new std::vector<uint8_t>(size);
  if (LoadFile(nihongo_buf->data(), size, *entry) != size) {
    delete nihongo_buf;
    exit(1);
  }
}
// #@@range_end(init_font)
