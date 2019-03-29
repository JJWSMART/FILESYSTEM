#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define SECTOR_SIZE 512
#define MAX_SUB_DIRC 20
#define MAX_DIR_NAME 20

char * capticalize (char * argv);
int getNextFreeFatIndex(char * p);
int total_disk_size(const char * p);
int getFatEntry(int n, const char * p);
int root_dir(const char * p, char * fn, int n);
int free_size_disk(int diskSize, const char * p);
void setFatEntry(int n, int value, char * p);
void copyFile(char* p, char* p2, char * fileName, int fileSize, int n);
void updateRootDirectory(char* fileName, int fileSize, int flc, char* p, int n);

char array[MAX_SUB_DIRC][MAX_DIR_NAME];
char * f[MAX_SUB_DIRC];
int dir_cnt;
int count;

#define handle_error(msg) \
do { perror(msg); exit(EXIT_FAILURE); } while (0)

void updateRootDirectory(char* fileName, int fileSize, int flc, char* p, int n) {
    
    //find free root dir address
    if(n==0){ p += SECTOR_SIZE * 19;}
    else{p += SECTOR_SIZE * (33 + n - 2);}
    
    while (p[0] != 0x00) {p += 32;}
    
    //set f_name & ext_sion
    int i;
    int done = -1;
    for (i = 0; i < 8; i++) {
        char character = toupper(fileName[i]);
        if (character == '.') {
            done = i;
        }
        p[i] = (done == -1) ? character : ' ';
    }
    
    for (i = 0; i < 3; i++) {
        p[i+8] = toupper(fileName[i+done+1]);
    }
    
    // set attributes
    p[11] = 0x00;
    
    // set create date/time
    time_t t = time(NULL);
    
    // set time
    struct tm * cur_t = localtime(&t);
    int year   = cur_t->tm_year + 1900;
    int month  = cur_t->tm_mon + 1;
    int day    = cur_t->tm_mday;
    int hour   = cur_t->tm_hour;
    int minute = cur_t->tm_min;
    
    p[14] = 0;
    p[15] = 0;
    p[16] = 0;
    p[17] = 0;
    p[17] |= (year - 1980) << 1;
    p[17] |= (month - ((p[16] & 0b11100000) >> 5)) >> 3;
    p[16] |= (month - (((p[17] & 0b00000001)) << 3)) << 5;
    p[16] |= (day & 0b00011111);
    p[15] |= (hour << 3) & 0b11111000;
    p[15] |= (minute - ((p[14] & 0b11100000) >> 5)) >> 3;
    p[14] |= (minute - ((p[15] & 0b00000111) << 3)) << 5;
    
    // set fls to first free FAT
    p[26] = (flc -(p[27] << 8)) & 0xFF;

    p[27] = (flc - p[26]) >> 8;
    
    // set f_size
    p[28] = (fileSize & 0x000000FF);
    p[29] = (fileSize & 0x0000FF00) >> 8;
    p[30] = (fileSize & 0x00FF0000) >> 16;
    p[31] = (fileSize & 0xFF000000) >> 24;
}


void file_attribute(const char * p, char * ft){
    
    if((p[11]&0x10)==0x10){
        ft[0] = 'D';
    }else if((p[11]&0x08)!=0x08 && (p[11]&0x0f)!=0x0f){
        ft[0] = 'F';
    }
}

/*
 * @param p: a pointer point to memory
 * @param n: logical sector
 */
int getFatEntry(int n, const char * p) {

    unsigned int flc;
    unsigned int fb;
    unsigned int sb;
    unsigned int F = 0x0F;
    unsigned int FF = 0xFF;
    
    if ((n % 2) == 0) {
        fb = (unsigned int)p[SECTOR_SIZE + ((3*n) / 2) + 1] & F;
        sb = (unsigned int)p[SECTOR_SIZE + ((3*n) / 2)] & FF;
        flc = (fb << 8) + sb;
    } else {
        fb = (unsigned int)p[SECTOR_SIZE + (int)((3*n) / 2)] & 0xF0;
        sb = (unsigned int)p[SECTOR_SIZE + (int)((3*n) / 2) + 1] & 0xFF;
        flc = (fb >> 4) + (sb << 4);
    }
    return flc;
}

