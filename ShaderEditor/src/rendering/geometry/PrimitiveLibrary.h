#pragma once

#include "rendering/geometry/PreviewPrimitive.h"

#include <string>
#include <vector>

namespace shadereditor {
class PrimitiveLibrary {
  public:
    PrimitiveLibrary();

    [[nodiscard]] const std::vector<PreviewPrimitive>& all() const { return primitives_; }
    [[nodiscard]] const PreviewPrimitive* findById(const std::string& id) const;

  private:
    std::vector<PreviewPrimitive> primitives_;
};
}  // namespace shadereditor
