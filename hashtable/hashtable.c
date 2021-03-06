#include "hashtable/hashtable.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <interpreter/YASL_Object.h>
#include <interpreter/YASL_string.h>

//#include "YASL_Object.h"
//#include "YASL_string.h"
#include "interpreter/refcount.h"

#define HT_BASESIZE 30

static int hash_function(const struct YASL_Object s, const int a, const int m) {
	long hash = 0;
	if (YASL_ISSTR(s)) {
		const int64_t len_s = yasl_string_len(s.value.sval);
		for (int i = 0; i < len_s; i++) {
			hash = (hash * a) ^ (s.value.sval->str + s.value.sval->start)[i];
			hash %= m;
		}
		return (int) hash;
	} else {
		long ll = s.value.ival & 0xFFFF;
		long lu = (s.value.ival & 0xFFFF0000) >> 16;
		long ul = (s.value.ival & 0xFFFF00000000) >> 32;
		long uu = (s.value.ival & 0xFFFF000000000000) >> 48;
		return (int) (((long) a * ll * ll * ll * ll ^ a * a * lu * lu * lu ^ a * a * a * ul * ul ^
			       a * a * a * a * uu) % m);
	}
}

static unsigned int get_hash(const struct YASL_Object s, const int num_buckets, const int attempt) {
	const int hash_a = hash_function(s, PRIME_A, num_buckets);
	if (attempt == 0) {
		return ((unsigned int)hash_a) % num_buckets;
	}
	const int hash_b = hash_function(s, PRIME_B, num_buckets);
	return ((unsigned int) (hash_a + (attempt * (hash_b + (hash_b == 0))))) % num_buckets;
}

static Item_t new_item(const struct YASL_Object k, const struct YASL_Object v) {
	Item_t item = {k, v};
	inc_ref(&item.value);
	inc_ref(&item.key);
	return item;
}

void del_item(Item_t *const item) {
	dec_ref(&item->key);
	dec_ref(&item->value);
}

struct Table *table_new_sized(const size_t base_size) {
	struct Table *table = (struct Table *)malloc(sizeof(struct Table));
	table->base_size = base_size;
	table->size = next_prime(table->base_size);
	table->count = 0;
	table->items = (Item_t *)calloc((size_t) table->size, sizeof(Item_t));
	return table;
}

struct Table *table_new(void) {
	return table_new_sized(HT_BASESIZE);
}

void table_del(struct Table *const table) {
	if (!table) return;
	FOR_TABLE(i, item, table) {
		del_item(item);
	}
	free(table->items);
	free(table);
}

struct RC_UserData *rcht_new_sized(const size_t base_size) {
        struct RC_UserData *ht = (struct RC_UserData *)malloc(sizeof(struct RC_UserData));
        ht->data = table_new_sized(base_size);
        ht->rc = rc_new();
        ht->tag = T_TABLE;
        ht->destructor = rcht_del_data;
        return ht;
}

struct RC_UserData *rcht_new(void) {
	return rcht_new_sized(HT_BASESIZE);
}

void rcht_del(struct RC_UserData *const hashtable) {
	table_del((struct Table *)hashtable->data);
	rc_del(hashtable->rc);
	free(hashtable);
}

void rcht_del_data(void *const hashtable) {
        table_del((struct Table *)hashtable);
}

void rcht_del_rc(struct RC_UserData *const hashtable) {
	rc_del(hashtable->rc);
	free(hashtable);
}

void table_del_string_int(struct Table *const table) {
	table_del(table);
}

static void table_resize(struct Table *const table, const size_t base_size) {
	if (base_size < HT_BASESIZE) return;
	struct Table *new_table = table_new_sized(base_size);
	FOR_TABLE(i, item, table) {
		table_insert(new_table, item->key, item->value);
	}
	table->base_size = new_table->base_size;
	table->count = new_table->count;

	const size_t tmp_size = table->size;
	table->size = new_table->size;
	new_table->size = tmp_size;

	Item_t *tmp_items = table->items;
	table->items = new_table->items;
	new_table->items = tmp_items;

	table_del(new_table);
}

static void table_resize_up(struct Table *table) {
	const size_t new_size = table->base_size * 2;
	table_resize(table, new_size);
}

static void table_resize_down(struct Table *table) {
	const size_t new_size = table->base_size / 2;
	table_resize(table, new_size);
}

void table_insert(struct Table *const table, const struct YASL_Object key, const struct YASL_Object value) {
	const int load = table->count * 100 / table->size;
	if (load > 70) table_resize_up(table);
	Item_t item = new_item(key, value);
	int index = get_hash(item.key, table->size, 0);
	Item_t curr_item = table->items[index];
	int i = 1;
	while (!YASL_ISUNDEF(curr_item.key)) {
		if (curr_item.key.type != Y_END) {
			if (!isfalsey(isequal(curr_item.key, item.key))) {
				del_item(&curr_item);
				table->items[index] = item;
				return;
			}
		}
		index = get_hash(item.key, table->size, i++);
		curr_item = table->items[index];
	}
	table->items[index] = item;
	table->count++;
}

void table_insert_string_int(struct Table *const table, const char *const key, const int64_t key_len, const int64_t val) {
	String_t *string = str_new_sized_heap(0, key_len, copy_char_buffer(key_len, key));
	struct YASL_Object ko = YASL_STR(string),
	  vo = YASL_INT(val);
	table_insert(table, ko, vo);
		     
		     
}

struct YASL_Object table_search(const struct Table *const table, const struct YASL_Object key) {
	size_t index = get_hash(key, table->size, 0);
	Item_t item = table->items[index];
	int i = 1;
	while (!YASL_ISUNDEF(item.key)) {
		if (!isfalsey(isequal(item.key, key))) {
			return item.value;
		}
		index = get_hash(key, table->size, i++);
		item = table->items[index];
	}
	return YASL_END();
}

struct YASL_Object table_search_string_int(const struct Table *const table, const char *const key, const int64_t key_len) {
	String_t *string = str_new_sized_heap(0, key_len, copy_char_buffer(key_len, key));
	struct YASL_Object object = YASL_STR(string);

	struct YASL_Object result = table_search(table, object);
	str_del(string);
	return result;
}

void table_rm(struct Table *table, struct YASL_Object key) {
	const int load = table->count * 100 / table->size;
	if (load < 10) table_resize_down(table);
	int index = get_hash(key, table->size, 0);
	Item_t item = table->items[index];
	int i = 1;
	while (!YASL_ISUNDEF(item.key)) {
		if (item.key.type != Y_END) {
			if (!isfalsey(isequal(item.key, key))) {
				del_item(&item);
				table->items[index] = TOMBSTONE;
			}
		}
		index = get_hash(key, table->size, i++);
		item = table->items[index];
	}
	table->count--;
}
