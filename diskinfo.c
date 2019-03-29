#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)
#define SECTOR_SIZE 512

void OS_name(const char *p, char *n);
void label_of_disk(const char *p, char *l);
int  total_size_disk(const char *p);
int  free_size_disk(int disksize, const char *p);
int  num_files_disk(const char *p, int n, char *l);
int  num_fat_copies(const char *p);
int  sectors_per_fat(const char *p);

const char * rd;
int sum_sub_cnt;

/*
 * @param p: a pointer point to memory
 * @param OS_name: receive file system's name
*/
void OS_name(const char *p, char *n){
    
    int i;
    for(i = 0; i<8; i++){
        n[i] = p[i+3];
    }
}

/*
 * @param p: a pointer point to memory
 * @param OS_name: receive file system's name
*/
void label_of_disk(const char *p, char *l){
    
    int i;
    if(p[43]==' '){
        if(l[0]==' '){
            strcpy(l, "NO NAME ");
        }
    }else{
        for(i = 0; i<11; i++){
            l[i] = p[i+43];
        }
    }
}

/*
 * @param p: a pointer point to memory
*/
int total_size_disk(const char *p){
    
    const char * b;
    const char * t;
    int bytes_sector;
    int total_sector;

    b = &p[11];
    bytes_sector = *((uint16_t *)b);//num bytes
    
    t = &p[19];
    total_sector = *((uint16_t *)t);//num sectors
    
    return bytes_sector * total_sector;
}

/*
 * @param p: a pointer point to memory
*/
int num_fat_copies(const char *p){
    
    int num = p[16];
    return num;
}

/*
 * @param p: a pointer point to memory
*/
int sectors_per_fat(const char *p){
    const char * f;
    f = &p[22];
    int num = *((uint16_t *)f);
    return num;
}


/*
 * @param diskSize: size of disk
 * @param p: a pointer point to memory
*/
int free_size_disk(int diskSize, const char *p){
    
    int freeSectors = 0;
    
    int i;
    for (i = 2; i < (diskSize / SECTOR_SIZE); i++){
        
        int value;
        int fb;
        int sb;
        
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
 * @param diskSize: size of disk
 * @param p: a pointer point to memory
*/
int num_files_disk(const char *p, int n, char * l){//const cannot change the stuff in this address
    
    int cnt = 0;
    int sub_cnt = 0;
    const char * rd = p;
    const char * f;
    
    if(n==0){p += SECTOR_SIZE * 19;}
    else{p += SECTOR_SIZE * (33 + n - 2);}
    
    while (p[0] != 0x00) {
        
        if ((p[11]&0x08) == 0x08 || (p[11]&0x0f) == 0x0f || p[0]=='.'){
            if ((p[11]&0x08) == 0x08){
                int i;
                for(i = 0; i<11;i++){l[i]=p[i];}
            }
            p += 32;
            continue;
        }
        //printf("file name : %s\n",p);
        if (p[11] == 0x10){
            f = &p[26];
            n = *((uint16_t *)f);
            sub_cnt = num_files_disk(rd,n,l);
            sum_sub_cnt += sub_cnt;
            cnt--;
        }
        cnt++;
        p += 32;
    }
    cnt = cnt + sum_sub_cnt;
    return cnt;
}

int main(int argc, char *argv[])
{
   int fd;
   const char *p;
   struct stat sb;
   //open file
   fd = open(argv[1], O_RDONLY);
   fstat(fd, &sb);
   
   //map pointer
   p = mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
   if (p == MAP_FAILED) handle_error("mmap");
   
   //OS_name
   char * n = (char*) malloc(sizeof(char)*8);
   OS_name(p,n);

   //disk total size
   int total_size;
   total_size = total_size_disk(p);
   
   //num of free sector
   int s;
   s = free_size_disk(total_size,p)*SECTOR_SIZE;
    
   //file
   int f;
   char * l = (char*) malloc(sizeof(char)*11);
   f = num_files_disk(p,0,l) ;
   
   //disk label
   label_of_disk(p,l);

   //num of fat copy
   int fat_copy;
   fat_copy = num_fat_copies(p);
   
   //sectors per fat
   int sector;
   sector = sectors_per_fat(p);
    
   //output
   printf("OS Name: %s\n", n);
   printf("Label of the disk: %s\n", l);
   printf("Total size of the disk: %d bytes\n", total_size);
   printf("Free size of the disk: %d bytes\n\n", s);
   printf("==============\n");
   printf("The number of files in the root directory (not including subdirectories): %d\n\n", f);
   printf("=============\n");
   printf("Number of FAT copies: %d\n", fat_copy);
   printf("Sectors per FAT: %d\n", sector);
   return 0;
}

