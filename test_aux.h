#ifndef TEST_AUX_H
#define TEST_AUX_H

#define TEST_SUCCESS 1

//colors to color the text:
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

//#define TEST(func)	
#define STR_VALUE(arg)      #arg
#define FUNCTION_NAME(name) STR_VALUE(name)

#define TEST(func)									\
	printf("running %-40s",FUNCTION_NAME(func));	\
	if(func() == TEST_SUCCESS){						\
		printf("%s[  OK  ]%s\n",KGRN,KWHT);			\
	}else{											\
		printf("%s[FAILED]%s\n",KRED,KWHT);			\
		return 1;									\
	}
#endif