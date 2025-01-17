//===----------------------------------------------------------------------===//
//                         DuckDB
//
// common/enums/catalog_type.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/constants.hpp"

namespace duckdb {

//===--------------------------------------------------------------------===//
// Catalog Types
//===--------------------------------------------------------------------===//
enum class CatalogType : uint8_t {
	INVALID = 0,
	TABLE = 1,
	SCHEMA = 2,
	TABLE_FUNCTION = 3,
	SCALAR_FUNCTION = 4,
	AGGREGATE_FUNCTION = 5,
	VIEW = 6,
	INDEX = 7,
	PREPARED_STATEMENT = 8,
	SEQUENCE = 9,

	UPDATED_ENTRY = 50,
	DELETED_ENTRY = 51,
};

} // namespace duckdb
