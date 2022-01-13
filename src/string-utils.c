#include "string-utils.h"

struct string_utils *parse_string(char *s) {
    if(s == NULL) return NULL;

    struct string_utils *ret = malloc(sizeof(struct string_utils));
    if(ret == NULL) return NULL;

    ret->string = s;
    ret->len = strlen(s);
    return ret;
}

uint8_t *uint16_to_byte_array(uint16_t n) {
    uint8_t *tab = calloc(sizeof(uint16_t), sizeof(uint8_t)); //need to be set before working with bit operators
    if(tab == NULL) return NULL;

    n = htobe16(n);

    uint16_t mask = 0b1;
    uint8_t maskb = 0b1;
    int b = 0;
    while(mask != 0b0) {
        if((n & mask) == mask)
            tab[b/8] |= maskb;
            
        if((b+1)%8 == 0)
            maskb = 0b1;
        else
            maskb <<= 1;
        mask <<= 1;
        b++;
    }
    return tab;
}

uint8_t *uint32_to_byte_array(uint32_t n) {
    uint8_t *tab = calloc(sizeof(uint32_t), sizeof(uint8_t)); //need to be set before working with bit operators
    if(tab == NULL) return NULL;

    n = htobe32(n);
    uint32_t mask = 0b1;
    uint8_t maskb = 0b1;
    int b = 0;
    while(mask != 0b0) {
        if((n & mask) == mask)
            tab[b/8] |= maskb;
            
        if((b+1)%8 == 0)
            maskb = 0b1;
        else
            maskb <<= 1;
        mask <<= 1;
        b++;
    }
    return tab;
}

uint8_t *uint64_to_byte_array(uint64_t n) {
    uint8_t *tab = calloc(sizeof(uint64_t), sizeof(uint8_t)); //need to be set before working with bit operators
    if(tab == NULL) return NULL;

    n = htobe64(n);
    uint64_t mask = 0b1;
    uint8_t maskb = 0b1;
    int b = 0;
    while(mask != 0b0) {
        if((n & mask) == mask)
            tab[b/8] |= maskb;
            
        if((b+1)%8 == 0)
            maskb = 0b1;
        else
            maskb <<= 1;
        mask <<= 1;
        b++;
    }
    return tab;
}

uint8_t *string_to_byte_array(char *s) {
    if(s == NULL) return NULL;

    uint16_t len = strlen(s);
    uint8_t *tab = calloc(len, sizeof(uint8_t)); //need to be set before working with bit operators
    if(tab == NULL) return NULL;

    for(int i = 0; i < len; i++) {
        uint8_t c = s[i];
        
        uint8_t mask = 0b1;
        while(mask != 0b0) {
            if((c & mask) == mask)
                tab[i] |= mask;
            mask <<= 1;
        }
        mask = 0b0;
    }
    return tab;
}

char *byte_array_to_string(uint8_t *tab, uint32_t len) {
    if(tab == NULL) return NULL;
    if(len < 0) return NULL;

    char *s = malloc(len + 1);
    if(s == NULL) return NULL;

    s[len] = '\0';
    for(int i = 0; i < len; i++) {
        s[i] = tab[i];
    }
    return s;
}

uint8_t *concat_byte_array(uint8_t *t1, uint32_t len1, uint8_t *t2, uint32_t len2) {
    if(t1 == NULL || t2 == NULL) return NULL;
    if(len1 < 0 || len2 < 0) return NULL;

    uint8_t *concat = malloc(len1 + len2);
    if(concat == NULL) return NULL;

    int i = 0;
    for(int j = 0; j < len1; j++) {
        concat[i] = t1[j];
        i++;
    }
    
    for(int j = 0; j < len2; j++) {
        concat[i] = t2[j];
        i++;
    }
    return concat;
}

void free_string_utils(struct string_utils *s) {
    if(s == NULL) return;

    free(s->string);
    s->string = NULL;
    free(s);
    s = NULL;
}