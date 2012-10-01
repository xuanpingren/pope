#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> /* for sleep */
#include "pope.h"

char *default_notify[] = {"1s", "12s", "1t", "7t"};  /* default notification time */

void *xmalloc(size_t size)
{
  register void *address = malloc(size);
  if (address == 0){
    fprintf(stderr, "Memory exhausted.\n");
    exit(1);
  }
  return address;
}


void error(char s[])
{
  printf("%s\n", s);
  exit(1);
}


int get_year(char s[])
{
  char *p;
  char year[5];
  time_t curtime;
  struct tm *t;

  while (isspace(*s))
    s++;

  p = strstr(s, "..");
  if (p) {
    strncpy(year, s, p-s);
    year[p-s] = '\0';
    return atoi(year);
  } else {
    curtime = time(NULL);
    t = localtime(&curtime);
    return (t->tm_year + 1900);
  }
}


int get_month(char s[])
{
  char *p, *q;
  char month[3];
  time_t curtime;
  struct tm *t;

  while (isspace(*s))
    s++;

  p = strrchr(s, '.');
  q = strstr(s, ".."); 
  if ((p && q && q + 1 == p) || (p == NULL)) { /* month unspecified, e.g., 2012..4, 23 */
    /* get current month */
    curtime = time(NULL);
    t = localtime(&curtime);
    return (t->tm_mon + 1);
  }

  if (q) { /* 2012..9.20 case */
    strncpy(month, q+2, p-q-2);
    month[p-q-2] = '\0';
  } else { /* 9.20 case */
    strncpy(month, s, p-s);
    month[p-s] = '\0';
  }

  return atoi(month);
}


int get_day(char s[])
{
  char *p, *q;
  char day[3];
  time_t curtime;
  struct tm *t;


  while (isspace(*s))
    s++;
  
  p = strrchr(s, '.');
  
  if (p == NULL) {
    q = strchr(s, ':');
    if (q - s < 3) {
      curtime = time(NULL);
      t = localtime(&curtime);
      return t->tm_mday;
    } else 
      strncpy(day, s, 2);
  } else 
    strncpy(day, p+1, 2);
  
  day[2] = '\0';
  return atoi(day);
}


int get_hour(char s[])
{
  char *p;
  int hour;

  p = strchr(s, ':');
  
  if (p == NULL) { /* search "am" or "pm"  */
    p = strstr(s, "am");
    if (p == NULL)
      p = strstr(s, "pm");
    if (p == NULL)
      return 0;
  }

  if (isdigit(*(p-2)) && isdigit(*(p-1)))
    hour =  10 * (*(p-2) - '0') + (*(p-1) - '0');
  else if (isdigit(*(p-1)))
    hour = *(p-1) - '0';
  else
    hour = 0;

  p = strstr(s, "pm");
  if (p && hour < 12)
    hour += 12;
  else {
    p = strstr(s, "am");
    if (p && hour >= 12)
      hour -= 12;
  }

  return hour;
}


int get_minute(char s[])
{
  char *p, *q;
  int minute = 0;
  
  p = strrchr(s, ':');
  q = strchr(s, ':');
  
  if (p == NULL)
    return 0;

  if (p > q) {
    q = q + 1;
    while (q < p) {
      minute = minute * 10 + (*q - '0');
      q++;
    }
  } else if (p == q) {
    q = q + 1;
    while (isdigit(*q)) {
      minute = minute * 10 + (*q - '0');
      q++;
    }
  }
    
  return minute;
}


int get_second(char s[])
{
  char *p, *q;
  int second = 0;

  p = strchr(s, ':');
  q = strrchr(s, ':');

  if (p == NULL || p == q)
    return 0;
  else {
    if (isdigit(*(q+1)) && isdigit(*(q+2)))
      second =  10 * (*(q+1) - '0') + (*(q+2) - '0');
    else if (isdigit(*(q+1)))
      second = *(q+1) - '0';
    else
      second = 0;
  }

  return second;
}


void make_date(record *r, char s[])
{
  r->time.tm_year = get_year(s) - 1900;
  r->time.tm_mon  = get_month(s) - 1;
  r->time.tm_mday = get_day(s);
  r->time.tm_hour = get_hour(s) - 1;
  r->time.tm_min  = get_minute(s);
  r->time.tm_sec  = get_second(s);
}


void make_message(record *r, char s[])
{
  while (isspace(*s)) /* skip heading spaces */
    s++;
  strcpy(r->msg, s);
}