/*
 * @param p: a pointer point to memory
 */
int getNextFreeFatIndex(char * p) {

    int n = 2;//sikp first_two entry
    while (getFatEntry(n, p) != 0x000) {n++;}
    return n;
}

/*
 * @param p: a pointer point to memory
 * @param n: logical sector
 * @param value: cur_log_sec value points to next log_sector
 */
void setFatEntry(int n, int value, char * p) {

    if ((n % 2) == 0) {
        p[SECTOR_SIZE + ((3*n) / 2) + 1] = (value >> 8) & 0x0F;
        p[SECTOR_SIZE + ((3*n) / 2)] = value & 0xFF;
    }else{
        p[SECTOR_SIZE + (int)((3*n) / 2)] = (value << 4) & 0xF0;
        p[SECTOR_SIZE + (int)((3*n) / 2) + 1] = (value >> 4) & 0xFF;
        
    }
}


/*
 * @param p: a pointer point to file system
 * @param p: a pointer point to cur_file
 * @param f_size: file size
 * @param n: cur_sub first_logical_sector
*/
void copyFile(char* p, char* pp, char* fileName, int f_size, int n){
    
        char * cp = p;
        int b = f_size;
        int sub_flc = getNextFreeFatIndex(p);
    
        int next=0;
        updateRootDirectory(fileName,f_size,sub_flc,cp,n);

        while (b > 0) {
            int PA = SECTOR_SIZE * (31 + sub_flc);
            int i;
            for (i = 0; i < SECTOR_SIZE; i++) {
                if (b == 0) {
                    setFatEntry(sub_flc, 0xFFF, p);
                    return;
                }
                p[i + PA] = pp[f_size - b];
                b--;
            }

            setFatEntry(sub_flc, 0xfff, p);
            next = getNextFreeFatIndex(cp);

            setFatEntry(sub_flc, next, p);
            sub_flc= next;
        }
}

/*
 * @param p: a pointer point to memory
 */
int total_disk_size(const char * p){
    
    /* total disk size */
    const char * total_sector_cnt;
    const char * bytes_per_sector;
    
    total_sector_cnt = &p[19];
    int sc = *((uint16_t*)total_sector_cnt);
    
    bytes_per_sector = &p[11];
    int bs = *((uint16_t*)bytes_per_sector);
    
    int total_disk_size = sc * bs;
    return total_disk_size;
}


/*
 * @param diskSize: size of disk
 * @param p: a pointer point to memory
 */
int free_size_disk(int disk_size, const char *p){
    
    int freeSectors = 0;
    int i;
    for (i = 2; i < (disk_size / SECTOR_SIZE); i++){
        
        int value,fb,sb;
        if ((i % 2) == 0) {
            fb  = p[SECTOR_SIZE  + ((3*i) / 2) + 1] & 0x0F;
            sb  = p[SECTOR_SIZE  + ((3*i) / 2)] & 0xFF;
            value = (fb << 8) + sb;
        } else {
            fb  = p[SECTOR_SIZE + (int)((3*i) / 2)] & 0xF0;
            sb = p[SECTOR_SIZE + (int)((3*i) / 2) + 1] & 0xFF;
            value = (fb >> 4) + (sb << 4);
        }
        if (value == 0x000) {freeSectors++;}
    }
    return freeSectors;
}

/*
 * @param fn: input string
*/
void sub_strtok(char * fn){
    char * token;
    /* first token */
    token = strtok(fn, "/");
    /* other tokens */
    while( token != NULL ) {
        strcpy(array[dir_cnt], token);
        f[dir_cnt] = array[dir_cnt];
        dir_cnt++;
        token = strtok(NULL, "/");
    }
}

/*
 * @param file_name: file_name
 */
