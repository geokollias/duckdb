#include "catch.hpp"
#include "test_helpers.hpp"

using namespace duckdb;
using namespace std;

TEST_CASE("Test COUNT operator", "[aggregate]") {
	unique_ptr<QueryResult> result;
	DuckDB db(nullptr);
	Connection con(db);

	// test counts on scalar values
	result = con.Query("SELECT COUNT(*), COUNT(1), COUNT(100), COUNT(NULL), COUNT(DISTINCT 1)");
	REQUIRE(CHECK_COLUMN(result, 0, {1}));
	REQUIRE(CHECK_COLUMN(result, 1, {1}));
	REQUIRE(CHECK_COLUMN(result, 2, {1}));
	REQUIRE(CHECK_COLUMN(result, 3, {0}));
	REQUIRE(CHECK_COLUMN(result, 4, {1}));

	// test counts on a set of values
	REQUIRE_NO_FAIL(con.Query("CREATE TABLE integers(i INTEGER);"));
	REQUIRE_NO_FAIL(con.Query("INSERT INTO integers VALUES (1), (2), (NULL)"));
	result = con.Query("SELECT COUNT(*), COUNT(1), COUNT(i), COUNT(COALESCE(i, 1)), COUNT(DISTINCT i), COUNT(DISTINCT "
	                   "1) FROM integers");
	REQUIRE(CHECK_COLUMN(result, 0, {3}));
	REQUIRE(CHECK_COLUMN(result, 1, {3}));
	REQUIRE(CHECK_COLUMN(result, 2, {2}));
	REQUIRE(CHECK_COLUMN(result, 3, {3}));
	REQUIRE(CHECK_COLUMN(result, 4, {2}));
	REQUIRE(CHECK_COLUMN(result, 5, {1}));
}

TEST_CASE("Test COVAR operators", "[aggregate]") {
	unique_ptr<QueryResult> result;
	DuckDB db(nullptr);
	Connection con(db);

	// test incorrect usage of COVAR_POP function
	REQUIRE_FAIL(con.Query("SELECT COVAR_POP()"));
	REQUIRE_FAIL(con.Query("SELECT COVAR_POP(1, 2, 3)"));
	REQUIRE_FAIL(con.Query("SELECT COVAR_POP(COVAR_POP(1))"));

	// test incorrect usage of COVAR_SAMP function
	REQUIRE_FAIL(con.Query("SELECT COVAR_SAMP()"));
	REQUIRE_FAIL(con.Query("SELECT COVAR_SAMP(1, 2, 3)"));
	REQUIRE_FAIL(con.Query("SELECT COVAR_SAMP(COVAR_SAMP(1))"));

	// test population covariance on scalar values
	result = con.Query("SELECT COVAR_POP(3,3), COVAR_POP(NULL,3), COVAR_POP(3,NULL), COVAR_POP(NULL,NULL)");
	REQUIRE(CHECK_COLUMN(result, 0, {0}));
	REQUIRE(CHECK_COLUMN(result, 1, {Value()}));
	REQUIRE(CHECK_COLUMN(result, 2, {Value()}));
	REQUIRE(CHECK_COLUMN(result, 3, {Value()}));

	// test sample covariance on scalar values
	result = con.Query("SELECT COVAR_SAMP(3,3), COVAR_SAMP(NULL,3), COVAR_SAMP(3,NULL), COVAR_SAMP(NULL,NULL)");
	REQUIRE(CHECK_COLUMN(result, 0, {Value()}));
	REQUIRE(CHECK_COLUMN(result, 1, {Value()}));
	REQUIRE(CHECK_COLUMN(result, 2, {Value()}));
	REQUIRE(CHECK_COLUMN(result, 3, {Value()}));

	// test population covariance on a sequence
	REQUIRE_NO_FAIL(con.Query("CREATE SEQUENCE seqx;"));
	REQUIRE_NO_FAIL(con.Query("CREATE SEQUENCE seqy;"));
	result = con.Query("SELECT COVAR_POP(nextval('seqx'),nextval('seqy'))");
	REQUIRE(CHECK_COLUMN(result, 0, {0}));
	result = con.Query("SELECT COVAR_POP(nextval('seqx'),nextval('seqy'))");
	REQUIRE(CHECK_COLUMN(result, 0, {0}));

	// test population covariance on a set of values
	REQUIRE_NO_FAIL(con.Query("CREATE TABLE integers(x INTEGER, y INTEGER);"));
	REQUIRE_NO_FAIL(con.Query("INSERT INTO integers VALUES (10,NULL), (10,11), (20,22), (25,NULL), (30,35)"));

	result = con.Query(
	    "SELECT COVAR_POP(x,y), COVAR_POP(x,1), COVAR_POP(1,y), COVAR_POP(x,NULL), COVAR_POP(NULL,y) FROM integers");
	REQUIRE(CHECK_COLUMN(result, 0, {80.0}));
	REQUIRE(CHECK_COLUMN(result, 1, {0.0}));
	REQUIRE(CHECK_COLUMN(result, 2, {0.0}));
	REQUIRE(CHECK_COLUMN(result, 3, {Value()}));
	REQUIRE(CHECK_COLUMN(result, 4, {Value()}));

	result = con.Query("SELECT COVAR_SAMP(x,y), COVAR_SAMP(x,1), COVAR_SAMP(1,y), COVAR_SAMP(x,NULL), "
	                   "COVAR_SAMP(NULL,y) FROM integers");
	REQUIRE(CHECK_COLUMN(result, 0, {120.0}));
	REQUIRE(CHECK_COLUMN(result, 1, {0.0}));
	REQUIRE(CHECK_COLUMN(result, 2, {0.0}));
	REQUIRE(CHECK_COLUMN(result, 3, {Value()}));
	REQUIRE(CHECK_COLUMN(result, 4, {Value()}));

	// test average on empty set
	result = con.Query("SELECT COVAR_POP(x,y), COVAR_SAMP(x,y) FROM integers WHERE x > 100");
	REQUIRE(CHECK_COLUMN(result, 0, {Value()}));
	REQUIRE(CHECK_COLUMN(result, 1, {Value()}));
}

