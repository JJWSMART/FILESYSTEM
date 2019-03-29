#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int year(const char * p);
int month(const char * p);
int day(const char * p);
int hour(const char *p);
int minute(const char * p);
void file_attribute(const char * p, char * ft);
void file_name(const char * p, char * fn);
void file_extension(const char * p, char * fe);
int file_size(const char * p);
int num_files_disk(const char *p, int n, char * fn, int space);

#define handle_error(msg) \
do { perror(msg); exit(EXIT_FAILURE); } while (0)
#define SECTOR_SIZE 512
int num_of_sub;

int year(const char * p){return (((p[17] & 0b11111110)) >> 1) + 1980;}

int month(const char * p){return ((p[16] & 0b11100000) >> 5) + (((p[17] & 0b00000001)) << 3);}

int day(const char * p){return (p[16] & 0b00011111);}

int hour(const char *p){return (p[15] & 0b11111000) >> 3;}

int minute(const char * p){return ((p[14] & 0b11100000) >> 5) + ((p[15] & 0b00000111) << 3);}

/*
 * @param p: a pointer point to memory
 * @param f: char pointer store file type
*/
void file_attribute(const char * p, char * ft){
    
    if((p[11]&0x10)==0x10){
        ft[0] = 'D';
    }else if((p[11]&0x08)!=0x08 && (p[11]&0x0f)!=0x0f){
        ft[0] = 'F';
    }else{
        ft[0] = 'F';
    }
}

/*
 * @param p: a pointer point to memory
 * @param fn: file name
 */
void file_name(const char * p, char * fn){
    int i;
    for(i = 0; i<8; i++){
        if(p[i]==' '){
            fn[i] = '\0';
            break;
        }
        fn[i]=p[i];
    }
}

/*
 * @param p: a pointer point to memory
 * @param fe: file extension name
 */
void file_extension(const char * p, char * fe){

    int i;
    for(i = 0; i<3; i++){
        if(p[i+8]==' '){
            fe[i] = '\0';
            break;
        }
        fe[i] = p[i+8];
    }
}

/*
 * @param p: a pointer point to memory
 */
int file_size(const char * p){
    const char * t;
    t = &p[28];
    return *((unsigned int *)t);
}

/*
 * @param p: a pointer point to memory
 * @param n: first logical sector
 * @param fn: file name
 * @param space: white spaces
*/
int num_files_disk(const char *p, int n, char * fn, int space){//const cannot change the stuff in address
    
    const char * rd = p;
    const char * f;
    
    if(n==0){
        printf("root directory\n");
        printf("==================\n");
        p += SECTOR_SIZE * 19;
    }else{
        p += SECTOR_SIZE * (33 + n - 2);
        printf("%*c(%s)==================\n",space,' ',fn);
    }
    
    while (p[0] != 0x00) {
        
        if ((p[11]&0x08) == 0x08 || (p[11]&0x0f) == 0x0f || p[0]=='.'){
            p += 32;
            continue;
        }
        
        char * ft = (char *)malloc(sizeof(char)* 1);
        file_attribute(p, ft);
        printf("%*c%s ",space,' ',ft);
        
        int fz = file_size(p);
        printf("%10d ",fz);
        
        char * fn = (char *)malloc(sizeof(char)* 8);
        file_name(p,fn);
        
        char * fe = (char *)malloc(sizeof(char)*3);
        file_extension(p,fe);

        //cat file
        if(*ft=='F'){
            strcat(fn, ".");
            strcat(fn, fe);
        }
        printf("%*c%20s   ",space,' ',fn);
        printf("%02d-%02d-%02d %02d:%02d\n",day(p),month(p),year(p),hour(p),minute(p));
        
        if (p[11] == 0x10){
            f = &p[26];
            n = *((uint16_t *)f);
           
            int a = num_files_disk(rd,n,fn,space+5);
            printf("%*c(%s)==================\n",a,' ',fn);
            
        }
        p += 32;
    }
    return space;
}

int main(int argc, char *argv[])
{
    int fd;
    const char *p;
    struct stat sb;
    
    fd = open(argv[1], O_RDONLY);
    fstat(fd, &sb);
    p = mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (p == MAP_FAILED) handle_error("mmap");
    
    num_files_disk(p, 0, NULL,0);
    
    return 0;
}
