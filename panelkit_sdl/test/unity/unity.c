/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007-21 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#include "unity.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Unity Storage */
UNITY_STORAGE_T Unity;

/* Default setUp/tearDown implementations */
void __attribute__((weak)) setUp(void) { }
void __attribute__((weak)) tearDown(void) { }
void __attribute__((weak)) suiteSetUp(void) { }
int __attribute__((weak)) suiteTearDown(int num_failures) { return num_failures; }

/*-------------------------------------------------------
 * Test Framework Core Functions
 *-------------------------------------------------------*/

void UnityBegin(const char* filename)
{
    Unity.TestFile = filename;
    Unity.CurrentTestName = NULL;
    Unity.CurrentTestLineNumber = 0;
    Unity.NumberOfTests = 0;
    Unity.TestFailures = 0;
    Unity.TestIgnores = 0;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
}

int UnityEnd(void)
{
    printf("\n-----------------------\n");
    printf("%u Tests %u Failures %u Ignored\n", 
           Unity.NumberOfTests, Unity.TestFailures, Unity.TestIgnores);
    
    if (Unity.TestFailures == 0)
    {
        printf("OK\n");
    }
    else
    {
        printf("FAIL\n");
    }
    
    return (int)Unity.TestFailures;
}

void UnityDefaultTestRun(UnityTestFunction Func, const char* FuncName, const int FuncLineNum)
{
    Unity.CurrentTestName = FuncName;
    Unity.CurrentTestLineNumber = (unsigned int)FuncLineNum;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
    Unity.NumberOfTests++;
    
    if (Unity.NumberOfTests > 1)
        printf("\n");
    
    printf("TEST(%s:%d): %s", Unity.TestFile, FuncLineNum, FuncName);
    
    /* Call setUp */
    setUp();
    
    /* Run the test */
    Func();
    
    /* Call tearDown */
    tearDown();
    
    /* Report results */
    if (Unity.CurrentTestIgnored)
    {
        Unity.TestIgnores++;
        printf(" IGNORED");
    }
    else if (Unity.CurrentTestFailed)
    {
        Unity.TestFailures++;
        /* Failure message already printed */
    }
    else
    {
        printf(" PASS");
    }
}

/*-------------------------------------------------------
 * Assertion Functions
 *-------------------------------------------------------*/

void Unity_TestAssert(const int condition, const char* msg, const int line, const UNITY_DISPLAY_STYLE_T style)
{
    (void)style; /* Unused for basic implementation */
    
    if (!condition)
    {
        Unity.CurrentTestFailed = 1;
        printf(" FAIL\n");
        printf("  %s:%d: %s\n", Unity.TestFile, line, msg);
    }
}

void Unity_TestAssertNull(const void* pointer, const int line)
{
    Unity_TestAssert(pointer == NULL, "Expected NULL", line, UNITY_DISPLAY_STYLE_INT);
}

void Unity_TestAssertNotNull(const void* pointer, const int line)
{
    Unity_TestAssert(pointer != NULL, "Expected Non-NULL", line, UNITY_DISPLAY_STYLE_INT);
}

void Unity_TestAssertEqualNumber(const UNITY_INT expected, const UNITY_INT actual, const int line, const UNITY_DISPLAY_STYLE_T style)
{
    (void)style; /* Unused for basic implementation */
    if (expected != actual)
    {
        Unity.CurrentTestFailed = 1;
        printf(" FAIL\n");
        printf("  %s:%d: Expected %d Was %d\n", Unity.TestFile, line, expected, actual);
    }
}

void Unity_TestAssertFloatsWithin(const UNITY_FLOAT expected, const UNITY_FLOAT actual, const UNITY_FLOAT delta, const int line)
{
    UNITY_FLOAT diff = expected - actual;
    if (diff < 0) diff = -diff;
    
    if (diff > delta)
    {
        Unity.CurrentTestFailed = 1;
        printf(" FAIL\n");
        printf("  %s:%d: Expected %f Was %f (Delta %f)\n", 
               Unity.TestFile, line, expected, actual, delta);
    }
}

void Unity_TestAssertEqualString(const char* expected, const char* actual, const int line)
{
    if (expected == NULL || actual == NULL)
    {
        Unity_TestAssert(expected == actual, "Expected both strings to be NULL or both non-NULL", line, UNITY_DISPLAY_STYLE_INT);
        return;
    }
    
    if (strcmp(expected, actual) != 0)
    {
        Unity.CurrentTestFailed = 1;
        printf(" FAIL\n");
        printf("  %s:%d: Expected '%s' Was '%s'\n", Unity.TestFile, line, expected, actual);
    }
}

void Unity_TestAssertEqualMemory(const void* expected, const void* actual, const UNITY_UINT32 length, const int line)
{
    if (expected == NULL || actual == NULL)
    {
        Unity_TestAssert(expected == actual, "Expected both pointers to be NULL or both non-NULL", line, UNITY_DISPLAY_STYLE_INT);
        return;
    }
    
    if (memcmp(expected, actual, length) != 0)
    {
        Unity.CurrentTestFailed = 1;
        printf(" FAIL\n");
        printf("  %s:%d: Memory mismatch\n", Unity.TestFile, line);
    }
}

void Unity_TestFail(const char* message, const int line)
{
    Unity.CurrentTestFailed = 1;
    printf(" FAIL\n");
    printf("  %s:%d: %s\n", Unity.TestFile, line, message);
}

void Unity_TestIgnore(const char* message, const int line)
{
    Unity.CurrentTestIgnored = 1;
    (void)message; /* Could print if desired */
    (void)line;
}