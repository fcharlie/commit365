////
#ifndef AZE_CONSOLE_WIN_ADAPTER_IPP
#define AZE_CONSOLE_WIN_ADAPTER_IPP
#include <charconv>
#include "adapter.hpp"

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

namespace aze::details {
// Enable VT
inline bool enablevtmode() {
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE) {
    return false;
  }

  DWORD dwMode = 0;
  if (!GetConsoleMode(hOut, &dwMode)) {
    return false;
  }

  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  if (!SetConsoleMode(hOut, dwMode)) {
    return false;
  }
  return true;
}

inline adapter::adapter() {
  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hConsole == INVALID_HANDLE_VALUE) {
    at = AdapterFile;
    return;
  }
  if (GetFileType(hConsole) == FILE_TYPE_DISK) {
    at = AdapterFile;
    return;
  }
  if (GetFileType(hConsole) == FILE_TYPE_CHAR) {
    at = AdapterConsole;
    DWORD mode;
    if (GetConsoleMode(hConsole, &mode) == TRUE &&
        (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0) {
      at = AdapterConsoleTTY;
    }
    return;
  }
  at = AdapterTTY;
}

inline std::wstring utf8towide(std::string_view str) {
  std::wstring wstr;
  auto N =
      MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
  if (N > 0) {
    wstr.resize(N);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], N);
  }
  return wstr;
}

inline std::string wchar2utf8(const wchar_t *buf, size_t len) {

  auto N = WideCharToMultiByte(CP_UTF8, 0, buf, (int)len, nullptr, 0, nullptr,
                               nullptr);
  std::string str(N, '\0');
  WideCharToMultiByte(CP_UTF8, 0, buf, (int)len, &str[0], N, nullptr, nullptr);
  return str;
}

// NOTICE, we support write file as UTF-8. GBK? not support it.
inline ssize_t adapter::writefile(int color, const char *data, size_t len) {
  return fwrite(data, len, out);
}

inline ssize_t adapter::writeoldconsole(int color, const char *data,
                                        size_t len) {
  if (color == -1) {
    auto wdata = utf8towide(std::string_view(data, len));
    DWORD dwWrite;
    WriteConsoleW(hConsole, wdata.data(), (DWORD)wdata.size(), &dwWrite,
                  nullptr);
    return dwWrite;
  }
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(hConsole, &csbi);
  WORD oldColor = csbi.wAttributes;
  WORD color_ = static_cast<WORD>(color);
  WORD newColor;
  if (color > fc::White) {
    newColor = (oldColor & 0x0F) | color_;
  } else {
    newColor = (oldColor & 0xF0) | color_;
  }
  SetConsoleTextAttribute(hConsole, newColor);
  auto wdata = utf8towide(std::string_view(data, len));
  DWORD dwWrite;
  WriteConsoleW(hConsole, wdata.data(), (DWORD)wdata.size(), &dwWrite, nullptr);
  SetConsoleTextAttribute(hConsole, oldColor);
  return dwWrite;
}
struct TerminalsColorTable {
  int index;
  bool blod;
};

namespace vt {
namespace fg {
enum Color {
  Black = 30,
  Red = 31,
  Green = 32,
  Yellow = 33,
  Blue = 34,
  Magenta = 35,
  Cyan = 36,
  Gray = 37,
  Reset = 39
};
}
namespace bg {
enum Color {
  Black = 40,
  Red = 41,
  Green = 42,
  Yellow = 43,
  Blue = 44,
  Magenta = 45,
  Cyan = 46,
  Gray = 47,
  Reset = 49
};
}
} // namespace vt

inline bool TerminalsConvertColor(int color, TerminalsColorTable &co) {
  static std::unordered_map<int, TerminalsColorTable> tables = {
      {fc::Black, {vt::fg::Black, false}},
      {fc::DarkBlue, {vt::fg::Blue, false}},
      {fc::DarkGreen, {vt::fg::Green, false}},
      {fc::DarkCyan, {vt::fg::Cyan, false}},
      {fc::DarkRed, {vt::fg::Red, false}},
      {fc::DarkMagenta, {vt::fg::Magenta, false}},
      {fc::DarkYellow, {vt::fg::Yellow, false}},
      {fc::DarkGray, {vt::fg::Gray, false}},
      {fc::Blue, {vt::fg::Blue, true}},
      {fc::Green, {vt::fg::Green, true}},
      {fc::Cyan, {vt::fg::Cyan, true}},
      {fc::Red, {vt::fg::Red, true}},
      {fc::Magenta, {vt::fg::Magenta, true}},
      {fc::Yellow, {vt::fg::Yellow, true}},
      {fc::White, {vt::fg::Gray, true}},
      {bc::Black, {vt::bg::Black, false}},
      {bc::Blue, {vt::bg::Blue, false}},
      {bc::Green, {vt::bg::Green, false}},
      {bc::Cyan, {vt::bg::Cyan, false}},
      {bc::Red, {vt::bg::Red, false}},
      {bc::Magenta, {vt::bg::Magenta, false}},
      {bc::Yellow, {vt::bg::Yellow, false}},
      {bc::DarkGray, {vt::bg::Gray, false}},
      {bc::LightBlue, {vt::bg::Blue, true}},
      {bc::LightGreen, {vt::bg::Green, true}},
      {bc::LightCyan, {vt::bg::Cyan, true}},
      {bc::LightRed, {vt::fg::Red, true}},
      {bc::LightMagenta, {vt::bg::Magenta, true}},
      {bc::LightYellow, {vt::bg::Yellow, true}},
      {bc::LightWhite, {vt::bg::Gray, true}},
  };
  auto iter = tables.find(color);
  if (iter == tables.end()) {
    return false;
  }
  co = iter->second;
  return true;
}

inline int adapter::WriteConsoleInternal(const char *buffer, size_t len) {
  DWORD dwWrite = 0;
  auto wdata = utf8towide(std::string_view(data, len));
  if (WriteConsoleW(hConsole, wdata.data(), (DWORD)wdata.size(), &dwWrite,
                    nullptr)) {
    return static_cast<int>(dwWrite);
  }
  return 0;
}
inline ssize_t adapter::writeconsole(int color, const char *data, size_t len) {
  if (color == -1) {
    auto wdata = utf8towide(std::string_view(data, len));
    DWORD dwWrite;
    WriteConsoleW(hConsole, wdata.data(), (DWORD)wdata.size(), &dwWrite,
                  nullptr);
    return dwWrite;
  }
  TerminalsColorTable co;
  if (!TerminalsConvertColor(color, co)) {
    return writeoldconsole(color, data, len);
  }
  std::string buf("\x1b[");
  if (co.blod) {
    buf.append("1;").append(std::to_wstring(co.index)).push_back('m');
  } else {
    buf.append(std::to_wstring(co.index)).push_back('m');
  }
  WriteConsoleInternal(buf.data(), (DWORD)buf.size());
  auto N = WriteConsoleInternal(data, (DWORD)len);
  WriteConsoleInternal("\x1b[0m", (sizeof("\x1b[0m") - 1));
  return N;
}

inline ssize_t adapter::writetty(int color, const char *data, size_t len) {
  if (color == -1) {
    return fwrite(data, 1, len, out);
  }
  TerminalsColorTable co;
  if (!TerminalsConvertColor(color, co)) {
    return static_cast<int>(fwrite(data, 1, len, out));
  }
  fprintf(out, "%s%dm", co.blod ? "\33[1;" : "\x33[", co.index);
  auto l = fwrite(data, 1, len, out);
  fwrite("\33[0m", 1, sizeof("\33[0m") - 1, out);
  return l;
}

} // namespace aze::details

#endif
