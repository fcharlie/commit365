#ifndef ARGVEX_HPP
#define ARGVEX_HPP
#include <cstring>
#include <climits>
#include <vector>
#include <string_view>
#include <functional>

namespace ax {
struct ErrorResult {
  std::string message;
  int errorcode{0};
  operator bool() { return errorcode == 0; }
};

inline unsigned char _Digit_from_char(const char _Ch) noexcept // strengthened
{ // convert ['0', '9'] ['A', 'Z'] ['a', 'z'] to [0, 35], everything else to 255
  static constexpr unsigned char _Digit_from_byte[] = {
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255,
      255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
      20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,
      35,  255, 255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,  17,
      18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
      33,  34,  35,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255};
  static_assert(std::size(_Digit_from_byte) == 256);

  return (_Digit_from_byte[static_cast<unsigned char>(_Ch)]);
}

template <typename Integer>
ErrorResult Integer_from_chars(std::string_view wsv, Integer &ov,
                               const int base) {
  bool msign = false;
  auto _begin = wsv.begin();
  auto _end = wsv.end();
  auto _iter = _begin;
  if constexpr (std::is_signed_v<Integer>) {
    if (_iter != _end && *_iter == '-') {
      msign = true;
      _iter++;
    }
  }
  using Unsigned = std::make_unsigned_t<Integer>;
  constexpr Unsigned _Uint_max = static_cast<Unsigned>(-1);

  Unsigned _risky_val;
  Unsigned _max_digit;

  if constexpr (std::is_signed_v<Integer>) {
    constexpr Unsigned _Int_max = static_cast<Unsigned>(_Uint_max >> 1);
    if (msign) {
      constexpr Unsigned _Abs_int_min = static_cast<Unsigned>(_Int_max + 1);
      _risky_val = static_cast<Unsigned>(_Abs_int_min / base);
      _max_digit = static_cast<Unsigned>(_Abs_int_min % base);
    } else {
      _risky_val = static_cast<Unsigned>(_Int_max / base);
      _max_digit = static_cast<Unsigned>(_Int_max % base);
    }
  } else {
    _risky_val = static_cast<Unsigned>(_Uint_max / base);
    _max_digit = static_cast<Unsigned>(_Uint_max % base);
  }

  Unsigned _value = 0;

  bool _overflowed = false;

  for (; _iter != _end; ++_iter) {
    char wch = *_iter;
    if (wch > CHAR_MAX) {
      return ErrorResult{"out of range", 1};
    }
    const unsigned char _digit = _Digit_from_char(static_cast<char>(wch));

    if (_digit >= base) {
      break;
    }

    if (_value < _risky_val // never overflows
        || (_value == _risky_val &&
            _digit <= _max_digit)) // overflows for certain digits
    {
      _value = static_cast<Unsigned>(_value * base + _digit);
    } else // _Value > _Risky_val always overflows
    {
      _overflowed = true; // keep going, _Next still needs to be updated, _Value
                          // is now irrelevant
    }
  }

  if (_iter - _begin == static_cast<ptrdiff_t>(msign)) {
    return ErrorResult{"invalid argument", 1};
  }

  if (_overflowed) {
    return ErrorResult{"result out of range", 1};
  }

  if constexpr (std::is_signed_v<Integer>) {
    if (msign) {
      _value = static_cast<Unsigned>(0 - _value);
    }
  }
  ov = static_cast<Unsigned>(_value); // implementation-defined for negative,
                                      // N4713 7.8 [conv.integral]/3
  return ErrorResult{};
}

class ParseArgv {
public:
  ParseArgv(int argc, char *const *argv) : argc_(argc), argv_(argv) {
    ///
  }
  ParseArgv(const ParseArgv &) = delete;
  ParseArgv &operator=(const ParseArgv &) = delete;
  enum HasArgs {
    required_argument, /// -i 11 or -i=xx
    no_argument,
    optional_argument /// -s --long --long=xx
  };
  struct option {
    const char *name;
    HasArgs has_args;
    int val;
  };
  using ArgumentCallback =
      std::function<bool(int, const char *optarg, const char *raw)>;
  ErrorResult ParseArgument(const std::vector<option> &opts,
                            const ArgumentCallback &callback) {
    if (argc_ == 0 || argv_ == nullptr) {
      return ErrorResult{"invalid argument input"};
    };
    index = 1;
    for (; index < argc_; index++) {
      std::string_view arg = argv_[index];
      if (arg[0] != '-') {
        uargs.push_back(arg);
        continue;
      }
      auto err = ParseInternal(arg, opts, callback);
      if (err.errorcode != 0) {
        return err;
      }
    }
    return ErrorResult{};
  }
  const std::vector<std::string_view> &UnresolvedArgs() const { return uargs; }

private:
  int argc_;
  char *const *argv_;
  std::vector<std::string_view> uargs;
  int index{0};
  ErrorResult ParseInternal(std::string_view arg,
                            const std::vector<option> &opts,
                            const ArgumentCallback &callback) {
    /*
    -x ; -x value -Xvalue
    --xy;--xy=value;--xy value
    */
    if (arg.size() < 2) {
      return ErrorResult{"Invalid argument", 1};
    }
    int ch = -1;
    HasArgs ha = optional_argument;
    const char *optarg = nullptr;

    if (arg[1] == '-') {
      /// parse long
      /// --name value; --name=value
      std::string_view name;
      auto pos = arg.find('=');
      if (pos != std::string_view::npos) {
        if (pos + 1 >= arg.size()) {
          return ErrorResult{std::string("Incorrect argument: ").append(arg),
                             1};
        }
        name = arg.substr(2, pos - 2);
        optarg = arg.data() + pos + 1;
      } else {
        name = arg.substr(2);
      }
      for (auto &o : opts) {
        if (name.compare(o.name) == 0) {
          ch = o.val;
          ha = o.has_args;
          break;
        }
      }
    } else {
      /// parse short
      ch = arg[1];

      /// -x=xxx
      if (arg.size() == 3 && arg[2] == '=') {
        return ErrorResult{std::string("Incorrect argument: ").append(arg), 1};
      }
      if (arg.size() > 3) {
        if (arg[2] == '=') {
          optarg = arg.data() + 3;
        } else {
          optarg = arg.data() + 2;
        }
      }
      for (auto &o : opts) {
        if (o.val == ch) {
          ha = o.has_args;
          break;
        }
      }
    }

    if (optarg != nullptr && ha == no_argument) {
      return ErrorResult{std::string("Unacceptable input: ").append(arg), 1};
    }
    if (optarg == nullptr && ha == required_argument) {
      if (index + 1 >= argc_) {
        return ErrorResult{
            std::string("Option name cannot be empty: ").append(arg), 1};
      }
      optarg = argv_[index + 1];
      index++;
    }
    if (callback(ch, optarg, arg.data())) {
      return ErrorResult{};
    }
    return ErrorResult{"skipped", 2};
  }
};

} // namespace ax

#endif
