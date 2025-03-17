
#define vec_t(type) struct { \
	size_t size; \
	size_t capacity; \
	size_t element_size; \
	type* elements; \
}

typedef vec_t(void) void_vec_t;

#define unused __attribute__((unused))

unused static void* _init_vec(size_t element_size) {
	void_vec_t* vec = malloc(sizeof(void_vec_t));

	vec->size = 0;
	vec->capacity = 32;
	vec->element_size = element_size;
	vec->elements = malloc(vec->capacity * element_size);

	return vec;
}

unused static void _free_vec(void_vec_t* vec) {
	free(vec->elements);
	free(vec);
}

unused static void _clear_vec(void_vec_t* vec) {
	memset(vec->elements, 0, vec->size * vec->element_size);
	vec->size = 0;
}

unused static void _double_capacity(void_vec_t* vec) {
	vec->capacity *= 2;
	vec->elements = realloc(vec->elements, vec->capacity * vec->element_size);

	assert(vec->elements != NULL);
}

#define init_vec(type) (void*)_init_vec(sizeof(type))
#define vec_push(vec, ...) (vec->size == vec->capacity ? _double_capacity((void_vec_t*)vec) : 0, vec->elements[vec->size++] = __VA_ARGS__)
#define vec_pop(vec) vec->elements[assert(vec->size > 0), --vec->size]
#define vec_head(vec) vec->elements[assert(vec->size > 0), 0]
#define vec_tail(vec) vec->elements[assert(vec->size > 0), vec->size - 1]
#define free_vec(vec) _free_vec((void_vec_t*)vec)
#define clear_vec(vec) _clear_vec((void_vec_t*)vec)
#define for_every_entry(vec, fn) for (size_t i = 0; i < vec->size; i++) fn(vec->elements + i)
