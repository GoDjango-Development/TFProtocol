#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include <errno.h>

/* FSTATLS header. */
#pragma pack(push, 1)
struct fstathdr {
    char code;
    char type;
    uint64_t size;
    uint64_t atime;
    uint64_t mtime;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct tfhdr {
    int32_t sz;
} rxhdr, txhdr;
#pragma pack(pop)

#define swapbo32(value) value = (value >> 24 & 0xff) | (value >> 16 & 0xff) << \
    8 | (value >> 8 & 0xff) << 16 | (value & 0xff) << 24;
#define swapbo64(value) value = (value >> 56 & 0xFF) | (value >> 48 & 0xFF) << \
    8 | (value >> 40 & 0xFF) << 16 | (value >> 32 & 0xFF) << 24 | (value >> 24 \
    & 0xFF) << 32 | (value >> 16 & 0xFF) << 40 | (value >> 8 & 0xFF) << 48 | \
    (value & 0xFF) << 56
    
#define bufsize 512 * 1024
int sock;

void lscmd(void);
void excmd(char *src, char *dst);
void sndfile(void);
void xs_ime_start(void);
static int32_t hdrgetsz(struct tfhdr *hdr);
int isbigendian(void);
static void hdrsetsz(int32_t sz, struct tfhdr *hdr);

char *deckey;
int deckey_len;
int64_t seed;
int64_t jump;

char *deckey_tx;
int64_t seed_tx;
int64_t jump_tx;

void decrypt(char *data, int len, int op);

int wrs(char *buf, int len);
int rds(char *buf, int len);
int rds_ex(char *buf, int len);
int wrs_ex(char *buf, int len);

int wrs_real(char *buf, int len);
int rds_real(char *buf, int len);
void *outofband(void *prms);
void *threadudp(void *);
void sig(int signo);
   char buf[bufsize]="";
void cmd_put(void);
void cmd_get(void);
static int writechunk(int fd, char *buf, int64_t len);
void cmd_putcan(void) ;
void cmd_getcan(void) ;

void xs_sqlite_module(void);
void sqlite_exec(void);
 struct sockaddr_in addr;
static int encrypt = 0;

void xs_ace(void);

void nigma(void);

void xs1_readv2(void);
void xs1_writev2(void);

void xs_ace_run(void);

void tlb(void);
void xs_gateway(void);
void *thgateway(void *prms);
void cmd_sdown(void);
void cmd_sup(void);
void fsize(void);
void fsizels(void);
void cmd_ftype(void);
void cmd_ftypels(void);
void cmd_fstatls(void);
void cmd_intread(void);
void cmd_intwrite(void);

int main(void)
{
        int retry_con = 2;
RETRY_CONNECT:;
    unsigned short port = 10345;
//unsigned short port = 11000;    
   //unsigned short port = 10346;    
	//unsigned short port = 10347;    

/* tfprotocol public testing server port */
  // unsigned short port = 10999;    
  
     sock = socket(AF_INET, SOCK_STREAM, 0);
    
   
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
 
  //  inet_pton(AF_INET, "10.0.0.2", &addr.sin_addr);
  //  inet_pton(AF_INET, "192.168.0.101", &addr.sin_addr);

inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
  //inet_pton(AF_INET, "45.79.37.243", &addr.sin_addr);
//    inet_pton(AF_INET, "209.222.17.119", &addr.sin_addr);
//     inet_pton(AF_INET, "10.0.0.1", &addr.sin_addr);
   int flags = fcntl(sock, F_GETFL, 0);
   flags |= O_NONBLOCK;
   fcntl(sock, F_SETFL, flags);
   fd_set set;
   FD_ZERO(&set);
   FD_SET(sock, &set);
   struct timeval timeout;
   timeout.tv_sec = 4;
    timeout.tv_usec = 0;
    if (connect(sock, (struct sockaddr *) &addr, sizeof addr)) {
        int res= select(FD_SETSIZE, NULL, &set, NULL, &timeout);
        if (!res){
            if (retry_con > 0 ) {
                retry_con--;
                close(sock);
                goto RETRY_CONNECT;
            }
            printf("%s\n", "error connecting");
            exit(-1);
        } else if (FD_ISSET(sock, &set)) {
            if (getpeername(sock, NULL, NULL) == -1 && errno == ENOTCONN) {
                 printf("%s\n", "error connecting");
                exit(-1);
            }
            flags &= ~O_NONBLOCK;
            fcntl(sock, F_SETFL, flags);
        }
    }
    printf("%s\n", "connected");
    fcntl(sock, F_SETOWN, getpid());
    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    sigdelset(&mask, SIGURG);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);
    struct sigaction sa={0};
    sa.sa_handler = sig;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGURG, &sa, NULL);
    int optval =1;
    int r = setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
        optval = 10;
    r += setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &optval, sizeof optval);
    optval = 5;
    r += setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &optval, sizeof optval);
    optval = 2;
    r += setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &optval, sizeof optval);
    optval = (10+5*2)*1000-1;
    r += setsockopt(sock, IPPROTO_TCP, TCP_USER_TIMEOUT, &optval, sizeof optval);
    struct timeval tmrcv;
    tmrcv.tv_sec = 15;
   // r += setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tmrcv, sizeof tmrcv);
    if (r)
        return 1;
    
    
    
      int rd =0;
      int k=0;
  
  
      
      //rd=write(sock, "0.0", sizeof "0.0");
    wrs("0.0", sizeof "0.0");
    //wrs("2.1.1", sizeof "2.1.1");
         //rd = read(sock, buf, sizeof buf);
         rd = rds(buf, sizeof buf);
       printf("%s\n", buf);
       

       /* tfprotocol public testing server public key */

 const char pubkey[] = "-----BEGIN PUBLIC KEY-----\n"
	"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuPfTLredGcSSpZCxavfU\n"
	"dElpcZm0Kls5JhtR08cBlYMD6KKLNQIMugR0WQ5VOufUznmX2xJ70BCoZXE+7ty4\n"
	"U1WeWSkkxPi/REGO1CKTHOMx+YpDV87L2+qyCryPc5w13ZFNIHZBg6Je9IrfnwW8\n"
	"iNOriFv2omXtKT9/GwnHgZ+tt+zH7OshE9wcL9yILrOtHUdQQrmaZxO/XBFXnXwW\n"
	"Ls6bYrzwPflT49hQsgvXkftXgHKInF/UyjFr14z80r4iNlU2lWqqfYCL47uWpb7d\n"
	"WEBxSmONk9dssVnKE6RKK0A9wgQ744zcJBZiHPhSRiloSX5Y24q466l8vf/DGJ6v\n"
	"1QIDAQAB\n"
	"-----END PUBLIC KEY-----\n";

   
        deckey = malloc(sizeof "this is a session key");
        deckey_tx = malloc(sizeof "this is a session key");
        strcpy(deckey, "this is a session key");
        deckey_len = sizeof "this is a session key";
        seed =  *(int64_t *) deckey;
        jump = *((int64_t *) deckey + 1);
        seed_tx =  *(int64_t *) deckey;
        jump_tx = *((int64_t *) deckey + 1);
        strcpy(deckey_tx, deckey);
        
         char enkey[2048/8]="";
         
          BIO *keybio = NULL;
        keybio = BIO_new_mem_buf(pubkey, -1);
        RSA *rsa = NULL;
        rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
        int pad = RSA_PKCS1_OAEP_PADDING;
        //int enbyt = RSA_public_encrypt( sizeof deckey, deckey, enkey, rsa, pad);
        int enbyt = RSA_public_encrypt(deckey_len, deckey, enkey, rsa, pad);
        RSA_free(rsa);
        BIO_free(keybio);
        
        //rd=write(sock, enkey, sizeof enkey); 
        wrs(enkey, sizeof enkey);
         memset(buf, 0, sizeof buf);
        //rd = read(sock, buf, sizeof buf);
         rd = rds(buf, sizeof buf);
