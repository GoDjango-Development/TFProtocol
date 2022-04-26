/*
    Coder: Bruno
    Email: rbruno2k15@outlook.com
    
    Reviewer: Luis Miguel
    Email: lmdelbahia@gmail.com
*/

#include <stdlib.h>
#include <stdio.h>
#include <mysql.h>

#include <string.h>

#include "xs_mysql.h"

/* infraestructure list.h/c here to avoid multiple-definitions */

#include <mysql.h>
#include <inttypes.h>

typedef struct node_list
{
    MYSQL* handler;
    struct node_list* next;
    struct node_list* prev;
    int64_t id;

} node_list;

typedef struct list
{
    node_list* head;
    node_list* tail;
    int size;

} list;

static list* new_list();

static node_list* find(list*, int);

static int64_t push_back(list*, MYSQL*);

static void erase(list*, node_list*);

static void free_list(list* _list);

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static list* new_list()  
{
    list* _list = (list*)malloc(sizeof(list));

    _list -> head = NULL;
    _list -> tail = NULL;
    _list -> size = 0;

    return _list;
}

static int64_t push_back(list* _list, MYSQL* handler)
{
    node_list* new_node = (node_list*)malloc(sizeof(node_list));

    new_node -> handler = handler;
    new_node -> next = NULL; 
    
    if(_list -> size == 0)
    {
        new_node -> prev = NULL;
        _list -> head = new_node;
        _list -> tail = new_node; 
        new_node -> id = 1;
    }
    else
    {
        new_node -> prev = _list -> tail;  
        _list -> tail -> next = new_node;        
        _list -> tail = new_node;
        new_node -> id = new_node -> prev -> id + 1;
    }

    _list -> size++;

    return new_node -> id;
}

static node_list* find(list* _list, int id)
{
    if(_list -> size == 0)
        return NULL;

    node_list* current = _list -> head;

    while(current -> id != id) 
    {
        if(current -> next == NULL) 
            return NULL;
        else 
            current = current -> next;        
    }      

    return current;
}

static void erase(list* _list, node_list* node)
{
    if(node -> prev == NULL && node -> next == NULL)
    {
        _list -> head = NULL;
        _list -> tail = NULL;
    }
    else if(node -> prev == NULL)
    {
        _list -> head -> prev = NULL;
        _list -> head = node -> next;
    }
    else if(node -> next == NULL)
    {
        _list -> tail = node -> prev;
        _list -> tail -> next = NULL;
    }
    else
    {
        node -> next -> prev = node -> prev;
        node -> prev -> next = node -> next;
    }
    
    _list -> size--;  

    free(node);
}

static void free_list(list* _list)
{
    node_list* current;

    while(_list -> head != NULL)
    {
        current = _list -> head;
        _list -> head = _list -> head -> next;
        mysql_close(current -> handler);
        free(current);
    }
}

/* end list.h/c */

static char bufrsp[RESPONSE_MAX];

static int isbigendian(void)
{
    int value = 1;
    char *pt = (char *) & value;
    if (*pt == 1)
        return 0;
    return 1;
}

static int read_command(IO_R readbuf, char** buffer)
{
    int64_t buffer_size;
    int rc = readbuf((char*)&buffer_size, sizeof(int64_t));
    if(rc == -1) return -1;    
    
    if(!isbigendian()) swapbo64(buffer_size);
    

    if(strlen(*buffer) < buffer_size + 1)
    {
        char* temp = (char*)realloc(*buffer, buffer_size + 1);
        if(temp) *buffer = temp;
        else return 0;     
    }

    rc = readbuf(*buffer, buffer_size);
    if(rc == -1) return -1;

    (*buffer)[buffer_size] = '\0';

    return 0;
}

static int write_command(IO_W writebuf, char* buffer)
{
    int64_t buffer_size = strlen(buffer);
    if(!isbigendian()) swapbo64(buffer_size);

    int rc = writebuf((char*)&buffer_size, sizeof(int64_t));
    if(rc == -1) return -1;

    if(buffer_size) return writebuf(buffer, strlen(buffer));

    return 0;
}

