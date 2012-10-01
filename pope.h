#define EVENT_FILE "file.txt"

#define DATE_SIZE  50
#define MSG_SIZE  200
#define PREDATE_SIZE 100
#define NOTIFY_SIZE  20
#define MAX_LINE_SIZE  300
#define MAX_NUM_LINE  5000
#define MAX_NUM_RECORD  1000
#define SEP  ';'  /* separator of items in each line */
#define DONE_SYMBOL  '%'
#define array_size(x) (sizeof((x))/sizeof((*x)))

typedef struct {
  unsigned long val; 
  int ord;} tuple;

typedef struct {
  struct tm time;
  char msg[MSG_SIZE];
  unsigned long notify[NOTIFY_SIZE];
  int notified[NOTIFY_SIZE];
  int num_pending;
  int num_notified;
  char line[MAX_LINE_SIZE];  /* a copy of the original line */
  unsigned int lineno;  /* the line number of the record in the orignal file */
} record;


record *parse_file(char *filename, int *n);
void update_file(char *filename, record *r, int n);
void *xmalloc(size_t size);