//        printf("%s %s\n", buf, " si");
        
        // START WITH CRYPTED DATA
        encrypt = 1;
         const char hash[]="testhash";
        memset(buf, 0, sizeof buf);
         strcpy(buf, hash);
            //decrypt(buf, sizeof hash); 
         //rd=write(sock, buf, sizeof hash);
         wrs(buf, sizeof hash);
         memset(buf, 0, sizeof buf);
          //rd = read(sock, buf, sizeof buf);
         rd = rds(buf, sizeof buf);
         //decrypt(buf, rd); 
         printf("%d %s\n", rd, buf);  
        //  con();
          
         
         //char x[BUFSZ]="";
         
       
      
         
         pthread_t th;
         pthread_t thudp;
            //int tid = pthread_create(&th, NULL, outofband, NULL);
         //int tidudp = pthread_create(&thudp, NULL, threadudp, NULL);
           
         do {
              memset(buf, 0, sizeof buf);
          //    memset(buf, 0, sizeof buf);
             
              printf("%s\n", "TFPROTOCOL read command");
            
        
        char*fres =NULL;
        //do 
            fres = fgets(buf, sizeof buf, stdin);
       // while (!fres && errno==EINTR);
        
            
        //strcpy(buf, "xsime_start\n");
        *strchr(buf  ,'\n') = '\0';
        if (strstr(buf, "fstatls ")) {
            wrs(buf, strlen(buf));
            cmd_fstatls();
            continue;
        }
        if (strstr(buf, "getfsperm "))
            goto KEEP_GOING;
        if (strstr(buf, "ftypels ")) {
            wrs(buf, strlen(buf));
            cmd_ftypels();
            continue;
        }
        if (strstr(buf, "ftype ")) {
            wrs(buf, strlen(buf));
            cmd_ftype();
            continue;
        }
        if (strstr(buf, "fsizels ")) {
            wrs(buf, strlen(buf));
            fsizels();
            continue;
        }
        if (strstr(buf, "fsize ")) {
            wrs(buf, strlen(buf));
            fsize();
            continue;
        }
        if (strstr(buf, "sdown ")) {
            wrs(buf, strlen(buf));
            cmd_sdown();
            continue;
        }
        if (strstr(buf, "intread ")) {
            wrs(buf, strlen(buf));
            cmd_intread();
            continue;
        }
        if (strstr(buf, "intwrite ")) {
            wrs(buf, strlen(buf));
            cmd_intwrite();
            continue;
        }
        if (strstr(buf, "sup ")) {
            wrs(buf, strlen(buf));
            cmd_sup();
            continue;
        }
        if (!strcmp(buf, "xs_gateway")) {
            xs_gateway();
            continue;
        }
        if (!strcmp(buf, "tcp_tlb")) {
            strcpy(buf, "tlb");
            wrs(buf, strlen(buf));
            memset(buf, 0, sizeof buf);
            rds(buf, sizeof buf);
            printf("%s\n", buf);
            continue;
        }
        if (!strcmp(buf, "xs1_readv2")) {
            xs1_readv2();
            continue;
        } if (!strcmp(buf, "xs1_writev2")) {
            xs1_writev2();
            continue;
        } if (strstr(buf, "nigma")) {
            int i = atoi(strstr(buf, "nigma ") +strlen("nigma "));
            wrs(buf, strlen(buf));
            memset(buf, 0, sizeof buf);
            rds(buf, sizeof buf);
            printf("%s\n", buf);
            if (!strcmp(buf, "OK"))
                nigma();
            continue;
        }
        if (!strcmp(buf, "xs_ace")) {
            wrs(buf, strlen(buf));
            memset(buf, 0, sizeof buf);
            rds(buf, sizeof buf);
            printf("%s\n", buf);
            int end = 1;
            while (end) {
                memset(buf, 0, sizeof buf);
                printf("Reading command for xs_ace subsystem\n");
                fres = fgets(buf, sizeof buf, stdin);
                char *p  = strchr(buf, '\n');
                if (p)
                    *p='\0';
                if (strstr(buf,"run_nl") || strstr(buf,"run_buf")) {
                    wrs(buf, strlen(buf));
                    xs_ace();
                    continue;
                
                } else if (strstr(buf, "run ")) {
                    wrs(buf, strlen(buf));
                    xs_ace_run();
                    continue;
                }
                if (!strcmp(buf, "exit") || !strcmp(buf, "goback"))
                    end = 0;
                wrs(buf, strlen(buf));
                memset(buf, 0, sizeof buf);
                rds(buf, sizeof buf);
                printf("%s\n", buf);
            }
            continue;
        }
        
        if (strstr(buf, "xs_sqlite")) {
            strcpy(buf, "XS_SQLITE");
            wrs(buf, strlen(buf));
             memset(buf, 0, sizeof buf);
            rds(buf, sizeof buf);
            if (!strcmp(buf, "OK")) {
                printf("%s %s\n", buf, "inside sqlite module");
                xs_sqlite_module();
            }
            continue;
        }
        if (strstr(buf, "xs_mysql")) {
            strcpy(buf, "XS_MYSQL");
            wrs(buf, strlen(buf));
            memset(buf, 0, sizeof buf);
            rds(buf, sizeof buf);
            if (!strcmp(buf, "OK")) {
                printf("%s\n", "inside mysql module");
                xs_sqlite_module();
            }
            continue;
        }
        if (strstr(buf, "xs_postgresql")) {
            strcpy(buf, "XS_POSTGRESQL");
            wrs(buf, strlen(buf));
            memset(buf, 0, sizeof buf);
            rds(buf, sizeof buf);
            if (!strcmp(buf, "OK")) {
                printf("%s\n", "inside postgresql module");
                xs_sqlite_module();
            }
            continue;
        }
        if (strstr(buf, "getcan")) {
            cmd_getcan();
            continue;
        }
        if (strstr(buf, "putcan")) {
            cmd_putcan();
            continue;
        }
        if (strstr(buf, "put")) {
            cmd_put();
            continue;
        }
        if (strstr(buf, "get")) {
            cmd_get();
            continue;
        }
        if (!strcmp(buf, "oob")) {
            outofband(NULL);
            continue;
        }
        if (!strcmp(buf, "hbeat")) {
            threadudp(NULL);
            continue;
        }
        if (!strcmp(buf, "tlb")) {
            tlb();
            continue;
        }
        if (strstr(buf,"xsime_start")) {
             wrs(buf, strlen(buf));
              memset(buf, 0, sizeof buf);
            rd = rds(buf, sizeof buf);
              printf("\n %s %d %s\n", "from server: ", rd, buf);
             xs_ime_start();
            continue;
         } 
        
         if (strstr(buf,"LS") || strstr(buf, "ls")) {
             //rd=write(sock, buf, strlen(buf));
             wrs(buf, strlen(buf));
             lscmd();
            continue;
         } 
         if (strstr(buf, "RCVFILE")||strstr(buf, "rcvfile")) {
            
          //rd=write(sock, buf, strlen(buf));
             wrs(buf, strlen(buf));
             lscmd();
             //printf("termino\n");
            continue;
         }
         if (strstr(buf, "SNDFILE")||strstr(buf, "sndfile")) {
             //rd=write(sock, buf, strlen(buf));
             wrs(buf, strlen(buf));
             sndfile();
             continue;
         }
KEEP_GOING:
         //rd=write(sock, buf, strlen(buf));
         wrs(buf, strlen(buf));
         //sleep(3);
         if (!strcmp(buf,"STARTNTFY")){
             printf("going to inverse\n");
             goto INVERSE;
         }
         memset(buf, 0, sizeof buf);
         //rd= read(sock, buf, sizeof buf);
         rd = rds(buf, sizeof buf);
        /* if (rd ==0 )
             return 0;
        else */if (rd == -1) {
              printf("%s\n", "connection lost");
              return 1;
        }
        printf("\n %s %d %s\n", "from server: ", rd, buf);
         
   
         } while (1);
