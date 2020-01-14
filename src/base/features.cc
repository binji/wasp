//
// Copyright 2019 WebAssembly Community Group participants
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "wasp/base/features.h"

namespace wasp {

void Features::EnableAll() {
#define WASP_V(variable, flag, default_) enable_##variable();
#include "wasp/base/features.def"
#undef WASP_V
}

void Features::UpdateDependencies(){
  if (reference_types_enabled_) {
    bulk_memory_enabled_ = true;
  }
}

}  // namespace wasp
