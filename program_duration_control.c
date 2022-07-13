#include <stdio.h> 
#include "stdlib.h"
#include <string.h>
#include "lib\sqlite3.h"
#include <time.h>

#define PROGRAM_NUM(program_list) (sizeof(program_list) / sizeof((program_list)[0]))

void split(char *src,const char *separator,char **dest,int *num);

/**
 * 每1分钟扫描一次
 * 如果存在进程
 *   如果存在记录
 *       查看记录时间 
 *    如果时间不是今天，删除记录，新增记录
 *    如果时间是今天
 *       查看持续时间是否超时
 *           超时则删除进程
 *           否则持续时间加一分钟
 **/

sqlite3 *db;

struct tm now;
time_t now_timestamp;

// long allow_duration_ordinary = 2*60;
long allow_duration_ordinary = 10;
long allow_duration_day_off = 5*60;

char *to_kill_pids[65525] = {0};
int to_kill_pid_num = 0;

char *program_list[] = {"wegame.exe"};

typedef struct Program_duration {
    int id;
    long start_time;
    int duration;
} *PD;

char *table = "CREATE TABLE IF NOT EXISTS \"main\" (\
  \"id\" integer(20) NOT NULL,\
  \"start_time\" TEXT,\
  \"duration\" TEXT,\
  PRIMARY KEY (\"id\")\
);";

void create() {
    sql_exec(table, NULL);
}

PD parse_CallBack(int f_num, char **f_val, char **f_nume) {
    PD pd = malloc(sizeof(PD));
    for (int i = 0; i < f_num; i++) {
        if (strcmp(f_nume[i], "id") == 0) 
            pd->id = atoi(f_val[i]);
        else if (strcmp(f_nume[i], "start_time") == 0)
            pd->start_time = atoi(f_val[i]);
        else if (strcmp(f_nume[i], "duration") == 0)
            pd->duration = atoi(f_val[i]);
    }
    return pd;
}

int exec_db_CallBack(void *para, int f_num, char **f_val, char **f_nume) {
    PD pd = parse_CallBack(f_num, f_val, f_nume);
    if (pd->id == NULL || pd->id == 0) {
        // 不存在记录，则新增
        insert();
        return 0;
    }
    
    if (is_today(pd->start_time)) {
        duration_handle(pd);
        return 0;
    }
    // update
    update_for_new_day(pd->id);
    return 0;
}

int count() {
    char *count_sql = "SELECT count(*) as count FROM main";
    char **result, *errmsg;
    int nrow, ncolumn, i, j, index;
    if (sqlite3_get_table(db, count_sql, &result, &nrow, &ncolumn, &errmsg) != 0) {
        printf("error : %s\n", errmsg);
        sqlite3_free(errmsg);
    }
    char *s = result[0];
    int count = atoi(result[1]);
    return count;
    
}

// After confirming program exists
void handle() {
    if (!count()) {
        insert();
        return;
    }

    char *sql;
    sql = "select * from main";
    // The logic is in the exec_db_CallBack
    sql_exec(sql, &exec_db_CallBack);

}

void update_for_new_day(int id) {
    char sql[100];
    sprintf(sql, "update main set start_time = %d, duration = 0 where id = %d", now_timestamp, id);
    sql_exec(sql, NULL);
}

void add_minute(int id, int duration) {
    char sql[100];
    sprintf(sql, "update main set duration = %d where id = %d", duration, id);
    sql_exec(sql, NULL);
}

void insert() {
    char sql[100];
    sprintf(sql, "insert into main (id, start_time, duration) values (1, %d, 0)", now_timestamp);
    sql_exec(sql, NULL);
}

// 处理超时
void duration_handle(PD pd) {
    long allow_duration = now.tm_wday > 5 ? allow_duration_day_off : allow_duration_ordinary;
    if (pd->duration > allow_duration) {
        // timeout for kill
        kill();
        return;
    }
    // duration add one minute
    add_minute(pd->id, (pd->duration) + 1);
}

int is_today(time_t t_time) {
    struct tm t_time_info;
    localtime_s(&t_time_info, &t_time);

    if (now.tm_year == t_time_info.tm_year && now.tm_mday == t_time_info.tm_mday) {
        return 1;
    }
    return 0;
}

void sql_exec(char *sql, sqlite3_callback Callback) {
    char *errmsg;
    // int nRes = sqlite3_exec(db, sql, exec_db_CallBack, NULL, &errmsg);
    char *s;
    if (sqlite3_exec(db, sql, Callback, s, &errmsg) != 0)  {
        printf("error: %s\n", sqlite3_errmsg(db));
    }
    printf("\n");
    return;
}

int exec_command(char *command, char *mode) {
    FILE *fp;

    if ((fp = _popen(command, mode)) == NULL) {
        perror("Fail to Open\n");
        return 1;
    }

    
    return 0;
}

void kill() {
    if (to_kill_pid_num > 0) {
        for (int i = 0; i < to_kill_pid_num; i++) {
            char command_prefix[50] = "taskkill /f /t /im ";
            char *to_kill_command = strcat(command_prefix, to_kill_pids[i]);
            system(to_kill_command);
        }
    }
}

/**
 * 追加到 to_kill_pids ， 总数to_kill_pid_num也追加
 **/
int get_to_kill_pids_real(char *program) {
    FILE *fp;
    //TODO program list
    char command[65525];
    sprintf(command, "tasklist | find \"%s\"", program);
    if ((fp = _popen(command, "r")) == NULL) {
        perror("Fail to Open\n");
        return;
    }
    int flag = 0;
    char buf[255] = {0};
    while (fgets(buf, 255, fp) != NULL) {
        char *revbuf[100] = {0};
        int num;
        split(buf," ", revbuf, &num);

        push(to_kill_pids, revbuf[1], 65525);
        to_kill_pid_num++;
        flag = 1;
    }
    return flag;
}

int get_to_kill_pids() {
    empty_to_kill_pids();
    int flag = 0;
    int c = PROGRAM_NUM(program_list);
    for (int i = 0; i < PROGRAM_NUM(program_list); i++) {
        if (program_list[i] != 0) {
            int flag_ = get_to_kill_pids_real(program_list[i]);
            flag = flag == 0 ? flag_ : flag;
        }
    }
    return flag;
}

void push(char *dest[], char *str, int d_num) {
    int offset = 0;
    for (int i = 0; i < d_num; i++) {
        if (dest[i] == NULL || dest[i] == 0) {
            offset = i;
            break;
        }
    }
    if (offset < d_num) {
        dest[offset] = str;
    }
}

void empty_to_kill_pids() {
    to_kill_pid_num = 0;
    for (int i = 0; i < 65525; i++) {
        to_kill_pids[i] = 0;
    }
}

void split(char *src,const char *separator,char **dest,int *num) {
     char *pNext;
     int count = 0;
     if (src == NULL || strlen(src) == 0)
        return;
     if (separator == NULL || strlen(separator) == 0)
        return;
     pNext = strtok(src,separator);
     while(pNext != NULL) {
          *dest++ = pNext;
          ++count;
         pNext = strtok(NULL,separator);
    }
    *num = count;
}

int open_sqlite3() {
    int r = sqlite3_open("program_duration_control.db", &db);
    if (r < 0) {
        printf("fail to sqlite3_open : %s\n", sqlite3_errmsg(db));
        return 0;
    }
    create();
    return 1;
}

int main() {
    while (1)
    {
        if (open_sqlite3()) {
            time(&now_timestamp);
            localtime_s(&now, &now_timestamp);


            int flag = get_to_kill_pids();
            if (flag) {
                handle();
            }
        }
        _sleep(2000);
    }
    
}