INVERSE:
         while (1) {
             //memset(buf, 0, sizeof buf);
             //rd= read(sock, buf, sizeof buf);
             rd = rds(buf, sizeof buf);
            if (rd == 0 )
                break;
            buf[rd]=0;
            printf("\n %s %d %s\n", "from server: ", rd, buf);
             fgets(buf, sizeof buf, stdin);
      
          *strchr(buf  ,'\n')='\0';
         //rd=write(sock, buf, strlen(buf));
         wrs(buf, strlen(buf));
         }

    return 0;
}


void sndfile(void)
{
    //char buf[BUFSZ];
     //printf("inside\n");
    int rd=0;
    char cmd[32]="\0";
    int fd = open("test", O_RDWR|O_CREAT, S_IRWXU);
    if (fd==-1) exit(0);
    int i =0;
    while (1) {
       
        
        memset(buf, 0, sizeof buf);
        memset(cmd, 0, sizeof cmd);
        //rd= read(sock, buf, sizeof buf);
        rd = rds(buf, sizeof buf);
        //if (rd==0)
          //  break;
        
        excmd(buf, cmd);
       printf("%d %s\n", rd, cmd);
        
      //   memset(buf, 0, sizeof buf);
        //if (!strcmp(cmd, "BREAK")||!strcmp(cmd, "OK"))
          //  break;
        if (strcmp(cmd, "CONT")) {
            printf("%s\n", buf);
            break;
        }
        if (!strcmp(cmd, "CONT")) {
            
            rd = read(fd, buf+strlen("CONT")+1, sizeof buf -strlen("CONT")-1);
            // fgets(buf, sizeof buf, stdin);
            //  *strchr(buf  ,'\n')='\0';
            if (rd) {
                strcpy(buf, "CONT");
                *strchr(buf  ,'\0')=' ';
                //write(sock, buf, strlen("CONT")+1+rd);
                wrs(buf,  strlen("CONT")+1+rd);
             } else{
                 
                 strcpy(buf, "OK");
                 //*strchr(buf  ,'\0')=' ';
                 //write(sock, buf, strlen("OK"));
                  wrs(buf,  strlen("OK"));
             }
              
              
        } /*else {
            printf("%s\n", buf);
            break;
        }*/
        
        
    }
    close(fd);
}

void lscmd(void)
{
    //char buf[BUFSZ];
    //printf("inside\n");
    int rd=0;
    char cmd[32]="\0";
    int fd = open("test", O_RDWR|O_TRUNC|O_CREAT, S_IRWXU);
    if (fd==-1) exit(0);
    int i =0;
    while (1) {
        memset(buf, 0, sizeof buf);
        //rd= read(sock, buf, sizeof buf);
         rd = rds(buf,  sizeof buf);
        //if (rd==0)
          //  break;
        excmd(buf, cmd);
        printf("%d %s\n", rd, cmd);
        if (!strcmp(cmd, "OK"))
            break;
        if (!strcmp(cmd, "CONT")) {
            
            write(fd, buf + strlen("CONT "), rd - strlen("CONT "));
            // fgets(buf, sizeof buf, stdin);
            //  *strchr(buf  ,'\n')='\0';
            strcpy(buf, "CONT");
             wrs(buf,  strlen(buf));
             // write(sock, buf, strlen(buf));
              
        } else {
            printf("%s\n", buf);
            break;
        }
        
        
    }
    close(fd);
}


void excmd(char *src, char *dst)
{
    memset(dst, 0, 32);
    while (*src && *src != ' ')
        *dst++ = *src++;
    //*dst='\0';
}

void decrypt(char *data, int len, int op)
{         
    int c = 0;
    int keyc = 0;
    if (!op) {
        for (; c < len; c++, keyc++) {
            if (keyc == deckey_len)
                keyc = 0;
             *(data + c) = *(data + c) - (seed >> 56 & 0xFF);
            *(data + c) ^= deckey[keyc];
            
            seed = seed * (seed>>8&0xFFFFFFFF)+ (seed>>40&0xFFFF);
            
            if (seed == 0)
                seed = *(int64_t *) deckey;
            deckey[keyc] = seed%256;
        }
    } else {
        for (; c < len; c++, keyc++) {
            if (keyc == deckey_len)
                keyc = 0;
            *(data + c) ^= deckey_tx[keyc];
            *(data + c) = (seed_tx>>56&0xFF) + *(data + c);
            seed_tx = seed_tx * (seed_tx>>8&0xFFFFFFFF)+ (seed_tx>>40&0xFFFF);
            
            if (seed_tx == 0)
                seed_tx = *(int64_t *) deckey_tx;
            deckey_tx[keyc] = seed_tx%256;
        }
    }
}


