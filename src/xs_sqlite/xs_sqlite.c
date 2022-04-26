/*
    Coder: Bruno
    Email: rbruno2k15@outlook.com
    
    Reviewer: Luis Miguel
    Email: lmdelbahia@gmail.com
*/

#include <stdlib.h>
#include <stdio.h>
#include <sqlite3.h>

#include <string.h>

#include "xs_sqlite.h"

/* infraestructure list.h/c here to avoid multiple-definitions */

/*
    Coder: Bruno
    Email: rbruno2k15@outlook.com
*/

#include <sqlite3.h>
#include <inttypes.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct node_list
{
    sqlite3* handler;
    char* data;
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

static int64_t push_back(list*, char*, sqlite3*);

static void erase(list*, node_list*);

static void free_list(list* _list);


static list* new_list()  
{
    list* _list = (list*)malloc(sizeof(list));

    _list -> head = NULL;
    _list -> tail = NULL;
    _list -> size = 0;

    return _list;
}

static int64_t push_back(list* _list, char* element, sqlite3* handler)
{
    node_list* new_node = (node_list*)malloc(sizeof(node_list));

    new_node -> data = element;
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
        sqlite3_close(current -> handler);
        free(current);
    }
}

/* end list.h/c */

typedef struct cb_args
{   
    IO_W writebuf;
    int row;
    char* path;
    int file;
} cb_args;


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

static int command_open(list* _list, IO_W writebuf, Jaildir jaildir)
{
    char* path = strtok(NULL, "");
    int rc = 0;

    char cleaned_path[PATH_MAX];
    if(!strcmp(path, ":memory:")) 
    {
        strcpy(cleaned_path, ":memory:");
        cleaned_path[strlen(":memory:")] = '\0';
    }
    else rc = jaildir(path, cleaned_path);
    
    if(rc == -1) {
        sprintf(bufrsp, "3 FAILED OPEN JAILDIR");
        return write_command(writebuf, bufrsp);
    }
    sqlite3* handler;
    rc = sqlite3_open(cleaned_path, &handler);
        
    if(rc) {
        sprintf(bufrsp, "2 FAILED OPEN SQLITE");
        rc = write_command(writebuf, bufrsp);
    } else
    {
        int64_t id = push_back(_list, cleaned_path, handler);
        
        sprintf(bufrsp, "1 BD OPENED OK WITH ID %lu", id);
        rc = write_command(writebuf, bufrsp);
    } 

    return rc;
}

static int callback(void* args, int argc, char **argv, char **azColName) 
{
    IO_W writebuf = ((cb_args*)args) -> writebuf;
    int row = ((cb_args*)args) -> row;
    char* path = ((cb_args*)args) -> path;
    int file = ((cb_args*)args) -> file;
    int rc = 0;

    FILE* pf;
    if(file) pf = fopen(path, "a");

    if(!row)
    {
        if(file)
        {
            sprintf(bufrsp, "7 EXECOF OK");
            rc = write_command(writebuf, bufrsp);
            if(rc == -1)
            {
                fclose(pf);
                return -1;
            }
        }
        else
        {
            sprintf(bufrsp, "4 EXEC OK");
            rc = write_command(writebuf, bufrsp);
            if(rc == -1) return -1;            
        }

        int header_size = 0;
        int i;
        for(i=0;i<argc;++i)
        {
            header_size += strlen(azColName[i]);
            if(i < argc - 1) header_size += 2;
        }
        
        char* header = (char*)malloc(header_size + 1);
        int header_counter = 0;
        for(i=0;i<argc;++i)
        {
            strcpy(header + header_counter, azColName[i]);
            header_counter += strlen(azColName[i]);
            if(i < argc - 1)
            {
                header[header_counter] = '@';
                header[header_counter + 1] = '@';
                header_counter += 2;
            }
        }

        if(file) fprintf(pf, "%s\n", header);
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
    }

    int buffer_size = 0;
    int i;
    for(i=0;i<argc;++i)
    {
        buffer_size += strlen(argv[i] ? argv[i] : "NULL");
        if(i < argc - 1) buffer_size += 2;
    }
    
    char* buffer = (char*)malloc(buffer_size + 1);
    int buffer_counter = 0;
    for(i=0;i<argc;++i)
    {
        strcpy(buffer + buffer_counter, argv[i] ? argv[i] : "NULL");
        buffer_counter += strlen(argv[i] ? argv[i] : "NULL");
        if(i < argc - 1)
        {
            buffer[buffer_counter] = '@';
            buffer[buffer_counter + 1] = '@';
            buffer_counter += 2;
        }
    }

    if(file)
    {
        fprintf(pf, "%s\n", buffer);
        fclose(pf);
    }
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
    ((cb_args*)args) -> row++;

    return 0;
}