unsigned long convert_to_second(char s[])
{
  unsigned long factor = 1;
  char *p;
  unsigned long n;
  
  n = strtoul(s, &p, 10);
  
  switch (tolower(*p)) {
  case 'm':
    factor = 1;
    break;
  case 'f':
    factor = 60;
    break;
  case 's':
    factor = 3600;
    break;
  case 't':
    factor = 86400;
    break;
  case 'y':
    factor = 2592000;
    break;
  case 'n':
    factor = 31104000;
    break;
  default:
    printf("%c: ", *p);
    error("Invalid unit for notification time");
  }

  return n * factor;
}


void make_pending_list(record *r, char s[])
{
  int i, j;
  char *number;
  char *p;

  r->num_notified = 0;

  i = 0;
  while ((number = strtok(i != 0 ? NULL : s, " ,")) != NULL && i < NOTIFY_SIZE) {
    p = strchr(number, '+');
    if (p != NULL) {
      r->notified[i] = 1;      
      r->num_notified++;
    } else 
      r->notified[i] = 0;
    r->notify[i] = convert_to_second(number);
    i++;
  }
  r->num_pending = i;

  if (i == 0) { /* set notification time at 1 hour, 12 hours, 1 day, 7 days */
    r->num_pending = 0;
    for (j = 0; j < array_size(default_notify); j++) {
      r->notify[j] = convert_to_second(default_notify[j]);
      r->notified[j] = 0;
      r->num_pending++;
    }
    
  }
}


void print_record(record r)
{
  int i;

  printf("line: %d : ", r.lineno);
  printf("year:%d, month:%d, day:%d, hour:%d, min:%d, sec:%d, ", r.time.tm_year, r.time.tm_mon, r.time.tm_mday, r.time.tm_hour, r.time.tm_min, r.time.tm_sec);

  printf("%s,  pending:%d, ", r.msg, r.num_pending);
  for (i = 0; i < r.num_pending; i++)
    printf("%lu  ", r.notify[i]);
  printf("\n\n");

}


record *parse_file(char *filename, int *n)
{
  FILE *fp;
  char line[MAX_LINE_SIZE];
  char date[DATE_SIZE];
  char msg[MSG_SIZE];
  char s[PREDATE_SIZE];
  char *p, *q;
  int lineno;
  record *r;

  if ((fp = fopen(filename, "r")) == NULL) {
    fprintf(stderr, "Cannot open file %s\n", filename);
    exit(1);
  }

  r = (record *) xmalloc(MAX_NUM_RECORD * sizeof(record));
  memset(r, 0, MAX_NUM_RECORD * sizeof(record));
  *n = 0;

  lineno = 0;
  while (fgets(line, MAX_LINE_SIZE, fp) != NULL) {
    p = strchr(line, DONE_SYMBOL);  /* current line does not contain a DONE_SYMBOL */
    if (p == NULL) {
      p = strchr(line, SEP);    
      if (p) {
	strncpy(date, line, p-line); 
	date[p-line] = '\0';
	/* printf("%s\n", date); */
	make_date(&r[*n], date);
      } else {
	printf("Warning: line %d is invalid (must contain at least a semicolon). [IGNORED]\n", lineno + 1);
	printf("For example, a line could look like this '2012..9.25 6:50pm; dinner'.\n");
	lineno++;
	continue;
      }
      
      q = strchr(p+1, SEP); /* looking for a 2nd separator SEP */
      if (q) {
	strncpy(msg, p+1, q-p-1);
	msg[q-p-1] = '\0';
	strcpy(s, q+1);
	make_pending_list(&r[*n], s);
      } else {
	strcpy(msg, p+1);
	msg[strlen(msg)-1] = '\0'; /* remove newline */
	make_pending_list(&r[*n], "");
      }
      make_message(&r[*n], msg);
      r[*n].lineno = lineno;
      strcpy(r[*n].line, line);  /* make a copy of the line */
      *n += 1;
    }
    lineno++;
  }
  
  fclose(fp);

  return r;
}


int smallest_notified(record *r)
{
  int i, min, index;

  min = r->notify[0];
  index = 0;
  for (i = 1; i < r->num_pending; i++)
    if (r->notify[i] <= min) {
      min = r->notify[i];
      index = i;
    }

  return r->notified[index] == 1;
}