int wrs(char *buf, int len)
{
    hdrsetsz(len, &txhdr);
    if (wrs_real((char *) &txhdr, sizeof txhdr) == -1)
        return -1;
    return wrs_real(buf, len);
}

int wrs_real(char *buf, int len)
{
    if (encrypt)
        decrypt(buf, len, 1);   
    int wb = 0;
    int writed = 0;
    //int left = len;
    while (len > 0) {
        do
            wb = write(sock, buf + writed, len);
        while (wb == -1 && errno == EINTR);
        if (wb == -1)
            return -1;
        len -= wb;
        writed += wb;
    }
    return writed;
}

int rds(char *buf, int len)
{   
    if (rds_real((char *) & rxhdr, sizeof rxhdr) == -1)
        return -1;
    len = hdrgetsz(&rxhdr);
    return rds_real(buf, len);
}


int rds_real(char *buf, int len) 
{
    int rb = 0;
    int readed = 0;
    //int left = len;
    while (len > 0) {
        do 
            rb = read(sock, buf + readed, len);
        while (rb == -1 && errno == EINTR);
        if (rb == 0 || rb == -1)
            return -1;
        len -= rb;
        readed += rb;
    }
    if (encrypt)
        decrypt(buf, readed, 0);
    return readed;
}


    pthread_t wrth;
    pthread_t rdth;
char outbuf[500];
char inbuf[500];
char opcstr[20];
#pragma pack(push, 1)
struct xsime_hdr {
    char opcode;
    int sz;
} rdhdr, wrhdr;
#pragma pack(pop)

void *wrthread(void *arg)
{
    int rd;
    while (1) {
        printf("%s\n", "enter ime opcode");
        fgets(opcstr, sizeof opcstr, stdin);
         *strchr(opcstr,'\n')='\0';
        wrhdr.opcode  = atoi(opcstr);      
        printf("%s\n", "enter payload");
          fgets(outbuf, sizeof outbuf, stdin);
              *strchr(outbuf,'\n')='\0';
              
              int sz=strlen(outbuf);
              //sprintf(wrhdr.sz, "%d", sz);
              wrhdr.sz=sz;
              if (wrhdr.opcode == 7) {
                  *(uint64_t *) (outbuf)= strtoull(outbuf, NULL,10);
                  swapbo64(*(uint64_t *) (outbuf));
                  sz = sizeof(uint64_t);
                  wrhdr.sz= sz;
             }
              swapbo32(wrhdr.sz);
              if (wrhdr.opcode == 4) {
                wrhdr.sz = atoi(outbuf);
                swapbo32(wrhdr.sz);
                printf("%s\n", "get timestamp:");
                char longnum[22];
                fgets(longnum, sizeof longnum, stdin);
                *strchr(longnum,'\n')='\0';
                long long ll = atoll(longnum);
                swapbo64(ll);
                memcpy(outbuf, &ll, sizeof ll);
                sz=sizeof (long long);
              }
            if (wrhdr.opcode == 0){
                wrs_ex((char *)&wrhdr, sizeof wrhdr);
                break;
            }
            wrs_ex((char *)&wrhdr, sizeof wrhdr);
             wrs_ex(outbuf, sz);        
    }
}

void *rdthread(void *arg)
{
    while (1) {
        rds_ex((char *) &rdhdr, sizeof rdhdr);
        if (rdhdr.opcode == 0) {
            printf("%s\n", "0 code received, exiting");
            
            break;
        }
        swapbo32(rdhdr.sz);
        rds_ex(inbuf, rdhdr.sz); 
        inbuf[rdhdr.sz] = '\0';
        printf("from server: %d %d %s\n", rdhdr.opcode, rdhdr.sz, inbuf);
        fflush(stdout);
    
    }
}

void xs_ime_start(void)
{
memset((char *)&rdhdr, 0 , sizeof rdhdr);
memset((char *)&wrhdr, 0 , sizeof wrhdr);
memset(opcstr, 0 , sizeof opcstr);
memset(outbuf, 0 , sizeof outbuf);
memset(inbuf, 0 , sizeof inbuf);
 int err = pthread_create(&wrth, NULL, wrthread, NULL);
    err += pthread_create(&rdth, NULL, rdthread, NULL);
    
      pthread_join(rdth, NULL);
    pthread_join(wrth, NULL);
}


static int32_t hdrgetsz(struct tfhdr *hdr)
{
    int32_t sz = hdr->sz;
    if (!isbigendian())
        swapbo32(sz);
    return sz;
}

static void hdrsetsz(int32_t sz, struct tfhdr *hdr)
{
    if (!isbigendian())
        swapbo32(sz);
    hdr->sz = sz;
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


int wrs_ex(char *buf, int len)
{
    return wrs_real(buf, len);
}

int rds_ex(char *buf, int len)
{
    return rds_real(buf, len);
}


void *outofband(void *v)
{
    int i=0;
    //while (1) {
       //sleep(1);
        send(sock,"a", 1, MSG_OOB);
        recv(sock,&i, 1,MSG_OOB);
      //  printf("%s\n", "out-of-band data received in thread\n");
       // i++;
    //}
    
   
    return NULL;
}

void sig(int signo)
{
   /* int i;
    if (recv(sock,&i, 1,MSG_OOB)==-1 && errno == EWOULDBLOCK) 
        printf("not blocked\n");*/
    printf("%s\n", "out-of-band data received in signal\n");
}


void *threadudp(void *prms)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr2 = addr;
    int len = sizeof addr2;
    char msg[400]="";
    printf("%s\n", "select type of heartbeat\n0\n1uniquekey\n2uniquekey");
    fgets(msg, sizeof msg, stdin);
     *strchr(msg, '\n')='\0';
    *msg = *msg - 48;
    int type = *msg;
        
    sendto(sock, msg, strlen(msg), 0, (struct sockaddr *) &addr2, len);
    recvfrom(sock, msg, sizeof msg, 0, (struct sockaddr *) &addr2, &len);
    if (*msg == 1)
           printf("%s\n", "respond udp_ok");
       else
           printf("%s\n", "respond udp_failed");
       if (type == 2)
           printf("%s\n", msg+1);
    /*while (1) {
         *msg=1;
         strcpy(msg+1, "/key_test");
         sendto(sock, msg, sizeof msg, 0, (struct sockaddr *) &addr2, len);
        recvfrom(sock, msg, sizeof msg, 0, (struct sockaddr *) &addr2, &len);
       if (*msg == 1)
           printf("%s\n", "respond udp_ok");
       else
           printf("%s\n", "respond udp_failed");
       
        sleep(1);
    }*/
    close(sock);
    return NULL;
}

volatile int cont;