void new_name(const char * p, char * file_name, char * file_ext){
    
    /* file name */
    int i;
    for(i = 0; i<8; i++){
        if(p[i] == ' '){
            file_name[i] = '\0';
            break;
        }
        file_name[i] = p[i];
    }
    
    /* file extension */
    int j;
    for(j = 0; j<3; j++){
        file_ext[j] = p[j+8];
    }
    
    char * ft = (char *)malloc(sizeof(char)* 1);
    file_attribute(p, ft);
    
    /* file full name */
    if(*ft == 'F'){
        strcat(file_name, ".");
        strcat(file_name, file_ext);
    }
    
}

/*
 * @param fn: sub_dir name
 * @param n: logical sector
 * @param p: a pointer point to memory
*/
int root_dir(const char * p, char * fn, int n){
    
    const char * f;
    /* root dir address */
    const char * rd = p;
    int flc;
    
    /* check file existence */
    if(n==0){p += SECTOR_SIZE * 19;}
    else{p += SECTOR_SIZE * (33 + n - 2);}
    
    while(p[0]!= 0x00){
        
        if ((p[11]&0x08) == 0x08 || (p[11]&0x0f) == 0x0f || p[0]=='.'){
            p += 32;
            continue;
        }
        
        char * file_name = (char *)malloc(sizeof(char)*9);
        char * file_ext  = (char *)malloc(sizeof(char)*3);
        
        new_name(p,file_name,file_ext);
  
        if(p[11] == 0x10 && dir_cnt > 1){
            f = &p[26];
            n = *((uint16_t *)f);
            if(strcmp(file_name,array[count]) == 0){
                count++;
                if(count < dir_cnt-1){flc = root_dir(rd,array[count],n);}
                else{return n;}
            }
        }
        p+=32;
        if(p[0]==0x00 && dir_cnt > 1){
            printf("subdirectory not found !\n");
            exit(1);
        }
  }
  return flc;
}


/*
 * @param p: input string
 * @return : capitalized string
 */
char * capticalize (char * argv){
    /* capticalize the file name */
    int len = strlen(argv);
    char * fn = (char *)malloc(sizeof(char)* len + 1);
    
    int i;
    for(i = 0; i < len+1; i++){
        fn[i] = toupper(argv[i]);
        if(i==len){fn[i] = '\0';}
    }
    return fn;
}


int main(int argc, char * argv[]){
    
    /* check input */
    if(argc < 3){
        fprintf(stderr, "usage: %s incorrect input\n", argv[0]);
        return -1;
    }
    count = 0;
    /* capitalize token */
    char * fn = capticalize(argv[2]);
    sub_strtok(fn);
    free(fn);

    int i = 0;
    while(array[dir_cnt-1][i]){
	array[dir_cnt-1][i]=tolower(array[dir_cnt-1][i]);
	i++;
    }
    //printf("%s\n", array[dir_cnt-1]);

    int fw;//file_system
    int fr;//cur_dir file
    
    char * p;
    char * pp;
    
    struct stat sw;
    struct stat sr;
    
    /* open a file disk */
    //printf("%s\n", argv[1]);
    fw = open(argv[1], O_RDWR);
    if (fw == -1) {printf("Error");}
    
    fstat(fw, &sw);
    
    p = mmap(NULL, sw.st_size, PROT_WRITE, MAP_SHARED, fw, 0);
    if (p == MAP_FAILED) handle_error("mmaptest");
  

    int n = root_dir(p,NULL,0);
    
    /* open a file from directory */
   
    fr = open(array[dir_cnt-1], O_RDWR);
    if (fr == -1) {printf("Error");}
    
    fstat(fr, &sr);
    
    pp = mmap(NULL, sr.st_size, PROT_READ, MAP_PRIVATE, fr, 0);
    if (pp == MAP_FAILED) handle_error("mmap");
    
    int total_size = total_disk_size(p);
    int free_size  = free_size_disk(total_size, p) * SECTOR_SIZE;
    
    if(free_size < sr.st_size){
        printf("No enough free space in the disk image\n");
        exit(1);
    }
    
    copyFile(p, pp, array[dir_cnt-1], sr.st_size, n);
    
    munmap(p,sw.st_size);
    munmap(pp,sr.st_size);
    close(fw);
    close(fr);
    return 0;
}



