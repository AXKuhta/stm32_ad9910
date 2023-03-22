typedef seq_entry_t vec_elem_t;

typedef struct vec_t {
	uint32_t size;
	uint32_t capacity;
	vec_elem_t* elements;
} vec_t;

vec_t* init_vec();
void free_vec(vec_t* vec);
void clear_vec(vec_t* vec);
void double_capacity(vec_t* vec);
void vec_push(vec_t* vec, vec_elem_t value);
void for_every_entry(vec_t* vec, void fn(vec_elem_t*));
