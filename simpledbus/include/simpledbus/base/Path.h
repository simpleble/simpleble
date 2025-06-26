#pragma once

#include <string>
#include <vector>

namespace SimpleDBus {

class Path {
  public:
    // Constructor with path string, validates input
    explicit Path(const std::string& path = "/");

    // Get the underlying path string
    std::string str() const { return _path; }

    // Comparison operators for std::map
    bool operator<(const Path& other) const { return _path < other._path; }
    bool operator==(const Path& other) const { return _path == other._path; }
    bool operator!=(const Path& other) const { return !(*this == other); }

    // Member functions delegating to PathUtils
    size_t count_elements() const;
    std::string fetch_elements(size_t count) const;
    std::vector<std::string> split_elements() const;
    bool is_descendant(const Path& base) const;
    bool is_ascendant(const Path& base) const;
    bool is_child(const Path& base) const;
    bool is_parent(const Path& base) const;
    std::string next_child(const Path& base) const;
    std::string next_child_strip(const Path& base) const;

  private:
    std::string _path;
};

class PathUtils {
  public:
    static size_t count_elements(const std::string& path);
    static std::string fetch_elements(const std::string& path, size_t count);
    static std::vector<std::string> split_elements(const std::string& path);

    static bool is_descendant(const std::string& base, const std::string& path);
    static bool is_ascendant(const std::string& base, const std::string& path);

    static bool is_child(const std::string& base, const std::string& path);
    static bool is_parent(const std::string& base, const std::string& path);

    static std::string next_child(const std::string& base, const std::string& path);
    static std::string next_child_strip(const std::string& base, const std::string& path);

    static void validate(const std::string& path);
};

}  // namespace SimpleDBus
