
#pragma once

#include "catch.hpp"
#include "duckdb.hpp"
#include "common/string_util.hpp"

namespace duckdb {
static void CHECK_COLUMN(std::unique_ptr<duckdb::DuckDBResult> &result,
                         size_t column_number,
                         std::vector<duckdb::Value> values) {
	if (!result->GetSuccess()) {
		fprintf(stderr, "Query failed with message: %s\n",
		        result->GetErrorMessage().c_str());
		FAIL(result->GetErrorMessage().c_str());
	}
	if (values.size() == 0) {
		if (result->data.size() != 0) {
			FAIL("Data size does not match value size!");
		} else {
			return;
		}
	}
	if (result->data.size() == 0) {
		FAIL("Data size does not match value size!");
	}
	if (column_number >= result->data[0]->column_count) {
		FAIL("Column number out of range of result!");
	}
	size_t chunk_index = 0;
	for (size_t i = 0; i < values.size();) {
		if (chunk_index > result->data.size()) {
			// ran out of chunks
			FAIL("Data size does not match value size!");
		}
		// check this vector
		auto &vector = result->data[chunk_index]->data[column_number];
		if (i + vector.count > values.size()) {
			vector.Print();
			// too many values in this vector
			FAIL("Too many values in result!");
		}
		for (size_t j = 0; j < vector.count; j++) {
			// NULL <> NULL, hence special handling
			if (vector.GetValue(j).is_null && values[i + j].is_null) {
				continue;
			}
			if (!duckdb::Value::Equals(vector.GetValue(j), values[i + j])) {
				FAIL("Incorrect result! Got " + vector.GetValue(j).ToString() +
				     " but expected " + values[i + j].ToString());
			}
		}
		chunk_index++;
		i += vector.count;
	}
}

static void RESULT_NO_ERROR(std::unique_ptr<duckdb::DuckDBResult> &result) {
	if (!result->GetSuccess()) {
		FAIL(result->GetErrorMessage().c_str());
	}
}

static bool parse_datachunk(std::string csv, DataChunk &result,
                            bool has_header) {
	std::istringstream f(csv);
	std::string line;

	if (has_header) {
		auto split = StringUtil::Split(line, '|');
		if (split.size() != result.column_count) {
			// column length is different
			return false;
		}
	}
	size_t row = 0;
	while (std::getline(f, line)) {
		auto split = StringUtil::Split(line, '|');
		if (split.size() != result.column_count) {
			// column length is different
			return false;
		}
		if (row >= result.maximum_size) {
			// chunk is full, but parsing so far was successful
			// return the partially parsed chunk
			// this function is used for printing, and we probably don't want to
			// print >1000 rows anyway
			return true;
		}
		// now compare the values
		for (size_t i = 0; i < split.size(); i++) {
			// first create a string value
			Value value(split[i]);
			value.is_null = split[i].empty();

			// cast to the type of the column
			try {
				value = value.CastAs(result.data[i].type);
			} catch (...) {
				return false;
			}
			// now perform a comparison
			result.data[i].count++;
			result.data[i].SetValue(row, value);
		}
		result.count++;
		row++;
	}
	return true;
}

//! Compares the result of a pipe-delimited CSV with the given DataChunk
//! Returns true if they are equal, and stores an error_message otherwise
static bool compare_result(std::string csv, DataChunk &result, bool has_header,
                    std::string &error_message) {
	auto types = result.GetTypes();
	DataChunk correct_result;

	std::istringstream f(csv);
	std::string line;
	size_t row = 0;
	/// read and parse the header line
	if (has_header) {
		std::getline(f, line);
		// check if the column length matches
		auto split = StringUtil::Split(line, '|');
		if (split.size() != types.size()) {
			// column length is different
			goto incorrect;
		}
	}
	// now compare the actual data
	while (std::getline(f, line)) {
		if (result.count <= row) {
			goto incorrect;
		}
		auto split = StringUtil::Split(line, '|');
		if (split.size() != types.size()) {
			// column length is different
			goto incorrect;
		}
		// now compare the values
		for (size_t i = 0; i < split.size(); i++) {
			// first create a string value
			Value value(split[i]);
			value.is_null = split[i].empty();

			// cast to the type of the column
			try {
				value = value.CastAs(types[i]);
			} catch (...) {
				goto incorrect;
			}
			// now perform a comparison
			Value result_value = result.data[i].GetValue(row);
			if (result_value.is_null && value.is_null) {
				// NULL = NULL in checking code
				continue;
			}
			if (value.type == TypeId::DECIMAL) {
				// round to two decimals
				auto left = StringUtil::Format("%.2f", value.value_.decimal);
				auto right = StringUtil::Format("%.2f", result_value.value_.decimal);
				if (left != right) {
					goto incorrect;
				}
			} else {
				if (!Value::Equals(value, result_value)) {
					goto incorrect;
				}
			}
		}
		row++;
	}
	if (result.count != row) {
		goto incorrect;
	}
	return true;
incorrect:
	correct_result.Initialize(types);
	if (!parse_datachunk(csv, correct_result, has_header)) {
		error_message = "Incorrect answer for query!\nProvided answer:\n" +
		                result.ToString() +
		                "\nExpected answer [could not parse]:\n" + std::string(csv) +
		                "\n";
	} else {
		error_message = "Incorrect answer for query!\nProvided answer:\n" +
		                result.ToString() + "\nExpected answer:\n" +
		                correct_result.ToString() + "\n";
	}
	return false;
}

static void COMPARE_CSV(std::unique_ptr<duckdb::DuckDBResult> &result,
                        std::string csv, bool header = false) {
	if (!result->GetSuccess()) {
		fprintf(stderr, "Query failed with message: %s\n",
		        result->GetErrorMessage().c_str());
		FAIL(result->GetErrorMessage().c_str());
	}
	duckdb::DataChunk big_chunk;
	result->GatherResult(big_chunk);
	std::string error;
	if (!compare_result(csv, big_chunk, header, error)) {
		FAIL(error);
	}
}
}

