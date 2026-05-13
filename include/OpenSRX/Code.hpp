#pragma once

/**
 * @file Code.hpp
 * @brief Data returned from a barcode read operation.
 */

#include <optional>
#include <string>
#include <utility>

namespace OpenSRX {

/**
 * @brief A point in image coordinates.
 */
struct Point {
    int x = 0;
    int y = 0;
};

/**
 * @brief Bounding box defined by four corner points.
 *
 * Corners are ordered: top-left, top-right, bottom-right, bottom-left.
 */
struct BoundingBox {
    Point topLeft;
    Point topRight;
    Point bottomRight;
    Point bottomLeft;
};

/**
 * @brief Result of a barcode read operation.
 *
 * Always contains the decoded code data. Optionally contains metadata
 * fields that the scanner appends when the corresponding
 * OperationParam appending settings are enabled (e.g.
 * CODE_VERTEX_APPENDING, CODE_CENTER_APPENDING, CODE_TYPE_APPENDING).
 */
struct Code {
    std::string data;                          ///< The decoded barcode string.
    std::optional<BoundingBox> boundingBox;    ///< Code vertex positions (param 308).
    std::optional<Point> center;               ///< Code center position (param 309).
    std::optional<std::string> codeType;       ///< Code type name (param 301).
    std::optional<int> bankNumber;             ///< Bank number (param 303).
    std::optional<double> angle;               ///< Decode angle in degrees (param 371).
};

}  // namespace OpenSRX
