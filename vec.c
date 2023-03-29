#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "timer.h"
#include "ad9910.h"
#include "sequencer.h"
#include "vec.h"

// Примитивный динамический массив
vec_t* init_vec() {
	vec_t* vec = malloc(sizeof(vec_t));

	vec->size = 0;
	vec->capacity = 32;
	vec->elements = malloc(vec->capacity * sizeof(vec_elem_t));

	return vec;
}

void free_vec(vec_t* vec) {
	free(vec->elements);
	free(vec);
}

void clear_vec(vec_t* vec) {
	memset(vec->elements, 0, vec->size * sizeof(vec_elem_t));
	vec->size = 0;
}

void double_capacity(vec_t* vec) {
	vec->capacity *= 2;
	vec->elements = realloc(vec->elements, vec->capacity * sizeof(vec_elem_t));

	assert(vec->elements != NULL);
}

void vec_push(vec_t* vec, vec_elem_t value) {
	if (vec->size == vec->capacity)
		double_capacity(vec);

	vec->elements[vec->size++] = value;
}

void for_every_entry(vec_t* vec, void fn(vec_elem_t*)) {
	for (uint32_t i = 0; i < vec->size; i++)
		fn(&vec->elements[i]);
}