void update_file(char *filename, record *r, int n)
{
  FILE *fp;
  char *buffer[MAX_NUM_LINE];
  char line[MAX_LINE_SIZE], temp[MAX_LINE_SIZE];
  char s[20];
  int lineno;
  int i, j;
  char *number;
  char *p, *q, *a;

  if ((fp = fopen(filename, "r")) == NULL) {
    fprintf(stderr, "Cannot open file %s\n", filename);
    exit(1);
  }

  lineno = 0;
  while (fgets(line, MAX_LINE_SIZE, fp) != NULL) {
    buffer[lineno] = (char *) xmalloc(MAX_LINE_SIZE);
    strcpy(buffer[lineno], line);
    lineno++;
  }

  fclose(fp);

  for (i = 0; i < n; i++) {

    p = strchr(r[i].line, SEP);
    q = strrchr(r[i].line, SEP);
    if (p == q) { /* only one SEP semicolon */
      a = strchr(r[i].line, '\n');
      strncpy(line, r[i].line, a - r[i].line);
      line[a-r[i].line] = '\0';
      strcat(line, ";");
      for (j = 0; j < r[i].num_pending; j++) {
	if (r[i].notified[j] == 0)
	  sprintf(s, "%s,", default_notify[j]);
	else if (r[i].notified[j] == 1)
	  sprintf(s, "%s+,", default_notify[j]);	  
	strcat(line, s);
      }
      /* strip last comma */
      a = strrchr(line, ',');
      *a = '\n';
      *(a+1) = '\0';
    } else { 
      j = 0;
      q = strrchr(r[i].line, SEP);
      strncpy(line, r[i].line, q - r[i].line + 1);
      line[q-r[i].line+1] = '\0';
      while ((number = strtok(j != 0 ? NULL : q + 1, " ,")) != NULL && j < NOTIFY_SIZE) {
	strcpy(s, number);
	/* get rid of spaces newline if there are any */
	p = strchr(s, '\n');
	if (p != NULL)
	  *p = '\0';
  	a = strchr(number, '+');
	if (r[i].notified[j] == 1 && a == NULL) 
	  strcat(s, "+");
	strcat(s, ",");
	strcat(line, s);
	j++;
      }
      /* strip last comma */
      a = strrchr(line, ',');
      *a = '\n';
      *(a+1) = '\0';
    }

    if (smallest_notified(&r[i])) {
      strcpy(temp, line);
      strcpy(line, "% ");
      strcat(line, temp);
      /* strcpy(line, "% "); 
	 strcat(line, "  %\n"); */
    }
    strcpy(buffer[r[i].lineno], line);
  }
  
  if ((fp = fopen(filename, "w")) == NULL) {
    fprintf(stderr, "Cannot open file %s for writing.\n", filename);
    exit(1);
  }

  for (i = 0; i < lineno; i++) {
    fprintf(fp, "%s", buffer[i]);
    free(buffer[i]);
  }
  
  fclose(fp);
}


/*
void sleep(time_t delay)
{
  time_t t0, t1;
  time(&t0);
  do {
    time(&t1);
  } while ((t1 - t0) < delay );
}
*/

double sec2min(double x)
{
  return x/60.0;
}

double sec2hr(double x)
{
  return x/3600.0;
}

double sec2day(double x)
{
  return x/(3600.0 * 24);
}

void notify(record *r, int n)
{
  int i, j;
  time_t curtime, futtime;
  struct tm now;
  double diff, low, high;
  char date[25];
  int *p;

  curtime = time(NULL);
  now = *localtime(&curtime);

  for (i = 0; i < n; i++) {
    /* printf("\nChecking record %d\n", i); */
    futtime = mktime(&(r[i].time));
    strcpy(date, asctime(&(r[i].time)));
    date[strlen(date)-1] = '\0'; 
    printf("Now: %s", asctime(&now));
    /* printf("Fut: %s", ctime(&futtime));  */
    diff = difftime(futtime, curtime);
    /* printf("Time difference: %f\n", diff); */
    if (diff < 0) {
      printf("Missed: %s [%s]\n", date, r[i].msg);      
    } else {
      for (j = r[i].num_pending - 1; j >= 0; j--) {
	if (j > 0)
	  low = r[i].notify[j-1];
	else
	  low = 0;
	high = r[i].notify[j];
	if (diff < high && diff >= low && r[i].notified[j] == 0) {
	  printf("++++ %s [%s]\n", date, r[i].msg);
	  p = &(r[i].notified[j]);
	  *p = 1;
	}
      }
    }
    
  }
}

