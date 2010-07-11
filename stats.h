/**
 * Global stats.
 */
struct __stats {
    pthread_mutex_t mutex;
    unsigned int  curr_items;
    unsigned int  total_items;
    uint64_t      curr_bytes;
    unsigned int  curr_conns;
    unsigned int  total_conns;
    unsigned int  conn_structs;
    uint64_t      total_evictions;
    time_t        started;          /* when the process was started */
    bool          accepting_conns;  /* whether we are currently accepting */
    uint64_t      listen_disabled_num;
} stats;

#define STAT_INC(stat) stats.stat++;
#define STAT_INC_BY(stat, amount) stats.stat += amount;
#define STAT_DEC(stat) stats.stat--;
#define STAT_DEC_BY(stat, amount) stats.stat -= amount;
#define STAT_SET(stat, value) stats.stat = value;
#define STAT_RESET(stat) stats.stat = 0;

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

void stats_lock_work(void);
void stats_unlock_work(void);

#define STATS_LOCK()      \
{                         \
    stats_lock_work(); 

#define STATS_UNLOCK()    \
    stats_unlock_work();  \
}