#define PUTFIN -127
#define PUTEND 0
#define PUTSTOP -1
#define PUTCANCEL -2
#define PUTCONT -3
void *putflow(void *prms)
{
    int64_t code;
    while (1) {
        rds_ex((char *) &code, sizeof code);
        swapbo64(code);
        if (code == PUTCANCEL) {
            printf("cancelado\n");
            cont = 0;
            continue;
        }
        if (code == PUTFIN) {
            
            break;
        }
    }
}

void cmd_put(void) 
{
    struct hpfile {
        uint64_t offset;
        int64_t sz;
    } hp={0,12000000};
    swapbo64(hp.offset);
    swapbo64(hp.sz);
    ////wrs_ex((char *) &hp, sizeof hp)
    strcat(buf, " ");
    memcpy(buf + strlen(buf) , (char *) &hp, sizeof hp); 
    wrs(buf, strlen(buf)-1+1+sizeof hp);
    memset(buf, 0, sizeof buf);
    int rd =rds(buf, sizeof buf);
    *strchr(buf  ,' ')='\0';
    if (strcmp(buf, "OK")) {
        printf("%s\n", buf);
        return;
    }
    memcpy((char *) &hp.sz, buf+3, sizeof hp.sz);
    swapbo64(hp.sz);
    int fd = open("test", O_RDWR|O_CREAT, S_IRWXU);
    if (fd==-1)
        exit(0);
    char *buf = malloc(hp.sz);
    cont = 1;
    pthread_t th;
    int tid = pthread_create(&th, NULL, putflow, NULL);
    int64_t rdb;
    //int autoerror =0;
    while (cont) {
        //memset(buf, 0, hp.sz);
        rdb = read(fd, buf, hp.sz);
        //printf("%d\n", rdb);
       /* if (autoerror <=7)
            autoerror++;
        else 
            rdb=-1;
        */
        if (rdb > 0) {
        
            int64_t rdbuf = rdb;
            swapbo64(rdbuf);
            wrs_ex((char *) &rdbuf, sizeof rdbuf);
                
            wrs_ex(buf, rdb);
            
        } else if (rdb == 0) {
            rdb = PUTEND;
             int64_t rdbuf = rdb;
             swapbo64(rdbuf);
            wrs_ex((char *) &rdbuf, sizeof rdbuf);
            break;
        } else if (rdb == -1) {
            rdb = PUTCANCEL;
             int64_t rdbuf = rdb;
             swapbo64(rdbuf);
            wrs_ex((char *) &rdbuf, sizeof rdbuf);
            break;
        }
    }

    
    pthread_join(th, NULL);
    rdb = PUTFIN;
    int64_t rdbuf = rdb;
    swapbo64(rdbuf);
    wrs_ex((char *) &rdbuf, sizeof rdbuf);
    free(buf);
    close(fd);
}

