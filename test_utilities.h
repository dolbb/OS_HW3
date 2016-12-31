#ifndef TEST_UTILITIES_H_
#define TEST_UTILITIES_H_

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define min(a,b) ((a)<(b) ? (a) : (b))
#define max(a,b) ((a)>(b) ? (a) : (b))



/* 
    Allow using ASSERT_*() macros in functions that are being called from the test_*() function.
    So that the test would fail even if the macro failed inside of the called function.
    The inner called must return a type that `0` is a valid value of that type. (ex: int, pointer, ..)
*/
static bool flgTestFailed = 0;
#define ResetTestFailedFlag() do { \
    flgTestFailed = 0; \
} while(0)
#define SetTestFailedFlag() do { \
    flgTestFailed = 1; \
} while(0)
#define IsTestFailed() (!!(flgTestFailed))

/* Use ROTF() in the test function after calling a function that may call ASSERT_*() */
#define ReturnOnTestFail() do { \
    if(IsTestFailed()) return 0; \
} while(0)
#define ROTF() ReturnOnFail()


static char* strTestName;
#define GetTestName() (strTestName)
#define SetTestName(str) do { strTestName = str; } while(0)
    

#define TEST_ADDITIONAL_INFO_STR_SIZE 1024
static char strTestAdditionalInfo[TEST_ADDITIONAL_INFO_STR_SIZE] = {0};
#define GetTestAdditionalInfo() (strTestAdditionalInfo)
#define SetTestAdditionalInfo(str) do { \
    memcpy(strTestAdditionalInfo, str, max(TEST_ADDITIONAL_INFO_STR_SIZE, strlen(str)+1)); \
    strTestAdditionalInfo[TEST_ADDITIONAL_INFO_STR_SIZE-1] = 0; \
} while(0)
#define ClearTestAdditionalInfo() do { strTestAdditionalInfo[0] = 0; } while(0)


/**
 * These macros are here to help you create tests more easily and keep them
 * clear
 *
 * The basic idea with unit-testing is create a test function for every real
 * function and inside the test function declare some variables and execute the
 * function under test.
 *
 * Use the ASSERT_TEST to verify correctness of values.
 */
 
#define ASSERT_TEST(b,c,op) do { \
        if (IsTestFailed()) return 0; \
        int v1 = (b);\
        int v2 = (c);\
        if (!((v1) op (v2))) { \
                SetTestFailedFlag(); \
                printf("\nAssertion failed at %s:%d. ![%s %s %s] ==> ![%d %s %d]\n",__FILE__,__LINE__,#b,#op,#c,v1,#op,v2); \
                if (strlen(GetTestAdditionalInfo()) > 0) \
                    printf("More info: %s\n", GetTestAdditionalInfo()); \
                return 0; \
        } \
} while (0)
    
#define ASSERT_BETWEEN(value, lower, upper) do { \
        if (IsTestFailed()) return 0; \
        int val = (value);\
        int lv = (lower);\
        int uv = (upper);\
        if (!((val) >= (lv) && (val) <= (uv))) { \
                SetTestFailedFlag(); \
                printf("\nAssertion failed at %s:%d. ![%s <= %s <= %s] ==> ![%d <= %d <= %d]\n",__FILE__,__LINE__,#lower,#value,#upper,lv,val,uv); \
                return 0; \
        } \
} while (0)

#define ASSERT(b, op) do { \
        if (IsTestFailed()) return 0; \
        if (!((b) op (NULL))) { \
                SetTestFailedFlag(); \
                printf("\nNULL Assertion failed at %s:%d. ![%s %s %s]\n",__FILE__,__LINE__,#b,#op,"NULL"); \
                if (strlen(GetTestAdditionalInfo()) > 0) \
                    printf("More info: %s\n", GetTestAdditionalInfo()); \
                return 0; \
        } \
} while (0)
 
#define ASSERT_EQUALS(b, c) ASSERT_TEST(b, c, ==)
#define ASSERT_EQ(b, c) ASSERT_EQUALS(b, c)
#define ASSERT_NEQ(b, c) ASSERT_TEST(b, c, !=)
#define ASSERT_NQ(b, c) ASSERT_NEQ(b, c)
#define ASSERT_GT(b, c) ASSERT_TEST(b, c, >)
#define ASSERT_GE(b, c) ASSERT_TEST(b, c, >=)
#define ASSERT_LT(b, c) ASSERT_TEST(b, c, <)
#define ASSERT_LE(b, c) ASSERT_TEST(b, c, <=)
#define ASSERT_POSITIVE(b) ASSERT_GT(b, 0)
#define ASSERT_ZERO(b) ASSERT_EQUALS(b, 0)
#define ASSERT_NOT_ZERO(b) ASSERT_NEQ((int)(b), 0)
#define ASSERT_NULL(b) ASSERT(b, ==)
#define ASSERT_NOT_NULL(b) ASSERT(b, !=)
#define ASSERT_FALSE(b) ASSERT_EQ(b, 0)


/**
 * Macro used for running a test from the main function
 */
#define RUN_TEST(test) do { \
        ResetTestFailedFlag(); \
        printf("Running "#test"... "); \
        fflush(stdout); \
        SetTestName(#test); \
        if (test()) { \
            printf("[OK]\n");\
        } else { \
            printf("[%s FAILED]\n", #test);\
        }\
} while(0)


#endif /* TEST_UTILITIES_H_ */