TEST_CASE("Test AVG operator", "[aggregate]") {
	unique_ptr<QueryResult> result;
	DuckDB db(nullptr);
	Connection con(db);

	// test average on a scalar value
	result = con.Query("SELECT AVG(3), AVG(NULL)");
	REQUIRE(CHECK_COLUMN(result, 0, {3}));
	REQUIRE(CHECK_COLUMN(result, 1, {Value()}));

	// test average on a sequence
	REQUIRE_NO_FAIL(con.Query("CREATE SEQUENCE seq;"));
	result = con.Query("SELECT AVG(nextval('seq'))");
	REQUIRE(CHECK_COLUMN(result, 0, {1}));
	result = con.Query("SELECT AVG(nextval('seq'))");
	REQUIRE(CHECK_COLUMN(result, 0, {2}));

	// test average on a set of values
	REQUIRE_NO_FAIL(con.Query("CREATE TABLE integers(i INTEGER);"));
	REQUIRE_NO_FAIL(con.Query("INSERT INTO integers VALUES (1), (2), (3)"));
	result = con.Query("SELECT AVG(i), AVG(1), AVG(DISTINCT i), AVG(NULL) FROM integers");
	REQUIRE(CHECK_COLUMN(result, 0, {2}));
	REQUIRE(CHECK_COLUMN(result, 1, {1}));
	REQUIRE(CHECK_COLUMN(result, 2, {2}));
	REQUIRE(CHECK_COLUMN(result, 3, {Value()}));

	// test average on empty set
	result = con.Query("SELECT AVG(i) FROM integers WHERE i > 100");
	REQUIRE(CHECK_COLUMN(result, 0, {Value()}));

	// test incorrect usage of AVG function
	REQUIRE_FAIL(con.Query("SELECT AVG()"));
	REQUIRE_FAIL(con.Query("SELECT AVG(1, 2, 3)"));
	REQUIRE_FAIL(con.Query("SELECT AVG(AVG(1))"));
}