static int command_open(list* _list, IO_W writebuf)
{
    char *host = strtok(NULL, " ");
    char *s_port = strtok(NULL, " ");
    char* user = strtok(NULL, " ");
    char* pass = strtok(NULL, " ");
    char* db = strtok(NULL, "");

    int port;
    sscanf(s_port, "%d", &port);
    
    MYSQL *handler = mysql_init(NULL);

    if (!handler)
    {
        sprintf(bufrsp, "2 FAILED OPEN MYSQL INIT");
        return write_command(writebuf, bufrsp);    
    } 
    if (!mysql_real_connect(handler, host, user, pass, db, port, NULL, 0))
    {
        sprintf(bufrsp, "3 FAILED OPEN MYSQL CONNECTION");
        return write_command(writebuf, bufrsp);
    } 

    int64_t id = push_back(_list, handler);    
    sprintf(bufrsp, "1 BD OPENED OK WITH ID %lu", id);
    
    return write_command(writebuf, bufrsp);
}

static int command_close(list* _list, IO_W writebuf)
{
    int64_t id;
    char* db_id_string = strtok(NULL, "");
    sscanf(db_id_string, "%lu", &id);

    node_list* node = find(_list, id);

    if(!node) {
        sprintf(bufrsp, "5 FAILED CLOSE BD DOES NOT EXISTS");
        return write_command(writebuf, bufrsp);
    }

    erase(_list, node);
    sprintf(bufrsp, "4 CLOSE OK");
    return write_command(writebuf, bufrsp);
}

static int write_result(MYSQL* handler, IO_W writebuf, char* path)
{
    MYSQL_RES *result = mysql_store_result(handler);

    if (!result) return 0;

    FILE* pf;
    if(path) pf = fopen(path, "a");

    unsigned int num_fields;
    MYSQL_FIELD *fields;

    num_fields = mysql_num_fields(result);
    fields = mysql_fetch_fields(result);

    int header_size = 0;
    int i;
    for(i = 0; i < num_fields; i++)
    {
        header_size += strlen(fields[i].name);
        if(i < num_fields - 1) header_size += 2;
    }
    
    char* header = (char*)malloc(header_size + 1);
    int header_counter = 0;

    for(i = 0; i < num_fields; i++)
    {
        strcpy(header + header_counter, fields[i].name);
        header_counter += strlen(fields[i].name);
        if(i < num_fields - 1)
        {
            header[header_counter] = '@';
            header[header_counter + 1] = '@';
            header_counter += 2;
        }
    }

    int rc = 0;

    if(path) fprintf(pf, "%s\n", header);
    else
    {
        rc = write_command(writebuf, header);
        if(rc == -1) 
        {
            free(header);
            return -1;
        }            
    }

    free(header);

    MYSQL_ROW row;

    while ((row = mysql_fetch_row(result)))
    {
        int buffer_size = 0;

        for(i = 0; i < num_fields; ++i)
        {
            buffer_size += strlen(row[i] ? row[i] : "NULL");
            if(i < num_fields - 1) buffer_size += 2;
        }
        
        char* buffer = (char*)malloc(buffer_size + 1);
        int buffer_counter = 0;
        for(i = 0; i < num_fields; ++i)
        {
            strcpy(buffer + buffer_counter, row[i] ? row[i] : "NULL");
            buffer_counter += strlen(row[i] ? row[i] : "NULL");
            if(i < num_fields - 1)
            {
                buffer[buffer_counter] = '@';
                buffer[buffer_counter + 1] = '@';
                buffer_counter += 2;
            }
        }

        if(path) fprintf(pf, "%s\n", buffer);
        else
        {
            rc = write_command(writebuf, buffer);
            if(rc == -1) 
            {
                free(buffer);
                return -1;
            }
        }
    
        free(buffer);
    }

    if(path) fclose(pf);

    mysql_free_result(result);
    
    return rc;
}

static int command_exec(list* _list, IO_W writebuf)
{
    int64_t id;
    char* db_id_string = strtok(NULL, " ");
    sscanf(db_id_string, "%lu", &id);

    char* sql = strtok(NULL, "");

    node_list* node = find(_list, id);

    if(!node) {
        sprintf(bufrsp, "7 FAILED EXEC BD DOES NOT EXISTS");
        return write_command(writebuf, bufrsp);
    } 
        
    if(mysql_query(node -> handler, sql)) 
    {
        sprintf(bufrsp, "8 FAILED EXEC MYSQL");
        return write_command(writebuf, bufrsp);
    }
    
    sprintf(bufrsp, "6 EXEC OK");
    int rc = write_command(writebuf, bufrsp);
    // if(rc) return rc;   
    
    if (rc == -1) return rc;
        
    rc = write_result(node -> handler, writebuf, NULL);
    
    
    return write_command(writebuf, "") | rc;
}

