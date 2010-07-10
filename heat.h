int start_slab_heat_calc_thread(void);
void stop_slab_heat_calc_thread(void);

#define HEAT_DEBUG 1

#ifdef HEAT_DEBUG
#define STR(s) #s
#define XSTR(s) STR(s)
#define DEBUG_PRINT_READ_WRITE(TYPE)                  \
{                                                     \
  int i;                                              \
  fprintf(stderr, "write_" XSTR(TYPE) ":");           \
  for (i = 0; i < MAX_NUMBER_OF_SLAB_CLASSES; i++) {  \
    fprintf(stderr, "%lu ", write_ ## TYPE[i]);       \
  }                                                   \
  fprintf(stderr, "\nread_" XSTR(TYPE) ":");          \
  for (i = 0; i < MAX_NUMBER_OF_SLAB_CLASSES; i++) {  \
    fprintf(stderr, "%lu ", read_ ## TYPE[i]);        \
  }                                                   \
  fprintf(stderr, "\n");                              \
}

#define DEBUG_PRINT_READ_WRITE_INT(TYPE)              \
{                                                     \
  int i;                                              \
  fprintf(stderr, "write_" XSTR(TYPE) ":");           \
  for (i = 0; i < MAX_NUMBER_OF_SLAB_CLASSES; i++) {  \
    fprintf(stderr, "%li ", write_ ## TYPE[i]);       \
  }                                                   \
  fprintf(stderr, "\nread_" XSTR(TYPE) ":");          \
  for (i = 0; i < MAX_NUMBER_OF_SLAB_CLASSES; i++) {  \
    fprintf(stderr, "%li ", read_ ## TYPE[i]);        \
  }                                                   \
  fprintf(stderr, "\n");                              \
}

#define DEBUG_PRINT_HEARTBEAT(STR) fprintf(stderr, STR)
#else
#define DEBUG_PRINT_READ_WRITE(TYPE) /* NOP */
#define DEBUG_PRINT_HEARTBEAT(STR) /* NOP */
#endif