static int command_exec(list* _list, IO_W writebuf)
{
    int64_t id;
    char* db_id_string = strtok(NULL, " ");
    sscanf(db_id_string, "%lu", &id);

    char* sql = strtok(NULL, "");

    node_list* node = find(_list, id);

    int rc = 0;

    if(!node) {
        sprintf(bufrsp, "5 FAILED EXEC BD DOES NOT EXIST");
        rc = write_command(writebuf, bufrsp);
    } else
    {
        cb_args args;
        args.row = 0;
        args.writebuf = writebuf;
        args.file = 0;
        args.path = NULL;
        int rc = sqlite3_exec(node -> handler, sql, callback, &args, NULL);
        if(rc) 
        {
            sprintf(bufrsp, "6 FAILED EXEC SQLITE");
            return write_command(writebuf, bufrsp);
        }
        else if(!args.row) 
        {
            sprintf(bufrsp, "4 EXEC OK");
            rc = write_command(writebuf, bufrsp);
            rc = write_command(writebuf, "");
        }
        else rc = write_command(writebuf, "");
    }
    
    return rc;    
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

    if(node == NULL) { 
        sprintf(bufrsp,  "8 FAILED EXECOF BD DOES NOT EXIST");
        rc = write_command(writebuf, bufrsp);
    } else
    {
        cb_args args;
        args.row = 0;
        args.writebuf = writebuf;
        args.path = cleaned_filename;
        args.file = 1;

        int rc = sqlite3_exec(node -> handler, sql, callback, &args, NULL);
        if(rc) { 
            sprintf(bufrsp, "9 FAILED EXECOF SQLITE");
            return write_command(writebuf, bufrsp);
        } else if(!args.row) {
            sprintf(bufrsp, "7 EXECOF OK");
            rc = write_command(writebuf, bufrsp);
        }
    }

    return rc;
}

static int command_close(list* _list, IO_W writebuf)
{
    int64_t id;
    char* db_id_string = strtok(NULL, "");
    sscanf(db_id_string, "%lu", &id);

    node_list* node = find(_list, id);

    int rc = 0;

    if(!node) {
        sprintf(bufrsp, "12 FAILED CLOSE BD DOES NOT EXIST");
        rc = write_command(writebuf, bufrsp);
    } else
    {
        int rc = sqlite3_close(node -> handler);
        if(rc) {
            sprintf(bufrsp, "11 FAILED CLOSE SQLITE");
            return rc = write_command(writebuf, bufrsp);
        }
        erase(_list, node);
        sprintf(bufrsp, "10 CLOSE OK");
        rc = write_command(writebuf, bufrsp);
    }

    return rc;
}

static int command_lastrowid(list* _list, IO_W writebuf)
{
    int64_t id;
    char* db_id_string = strtok(NULL, "");
    sscanf(db_id_string, "%lu", &id);

    node_list* node = find(_list, id);
    
    int rc = 0;

    if(!node) {
        sprintf(bufrsp, "15 FAILED LASTROWID BD DOES NOT EXIST");
        rc = write_command(writebuf, bufrsp);
    } else
    {
        sqlite3_int64 ret = sqlite3_last_insert_rowid(node -> handler);
        if(!ret) {
            sprintf(bufrsp, "14 FAILED LASTROWID SQLITE");
            rc = write_command(writebuf, bufrsp);
        } else
        {
            char ans[RESPONSE_MAX];
            sprintf(ans, "13 LASTROWID OK WITH ROWID %lld", ret);
            rc = write_command(writebuf, ans);
        }
    }

    return rc;
}

static int command_softheap(IO_W writebuf)
{
    int64_t limit;
    char* limit_string = strtok(NULL, "");
    sscanf(limit_string, "%lu", &limit);

    sqlite3_int64 ret = sqlite3_soft_heap_limit64(limit);
    int rc = 0;

    if(ret < 0) { 
        sprintf(bufrsp, "17 FAILED SOFTHEAP SQLITE");
        rc = write_command(writebuf, bufrsp);
    } else {
        sprintf(bufrsp, "16 SOFTHEAP OK");
        rc = write_command(writebuf, bufrsp);
    }

    return rc;
}

