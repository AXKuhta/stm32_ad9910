#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "vec.h"

//
// This is a minimal JSON parser designed specifically to use little RAM
//
// It trades speed for memory.
//
// The operation centers around queries.
//
// Prior art:
// https://www.codeproject.com/Articles/885389/jRead-an-in-place-JSON-element-reader
// https://github.com/recp/json
//

typedef enum scope_t {
	ROOT,
	OBJECT,
	KEY,
	VALUE,
	STR,
	ARRAY,
	PRIMITIVE
} scope_t;

typedef struct span_t {
	size_t idx;
	size_t sz;
} span_t;

typedef struct json_ctx_t json_t;

typedef struct json_ctx_t {
	vec_t(scope_t)* stack;
	vec_t(span_t)* path;
	vec_t(int)* acc;

	const char* start;
	const char* x;

	void (*leave_cb)(json_t* self, scope_t scope);
	void* extra;

	// Location counter
	size_t locs;

	// Used by:
	// - Key handlers
	// - String handlers
	// - Primitive handlers
	size_t span_idx;
	size_t span_sz;
} json_t;

#define ENCURL     '{'
#define DECURL     '}'
#define ENBOX      '['
#define DEBOX      ']'
#define QUOT       '"'
#define COLON      ':'
#define COMMA      ','
#define ESCAPE     '\\'

#define WHITESPACE " \r\n\t"
#define ROOT_ALLOWED   (WHITESPACE "{" "[")
#define OBJECT_ALLOWED (WHITESPACE "}" "\"" ":")

#define FLOAT_EXTRA ("-.")

// Similar to strchr() but does not treat \0 as findable, ever
// Compiler may generate optimized code for short strings
static int includes(const char* str, char c) {
	return memchr( str, c, strlen(str) ) != NULL;
}

// Scope entry
//
// Handles path maintenance
static void json_enter(json_t* self, scope_t scope) {
	switch (scope) {
		case KEY:
		case STR:
		case PRIMITIVE:
			self->span_idx = (self->x - self->start);
			self->span_sz = 0;
			break;
		case OBJECT:
		case ARRAY:
			vec_push(self->acc, 0);
			break;
		case VALUE: // Every value has a path to it
			switch ( vec_tail(self->stack) ) {
				case OBJECT: // Append key to path
					vec_push(self->path, (span_t){ .idx = self->span_idx + 1, .sz = self->span_sz });
					break;
				case ARRAY: // Append array index to path
					vec_push(self->path, (span_t){ .idx = vec_tail(self->acc), .sz = 0 });
					break;
				default:
					assert(0);
			}
			self->locs++;
			break;
		default:
			assert(0);
	}

	vec_push(self->stack, scope);
}

// Print path segments
//
// The meaning of span_t:
// - When sz > 0, (idx, sz) forms a text location vector, a span of text
// - When sz == 0, idx is an array index
static void json_print_path(json_t* self) {
	for (size_t i = 0; i < self->path->size; i++) {
		if (self->path->elements[i].sz) {
			printf("%.*s ",
				self->path->elements[i].sz,
				self->path->elements[i].idx + self->start);
		} else {
			printf("[%zu] ", self->path->elements[i].idx);
		}
	}
}

// Scope exit
//
// Invokes the callback, which might:
// - Test paths against target
static void json_leave(json_t* self, scope_t scope) {
	self->leave_cb(self, scope);

	assert( vec_pop(self->stack) == scope );

	switch (scope) {
		case STR:
			//json_print_path(self);
			//printf("STR<%.*s>\n", self->span_sz, self->start + self->span_idx + 1);
			break;
		case PRIMITIVE:
			//json_print_path(self);
			//printf("PRIMITIVE<%.*s>\n", self->span_sz, self->start + self->span_idx);
			break;
		case OBJECT:
			//printf("Close object of %d items\n", vec_tail(self->acc));
			(void)vec_pop(self->acc);
			break;
		case ARRAY:
			//printf("Close array of %d items\n", vec_tail(self->acc));
			(void)vec_pop(self->acc);
			break;
		case VALUE:
			(void)vec_pop(self->path);
			break;
		default:
			break;
	}
}

