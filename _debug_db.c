#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <lmdb.h>
#include <stdio.h>
#include <string.h>

#define DB_COUNT_MAX 10
static const char *data_dir = "./data/";
static MDB_env *env;
static MDB_dbi db_properties;
static MDB_dbi db_balance;
static uint64_t p2pool_height_prev = 2706515;
static int64_t p2pool_balance_prev = 51954605520; // 51954605520 for test nothing to pay


int main(int argc, char* argv[])
{
    printf("_debug_db.c\n");
    if (argc != 3)
    {
        printf("Usage: %s <p2pool_height_prev> <p2pool_balance_prev>\n", argv[0]);
        return -1;
    }

    if (sscanf(argv[1], "%"SCNu64"", &p2pool_height_prev) != 1)
    {
        printf("bad p2pool_height_prev %s\n", argv[1]);
        return -2;
    }
    if (sscanf(argv[2], "%"SCNi64"", &p2pool_balance_prev) != 1)
    {
        printf("bad p2pool_balance_prev %s\n", argv[2]);
        return -3;
    }
    printf("Using p2pool_height_prev=%"PRIu64", p2pool_balance_prev=%"PRIi64"...\n", p2pool_height_prev, p2pool_balance_prev);

    int rc = 0;
    char *err = NULL;
    MDB_txn *txn = NULL;
    MDB_val k, v;
    rc = mdb_env_create(&env);
    mdb_env_set_maxdbs(env, (MDB_dbi) DB_COUNT_MAX);
    if ((rc = mdb_env_open(env, data_dir, 0, 0664)) != 0)
    {
        err = mdb_strerror(rc);
        printf("%s: (%s)\n", err, data_dir);
        return rc;
    }
    if ((rc = mdb_txn_begin(env, NULL, 0, &txn)))
    {
        err = mdb_strerror(rc);
        printf("%s\n", err);
        return rc;
    }
    if ((rc = mdb_dbi_open(txn, "properties", 0, &db_properties)) != 0)
    {
        err = mdb_strerror(rc);
        printf("%s\n", err);
        mdb_txn_abort(txn);
        return rc;
    }

    k.mv_data = "p2pool_height_prev";
    k.mv_size = strlen(k.mv_data);
    v.mv_data = &p2pool_height_prev;
    v.mv_size = sizeof(p2pool_height_prev);
    if ((rc = mdb_put(txn, db_properties, &k, &v, 0)))
    {
        err = mdb_strerror(rc);
        printf("%s\n", err);
        mdb_txn_abort(txn);
        return rc;
    }
    k.mv_data = "p2pool_balance_prev";
    k.mv_size = strlen(k.mv_data);
    v.mv_data = &p2pool_balance_prev;
    v.mv_size = sizeof(p2pool_balance_prev);
    if ((rc = mdb_put(txn, db_properties, &k, &v, 0)))
    {
        err = mdb_strerror(rc);
        printf("%s\n", err);
        mdb_txn_abort(txn);
        return rc;
    }

    if ((rc = mdb_dbi_open(txn, "balance", 0, &db_balance)) != 0)
    {
        err = mdb_strerror(rc);
        printf("%s\n", err);
        mdb_txn_abort(txn);
        return rc;
    }
    MDB_cursor *cursor = NULL;
    if ((rc = mdb_cursor_open(txn, db_balance, &cursor)) != 0)
    {
        err = mdb_strerror(rc);
        printf("%s\n", err);
        mdb_txn_abort(txn);
        return rc;
    }

    MDB_cursor_op op = MDB_FIRST;
    while (1)
    {
        MDB_val key;
        MDB_val val;
        rc = mdb_cursor_get(cursor, &key, &val, op);
        op = MDB_NEXT;
        if (rc == MDB_NOTFOUND)
            goto all_done;
        if (rc != 0)
        {
            err = mdb_strerror(rc);
            printf("%s\n", err);
            mdb_txn_abort(txn);
            return rc;
        }
        uint64_t zero_amount = 0;
        MDB_val new_val = {sizeof(zero_amount), (void*)&zero_amount};
        rc = mdb_cursor_put(cursor, &key, &new_val, MDB_CURRENT);
        if (rc != 0)
        {
            err = mdb_strerror(rc);
            printf("%s\n", err);
            mdb_txn_abort(txn);
            return rc;
        }
    }
all_done:
    if ((rc = mdb_txn_commit(txn)))
    {
        err = mdb_strerror(rc);
        printf("%s\n", err);
        mdb_txn_abort(txn);
        return rc;
    }
    printf("_debug_db.c success\n");
    return 0;
}
