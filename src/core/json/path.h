// Copyright 2024, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.
//

#pragma once

#include <absl/functional/function_ref.h>

#include <string>
#include <variant>
#include <vector>

#include "base/expected.hpp"
#include "src/core/json/json_object.h"

namespace dfly::json {

enum class SegmentType {
  IDENTIFIER = 1,  // $.identifier
  INDEX = 2,       // $.array[0]
  WILDCARD = 3,    // $.array[*] or $.*
  DESCENT = 4,     // $..identifier
  FUNCTION = 5,    // max($.prices[*])
};

const char* SegmentName(SegmentType type);

class AggFunction {
 public:
  virtual ~AggFunction() {
  }

  void Apply(const JsonType& src) {
    if (valid_ != 0)
      valid_ = ApplyImpl(src);
  }

  // returns null if Apply was not called or ApplyImpl failed.
  JsonType GetResult() const {
    return valid_ == 1 ? GetResultImpl() : JsonType::null();
  }

 protected:
  virtual bool ApplyImpl(const JsonType& src) = 0;
  virtual JsonType GetResultImpl() const = 0;

  int valid_ = -1;
};

class PathSegment {
 public:
  PathSegment() : PathSegment(SegmentType::IDENTIFIER) {
  }

  PathSegment(SegmentType type, std::string identifier = std::string())
      : type_(type), value_(std::move(identifier)) {
  }

  PathSegment(SegmentType type, unsigned index) : type_(type), value_(index) {
  }

  explicit PathSegment(std::shared_ptr<AggFunction> func)
      : type_(SegmentType::FUNCTION), value_(std::move(func)) {
  }

  SegmentType type() const {
    return type_;
  }

  const std::string& identifier() const {
    return std::get<std::string>(value_);
  }

  unsigned index() const {
    return std::get<unsigned>(value_);
  }

  void Evaluate(const JsonType& json) const;
  JsonType GetResult() const;

 private:
  SegmentType type_;

  // shared_ptr to preserve copy semantics.
  std::variant<std::string, unsigned, std::shared_ptr<AggFunction>> value_;
};

using Path = std::vector<PathSegment>;

// Passes the key name for object fields or nullopt for array elements.
// The second argument is a json value of either object fields or array elements.
using PathCallback = absl::FunctionRef<void(std::optional<std::string_view>, const JsonType&)>;

// Returns true if the entry should be deleted, false otherwise.
using MutateCallback = absl::FunctionRef<bool(std::optional<std::string_view>, JsonType*)>;

void EvaluatePath(const Path& path, const JsonType& json, PathCallback callback);

// returns number of matches found with the given path.
unsigned MutatePath(const Path& path, MutateCallback callback, JsonType* json);
nonstd::expected<Path, std::string> ParsePath(std::string_view path);

}  // namespace dfly::json
