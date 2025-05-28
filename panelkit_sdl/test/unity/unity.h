/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007-21 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#ifndef UNITY_FRAMEWORK_H
#define UNITY_FRAMEWORK_H
#define UNITY

#define UNITY_VERSION_MAJOR    2
#define UNITY_VERSION_MINOR    6
#define UNITY_VERSION_BUILD    0

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <float.h>

/*-------------------------------------------------------
 * Test Setup / Teardown
 *-------------------------------------------------------*/

/* These functions are intended to be called before and after each test.
 * If using unity directly, these will need to be provided for each test
 * executable built. If using the test runner generator and/or CMock, these
 * are optional. */
void setUp(void);
void tearDown(void);

/* These functions are intended to be called at the beginning and end of an
 * entire test suite.  suiteTearDown() is passed the number of tests that
 * failed, and its return value becomes the exit code of main(). */
void suiteSetUp(void);
int suiteTearDown(int num_failures);

/*-------------------------------------------------------
 * Test Reset and Verify
 *-------------------------------------------------------*/

/* Call this function to reset the test framework before running a test. */
void UnityBegin(const char* filename);

/* Call this function at the end of your test executable to get a summary of
 * results. Returns the number of failed tests. */
int UnityEnd(void);

/* Use these macros to declare and run a test function. */
#define TEST_CASE(name) \
    void name(void)

#define RUN_TEST(func) UnityDefaultTestRun(func, #func, __LINE__)

/*-------------------------------------------------------
 * Basic Assertion Macros
 *-------------------------------------------------------*/

#define TEST_ASSERT(condition) \
    Unity_TestAssert((condition), "Expression Evaluated To FALSE", __LINE__, UNITY_DISPLAY_STYLE_INT)

#define TEST_ASSERT_TRUE(condition) \
    Unity_TestAssert((condition), "Expected TRUE Was FALSE", __LINE__, UNITY_DISPLAY_STYLE_INT)

#define TEST_ASSERT_FALSE(condition) \
    Unity_TestAssert(!(condition), "Expected FALSE Was TRUE", __LINE__, UNITY_DISPLAY_STYLE_INT)

#define TEST_ASSERT_NULL(pointer) \
    Unity_TestAssertNull((pointer), __LINE__)

#define TEST_ASSERT_NOT_NULL(pointer) \
    Unity_TestAssertNotNull((pointer), __LINE__)

#define TEST_ASSERT_EQUAL(expected, actual) \
    Unity_TestAssertEqualNumber((UNITY_INT)(expected), (UNITY_INT)(actual), __LINE__, UNITY_DISPLAY_STYLE_INT)

#define TEST_ASSERT_EQUAL_INT(expected, actual) \
    Unity_TestAssertEqualNumber((UNITY_INT)(expected), (UNITY_INT)(actual), __LINE__, UNITY_DISPLAY_STYLE_INT)

#define TEST_ASSERT_EQUAL_UINT(expected, actual) \
    Unity_TestAssertEqualNumber((UNITY_INT)(expected), (UNITY_INT)(actual), __LINE__, UNITY_DISPLAY_STYLE_UINT)

#define TEST_ASSERT_EQUAL_FLOAT(expected, actual) \
    Unity_TestAssertFloatsWithin((UNITY_FLOAT)(expected), (UNITY_FLOAT)(actual), (UNITY_FLOAT)0.00001f, __LINE__)

#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual) \
    Unity_TestAssertFloatsWithin((UNITY_FLOAT)(expected), (UNITY_FLOAT)(actual), (UNITY_FLOAT)(delta), __LINE__)

#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
    Unity_TestAssertEqualString((expected), (actual), __LINE__)

#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, len) \
    Unity_TestAssertEqualMemory((expected), (actual), (UNITY_UINT32)(len), __LINE__)

#define TEST_FAIL_MESSAGE(message) \
    Unity_TestFail((message), __LINE__)

#define TEST_FAIL() \
    Unity_TestFail("Failed", __LINE__)

#define TEST_IGNORE_MESSAGE(message) \
    Unity_TestIgnore((message), __LINE__)

#define TEST_IGNORE() \
    Unity_TestIgnore("Ignored", __LINE__)

/*-------------------------------------------------------
 * Test Execution Tracking
 *-------------------------------------------------------*/

typedef void (*UnityTestFunction)(void);

typedef struct UNITY_STORAGE_T
{
    const char* TestFile;
    const char* CurrentTestName;
    unsigned int CurrentTestLineNumber;
    unsigned int NumberOfTests;
    unsigned int TestFailures;
    unsigned int TestIgnores;
    unsigned int CurrentTestFailed;
    unsigned int CurrentTestIgnored;
} UNITY_STORAGE_T;

extern UNITY_STORAGE_T Unity;

/*-------------------------------------------------------
 * Internal Functions - Not for direct use
 *-------------------------------------------------------*/

typedef int UNITY_INT;
typedef unsigned int UNITY_UINT;
typedef unsigned int UNITY_UINT32;
typedef float UNITY_FLOAT;
typedef double UNITY_DOUBLE;

typedef enum
{
    UNITY_DISPLAY_STYLE_INT = 0,
    UNITY_DISPLAY_STYLE_UINT,
    UNITY_DISPLAY_STYLE_HEX8,
    UNITY_DISPLAY_STYLE_HEX16,
    UNITY_DISPLAY_STYLE_HEX32,
} UNITY_DISPLAY_STYLE_T;

void UnityDefaultTestRun(UnityTestFunction Func, const char* FuncName, const int FuncLineNum);
void Unity_TestAssert(const int condition, const char* msg, const int line, const UNITY_DISPLAY_STYLE_T style);
void Unity_TestAssertNull(const void* pointer, const int line);
void Unity_TestAssertNotNull(const void* pointer, const int line);
void Unity_TestAssertEqualNumber(const UNITY_INT expected, const UNITY_INT actual, const int line, const UNITY_DISPLAY_STYLE_T style);
void Unity_TestAssertFloatsWithin(const UNITY_FLOAT expected, const UNITY_FLOAT actual, const UNITY_FLOAT delta, const int line);
void Unity_TestAssertEqualString(const char* expected, const char* actual, const int line);
void Unity_TestAssertEqualMemory(const void* expected, const void* actual, const UNITY_UINT32 length, const int line);
void Unity_TestFail(const char* message, const int line);
void Unity_TestIgnore(const char* message, const int line);

#ifdef __cplusplus
}
#endif

#endif /* UNITY_FRAMEWORK_H */