/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <util.h>
#include <init.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <cmd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <ctype.h>
#include <dirent.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define SECDIR_TOK ".sd/"
#define RDNULLBUF 64 * 1024

/* Temp file internal structure */
struct tmp {
    FILE *file;
    char path[PATH_MAX];
};

/* File Api internal structure */
struct fapi {
    int fd;
    void *buf;
};

/* Parameters for the thread resolver. */
struct rsvparams {
    const char *host;
    char ip[IPV6LEN];
    int v6;
    pthread_cond_t *cond;
    pthread_mutex_t *mut;
    volatile int done;
};

extern char *strptime(char *s, const char *fmt, struct tm *tp);
static void *thresolv(void *pp);

static const char *efapi;

int jaildir(const char *src, char *dst)
{
    char *dstcpy = dst;
    strcpy(dst, tfproto.dbdir);
    dst += strlen(dst);
    const char *pdir = "/../";
    int c = 0;
    while (*src && c < PATH_MAX - 1) {
        *dst++ = *src++;
        c++;
    }
    *dst = *src;
    if (strstr(dstcpy, pdir))
        return -1;
    return chkpath(tfproto.dbdir);
}

Tmp mktmp(const char *cat)
{
    Tmp tmp = malloc(sizeof(struct tmp));
    if (!tmp)
        return NULL;
    char num[12];
    sprintf(num, "%d", getpid());
    strcpy(tmp->path, tfproto.dbdir);
    strcat(tmp->path, num);
    strcat(tmp->path, cat);
    tmp->file = fopen(tmp->path, "w");
    if (!tmp->file)
        return NULL;
    else
        return tmp;
}

void freetmp(Tmp tmp)
{
    if (tmp) {
        fclose(tmp->file);
        unlink(tmp->path);
        free(tmp);
    }
}

void writetmp(Tmp tmp, const char *str)
{   
    if (tmp)
        fprintf(tmp->file, "%s", str);
    fflush(tmp->file);
}

const char *tmppath(Tmp tmp)
{
    if (tmp)
        return tmp->path;
    else
        return NULL;
}

int chkpath(const char *path)
{
    return access(path, F_OK);
}

Fapi fapinit(const char *path, void *buf, enum fapiopc code)
{
    Fapi api = malloc(sizeof(struct fapi));
    if (!api) {
        efapi = CMD_EFAPINTE;
        return NULL;
    }
    api->buf = buf;
    struct stat st = { 0 };
    if (code == FAPI_READ) {
        if (access(path, F_OK)) {
            free(api);
            efapi = CMD_EFILENOENT;
            return NULL;
        }
        stat(path, &st);
        if (S_ISDIR(st.st_mode)) {
            free(api);
            efapi = CMD_EISDIR;
            return NULL;
        }
        api->fd = open(path, O_RDONLY, S_IREAD | S_IWRITE);
        if (api->fd == -1) {
            if (errno == EACCES)
                efapi = CMD_EACCESS;
            else
                efapi = CMD_EFAPINTE;
            free(api);
            return NULL;
        }
    } else if (code == FAPI_WRITE) {
        if (!access(path, F_OK)) {
            free(api);
            efapi = CMD_EFILEXIST;
            return NULL;
        }
        api->fd = open(path, O_WRONLY | O_CREAT, S_IREAD | S_IWRITE);
        if (api->fd == -1) {
            if (errno == EACCES)
                efapi = CMD_EACCESS;
            else
                efapi = CMD_EFAPINTE;
            free(api);
            return NULL;
        }
    } else if (code == FAPI_OVERWRITE) {
        stat(path, &st);
        if (S_ISDIR(st.st_mode)) {
            free(api);
            efapi = CMD_EISDIR;
            return NULL;
        }
        api->fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
        if (api->fd == -1) {
            if (errno == EACCES)
                efapi = CMD_EACCESS;
            else
                efapi = CMD_EFAPINTE;
            free(api);
            return NULL;
        }
    }
    return api;
}

void fapifree(Fapi api)
{
    if (api) {
        close(api->fd);
        free(api);
    }
}

int fapiread(Fapi api, int offset, int len)
{
    int rb;
    do 
        rb = read(api->fd, api->buf + offset, len);
    while (rb == -1 && errno == EINTR);
    return rb;
}

int fapiwrite(Fapi api, int offset, int len)
{
    int wb;
    do 
        wb = write(api->fd, api->buf + offset, len);
    while (wb == -1 && errno == EINTR);
    return wb;
}

const char *fapierr(void)
{
    return efapi;
}