TEST_CASE("Test implicit aggregate operators", "[aggregate]") {
	unique_ptr<QueryResult> result;
	DuckDB db(nullptr);
	Connection con(db);

	// test implicit aggregates on empty set
	REQUIRE_NO_FAIL(con.Query("CREATE TABLE integers(i INTEGER);"));
	result = con.Query("SELECT COUNT(*), COUNT(i), STDDEV_SAMP(i), SUM(i), SUM(DISTINCT i), FIRST(i), MAX(i), MIN(i) "
	                   "FROM integers WHERE i > 100");
	REQUIRE(CHECK_COLUMN(result, 0, {0}));
	REQUIRE(CHECK_COLUMN(result, 1, {0}));
	REQUIRE(CHECK_COLUMN(result, 2, {Value()}));
	REQUIRE(CHECK_COLUMN(result, 3, {Value()}));
	REQUIRE(CHECK_COLUMN(result, 4, {Value()}));
	REQUIRE(CHECK_COLUMN(result, 5, {Value()}));
	REQUIRE(CHECK_COLUMN(result, 6, {Value()}));
	REQUIRE(CHECK_COLUMN(result, 7, {Value()}));
}

TEST_CASE("Test built in aggregate operator usage", "[aggregate]") {
	unique_ptr<QueryResult> result;
	DuckDB db(nullptr);
	Connection con(db);

	// test incorrect usage of the COUNT aggregate
	REQUIRE_FAIL(con.Query("SELECT COUNT(1, 2, 3)"));
	REQUIRE_FAIL(con.Query("SELECT COUNT(COUNT(1))"));

	// test incorrect usage of STDDEV_SAMP aggregate
	REQUIRE_FAIL(con.Query("SELECT STDDEV_SAMP()"));
	REQUIRE_FAIL(con.Query("SELECT STDDEV_SAMP(1, 2, 3)"));
	REQUIRE_FAIL(con.Query("SELECT STDDEV_SAMP(STDDEV_SAMP(1))"));

	// test incorrect usage of SUM aggregate
	REQUIRE_FAIL(con.Query("SELECT SUM()"));
	REQUIRE_FAIL(con.Query("SELECT SUM(1, 2, 3)"));
	REQUIRE_FAIL(con.Query("SELECT SUM(SUM(1))"));

	// test incorrect usage of FIRST aggregate
	REQUIRE_FAIL(con.Query("SELECT FIRST()"));
	REQUIRE_FAIL(con.Query("SELECT FIRST(1, 2, 3)"));
	REQUIRE_FAIL(con.Query("SELECT FIRST(FIRST(1))"));

	// test incorrect usage of MAX aggregate
	REQUIRE_FAIL(con.Query("SELECT MAX()"));
	REQUIRE_FAIL(con.Query("SELECT MAX(1, 2, 3)"));
	REQUIRE_FAIL(con.Query("SELECT MAX(MAX(1))"));

	// test incorrect usage of MIN aggregate
	REQUIRE_FAIL(con.Query("SELECT MIN()"));
	REQUIRE_FAIL(con.Query("SELECT MIN(1, 2, 3)"));
	REQUIRE_FAIL(con.Query("SELECT MIN(MIN(1))"));
}

TEST_CASE("Test GROUP BY on expression", "[aggregate]") {
	unique_ptr<QueryResult> result;
	DuckDB db(nullptr);
	Connection con(db);

	REQUIRE_NO_FAIL(con.Query("CREATE TABLE integer(i INTEGER, j INTEGER);"));
	REQUIRE_NO_FAIL(con.Query("INSERT INTO integer VALUES (3, 4), (3, 5), (3, 7);"));
	result = con.Query("SELECT j * 2 FROM integer GROUP BY j * 2 ORDER BY j * 2;");
	REQUIRE(CHECK_COLUMN(result, 0, {8, 10, 14}));
}

TEST_CASE("Test GROUP BY with many groups", "[aggregate]") {
	unique_ptr<QueryResult> result;
	DuckDB db(nullptr);
	Connection con(db);

	REQUIRE_NO_FAIL(con.Query("CREATE TABLE integers(i INTEGER, j INTEGER);"));
	for (index_t i = 0; i < 10000; i++) {
		REQUIRE_NO_FAIL(con.Query("INSERT INTO integers VALUES (" + to_string(i) + ", 1), (" + to_string(i) + ", 2);"));
	}
	result = con.Query("SELECT SUM(i), SUM(sums) FROM (SELECT i, SUM(j) AS sums FROM integers GROUP BY i) tbl1");
	REQUIRE(CHECK_COLUMN(result, 0, {49995000}));
	REQUIRE(CHECK_COLUMN(result, 1, {30000}));
}
