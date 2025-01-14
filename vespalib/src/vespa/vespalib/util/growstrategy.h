// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <cstddef>

namespace vespalib {

class GrowStrategy {
private:
    size_t _initialCapacity;
    size_t _minimumCapacity;
    size_t _growDelta;
    float  _growFactor;
public:
    GrowStrategy() noexcept
        : GrowStrategy(1024, 0.5, 0, 0)
    {}
    GrowStrategy(size_t initialCapacity, float growPercent, size_t growDelta, size_t minimumCapacity) noexcept
        : _initialCapacity(initialCapacity),
          _minimumCapacity(minimumCapacity),
          _growDelta(growDelta),
          _growFactor(growPercent)
    {
    }

    size_t getMinimumCapacity() const noexcept { return _minimumCapacity; }
    size_t getInitialCapacity() const noexcept { return _initialCapacity; }
    float       getGrowFactor() const noexcept { return _growFactor; }
    size_t       getGrowDelta() const noexcept { return _growDelta; }

    bool operator==(const GrowStrategy & rhs) const noexcept {
        return (_initialCapacity == rhs._initialCapacity &&
                _minimumCapacity == rhs._minimumCapacity &&
                _growFactor == rhs._growFactor &&
                _growDelta == rhs._growDelta);
    }
    bool operator!=(const GrowStrategy & rhs) const noexcept {
        return !(operator==(rhs));
    }
};

}