unsigned long long freespace(const char *path)
{
    struct statvfs stat = { 0 };
    if (statvfs(path, &stat) != 0)
        return -1;
    return stat.f_bsize * stat.f_bavail;
}

void listdir(const char *path, struct mempool *dl, int recur)
{
    struct dirent *den;
    DIR *dir = opendir(path);
    if (dir == NULL)
        return;
    char *newp = malloc(PATH_MAX);
    static char file[PATH_MAX];
    while (den = readdir(dir)) {
        if (den->d_type == DT_DIR && den->d_type != DT_LNK && 
            strcmp(den->d_name, ".") && strcmp(den->d_name, "..")) {
            strcpy(newp, path);
            strcat(newp, "/");
            strcat(newp, den->d_name);
            if (!mpool_addblk(dl)) {
                free(newp);
                closedir(dir);
                return;
            }
            strcpy(dl->data + (dl->count - 1) * dl->blksz, newp);
            if (recur)
                listdir(newp, dl, recur);
        } else if (den->d_type != DT_DIR) {
            strcpy(file, path);
            strcat(file, "/");
            strcat(file, den->d_name);
            if (!mpool_addblk(dl)) {
                free(newp);
                closedir(dir);
                return;
            }
            strcpy(dl->data + (dl->count - 1) * dl->blksz, file);
        }
    }
    free(newp);
    closedir(dir);
}

int cpr(const char *srcp, const char *dstp)
{
    char path[PATH_MAX] = "";
    struct stat st = { 0 };
    struct mempool list;
    mpool_create(&list, PATH_MAX);
    listdir(srcp, &list, 1);
    struct {
        char path[PATH_MAX];
    } *prec = list.data;
    int  i = 0;
    if (mkdir(dstp, S_IRWXU)) {
        mpool_destroy(&list);
        return -1;
    }
    while (i < list.count) {
        stat(prec->path, &st);
        if (S_ISDIR(st.st_mode)) {
            char *ppt = prec->path + strlen(srcp);
            strcpy(path, dstp);
            strcat(path, ppt);
            if (mkdir(path, S_IRWXU)) {
                mpool_destroy(&list);
                return -1;
            }
        } else if (!S_ISDIR(st.st_mode)) {
            char *ppt = prec->path + strlen(srcp);
            strcpy(path, dstp);
            strcat(path, ppt);
            if (link(prec->path, path) == -1) {
                mpool_destroy(&list);
                return -1;
            }
        }
        prec++, i++;
    }
    mpool_destroy(&list); 
    return 0;
}

int rmr(const char *path, const char *excl)
{
    struct mempool list;
    mpool_create(&list, PATH_MAX);
    if (!mpool_addblk(&list)) {
        mpool_destroy(&list);
        return -1;
    }
    strcpy(list.data, path);
    listdir(path, &list, 1);
    struct {
        char path[PATH_MAX];
    } *prec = list.data + (list.count - 1) * list.blksz;
    int i = list.count - 1;
    int rmf = 0;
    while (i >= 0) {
        strcat(prec->path, "/");
        if (excl) {
            if (!strstr(prec->path, excl)) {
                rmdir(prec->path);
                *(prec->path + strlen(prec->path) - 1) = '\0';
                unlink(prec->path);
            } 
        } else {
            rmdir(prec->path);
            *(prec->path + strlen(prec->path) - 1) = '\0';
            unlink(prec->path);
        }
        if (!access(prec->path, F_OK))
            rmf = -1;
        prec--, i--;
    }
    mpool_destroy(&list);
    return rmf;
}

int trylck(const char *path, const char *file, int rmtrail)
{
    char filenam[PATH_MAX] = "";
    strcpy(filenam, path);
    if (rmtrail) {
        char *pt = filenam + strlen(filenam) - 1;
        if (*pt == '/')
            *pt = '\0';
        while (pt >= filenam) {
            if (*pt == '/') {
                *pt = '\0';
                break;
            }
            pt--;
        }
    }
    strcat(filenam, "/");
    strcat(filenam, file);
    if (!strcmp(file, ""))
        return -1;
    return access(filenam, F_OK);
}

void normpath(const char *path, char *rst)
{
    int slash = 0;
    int i = 0;
    while (*path && i < PATH_MAX) {
        if (*path == '/' && slash) {
            path++;
            continue;
        } else if (*path == '/')
            slash = 1;
        else 
            slash = 0;
        *rst++ = *path++, i++;
    }
    if (i < PATH_MAX - 1)
        *rst = '\0';
    else 
        *(rst + PATH_MAX - 1) = '\0';
}

void gettm(time_t *t, char *time)
{
    sprintf(time, "%lld", (long long int) *t);
}

