// Copyright 2022, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.
//

#pragma once

#include <absl/container/flat_hash_map.h>

#include <array>
#include <optional>

#include "server/conn_context.h"

namespace dfly {

class EngineShardSet;
class Interpreter;

// This class has a state through the lifetime of a server because it manipulates scripts
class ScriptMgr {
 public:
  struct ScriptParams {
    bool atomic = true;            // Whether script must run atomically.
    bool undeclared_keys = false;  // Whether script accesses undeclared keys.

    // Return GenericError if some flag was invalid.
    // Valid flags are:
    // - allow-undeclared-keys -> undeclared_keys=true
    // - disable-atomicity     -> atomic=false
    static GenericError ApplyFlags(std::string_view flags, ScriptParams* params);
  };

  struct ScriptData : public ScriptParams {
    std::string body;       // script source code present in lua interpreter
    std::string orig_body;  // original code, before removing header and adding async
  };

  struct ScriptKey : public std::array<char, 40> {
    ScriptKey() = default;
    ScriptKey(std::string_view sha);
  };

 public:
  ScriptMgr();

  void Run(CmdArgList args, ConnectionContext* cntx);

  // Insert script and return sha. Get possible error from compilation or parsing script flags.
  io::Result<std::string, GenericError> Insert(std::string_view body, Interpreter* interpreter);

  // Get script body by sha, returns nullptr if not found.
  std::optional<ScriptData> Find(std::string_view sha) const;

  // Returns a list of all scripts in the database with their sha and body.
  std::vector<std::pair<std::string, ScriptData>> GetAll() const;

  void FlushAllScript();

  // Returns if scripts run as global transactions by default
  bool AreGlobalByDefault() const;

 private:
  void ExistsCmd(CmdArgList args, ConnectionContext* cntx) const;
  void FlushCmd(CmdArgList args, ConnectionContext* cntx);
  void LoadCmd(CmdArgList args, ConnectionContext* cntx);
  void ConfigCmd(CmdArgList args, ConnectionContext* cntx);
  void ListCmd(ConnectionContext* cntx) const;
  void LatencyCmd(ConnectionContext* cntx) const;

  void UpdateScriptCaches(ScriptKey sha, ScriptParams params) const;

 private:
  struct InternalScriptData : public ScriptParams {
    std::unique_ptr<char[]> body{};
    std::unique_ptr<char[]> orig_body{};
  };

  ScriptParams default_params_;

  absl::flat_hash_map<ScriptKey, InternalScriptData> db_;
  mutable Mutex mu_;
};

}  // namespace dfly
