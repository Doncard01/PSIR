/*
Tuple* in(TupleSpace *tSpace, Tuple *pattern) {
    pthread_mutex_lock(&tSpace->mutex);

    for (int i = 0; i < tSpace->count; ++i) {
        if (match_tuples(tSpace->entries[i].tuple, pattern)) {
            if (--tSpace->entries[i].count == 0) {
                // Remove the entry if the count is zero
                free_tuple(tSpace->entries[i].tuple);
                for (int j = i; j < tSpace->count - 1; ++j) {
                    tSpace->entries[j] = tSpace->entries[j + 1];
                }
                tSpace->count--;
            }

            pthread_mutex_unlock(&tSpace->mutex);
            return copy_tuple(tSpace->entries[i].tuple);
        }
    }

    pthread_mutex_unlock(&tSpace->mutex);
    return NULL;  // No matching tuple found
}

Tuple* inp(TupleSpace *tSpace, Tuple *pattern) {
    pthread_mutex_lock(&tSpace->mutex);

    for (int i = 0; i < tSpace->count; ++i) {
        if (match_tuples(tSpace->entries[i].tuple, pattern)) {
            pthread_mutex_unlock(&tSpace->mutex);
            return copy_tuple(tSpace->entries[i].tuple);
        }
    }

    pthread_mutex_unlock(&tSpace->mutex);
    return NULL;  // No matching tuple found
}

Tuple* rd(TupleSpace *tSpace, Tuple *pattern) {
    pthread_mutex_lock(&tSpace->mutex);

    for (int i = 0; i < tSpace->count; ++i) {
        if (match_tuples(tSpace->entries[i].tuple, pattern)) {
            pthread_mutex_unlock(&tSpace->mutex);
            return copy_tuple(tSpace->entries[i].tuple);
        }
    }

    pthread_mutex_unlock(&tSpace->mutex);
    return NULL;  // No matching tuple found
}

Tuple* rds(TupleSpace *tSpace, Tuple *pattern) {
    pthread_mutex_lock(&tSpace->mutex);

    for (int i = 0; i < tSpace->count; ++i) {
        if (match_tuples(tSpace->entries[i].tuple, pattern)) {
            pthread_mutex_unlock(&tSpace->mutex);
            return copy_tuple(tSpace->entries[i].tuple);
        }
    }

    pthread_mutex_unlock(&tSpace->mutex);
    return NULL;  // No matching tuple found
}

int match_tuples(Tuple *tuple1, Tuple *tuple2) {
    if (tuple1->num_fields != tuple2->num_fields) {
        return 0;  // Number of fields does not match
    }

    for (int i = 0; i < tuple1->num_fields; ++i) {
        if (tuple1->fields[i].type != tuple2->fields[i].type) {
            return 0;  // Types of fields do not match
        }

        if (tuple1->fields[i].type == TS_INT) {
            if (tuple1->fields[i].data.int_field != tuple2->fields[i].data.int_field) {
                return 0;  // Integer values do not match
            }
        } else if (tuple1->fields[i].type == TS_FLOAT) {
            if (tuple1->fields[i].data.float_field != tuple2->fields[i].data.float_field) {
                return 0;  // Float values do not match
            }
        } else if (tuple1->fields[i].type == TS_STRING) {
            if (strcmp(tuple1->fields[i].data.string_field.value, tuple2->fields[i].data.string_field.value) != 0) {
                return 0;  // String values do not match
            }
        }
    }

    return 1;  // All fields match
}

// Helper function to create a deep copy of a tuple
Tuple* copy_tuple(Tuple *src) {
    Tuple *dest = (Tuple *)malloc(sizeof(Tuple));
    dest->num_fields = src->num_fields;
    dest->fields = (Field *)malloc(dest->num_fields * sizeof(Field));

    for (int i = 0; i < dest->num_fields; ++i) {
        dest->fields[i].type = src->fields[i].type;

        if (src->fields[i].type == TS_INT) {
            dest->fields[i].data.int_field = src->fields[i].data.int_field;
        } else if (src->fields[i].type == TS_FLOAT) {
            dest->fields[i].data.float_field = src->fields[i].data.float_field;
        } else if (src->fields[i].type == TS_STRING) {
            dest->fields[i].data.string_field.length = src->fields[i].data.string_field.length;
            dest->fields[i].data.string_field.value = strdup(src->fields[i].data.string_field.value);
        }
    }

    return dest;
}
*/