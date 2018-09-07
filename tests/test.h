#ifndef TEST_H
#define TEST_H

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define ASSERT_TRUE( expr, msg ) do { \
    if (!(expr)) { return msg;} \
    } while (0)

struct test_def {
    const char *test_name;
    const char* (*test_func)(void);
};

extern const char *test_suite_name;
extern struct test_def test_list[];

#endif