int gettmf(time_t *t, char *time)
{
    struct tm *tm = gmtime(t);
    if (tm) {
        strftime(time, HTIMELEN, "%Y-%m-%d %H:%M:%S", tm);
        return 0;
    }
    else
        return -1;
}

void cmdtoupper(char *cmd)
{
    while (*cmd)
        *cmd = toupper(*cmd), cmd++;    
}

void strtotm(char *date)
{
    struct tm tm = { 0 };
    strptime(date, "%Y-%m-%d %H:%M:%S", &tm);
    time_t t = mktime(&tm) - timezone;
    sprintf(date, "%lld", (long long int) t);
}

void strcpy_sec(char *to, const char *from, int sz)
{
    int c = 0;
    while (c < sz && *from != '\0') {
        *to++ = *from++;
        c++;
    }
    if (c == sz)
        *(to - 1) = '\0';
    else
        *to = '\0';
}

char *itostr(long long v)
{
    static char num[LLDIGITS];
    sprintf(num, "%lld", v);
    return num;
}

int isbigendian(void)
{
    int value = 1;
    char *pt = (char *) &value;
    if (*pt == 1)
        return 0;
    else
        return 1;
}

int crtlock(/*const char *path*/ int fd, enum crtlock lck/*, int creat*/)
{
    /*int fd;
    if (creat)
        fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    else
        fd = open(path, O_RDWR);*/
    struct flock lock;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    int lprm;
    if (lck == BLK) {
        lock.l_type = F_WRLCK;    
        lprm = F_SETLKW;
    } else if (lck == NONBLK) {
        lock.l_type = F_WRLCK;
        lprm = F_SETLK;
    } else if (lck == UNLOCK) {
        lock.l_type = F_UNLCK;
        lprm = F_SETLK;
    }
    int lr = fcntl(fd, lprm, &lock);
    return lr;
}

int64_t writechunk(int fd, char *buf, int64_t len)
{
    int64_t wb;
    int64_t written = 0;
    do {
        do 
            wb = write(fd, buf + written, len - written);
        while (wb == -1 && errno == EINTR);
        written += wb;
    } while (wb != -1 && len - written > 0);
    return wb;
}

int64_t readchunk(int fd, char *buf, int64_t len)
{
    int64_t rb;
    do 
        rb = read(fd, buf, len);
    while (rb == -1 && errno == EINTR);
    return rb;
}

int issecdir(const char *path)
{
    if (strstr(path, SECDIR_TOK))
        return 1;
    else 
        return 0;
}

void bytetohex(const unsigned char *bytes, int64_t sz, char *hex)
{
    int64_t i = 0;
    for (; i < sz; i++)
        sprintf(hex + i * HEXDIG_LEN, "%02x", *(bytes + i));
    *(hex + sz * HEXDIG_LEN) = '\0';
}

void randent(char *ran, int len)
{
    srand((int) time(0));
    int c = 0;
    int ilen = len / sizeof(int);
    for (; c < ilen; c++) {
        int num = rand();
        *((int *) ran + c) = num;
    }
}

void rdnull(int fd, int64_t sz)
{
    void *buf = malloc(RDNULLBUF);
    int64_t secbuf;
    int bufsz;
    if (buf)
        bufsz = RDNULLBUF;
    else {
        bufsz = sizeof secbuf;
        buf = &secbuf;
    }
    while (sz > 0)
        sz -= read(fd, buf, bufsz);
    if (buf != &secbuf)
        free(buf);
}

int sha256sum(const char *path, char *sha256_str)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1)
        return -1;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    struct stat fi;
    stat(path, &fi);
    int bufsz = fi.st_blksize * BLKSZFAC;
    unsigned char *buf = malloc(bufsz);
    if (!buf) {
        close(fd);
        return -1;
    }
    int rd;
    while ((rd = readchunk(fd, buf, bufsz))) {
        if (rd == -1) {
            close(fd);
            free(buf);
            return -1;
        }
        SHA256_Update(&sha256, buf, rd);
    }
    close(fd);
    free(buf);
    SHA256_Final(hash, &sha256);
    bytetohex(hash, SHA256_DIGEST_LENGTH, sha256_str);
    return 0;
}

