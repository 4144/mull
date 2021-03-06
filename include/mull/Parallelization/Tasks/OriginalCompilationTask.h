#pragma once

#include "mull/MullModule.h"

#include <llvm/Object/ObjectFile.h>
#include <vector>

namespace mull {
class Toolchain;
class progress_counter;

class OriginalCompilationTask {
public:
  using In = std::vector<std::unique_ptr<MullModule>>;
  using Out = std::vector<llvm::object::OwningBinary<llvm::object::ObjectFile>>;
  using iterator = In::const_iterator;

  explicit OriginalCompilationTask(Toolchain &toolchain);

  void operator()(iterator begin, iterator end, Out &storage,
                  progress_counter &counter);

private:
  Toolchain &toolchain;
};
} // namespace mull
