///
#ifndef AZE_CONSOLE_WIN_IMPL_HPP
#define AZE_CONSOLE_WIN_IMPL_HPP
#include "adapter.hpp"

namespace aze {

enum FcColor : WORD {
  Black = 0,
  DarkBlue = 1,
  DarkGreen = 2,
  DarkCyan = 3,
  DarkRed = 4,
  DarkMagenta = 5,
  DarkYellow = 6,
  Gray = 7,
  DarkGray = 8,
  Blue = 9,
  Green = 10,
  Cyan = 11,
  Red = 12,
  Magenta = 13,
  Yellow = 14,
  White = 15
};

namespace bc {
enum Color : WORD {
  Black = 0,
  DarkGray = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED,
  Blue = BACKGROUND_BLUE,
  Green = BACKGROUND_GREEN,
  Red = BACKGROUND_RED,
  Yellow = BACKGROUND_RED | BACKGROUND_GREEN,
  Magenta = BACKGROUND_RED | BACKGROUND_BLUE,
  Cyan = BACKGROUND_GREEN | BACKGROUND_BLUE,
  LightWhite = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED |
               BACKGROUND_INTENSITY,
  LightBlue = BACKGROUND_BLUE | BACKGROUND_INTENSITY,
  LightGreen = BACKGROUND_GREEN | BACKGROUND_INTENSITY,
  LightRed = BACKGROUND_RED | BACKGROUND_INTENSITY,
  LightYellow = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY,
  LightMagenta = BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
  LightCyan = BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
};
}

template <typename T> T Argument(T value) noexcept { return value; }
template <typename T>
T const *Argument(std::basic_string<T> const &value) noexcept {
  return value.c_str();
}

template <typename T>
T const *Argument(std::basic_string_view<T> const &value) noexcept {
  return value.data();
}

template <typename... Args>
int StringPrint(char *const buffer, size_t const bufferCount,
                char const *const format, Args const &... args) noexcept {
  int const result = sprintf(buffer, bufferCount, format, Argument(args)...);
  // ASSERT(-1 != result);
  return result;
}

template <typename... Args> ssize_t Print(const char *format, Args... args) {
  std::string buffer;
  size_t size = StringPrint(nullptr, 0, format, args...);
  buffer.resize(size);
  size = StringPrint(&buffer[0], buffer.size() + 1, format, args...);
  return details::adapter::instance().adapterwrite(-1, buffer.data(), size);
}

template <typename... Args>
ssize_t Verbose(bool verbose, const char *format, Args... args) {
  if (!verbose) {
    return 0;
  }
  std::string buffer;
  size_t size = StringPrint(nullptr, 0, format, args...);
  buffer.resize(size);
  size = StringPrint(&buffer[0], buffer.size() + 1, format, args...);
  return details::adapter::instance().adapterwrite(aze::Yellow, buffer.data(),
                                                   size);
}

// ChangePrintMode todo
inline bool ChangePrintMode(bool isstderr) {
  return details::adapter::instance().changeout(isstderr);
}

} // namespace aze
#include "adapter.ipp"
#endif