int resolvhn(const char *host, char *ip, int v6, int timeout)
{
    pthread_cond_t cond;
    pthread_mutex_t mut;
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&mut, NULL);
    struct rsvparams params;
    params.host = host;
    params.v6 = v6;
    params.mut = &mut;
    params.cond = &cond;
    params.done = 0;
    pthread_mutex_lock(&mut);
    pthread_t th;
    pthread_create(&th, NULL, thresolv, &params);
    if (timeout > 0) {
        struct timespec ts;
        struct timeval now;
        gettimeofday(&now, NULL);
        ts.tv_sec = now.tv_sec + timeout;
        ts.tv_nsec = now.tv_usec * 1000;
        pthread_cond_timedwait(&cond, &mut, &ts);
    } else
        pthread_cond_wait(&cond, &mut);
    pthread_cancel(th);
    pthread_mutex_destroy(&mut);
    pthread_cond_destroy(&cond);
    if (params.done) {
        strcpy(ip, params.ip);
        return 0;
    }
    return -1;
}

static void *thresolv(void *pp)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    struct rsvparams *rsv = pp;
    struct addrinfo hints;
    struct addrinfo *rs;
    memset(&hints, 0, sizeof hints);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    if (rsv->v6) {
        hints.ai_family = AF_INET6;
        if (getaddrinfo(rsv->host, NULL, &hints, &rs) != 0) {
            pthread_mutex_lock(rsv->mut);
            pthread_cond_signal(rsv->cond);
            pthread_mutex_unlock(rsv->mut);
            return NULL;
        }
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *) rs->ai_addr)->sin6_addr, 
            rsv->ip, sizeof (struct sockaddr_in6));
    } else {
        hints.ai_family = AF_INET;
        if (getaddrinfo(rsv->host, NULL, &hints, &rs) != 0) {
            pthread_mutex_lock(rsv->mut);
            pthread_cond_signal(rsv->cond);
            pthread_mutex_unlock(rsv->mut);
            return NULL;
        }
        inet_ntop(AF_INET, &((struct sockaddr_in *) rs->ai_addr)->sin_addr, 
            rsv->ip, sizeof (struct sockaddr_in));
    }
    freeaddrinfo(rs);
    rsv->done = 1;
    pthread_mutex_lock(rsv->mut);
    pthread_cond_signal(rsv->cond);
    pthread_mutex_unlock(rsv->mut);
    return NULL;
}

void rmtrdir(char *str)
{
    int len = strlen(str);
    if (len <= 1)
        return;
    char *pt = str + len - 1;
    while (*--pt != '/');
    *pt = '\0';
}

int lsr_iter(const char *path, int rec, lsr_iter_callback callback)
{
    const int diralloc = 32;
    char pathcp[PATH_MAX];
    char filename[PATH_MAX];
    strcpy(pathcp, path);
    struct dirent *den;
    int lvl = 0;
    DIR **dir = malloc(diralloc * sizeof(DIR *));
    if (!dir)
        return -1;
    int c = 0;
    int dirmax = diralloc;
    for (; c < dirmax; c++)
        *(dir + c) = NULL;
    *(dir + lvl) = opendir(pathcp);
    if (!*(dir + lvl)) {
        free(dir);
        return -1;
    }
    do {
        while (den = readdir(*(dir + lvl))) {
            if (den->d_type == DT_DIR && den->d_type != DT_LNK && 
                strcmp(den->d_name, ".") && strcmp(den->d_name, "..")) {
                if (rec) {
                    if (strcmp(pathcp, "/"))
                        strcat(pathcp, "/");
                    strcat(pathcp, den->d_name);
                    if (callback)
                        callback(pathcp, 1);
                    lvl++;
                    if (lvl == dirmax) {
                        int ndirmax = dirmax + diralloc;
                        void *tmp = realloc(dir, ndirmax * sizeof(DIR *));
                        if (!tmp) {
                            for (c = 0; c < dirmax; c++)
                                if (*(dir + c))
                                    closedir(*(dir + c));
                            free(dir);
                            return -1;
                        }
                        dir = tmp;
                        dirmax = ndirmax;
                    }
                    *(dir + lvl) = opendir(pathcp);
                    if (!*(dir + lvl)) {
                        rmtrdir(pathcp);
                        lvl--;
                        continue;
                    }
                } else {
                    strcpy(filename, pathcp);
                    if (strcmp(filename, "/"))
                        strcat(filename, "/");
                    strcat(filename, den->d_name);
                    if (callback)
                        callback(filename, 1);
                }
            } else if (den->d_type != DT_DIR) {
                strcpy(filename, pathcp);
                if (strcmp(filename, "/"))
                    strcat(filename, "/");
                strcat(filename, den->d_name);
                if (callback)
                    callback(filename, 0);
            }
        }
        closedir(*(dir + lvl));
        *(dir + lvl) = NULL;
        lvl--;
        if (rec && lvl > -1)
            rmtrdir(pathcp);
    } while (lvl > -1);
    free(dir);
    return 0;
}
