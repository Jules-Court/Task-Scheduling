#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <endian.h>

struct string_utils {
    uint32_t len;
    char *string;
};

//return a struct string_utils*
struct string_utils *parse_string(char *s);


//char * to byte array -> 1 char = 1 byte -> unsigned char[strlen(s)]
uint8_t *string_to_byte_array(char *s);


//return a char* from the byte tab
//len is the number of char on tab - also the number of bytes
char *byte_array_to_string(uint8_t *tab, uint32_t len);

//concat two array bytes
uint8_t *concat_byte_array(uint8_t *t1, uint32_t len1, uint8_t *t2, uint32_t len2);


//uintNN_t to byte array / tab need to be of len sizeof(uintNN_t)
uint8_t *uint16_to_byte_array(uint16_t n);
uint8_t *uint32_to_byte_array(uint32_t n);
uint8_t *uint64_to_byte_array(uint64_t n);


//free string_utils
void free_string_utils(struct string_utils *s);



#endif