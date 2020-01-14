#ifndef CDHX_H
#define CDHX_H

#define GPIO_NODE				"/dev/cdhx_gpio"

typedef enum
{
    GPIO_1=0,
    GPIO_2,
    GPIO_3,
    GPIO_4,
    GPIO_5,
    GPIO_6,
    GPIO_7,
    GPIO_8,
    GPIO_9,
    GPIO_10,
    GPIO_11,
    GPIO_12,
    GPIO_13,
    GPIO_14,
    GPIO_15,
    GPIO_16,
    GPIO_17,
    GPIO_18,
    GPIO_19,
    GPIO_20,
}DEV_GPIO;

typedef enum                                             
{   
	CDHX_GPIO=1010,    
}CDHX_DEV;  
          
typedef enum
{
    ACTION_MIN=0,
    ACTION_ENABLE_INPUT,
    ACTION_ENABLE_OUTPUT,
    ACTION_DISENABLE_INPUT,
    ACTION_DISENABLE_OUTPUT,
    ACTION_GET_VALUE,
    ACTION_SET_VALUE,
}CDHX_DEV_ACTION;

typedef struct
{
		DEV_GPIO gpio;
		CDHX_DEV_ACTION action;
		int option;
}CDHX_DEV_PARAM;
	                      
#endif 
