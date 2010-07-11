/**
 * Global stats.  Aligned on 64-bit boundaries for atomic ops.
 */
struct __stats {
    uint64_t      curr_bytes;
    uint64_t      total_evictions;
    uint64_t      listen_disabled_num;
    unsigned int  curr_items;
    unsigned int  pad0;
    unsigned int  total_items;
    unsigned int  pad1;
    unsigned int  curr_conns;
    unsigned int  pad2;
    unsigned int  total_conns;
    unsigned int  pad3;
    unsigned int  conn_structs;
    unsigned int  pad4;
    bool          accepting_conns;  /* whether we are currently accepting */
} _stats;

/* stats */
void stats_init(void);
void stats_reset(void);
void global_stats(ADD_STAT add_stats, void *c);
void stats_get_server_state(unsigned int *curr_conns,
                            unsigned int *total_conns,
                            unsigned int *conn_structs,
                            unsigned int *accepting_conns,
                            uint64_t *listen_disabled_num);
void stats_prefix_init(void);
void stats_prefix_clear(void);
void stats_prefix_record_get(const char *key, const size_t nkey, const bool is_hit);
void stats_prefix_record_delete(const char *key, const size_t nkey);
void stats_prefix_record_set(const char *key, const size_t nkey);
/*@null@*/
char *stats_prefix_dump(int *length);

#ifndef USE_ATOMIC_STATS
void stats_lock_work(void);
void stats_unlock_work(void);

#define STATS_LOCK()      \
{                         \
    stats_lock_work(); 

#define STATS_UNLOCK()    \
    stats_unlock_work();  \
}

#define STAT_INC(stat) _stats.stat++;
#define STAT_INC_BY(stat, amount) _stats.stat += amount;
#define STAT_DEC(stat) _stats.stat--;
#define STAT_DEC_BY(stat, amount) _stats.stat -= amount;
#define STAT_SET_TRUE(stat) _stats.stat = true;
#define STAT_SET_FALSE(stat) _stats.stat = false;
#define STAT_RESET(stat) _stats.stat = 0;

#else

#define STATS_LOCK()  /* NOP */
#define STATS_UNLOCK()  /* NOP */

#define STAT_INC(stat) __sync_fetch_and_add(&_stats.stat, 1);
#define STAT_INC_BY(stat, amount) __sync_fetch_and_add(&_stats.stat, amount);
#define STAT_DEC(stat) __sync_fetch_and_sub(&_stats.stat, 1);
#define STAT_DEC_BY(stat, amount) __sync_fetch_and_sub(&_stats.stat, amount);
#define STAT_SET_TRUE(stat) __sync_bool_compare_and_swap(&_stats.stat, false, true);
#define STAT_SET_FALSE(stat) __sync_bool_compare_and_swap(&_stats.stat, true, false);
#define STAT_RESET(stat) __sync_fetch_and_and(&_stats.stat, 0);

#endif