void cmd_get(void)
{
    struct hpfile {
        uint64_t offset;
        int64_t sz;
    } hp={0,12000000};
    swapbo64(hp.offset);
    swapbo64(hp.sz);
    ////wrs_ex((char *) &hp, sizeof hp)
    strcat(buf, " ");
    memcpy(buf + strlen(buf) , (char *) &hp, sizeof hp); 
    wrs(buf, strlen(buf)-1+1+sizeof hp);
    memset(buf, 0, sizeof buf);
    int rd =rds(buf, sizeof buf);
    *strchr(buf  ,' ')='\0';
    if (strcmp(buf, "OK")) {
         *strchr(buf  ,'\0')=' ';
        printf("%s\n", buf);
        return;
    }
    memcpy((char *) &hp.sz, buf+3, sizeof hp.sz);
    swapbo64(hp.sz);
    
    char *buf = malloc(hp.sz);
    int fd = open("test", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    swapbo64(hp.offset);
    lseek(fd, hp.offset, SEEK_SET);
    if (fd == -1)
        exit(0);
    int64_t len;
    int64_t code;
    int autoerror=0;
    int i=0;
    while (1) {
        rds_ex((char *) &len, sizeof len);
        
        swapbo64(len);
        if (len == PUTEND)
            break;
        else if (len == PUTSTOP)
            break;
        else if (len == PUTCANCEL) {
            unlink("test");
            break;
        } else {
            
            rds_ex(buf, len);
            
            int wb = writechunk(fd, buf, len);
            
            //printf("%d\n", wb);
           /* if (autoerror <= 5)
                autoerror++;
            else
                wb=-1;*/
            if (wb != len) {
                
                code = PUTCANCEL;
                swapbo64(code);
                printf("sending canccel\n");
                wrs_ex((char *) &code, sizeof code);
                break;
            }
        }
    }

    code = PUTFIN;
    swapbo64(code);
    wrs_ex((char *) &code, sizeof code);
    while (1) {
        rds_ex((char *) &len, sizeof len);
        swapbo64(len);
        if (len == PUTCANCEL || len == PUTEND || len == PUTSTOP)
            continue;
        if (len == PUTFIN)
            break;
        else {
            rds_ex(buf , len);
        //    printf("%d %s\n",i++, "extra reading\n");
            
        }
    }
   
}

static int writechunk(int fd, char *buf, int64_t len)
{
    int64_t wb;
    do 
        wb = write(fd, buf, len);
    while (wb == -1 && errno == EINTR);
    return wb;
}


void cmd_putcan(void) 
{
    
    struct hpfile {
        uint64_t offset;
        int64_t sz;
        uint64_t canpt;
    } hp={0, 1000000, 5};
    swapbo64(hp.offset);
    swapbo64(hp.sz);
    int canpt = hp.canpt;
    int canstep = 0;
    swapbo64(hp.canpt);
    ////wrs_ex((char *) &hp, sizeof hp)
    strcat(buf, " ");
    memcpy(buf + strlen(buf) , (char *) &hp, sizeof hp); 
    wrs(buf, strlen(buf)-1+1+sizeof hp);
    memset(buf, 0, sizeof buf);
    int rd =rds(buf, sizeof buf);
    *strchr(buf  ,' ')='\0';
    if (strcmp(buf, "OK")) {
        printf("%s\n", buf);
        return;
    }
    memcpy((char *) &hp.sz, buf+3, sizeof hp.sz);
    swapbo64(hp.sz);
    int fd = open("test", O_RDWR|O_CREAT, S_IRWXU);
    if (fd==-1)
        exit(0);
    
    char *buf = malloc(hp.sz);
    if (!buf)
        exit(-1);
    int64_t rdb;
    //int autoerror =0;
    int64_t code=0;
    int autoerr =0;
    while (1) {
        //memset(buf, 0, hp.sz);
        
        rdb = read(fd, buf, hp.sz);

        if (rdb > 0) {
         
            int64_t rdbuf = rdb;
            swapbo64(rdbuf);
            wrs_ex((char *) &rdbuf, sizeof rdbuf);
                
            wrs_ex(buf, rdb);
            
        } else if (rdb == 0) {
             
            rdb = PUTEND;
             int64_t rdbuf = rdb;
             swapbo64(rdbuf);
            wrs_ex((char *) &rdbuf, sizeof rdbuf);
            break;
        } else if (rdb == -1) {
            rdb = PUTCANCEL;
             int64_t rdbuf = rdb;
             swapbo64(rdbuf);
            wrs_ex((char *) &rdbuf, sizeof rdbuf);
            break;
        }
       
        if (canpt >= 1) {
            
            canstep++;
            if (canstep == canpt) {
                rds_ex((char *) &code, sizeof code);
                swapbo64(code);
                if (code != PUTCONT)
                    break;
                canstep = 0;
            }
        }
            /*if (autoerr++ == 3){
                printf("%lld\n", code);
            rdb = PUTSTOP;
             int64_t rdbuf = rdb;
             swapbo64(rdbuf);
            wrs_ex((char *) &rdbuf, sizeof rdbuf);
            break;
        }*/
        
    }
    
    free(buf);
    close(fd);
}

void cmd_getcan(void)
{
    struct hpfile {
        uint64_t offset;
        int64_t sz;
        uint64_t canpt;
    } hp={0, 30, 5};
    swapbo64(hp.offset);
    swapbo64(hp.sz);
    int canpt = hp.canpt;
    int canstep = 0;
    int end =0;
    swapbo64(hp.canpt);
    ////wrs_ex((char *) &hp, sizeof hp)
    strcat(buf, " ");
    memcpy(buf + strlen(buf) , (char *) &hp, sizeof hp); 
    wrs(buf, strlen(buf)-1+1+sizeof hp);
    memset(buf, 0, sizeof buf);
    int rd =rds(buf, sizeof buf);
    *strchr(buf, ' ')='\0';
    if (strcmp(buf, "OK")) {
         *strchr(buf  ,'\0')=' ';
        printf("%s\n", buf);
        return;
    }
    memcpy((char *) &hp.sz, buf+3, sizeof hp.sz);
    swapbo64(hp.sz);
    
    char *buf = malloc(hp.sz);
    int fd = open("test", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    swapbo64(hp.offset);
    lseek(fd, hp.offset, SEEK_SET);
    if (fd == -1)
        exit(0);
    int64_t len;
    int64_t code;
    int autoerror=0;
    int i=0;
    while (1) {
        rds_ex((char *) &len, sizeof len);
        
        swapbo64(len);
        if (len == PUTEND)
            break;
        else if (len == PUTSTOP)
            break;
        else if (len == PUTCANCEL) {
           // unlink("test");
            break;
        } else {
            rds_ex(buf, len);
            //printf("%d\n", wb);
           /* if (autoerror <= 5)
                autoerror++;
            else
                wb=-1;*/
            if (!end) {
                int wb = writechunk(fd, buf, len);
                if (wb != len) {
                    code = PUTCANCEL;
                    end = 1;
                } else 
                    code = PUTCONT;
            }
        }
        if (canpt >= 1) {
            canstep++;
            if (canstep == canpt) {
                swapbo64(code);
                wrs_ex((char *) &code, sizeof code);
                if (end)
                    break;
                canstep = 0;
            }
        }
    }
   
}

void xs_sqlite_module(void)
{
    
    
     int64_t head;
     int len ;
     int cont =1;
    while (cont) {
        
    char *fpt = fgets(buf, sizeof buf, stdin);
     *strchr(buf  ,'\n') = '\0';
     char tmp[2048];
     strcpy(tmp, buf);
     char *tok = strtok(tmp, " ");
     if (!strcmp(tok, "EXEC")) {
         len = strlen(buf);
        head = len;
        swapbo64(head);
        wrs_ex((char *)&head, sizeof head);
        wrs_ex(buf, len);
        
         rds_ex((char *)&head, sizeof head);
        swapbo64(head);
        memset(buf, 0, sizeof buf);
        rds_ex(buf, head);
        if (strstr(buf, "EXEC OK")) {
            printf("%s\n", "entering callback receiving");
            printf("%s\n", buf);
            sqlite_exec();
        } else
            printf("%s\n", buf);
         continue;
     }
     if (!strcmp(buf, "TERMINATE") || !strcmp(buf, "EXIT"))
            cont =0;
        len = strlen(buf);
        head = len;
        swapbo64(head);
        wrs_ex((char *)&head, sizeof head);
        wrs_ex(buf, len);
        if (!cont)
            break;
        
        rds_ex((char *)&head, sizeof head);
        swapbo64(head);
        memset(buf, 0, sizeof buf);
        rds_ex(buf, head);
        printf("%s\n", buf);
        
    }
}

void sqlite_exec(void)
{
    char *buf;
    int64_t head;
    do {
        rds_ex((char *)&head, sizeof head);
        swapbo64(head);
        buf=malloc(head+ 1);
        memset(buf, 0, head+1);
        rds_ex(buf, head);
        printf("%s\n", buf);
        free(buf);
    } while (head);
}


// xs_ace run_nl multi-thread.

#define XS_ERRBFLEN 256
#define XS_ACE_RUNEND -129000
#define XS_ACE_ERROR -128000
#define XS_ACE_OK -127000
#define XS_ACE_SIGKILL -130000
#define XS_ACE_SIGTERM -131000
#define XS_ACE_SIGUSR1 -132000
#define XS_ACE_SIGUSR2 -133000
#define XS_ACE_SIGHUP -134000

pthread_t xsace_rdth;
pthread_t xsace_wrth;
int64_t xs_txhdr;
int64_t xs_rxhdr;
/* thoes buffers below are intended for test purpose only.
    its size is limited, so dont use payloads to long. */
char xs_txbuf[bufsize];
char xs_rxbuf[bufsize];
char xs_errbuf[XS_ERRBFLEN];

void *runnl_rdth(void *prms);
void *runnl_wrth(void *prms);
volatile int done;

void xs_ace(void)
{
    int run =0;
    int cont = 1;
    done =0;
    int rs = pthread_create(&xsace_rdth, NULL, runnl_rdth, NULL);
    pthread_join(xsace_rdth, NULL);
    pthread_join(xsace_wrth, NULL);
    int32_t es;
    rds_ex((char *) &es, sizeof es);
    swapbo32(es);
    printf("Exit status: %d\n", WEXITSTATUS(es));
}

void xs_ace_run(void)
{
    /*intended for the example xs_ace_demo_run.c */
    int loop = 1;
    memset(buf, 0, sizeof buf);
        rds(buf, sizeof buf);
        printf("%s\n", buf);
    while (loop) {
        memset(buf, 0, sizeof buf);
        printf("Reading data for xs_run\n");
        fgets(buf, sizeof buf, stdin);
        char *p  = strchr(buf, '\n');
        if (p)
            *p='\0';
        if (!strcmp(buf, "quit"))
            loop = 0;
        wrs(buf, strlen(buf));
        memset(buf, 0, sizeof buf);
        rds(buf, sizeof buf);
        printf("%s\n", buf);
    }
    int32_t es;
    rds_ex((char *) &es, sizeof es);
    swapbo32(es);
    printf("Exit status: %d\n", WEXITSTATUS(es));
}

void *runnl_rdth(void *prms)
{
    int64_t rd;
    while (1) {
        rd = rds_ex((char *) &xs_rxhdr, sizeof xs_rxhdr);
        if (rd == -1) {
            printf("connection lost\n");
            exit(0);
        }
        swapbo64(xs_rxhdr);
        if (xs_rxhdr == XS_ACE_ERROR) {
            rds_ex(xs_errbuf, sizeof xs_errbuf);
            printf("error: %s\n", xs_errbuf);
            break;
        } 
        if (xs_rxhdr == XS_ACE_OK) {
            printf("%s\n", "Module starting execution\n");
            int rs = pthread_create(&xsace_wrth, NULL, runnl_wrth, NULL);
            continue;
        }
        if (xs_rxhdr == XS_ACE_RUNEND) {
            printf("%s\n", "Module execution ended; Hit Enter\n");
            done =1;
            break;
        }
        memset(xs_rxbuf, 0, sizeof xs_rxbuf);
        rd = rds_ex(xs_rxbuf, xs_rxhdr);
        if (rd == -1) {
            printf("connection lost\n");
            exit(0);
        }
        printf("from server: %s\n", xs_rxbuf);
    }
    return NULL;
}


void *runnl_wrth(void *prms)
{
    int64_t wr;
    while (1) {
        memset(xs_txbuf, 0, sizeof bufsize);
        char*fres = fgets(xs_txbuf, sizeof xs_txbuf, stdin);
        if (done)
            break;
        char *p = strchr(xs_txbuf, '\n');
        if (p)
            *p='\0';
        if (!strcmp(xs_txbuf, "sigkill")) {
            printf("sending sigkill\n");
            xs_txhdr = XS_ACE_SIGKILL;
            swapbo64(xs_txhdr);
            wr = wrs_ex((char *) &xs_txhdr, sizeof xs_txhdr);
            if (wr == -1) {
                printf("connection lost\n");
                exit(0);
            }
            continue;
        } else if (!strcmp(xs_txbuf, "sigterm")) {
            printf("sending sigterm\n");
            xs_txhdr = XS_ACE_SIGTERM;
            swapbo64(xs_txhdr);
            wr = wrs_ex((char *) &xs_txhdr, sizeof xs_txhdr);
            if (wr == -1) {
                printf("connection lost\n");
                exit(0);
            }
            continue;
        } else if (!strcmp(xs_txbuf, "sigusr1")) {
            printf("sending sigusr1\n");
            xs_txhdr = XS_ACE_SIGUSR1;
            swapbo64(xs_txhdr);
            wr = wrs_ex((char *) &xs_txhdr, sizeof xs_txhdr);
            if (wr == -1) {
                printf("connection lost\n");
                exit(0);
            }
            continue;
        } else if (!strcmp(xs_txbuf, "sigusr2")) {
            printf("sending sigusr2\n");
            xs_txhdr = XS_ACE_SIGUSR2;
            swapbo64(xs_txhdr);
            wr = wrs_ex((char *) &xs_txhdr, sizeof xs_txhdr);
            if (wr == -1) {
                printf("connection lost\n");
                exit(0);
            }
            continue;
        } else if (!strcmp(xs_txbuf, "sighup")) {
            printf("sending sighup\n");
            xs_txhdr = XS_ACE_SIGHUP;
            swapbo64(xs_txhdr);
            wr = wrs_ex((char *) &xs_txhdr, sizeof xs_txhdr);
            if (wr == -1) {
                printf("connection lost\n");
                exit(0);
            }
            continue;
        }
        int64_t len = strlen(xs_txbuf);
        xs_txhdr = len;
        swapbo64(xs_txhdr);
        wr = wrs_ex((char *) &xs_txhdr, sizeof xs_txhdr);
        if (wr == -1) {
            printf("connection lost\n");
            exit(0);
        }
        wr = wrs_ex(xs_txbuf, len);
        if (wr == -1) {
            printf("connection lost\n");
            exit(0);
        }
    }
    xs_txhdr = XS_ACE_OK;
    swapbo64(xs_txhdr);
    wr = wrs_ex((char *) &xs_txhdr, sizeof xs_txhdr);
    if (wr == -1) {
        printf("connection lost\n");
        exit(0);
    }
    return NULL;
}

void nigma(void)
{
    int hdr;
    rds_ex((char *) &hdr, sizeof hdr);
    swapbo32(hdr);
    char *key = malloc(hdr);
    int d = rds_ex(key, hdr);
    free(deckey);
    deckey = key;
    deckey_len = hdr;
    
    free(deckey_tx);
    deckey_tx = malloc(hdr);
    memcpy(deckey_tx, key, deckey_len);
}


/* just to check with the 0 open-descriptor id */
#pragma pack(push, 1)
typedef struct v2hdr {
    int32_t fdidx;
    uint64_t bytes;
} V2hdr;
#pragma pack(pop)

char v2err[126];
char sometxt[] = "this is some text";

void xs1_readv2(void)
{
    V2hdr hdr = { 0, 100 };
    memset(buf, 0, sizeof buf);
    strcpy(buf, "XS1_READV2");
    wrs(buf, strlen(buf));
    swapbo64(hdr.bytes);
    swapbo32(hdr.fdidx);
    wrs_ex((char *) &hdr, sizeof hdr);
    int64_t hz;
    rds_ex((char *) &hz, sizeof hz);
    swapbo64(hz);
    if (hz == -1) {
        rds_ex(v2err, sizeof v2err);
        printf("%s\n", v2err);
        return;
    } else if (hz == 0) 
        printf("%s\n", "readed 0");
    char *buf = malloc(hz);
    memset(buf, 0, hz);
    rds_ex(buf, hz);
    printf("%s\n", buf);
    free(buf);
}

void xs1_writev2(void)
{
    V2hdr hdr = { 0, 0 };
    memset(buf, 0, sizeof buf);
    strcpy(buf, "XS1_WRITEV2");
    wrs(buf, strlen(buf));
    hdr.bytes = strlen(sometxt);
    swapbo64(hdr.bytes);
    swapbo32(hdr.fdidx);
    wrs_ex((char *) &hdr, sizeof hdr);
    wrs_ex(sometxt, strlen(sometxt));
    int64_t sz;
    rds_ex((char *) &sz, sizeof sz);
    swapbo64(sz);
     if (sz == -1) {
        rds_ex(v2err, sizeof v2err);
        printf("%s\n", v2err);
        return;
    }
    printf("%s %d\n", "written", sz);
}


void tlb(void)
{
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr2 = addr;
    int len = sizeof addr2;
    char msg[400]="";
    *msg = 3;
        
    sendto(sock, msg, 1, 0, (struct sockaddr *) &addr2, len);
    recvfrom(sock, msg, sizeof msg, 0, (struct sockaddr *) &addr2, &len);
    if (!*msg) 
        printf("TLB failed\n");
    else
        printf("%s\n", msg + 1);
    close(sock);
}

void xs_gateway(void)
{
    char *fres;
    int64_t rd = wrs(buf, strlen(buf));
    memset(buf, 0, sizeof buf);
    rd = rds(buf, sizeof buf);
    if (strstr(buf, "UNKNOWN"))
        return;
    if (rd == -1) {
        printf("%s\n", "connection lost");
        return;
    }
    printf("\n %s %d %s\n", "xs_gateway response: ", rd, buf);
    printf("Set own identity\n");
    fres = fgets(buf, sizeof buf, stdin);
     *strchr(buf  ,'\n') = '\0';
     rd = wrs(buf, strlen(buf));
     if (rd == -1)
         return;
     memset(buf, 0, sizeof buf);
    rd = rds(buf, sizeof buf);
    if (rd == -1) {
        printf("%s\n", "connection lost");
        return;
    }
    printf("identity result %s\n", buf);
    if (strstr(buf, "Failed"))
        return;
    pthread_t th;
    pthread_create(&th, NULL, thgateway, NULL);
    do {
        printf("peer id:\n");
        memset(buf, 0, sizeof buf);
        fres = fgets(buf, sizeof buf, stdin);
        *strchr(buf  ,'\n') = '\0';
        if (!strcmp(buf, "close")) {
            int32_t hdr = -1;
            swapbo32(hdr);
            rd = wrs_ex((char *) &hdr, sizeof hdr);
            break;
        }
        rd = wrs(buf, strlen(buf));
        if (rd == -1)
            break;
        printf("peer message:\n");
        memset(buf, 0, sizeof buf);
        fres = fgets(buf, sizeof buf, stdin);
        *strchr(buf  ,'\n') = '\0';
        rd = wrs(buf, strlen(buf));
        if (rd == -1)
            break;
    } while (1);
}

void *thgateway(void *prms)
{
    char *buf = malloc(bufsize);
    while (1) {
        memset(buf, 0, bufsize);
        int rd = rds(buf, sizeof buf);
        if (!strcmp(buf, "thread.close"))
            break;
        printf("\n\nfrom gateway: %s\n\n", buf);
    }
    free(buf);
    return NULL; 
}

void cmd_sdown(void)
{
    int fd = open("test", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
    int32_t hdr;
    while (1) {
        rds_ex((char *) &hdr, sizeof hdr);
        swapbo32(hdr);
        if (hdr > 0) {
            rds_ex(buf, hdr);
            if (fd != -1)
                write(fd, buf, hdr);
        } else {
            printf("%d\n", hdr);
            if (hdr < 0)
                printf("error downloading\n");
            else 
                printf("download compleate\n");
            break;
        }
    }
    close(fd);
}

void cmd_sup(void)
{
    int fd = open("test", O_RDONLY);
    int32_t hdr;
    if (fd == -1) {
        hdr = -1;
        swapbo32(hdr);
        wrs_ex((char *) &hdr, sizeof hdr);
        return;
    }
    int rc;
    while ((rc = read(fd, buf, sizeof buf)) > 0) {
        hdr = rc;
        swapbo32(hdr);
        wrs_ex((char *) &hdr, sizeof hdr);
        wrs_ex(buf, rc);
    }
    hdr = 0;
    rc = hdr;
    wrs_ex((char *) &hdr, sizeof hdr);
    if (!rc) {
        rds_ex((char *) &hdr, sizeof hdr);
        if (hdr == -1)
            printf("upload failed\n");
        else if (hdr == 0)
            printf("upload succeeded\n");
    } else 
        printf("upload canceled\n");
    close(fd);
}

void fsize(void)
{
    int64_t hdr;
    rds_ex((char *) &hdr, sizeof hdr);
    swapbo64(hdr);
    if (hdr == -1)
        printf("failed getting file size\n");
    else if (hdr == -2)
        printf("file path is not regular file\n");
    else
        printf("%lld\n", hdr);
}

void fsizels(void)
{
    int64_t hdr;
    while (1) {
        rds_ex((char *) &hdr, sizeof hdr);
        swapbo64(hdr);
        if (hdr == -1)
            printf("failed getting file size\n");
        else if (hdr == -2)
            printf("file path is not regular file\n");
        else if (hdr == -3) {
            printf("Getting file sizes list finished\n");
            break;
        } else
            printf("%lld\n", hdr);
    }
}

void cmd_ftype(void)
{
    char x;
    rds_ex(&x, sizeof x);
    printf("%d\n", x);
}

void cmd_ftypels(void)
{
    char x;
    do {
        rds_ex(&x, sizeof x);
        printf("%d\n", x);
    } while (x != -2);
}

void cmd_fstatls(void)
{
    struct fstathdr hdr;
    do {
        rds_ex((char *) &hdr, sizeof hdr);
        printf("code %d\n", hdr.code);
        printf("type %d\n", hdr.type);
        swapbo64(hdr.size);
        swapbo64(hdr.atime);
        swapbo64(hdr.mtime);
        printf("size %llu\n", hdr.size);
        printf("access time %llu\n", hdr.atime);
        printf("modified time %llu\n\n", hdr.mtime);
    } while (hdr.code != -2);
}

void cmd_intread(void)
{
    int fd = open("test", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
    int32_t hdr;
    rds_ex((char *) &hdr, sizeof hdr);
    swapbo32(hdr);
    char sha256str[100]="";
    void *buf = malloc(hdr);
    if (hdr >= 0) {
        rds_ex(sha256str, 66);
        rds_ex(buf, hdr);
        if (fd != -1)
            write(fd, buf, hdr);
        printf("download compleate\n");
        printf("%s\n", sha256str);
        free(buf);
        close(fd);
    } else if (hdr == -1) {
        free(buf);
        close(fd);
        printf("error downloading\n");
    }
}

void cmd_intwrite(void)
{
    char sha256str[100]="";
    fgets(sha256str, sizeof sha256str, stdin);
    *strchr(sha256str  ,'\n') = '\0';
    wrs_ex(sha256str, strlen(sha256str));
    int fd = open("test", O_RDONLY);
    int32_t hdr;
    int rc;
    struct stat st = {0};
    stat("test", &st); 
    void *bytes = malloc(st.st_size);
    rc = read(fd, bytes, st.st_size);
    hdr = rc;
    swapbo32(hdr);
    wrs_ex((char *) &hdr, sizeof hdr);
    wrs_ex(bytes, rc);
    free(bytes);
    rds_ex((char *) &hdr, sizeof hdr);
    if (hdr == -1)
        printf("upload failed\n");
    else if (hdr == 0)
        printf("upload succeeded\n");
    close(fd);
}
