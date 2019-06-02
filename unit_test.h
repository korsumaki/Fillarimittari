// unit testing

void unit_test();

#define UT_BEGIN() int UT_tests = 0; int UT_fails = 0;

#define UT_END() Serial.print("UNIT TESTS: "); \
	if (UT_fails > 0) { \
		Serial.print("FAILED, "); \
	} else { \
		Serial.print("PASSED, "); \
	} \
	Serial.print(UT_fails); \
	Serial.print(" / "); \
	Serial.println(UT_tests); \
	//return UT_fails;

// UT_EXPECT is used with integer values. It requires values to be exactly the same.
#define UT_EXPECT(expected, actual) \
	UT_tests++; \
	if ((expected) != (actual)) \
	{ \
		UT_fails++; \
		Serial.print("ERROR: expected="); \
		Serial.print(expected); \
		Serial.print(", actual="); \
		Serial.print(actual); \
		Serial.print(": line "); \
		Serial.println(__LINE__); \
	}

// UT_EXPECT_FLOAT is used with floats. It will accept small difference in float values (it just happens sometimes with floats).
#define UT_EXPECT_FLOAT(expected, actual) \
	UT_tests++; \
	if (abs((expected) - (actual)) > 0.0001) \
	{ \
		UT_fails++; \
		Serial.print("ERROR: expected="); \
		Serial.print(expected); \
		Serial.print(", actual="); \
		Serial.print(actual); \
		Serial.print(": line "); \
		Serial.println(__LINE__); \
	}
