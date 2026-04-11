#include "rendering/geometry/PrimitiveLibrary.h"

#include "rendering/geometry/BuiltInPrimitives.h"

namespace shadereditor {
// The library is currently static, so it is fully populated at construction time.
PrimitiveLibrary::PrimitiveLibrary() : primitives_(makeBuiltInPrimitives()) {}

const PreviewPrimitive* PrimitiveLibrary::findById(const std::string& id) const {
    for (const auto& primitive : primitives_) {
        if (primitive.id == id) {
            return &primitive;
        }
    }
    return nullptr;
}
}  // namespace shadereditor
