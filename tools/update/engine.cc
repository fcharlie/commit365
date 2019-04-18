////////
#include <json.hpp>
#include <cstdio>
#include "engine.hpp"

inline std::vector<std::string_view> PathSplit(std::string_view sv) {
  std::vector<std::string_view> output;
  size_t first = 0;
  while (first < sv.size()) {
    const auto second = sv.find_first_of('/', first);
    if (first != second) {
      auto s = sv.substr(first, second - first);
      if (s == "..") {
        if (!output.empty()) {
          output.pop_back();
        }
      } else if (s != ".") {
        output.emplace_back(s);
      }
    }
    if (second == std::string_view::npos) {
      break;
    }
    first = second + 1;
  }
  return output;
}

/// NOTLIKE Unix Path ---> Git  not start root '/'
std::string PathClean(std::string_view p) {
  std::string s;
  auto sv = PathSplit(p);
  if (sv.empty()) {
    return "";
  }
  s.assign(sv[0]);
  for (size_t i = 1; i < sv.size(); i++) {
    s.append("/").append(sv[i]);
  }
  return s;
}

// when sub startswith parant
// 1.  size equal --> path equal
// 2.  path[p.size()]=='/' At this point, 'path' is a subpath of p
inline bool IsPathContains(std::string_view parent, std::string_view sub) {
  return (parent.size() <= sub.size() &&
          memcmp(parent.data(), sub.data(), parent.size()) == 0 &&
          (sub.size() == parent.size() || sub[parent.size()] == '/'));
}

bool RulesEngine::AddPrefix(std::string_view path) {
  auto p = PathClean(path);
  if (p.empty()) {
    return false;
  }
  prefix.push_back(p);
  return true;
}

bool RulesEngine::AddRegex(std::string_view rx) {
  // re2::StringPiece(const char *str, size_type len)
  auto re = new RE2(rx);
  if (re->ok()) {
    rules.push_back(re);
    return true;
  }
  return false;
}

bool RulesEngine::FullMatch(std::string_view path) {
  for (const auto &p : prefix) {
    if (IsPathContains(p, path)) {
      return true;
    }
  }
  for (auto c : rules) {
    if (RE2::FullMatch(path, *c)) {
      return true;
    }
  }
  return false;
}

struct file {
  FILE *handle{nullptr};
  ~file() {
    if (handle != nullptr) {
      fclose(handle);
    }
  }
};

bool RulesEngine::PreInitialize(std::string_view jf, std::string_view branch) {
  try {
    file f;
    f.handle = fopen(jf.data(), "r");
    if (f.handle == nullptr) {
      // BAD file
      return false;
    }
    auto j = nlohmann::json::parse(f.handle);
    auto ki = j.find(branch);
    if (ki == j.end()) {
      ki = j.find(".all");
      if (ki == j.end()) {
        return true;
      }
    }
    auto obj = ki.value();
    auto it = obj.find("dirs");
    if (it != obj.end() && it->is_array()) {
      auto av = it.value();
      if (av.size() > 256) {
        fprintf(stderr, "readonly prefix size over 256: %zu\n", av.size());
        return false;
      }
      for (const auto a : av) {
        if (!a.is_string()) {
          continue;
        }
        AddPrefix(a.get<std::string>());
      }
    }
    ////////////
    it = obj.find("regex");
    if (it != obj.end() && it->is_array()) {
      auto av = it.value();
      if (av.size() > 64) {
        fprintf(stderr, "readonly regex size over 64: %zu\n", av.size());
        return false;
      }
      for (const auto a : av) {
        if (!a.is_string()) {
          continue;
        }
        AddRegex(a.get<std::string>());
      }
    }
  } catch (const std::exception &e) {
    (void)e;
    fprintf(stderr, "bad options%s\n", e.what());
    return false;
  }
  return true;
}