static void json_root(json_t* self) {
	assert( includes(ROOT_ALLOWED, *self->x) );

	switch (*self->x) {
		case ENCURL:
			json_enter(self, OBJECT);
			break;
		case ENBOX:
			json_enter(self, ARRAY);
			json_enter(self, VALUE);
			break;
	}
}

static void json_tick(json_t* self) {
	vec_tail(self->acc)++;
}

static void json_object(json_t* self) {
	assert( includes(OBJECT_ALLOWED, *self->x) );

	switch (*self->x) {
		case DECURL:
			json_leave(self, OBJECT);
			break;
		case QUOT:
			json_enter(self, KEY);
			break;
		case COLON:
			json_enter(self, VALUE);
			break;
	}
}


static void json_key(json_t* self) {
	assert( isascii(*self->x) );

	switch (*self->x) {
		case QUOT:
			json_leave(self, KEY);
			break;
		default:
			self->span_sz++;
			break;
	}
}

// Forward declaration required
static void json_primitive(json_t* self);

// Strings, objects and arrays have a prefix
// primitives do not, so we detect them
// after the fact, back down and invoke
// the primitive handler
static void json_value(json_t* self) {
	if ( isalnum(*self->x) || includes(FLOAT_EXTRA, *self->x) ) {
		json_tick(self);
		json_enter(self, PRIMITIVE);
		json_primitive(self);
	} else switch (*self->x) {
		case QUOT:
			json_tick(self);
			json_enter(self, STR);
			break;
		case ENCURL:
			json_tick(self);
			json_enter(self, OBJECT);
			break;
		case ENBOX:
			json_tick(self);
			json_enter(self, ARRAY);
			json_enter(self, VALUE);
			break;
		case COMMA:
			json_leave(self, VALUE);
			if (vec_tail(self->stack) == ARRAY)
				json_enter(self, VALUE);
			break;
		case DECURL: // Special exit
			json_leave(self, VALUE);
			json_leave(self, OBJECT);
			break;
		case DEBOX: // Special exit
			json_leave(self, VALUE);
			json_leave(self, ARRAY);
			break;
	}
}

// Similarly, primitives lack a distinctive suffix
// So we detect end of primitive after the fact
// and invoke the value handler
static void json_primitive(json_t* self) {
	if (isalnum(*self->x) || includes(FLOAT_EXTRA, *self->x) ) {
		self->span_sz++;
	} else {
		json_leave(self, PRIMITIVE);
		json_value(self);
	}
}


// Escape sequences not supported for now
static void json_str(json_t* self) {
	switch (*self->x) {
		case QUOT:
			json_leave(self, STR);
			break;
		case ESCAPE:
			assert(0);
		default:
			self->span_sz++;
	}
}

// State handler dispatch
// Invoke this from a loop
static void json_dispatch(json_t* self) {
	//printf("%c : %d\n", *self->x, vec_tail(self->stack));

	switch (vec_tail(self->stack)) {
		case ROOT: 	json_root(self); 	break;
		case OBJECT: 	json_object(self); 	break;
		case KEY: 	json_key(self); 	break;
		case VALUE: 	json_value(self); 	break;
		case STR: 	json_str(self); 	break;
		case PRIMITIVE: json_primitive(self); 	break;
		default:
			assert(0);
	}

	self->x++;
}

// Advance until \0 is found
void json_parse_until_end(json_t* self) {
	while (*self->x) json_dispatch(self);
}

// Advance until a new location is found
void json_parse_until_new_location(json_t* self) {
	size_t prev = self->locs;

	while (*self->x) {
		json_dispatch(self);

		if (self->locs > prev)
			return;
	}
}