static int command_execof(list* _list, IO_W writebuf, Jaildir jaildir)
{
        
    char* filename = strtok(NULL, " ");
    char cleaned_filename[PATH_MAX];
    int rc = jaildir(filename, cleaned_filename);
    
    if(rc == -1) return 0;

    int64_t id;
    char* db_id_string = strtok(NULL, " ");
    sscanf(db_id_string, "%lu", &id);

    char* sql = strtok(NULL, "");

    node_list* node = find(_list, id);

    if(!node) {
        sprintf(bufrsp, "10 FAILED EXECOF BD DOES NOT EXISTS");
        return write_command(writebuf, bufrsp);
    } 
        
    if(mysql_query(node -> handler, sql)) 
    {
        sprintf(bufrsp, "11 FAILED EXECOF MYSQL");
        return write_command(writebuf, bufrsp);
    }
    
    sprintf(bufrsp, "9 EXECOF OK");
    rc = write_command(writebuf, bufrsp);
    
    //if(rc) return rc;   
    if(rc == -1) return -1;   
    return write_result(node -> handler, writebuf, cleaned_filename);
}

static int command_lastrowid(list* _list, IO_W writebuf)
{
    int64_t id;
    char* db_id_string = strtok(NULL, "");
    sscanf(db_id_string, "%lu", &id);

    node_list* node = find(_list, id);

    if(!node) {
        sprintf(bufrsp, "13 FAILED LASTROWID BD DOES NOT EXISTS");
        return write_command(writebuf, bufrsp);
    }
    
    int ret = mysql_insert_id(node -> handler);
    
    if(!ret) {
        sprintf(bufrsp, "14 FAILED LASTROWID MYSQL");
        return write_command(writebuf, bufrsp);
    }
    
    char ans[RESPONSE_MAX];
    sprintf(ans, "12 LASTROWID OK WITH ROWID %d", ret);
    return write_command(writebuf, ans);
}

static int command_blobin(list* _list, IO_W writebuf, Jaildir jaildir)
{
    int64_t id;
    char* db_id_string = strtok(NULL, " ");
    sscanf(db_id_string, "%lu", &id);

    char* table = strtok(NULL, " ");
    char* name = strtok(NULL, " ");
    char* path = strtok(NULL, "");

    node_list* node = find(_list, id);

    int rc = 0;

    if(!node) 
    {
        sprintf(bufrsp, "16 FAILED BLOBIN BD DOES NOT EXISTS");
        return write_command(writebuf, bufrsp);
    } 
    else
    {
        char cleaned_path[PATH_MAX];
        jaildir(path, cleaned_path);

        FILE *fp = fopen(cleaned_path, "rb");

        if (fp == NULL) {
            sprintf(bufrsp, "17 FAILED BLOBIN FILESYSTEM");
            return write_command(writebuf, bufrsp);
        }

        rc = fseek(fp, 0, SEEK_END);

        if(rc) 
        {
            fclose(fp);
            sprintf(bufrsp, "17 FAILED BLOBIN FILESYSTEM");
            return write_command(writebuf, bufrsp);
        }

        int64_t flen = ftell(fp);
        if(!flen)
        {
            fclose(fp);
            sprintf(bufrsp, "17 FAILED BLOBIN FILESYSTEM");
            return write_command(writebuf, bufrsp);
        } 

        rc = fseek(fp, 0, SEEK_SET);
        if(rc)
        {
            fclose(fp);
            sprintf(bufrsp, "17 FAILED BLOBIN FILESYSTEM");
            return write_command(writebuf, bufrsp);
        }

        char* data = (char*)malloc(flen+1);

        int64_t size = fread(data, 1, flen, fp);

        fclose(fp);

        char* chunk = (char*)malloc(2*size+1);
        mysql_real_escape_string(node -> handler, chunk, data, size);

        char sql[SQL_MAX];
        sprintf(sql, "INSERT INTO %s (Name, Data) VALUES ('%s', %s);", table, name, "'%s'");
        
        size_t st_len = strlen(sql);

        char* query = (char*)malloc(st_len + 2*size+1);
        int64_t len = snprintf(query, st_len + 2*size+1, sql, chunk);

        if (mysql_real_query(node -> handler, query, len))
        {
            sprintf(bufrsp, "18 FAILED BLOBIN MYSQL");
            rc = write_command(writebuf, bufrsp);
        }
        else
        {
            sprintf(bufrsp, "15 BLOBIN OK");
            rc = write_command(writebuf, bufrsp);
        }        

        free(data);
        free(query);
        free(chunk);
    }

    return rc;
}

