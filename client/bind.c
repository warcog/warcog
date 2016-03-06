#include "bind.h"
#include "resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//TODO improve

void bind_init(bindlist_t *b, const abilitydef_t *def, unsigned count, unsigned map)
{
    data_t file;
    bind_t *p;
    unsigned i, j, k, id;

    if (!read_file(&file, "binds", 0))
        return;

    if (file.len % sizeof(bind_t)) {
        free(file.data);
        return;
    }

    memset(b->data, 0, sizeof(b->data)); //

    for (i = 0, j = 0, k = 0, p = (void*) file.data; i < file.len / sizeof(bind_t); i++) {
        if (p[i].map != map) {
            p[k++] = p[i];
            continue;
        }

        switch (p[i].info & bind_type) {
        case bind_ability:
            for (id = 0; id < count; id++)
                if (p[i].ability == def[id].hash)
                    break;

            if (id == count || b->data[id].key_int)
                continue;
            break;
        case bind_builtin:
            if (p[i].ability >= 16)
                continue;
            id = 32768 + p[i].ability;
            break;
        default:
            assert(0);
        }

        b->data[id] = p[i];
        b->list[j++] = id;
    }

    printf("loaded %u binds for this map\n", j);

    b->count = j;
    b->extra_count = k;
    b->extra = p;
}

void bind_set(bindlist_t *b, unsigned id, bind_t data)
{
    assert(data.key_int);

    if (!b->data[id].key_int)
        b->list[b->count++] = id;

    b->data[id] = data;
}

void bind_save(bindlist_t *b)
{
    FILE *file;
    unsigned i;

    file = fopen("binds", "wb");
    if (!file)
        return;

    fwrite(b->extra, 1, b->extra_count * sizeof(bind_t), file);
    for (i = 0; i < b->count; i++)
        fwrite(&b->data[b->list[i]], 1, sizeof(bind_t), file);
    fclose(file);

    free(b->extra);
    b->extra = 0;
}

bool bind_match(bind_t b, unsigned key, uint8_t *keys, unsigned num_key)
{
    unsigned i, j;

    j = b.info & 3;

    if (num_key > j)
        return 0;

    for (i = 0; i < j; i++)
        if (b.keys[i] != keys[i])
            return 0;

    if (key != b.keys[2])
        return 0;

    return 1;
}