// Path testing
// - When the level of nesting differs, no match
// - Test backwards
//	- Ought to go quicker because tree prefixes wont be matched repeatedly
//	- When query element size != span size, no match
//
// Array indices are treated as if they were object keys, i.e.
// ["u", "v", "w"] is like { "0": "u", "1": "v", "2": "w" }
//
static int json_match_path(json_t* self, size_t levels, const char* query[]) {
	if (self->path->size != levels)
		return 0;

	for (size_t i = self->path->size; i-- > 0;) {
		span_t span = self->path->elements[i];
		const char* a = self->start + span.idx;
		const char* b = query[i];

		if (span.sz) {
			if (strlen(b) != span.sz)
				return 0;

			if (memcmp(a, b, span.sz) != 0)
				return 0;
		} else {
			int idx = strtoul(b, NULL, 0);

			if (idx != span.idx)
				return 0;
		}
	}

	return 1;
}

static void noop(json_t* self, scope_t scope) {
	(void)self;
	(void)scope;
}

static size_t level_count(const char* query[]) {
	for (size_t levels = 0;; levels++)
		if (query[levels] == NULL)
			return levels;
}

// Query location in text
// Returns -1 on no match
int json_query_location(const char* json, const char* query[]) {
	json_t ctx = {
		.stack = init_vec(scope_t),
		.path = init_vec(span_t),
		.acc = init_vec(int),
		.leave_cb = noop,
		.start = json,
		.x = json,
	};

	vec_push(ctx.stack, ROOT);

	int result = -1;

	// Query prep
	size_t levels = level_count(query);

	while (*ctx.x) {
		json_parse_until_new_location(&ctx);
		//json_print_path(&ctx);
		//printf("\n");

		if (json_match_path(&ctx, levels, query)) {
			result = ctx.x - ctx.start;
			break;
		}
	}

	printf("Memory:\n");
	printf("Stack: %zu\n", sizeof(ctx));
	printf("Vec A: %zu\n", sizeof(scope_t) * ctx.stack->capacity);
	printf("Vec B: %zu\n", sizeof(span_t) * ctx.path->capacity);
	printf("Vec C: %zu\n", sizeof(int) * ctx.acc->capacity);

	free_vec(ctx.stack);
	free_vec(ctx.path);
	free_vec(ctx.acc);

	return result;
}

typedef struct count_query_t {
	const char** query;
	size_t levels;
	int status;
} count_query_t;

static void count_query_cb(json_t* self, scope_t scope) {
	count_query_t* extra = self->extra;

	if (scope == ARRAY || scope == OBJECT) {
		if (json_match_path(self, extra->levels, extra->query)) {
			extra->status = vec_tail(self->acc);
		}
	}
}

int json_query_count(const char* json, const char* query[]) {
	count_query_t extra = {
		.levels = level_count(query),
		.query = query,
		.status = -1
	};

	json_t ctx = {
		.stack = init_vec(scope_t),
		.path = init_vec(span_t),
		.acc = init_vec(int),
		.leave_cb = count_query_cb,
		.extra = &extra,
		.start = json,
		.x = json,
	};

	vec_push(ctx.stack, ROOT);

	int result = -1;

	// Query prep
	size_t levels = 0;

	while (*ctx.x) {
		json_dispatch(&ctx);

		if (extra.status != -1) {
			break;
		}
	}

	printf("Memory:\n");
	printf("Stack: %zu\n", sizeof(ctx) + sizeof(extra));
	printf("Vec A: %zu\n", sizeof(scope_t) * ctx.stack->capacity);
	printf("Vec B: %zu\n", sizeof(span_t) * ctx.path->capacity);
	printf("Vec C: %zu\n", sizeof(int) * ctx.acc->capacity);

	free_vec(ctx.stack);
	free_vec(ctx.path);
	free_vec(ctx.acc);

	return extra.status;
}

int json_query_i32(const char* json, const char* query[]) {
	int loc = json_query_location(json, query);

	assert(loc != -1);

	const char* start = json + loc;
	char* end = NULL;

	int result = strtol(start, &end, 0);

	assert(start != end);

	return result;
}
