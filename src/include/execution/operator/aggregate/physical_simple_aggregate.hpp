//===----------------------------------------------------------------------===//
//                         DuckDB
//
// execution/operator/aggregate/physical_simple_aggregate.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "execution/physical_operator.hpp"

namespace duckdb {

//! PhysicalSimpleAggregate is an aggregate operator that can only perform aggregates (1) without any groups, and (2)
//! without any DISTINCT aggregates
class PhysicalSimpleAggregate : public PhysicalOperator {
public:
	PhysicalSimpleAggregate(vector<TypeId> types, vector<unique_ptr<Expression>> expressions);

	//! The aggregates that have to be computed
	vector<unique_ptr<Expression>> aggregates;

public:
	void GetChunkInternal(ClientContext &context, DataChunk &chunk, PhysicalOperatorState *state) override;

	unique_ptr<PhysicalOperatorState> GetOperatorState() override;
};

class PhysicalSimpleAggregateOperatorState : public PhysicalOperatorState {
public:
	PhysicalSimpleAggregateOperatorState(PhysicalSimpleAggregate *parent, PhysicalOperator *child);

	//! The aggregate values
	vector<Value> aggregates;
	//! The payload chunk
	DataChunk payload_chunk;
};
} // namespace duckdb
