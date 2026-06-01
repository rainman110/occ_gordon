{{LICENSE_BANNER}}

#ifndef OCC_GORDON_SINGLE_HEADER_HPP
#define OCC_GORDON_SINGLE_HEADER_HPP

// Standalone builds do not generate the export header, so keep the macro local.
#ifndef OCC_GORDON_EXPORT
#define OCC_GORDON_EXPORT
#endif

// Keep the amalgamated header self-sufficient for the implementation section.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Public API declarations.
{{INLINE:src/occ_gordon/occ_gordon.h}}

// Implementation is opt-in so this header can still be included from many TUs.
#ifdef OCC_GORDON_IMPLEMENTATION

{{INLINE_CPP_TREE:src}}

#endif // OCC_GORDON_IMPLEMENTATION

#endif // OCC_GORDON_SINGLE_HEADER_HPP
