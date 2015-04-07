#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <db.h>

#define DATABASE "texas.db"
DB *g_db;

int texas_db_init()
{
    int ret;

    if ((ret = db_create(&g_db, NULL, 0)) != 0) {
        fprintf(stderr, "db_create: %s\n", db_strerror(ret));
        return -1;
    }
    if ((ret = g_db->open(g_db, NULL, DATABASE, NULL, DB_BTREE, DB_CREATE, 0664)) != 0) {
        g_db->err(g_db, ret, "%s", DATABASE);
        return -1;
    }
    return 0;
}

int texas_db_close()
{
    int ret;

    if ((ret = g_db->close(g_db, 0)) != 0 && ret != 0) {
        return -1;
    }
    return 0;
}

int texas_db_put(void *key, size_t key_len, void *value, size_t val_len)
{
    int ret;
    DBT k, v;

    memset(&k, 0, sizeof(k));
    memset(&v, 0, sizeof(v));
    k.data = key;
    k.size = key_len;
    v.data = value;
    v.size = val_len;

    if ((ret = g_db->put(g_db, NULL, &k, &v, 0)) != 0) {
        g_db->err(g_db, ret, "DB->put");
        return -1;
    }
    if ((ret = g_db->sync(g_db, 0)) != 0) {
        g_db->err(g_db, ret, "DB->sync");
        return -1;
    }
    return 0;
}

int texas_db_get(void *key, size_t key_len, void *value, size_t *val_len)
{
    int ret;
    DBT k, v;

    memset(&k, 0, sizeof(k));
    memset(&v, 0, sizeof(v));
    k.data = key;
    k.size = key_len;
    v.data = NULL;
    v.size = 0;

    if ((ret = g_db->get(g_db, NULL, &k, &v, 0)) != 0) {
        g_db->err(g_db, ret, "DB->get");
        return -1;
    }
    memcpy(value, v.data, v.size);
    *val_len = v.size;
    return 0;
}