static int command_hardheap(IO_W writebuf)
{
    int64_t limit;
    char* limit_string = strtok(NULL, "");
    sscanf(limit_string, "%lu", &limit);

    sqlite3_int64 ret = 0; /*sqlite3_hard_heap_limit64(limit);*/

    int rc = 0;

    if(ret < 0) { 
        sprintf(bufrsp, "19 FAILED HARDHEAP SQLITE");
        rc = write_command(writebuf, bufrsp);
    } else {
        sprintf(bufrsp, "18 HARDHEAP OK");
        rc = write_command(writebuf, bufrsp);
    }
    return rc;
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

    if(!node) {
        sprintf(bufrsp, "21 FAILED BLOBIN BD DOES NOT EXIST");
        rc = write_command(writebuf, bufrsp);
    } else
    {
        char cleaned_path[PATH_MAX];
        jaildir(path, cleaned_path);

        FILE *fp = fopen(cleaned_path, "rb");
        
        if (fp == NULL) {
            sprintf(bufrsp, "22 FAILED BLOBIN FILESYSTEM");
            return write_command(writebuf, bufrsp);
        }
        rc = fseek(fp, 0, SEEK_END);
        if(rc) 
        {
            fclose(fp);
            sprintf(bufrsp, "22 FAILED BLOBIN FILESYSTEM");
            return write_command(writebuf, bufrsp);
        }

        int64_t flen = ftell(fp);
        if(!flen)
        {
            fclose(fp);
            sprintf(bufrsp, "22 FAILED BLOBIN FILESYSTEM");
            return write_command(writebuf, bufrsp);
        } 

        rc = fseek(fp, 0, SEEK_SET);
        if(rc)
        {
            fclose(fp);
            sprintf(bufrsp, "22 FAILED BLOBIN FILESYSTEM");
            return write_command(writebuf, bufrsp);
        }

        char* data = (char*)malloc(flen+1);
        
        int64_t size = fread(data, 1, flen, fp);

        fclose(fp);

        char sql[SQL_MAX];
        sprintf(sql, "INSERT INTO %s (Name, Data) VALUES(?1,?2)", table);

        sqlite3_stmt *pStmt;

        rc = sqlite3_prepare(node -> handler, sql, -1, &pStmt, 0);
        if(rc) 
        {
            free(data);
            sprintf(bufrsp, "23 FAILED BLOBIN SQLITE");
            return write_command(writebuf, bufrsp);
        }

        rc = sqlite3_bind_text(pStmt, 1, name, strlen(name), SQLITE_STATIC);
        if(rc) 
        {
            free(data);
            sprintf(bufrsp, "23 FAILED BLOBIN SQLITE");
            return write_command(writebuf, bufrsp);
        }
        
        rc = sqlite3_bind_blob(pStmt, 2, data, size, SQLITE_STATIC);
        if(rc) 
        {
            free(data);
            sprintf(bufrsp, "23 FAILED BLOBIN SQLITE");
            return write_command(writebuf, bufrsp);
        }

        rc = sqlite3_step(pStmt);

        rc = sqlite3_finalize(pStmt);
        if(rc)
        {
            free(data);
            sprintf(bufrsp, "23 FAILED BLOBIN SQLITE");
            return write_command(writebuf, bufrsp);
        } 

        free(data);
        sprintf(bufrsp, "20 BLOBIN OK");
        rc = write_command(writebuf, bufrsp);
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
        sprintf(bufrsp, "25 FAILED BLOBOUT BD DOES NOT EXIST");
        rc = write_command(writebuf, bufrsp);
    } else
    {
        char cleaned_path[PATH_MAX];
        jaildir(path, cleaned_path);

        FILE *fp = fopen(cleaned_path, "wb");
        
        if (fp == NULL) {
            sprintf(bufrsp, "26 FAILED BLOBOUT FILESYSTEM");
            return write_command(writebuf, bufrsp);
        }
        char sql[SQL_MAX];
        sprintf(sql, "SELECT Data FROM %s WHERE Name = ?1", table);

        sqlite3_stmt *pStmt;
        
        rc = sqlite3_prepare(node -> handler, sql, -1, &pStmt, 0);
        if(rc) 
        {
            fclose(fp);
            sprintf(bufrsp, "27 FAILED BLOBOUT SQLITE");
            return write_command(writebuf, bufrsp);
        }

        rc = sqlite3_bind_text(pStmt, 1, name, strlen(name), SQLITE_STATIC);
        if(rc) 
        {
            fclose(fp);
            sprintf(bufrsp, "27 FAILED BLOBOUT SQLITE");
            return write_command(writebuf, bufrsp);
        }

        rc = sqlite3_step(pStmt);
    
        int bytes = 0;
        
        if (rc == SQLITE_ROW) {

            bytes = sqlite3_column_bytes(pStmt, 0);
        }
            
        fwrite(sqlite3_column_blob(pStmt, 0), bytes, 1, fp);

        fclose(fp);

        rc = sqlite3_finalize(pStmt);  
        if(rc) {
            sprintf(bufrsp, "27 FAILED BLOBOUT SQLITE");
            return write_command(writebuf, bufrsp);
        }
        sprintf(bufrsp, "24 BLOBOUT OK");
        rc = write_command(writebuf, bufrsp);
    }
    
    return rc;
}

int xs_sqlite(IO_R readbuf, IO_W writebuf, Jaildir jaildir)
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
        if(!strcmp(ptr, "OPEN")) rc = command_open(_list, writebuf, jaildir);
        else if(!strcmp(ptr, "EXEC")) rc = command_exec(_list, writebuf);
        else if(!strcmp(ptr, "CLOSE")) rc = command_close(_list, writebuf);
        else if(!strcmp(ptr, "LASTROWID")) rc = command_lastrowid(_list, writebuf);
        else if(!strcmp(ptr, "EXECOF")) rc = command_execof(_list, writebuf, jaildir);
        else if(!strcmp(ptr, "SOFTHEAP")) rc = command_softheap(writebuf);
        else if(!strcmp(ptr, "HARDHEAP")) rc = command_hardheap(writebuf);
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
