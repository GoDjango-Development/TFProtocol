/*
    Coder: Bruno
    Email: rbruno2k15@outlook.com
    
    Reviewer: Luis Miguel
    Email: lmdelbahia@gmail.com
*/

#include <stdlib.h>
#include <stdio.h>
#include <libpq-fe.h>

#include <string.h>

#include "xs_postgresql.h"

/* infraestructure list.h/c here to avoid multiple-definitions */

#include <libpq-fe.h>
#include <inttypes.h>

typedef struct node_list
{
    PGconn* handler;
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

static int64_t push_back(list*, PGconn*);

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

static int64_t push_back(list* _list, PGconn* handler)
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
        PQfinish(current -> handler);
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
    char *port = strtok(NULL, " ");
    char* user = strtok(NULL, " ");
    char* pass = strtok(NULL, " ");
    char* db = strtok(NULL, "");

    
    char conn_string[CONNECTION_MAX];
    sprintf(conn_string, "user=%s password=%s dbname=%s host=%s port=%s",user,
        pass, db, host, port);

    PGconn *handler = PQconnectdb(conn_string);

    if (PQstatus(handler) == CONNECTION_BAD)
    {
        sprintf(bufrsp, "2 FAILED OPEN POSTGRESQL CONNECTION");
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
        sprintf(bufrsp, "4 FAILED CLOSE BD DOES NOT EXISTS");
        return write_command(writebuf, bufrsp);
    }

    erase(_list, node);

    sprintf(bufrsp, "3 CLOSE OK");
    return write_command(writebuf, bufrsp);
}

static int write_result(PGresult* res, IO_W writebuf, char* path)
{
    if (PQresultStatus(res) != PGRES_TUPLES_OK) return 0;

    FILE* pf;
    if(path) pf = fopen(path, "a");

    int fields = PQnfields(res);

    int header_size = 0;
    int i = 0;

    for(i = 0; i < fields; i++)
    {
        header_size += strlen(PQfname(res, i));
        if(i < fields - 1) header_size += 2;
    }

    char* header = (char*)malloc(header_size + 1);
    int header_counter = 0;

    for(i = 0; i < fields; i++)
    {
        strcpy(header + header_counter, PQfname(res, i));
        header_counter += strlen(PQfname(res, i));
        if(i < fields - 1)
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

    int rows = PQntuples(res);

    int j;
    for(j = 0; j < rows; ++j)
    {
        int buffer_size = 0;

        for(i = 0; i < fields; ++i)
        {
            buffer_size += strlen(PQgetvalue(res, j, i) ? PQgetvalue(res, j, i) : "NULL");
            if(i < fields - 1) buffer_size += 2;
        }
        
        char* buffer = (char*)malloc(buffer_size + 1);
        int buffer_counter = 0;
        for(i = 0; i < fields; ++i)
        {
            strcpy(buffer + buffer_counter, PQgetvalue(res, j, i) ? PQgetvalue(res, j, i) : "NULL");
            buffer_counter += strlen(PQgetvalue(res, j, i) ? PQgetvalue(res, j, i) : "NULL");
            if(i < fields - 1)
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

    PQclear(res);
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
        sprintf(bufrsp, "6 FAILED EXEC BD DOES NOT EXISTS");
        return write_command(writebuf, bufrsp);
    } 
    
    PGresult *res = PQexec(node -> handler, sql);

    if(PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        sprintf(bufrsp, "7 FAILED EXEC POSTGRESQL");
        return write_command(writebuf, bufrsp);
    }

    sprintf(bufrsp, "5 EXEC OK");
    int rc = write_command(writebuf, bufrsp);
    ///if(rc) return rc;   
    if(rc == -1) return -1;   
    rc = write_result(res, writebuf, NULL);
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
        sprintf(bufrsp, "9 FAILED EXECOF BD DOES NOT EXISTS");
        return write_command(writebuf, bufrsp);
    } 
        
    PGresult *res = PQexec(node -> handler, sql);

    if(PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        sprintf(bufrsp, "10 FAILED EXECOF POSTGRESQL");
        return write_command(writebuf, bufrsp);
    }
    
    sprintf(bufrsp, "8 EXECOF OK");
    rc = write_command(writebuf, bufrsp);
    //if(rc) return rc;   
    if(rc == -1) return -1;   
    
    return write_result(res, writebuf, cleaned_filename);
}


int xs_postgresql(IO_R readbuf, IO_W writebuf, Jaildir jaildir)
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
