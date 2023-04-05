
// Примитивный динамический массив
// Реализован в рамках заголовочного файла ради возможности использовать разные типы данных
// Перед подключением должен быть объявлен тип данных:
// #define VEC_ELEM_TYPE char
// #include "vec.h"

#define unused __attribute__((unused))

#ifndef VEC_ELEM_TYPE
#error Please define VEC_ELEM_TYPE
#endif

typedef struct vec_t {
	uint32_t size;
	uint32_t capacity;
	VEC_ELEM_TYPE* elements;
} vec_t;

unused static vec_t* init_vec() {
	vec_t* vec = malloc(sizeof(vec_t));

	vec->size = 0;
	vec->capacity = 32;
	vec->elements = malloc(vec->capacity * sizeof(VEC_ELEM_TYPE));

	return vec;
}

unused static void double_capacity(vec_t* vec) {
	vec->capacity *= 2;
	vec->elements = realloc(vec->elements, vec->capacity * sizeof(VEC_ELEM_TYPE));

	assert(vec->elements != NULL);
}

unused static void vec_push(vec_t* vec, VEC_ELEM_TYPE value) {
	if (vec->size == vec->capacity)
		double_capacity(vec);

	vec->elements[vec->size++] = value;
}

unused static void free_vec(vec_t* vec) {
	free(vec->elements);
	free(vec);
}

unused static void clear_vec(vec_t* vec) {
	memset(vec->elements, 0, vec->size * sizeof(VEC_ELEM_TYPE));
	vec->size = 0;
}

unused static void for_every_entry(vec_t* vec, void fn(VEC_ELEM_TYPE*)) {
	for (uint32_t i = 0; i < vec->size; i++)
		fn(&vec->elements[i]);
}