static int command_blobout(list* _list, IO_W writebuf, Jaildir jaildir)
{
    int64_t id;
    char* db_id_string = strtok(NULL, " ");
    sscanf(db_id_string, "%lu", &id);

    char* table = strtok(NULL, " ");
    char* name = strtok(NULL, " ");
    char* path = strtok(NULL, "");

    node_list* node = find(_list, id);

    int rc = 0;

    if(!node) { 
        sprintf(bufrsp, "20 FAILED BLOBOUT BD DOES NOT EXISTS");
        rc = write_command(writebuf, bufrsp);
    } else
    {
        char cleaned_path[PATH_MAX];
        jaildir(path, cleaned_path);

        FILE *fp = fopen(cleaned_path, "wb");
        
        if (fp == NULL) {
            sprintf(bufrsp, "21 FAILED BLOBOUT FILESYSTEM");
            return write_command(writebuf, bufrsp);
        }

        char sql[SQL_MAX];
        sprintf(sql, "SELECT Data FROM %s WHERE Name = '%s'", table, name);

        if (mysql_query(node -> handler, sql))
        {
            sprintf(bufrsp, "22 FAILED BLOBOUT MYSQL");
            rc = write_command(writebuf, bufrsp);
        }

        MYSQL_RES *result = mysql_store_result(node -> handler);

        if (result == NULL)
        {
            sprintf(bufrsp, "22 FAILED BLOBOUT MYSQL");
            rc = write_command(writebuf, bufrsp);
        }

        MYSQL_ROW row = mysql_fetch_row(result);
        unsigned long *lengths = mysql_fetch_lengths(result);

        if (lengths == NULL) {
            sprintf(bufrsp, "22 FAILED BLOBOUT MYSQL");
            rc = write_command(writebuf, bufrsp);
        }

        fwrite(row[0], lengths[0], 1, fp);

        mysql_free_result(result);

        fclose(fp);

        if(rc) {
            sprintf(bufrsp, "22 FAILED BLOBOUT MYSQL");
            return write_command(writebuf, bufrsp);
        }
        sprintf(bufrsp, "19 BLOBOUT OK");
        rc = write_command(writebuf, bufrsp);
    }
    
    return rc;
}

int xs_mysql(IO_R readbuf, IO_W writebuf, Jaildir jaildir)
{
    static list* _list = NULL;
    if (!_list) 
        _list = new_list();

    static char* buffer = NULL;
    if (!buffer) {
        buffer = (char*)malloc(INIT_BUFFER_SIZE);
        *buffer = '\0';
    }
    
    while(1)
    {

        int rc = read_command(readbuf, &buffer);
        
        if(rc == -1) return -1;

        if(!strcmp(buffer, "EXIT")) break;
        if(!strcmp(buffer, "TERMINATE"))
        {
            free_list(_list);
            free(buffer);
            buffer = NULL;
            _list = NULL;
            break;
        }

        char* ptr = strtok(buffer, " ");
        if(!strcmp(ptr, "OPEN")) rc = command_open(_list, writebuf);
        else if(!strcmp(ptr, "CLOSE")) rc = command_close(_list, writebuf);
        else if(!strcmp(ptr, "EXEC")) rc = command_exec(_list, writebuf);
        else if(!strcmp(ptr, "EXECOF")) rc = command_execof(_list, writebuf, jaildir);
        else if(!strcmp(ptr, "LASTROWID")) rc = command_lastrowid(_list, writebuf);
        else if(!strcmp(ptr, "BLOBIN")) rc = command_blobin(_list, writebuf, jaildir);
        else if(!strcmp(ptr, "BLOBOUT")) rc = command_blobout(_list, writebuf, jaildir);
        else  {
            strcpy(bufrsp, "UNKNOWN COMMAND");
            rc = write_command(writebuf, bufrsp);        
        }

        if(rc == -1) 
        {
            free_list(_list);
            free(buffer);
            buffer = NULL;
            _list = NULL;
            return -1;
        }
    }

    return 0;
}
