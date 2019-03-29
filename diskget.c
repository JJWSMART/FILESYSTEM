#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define handle_error(msg) \
do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define SECTOR_SIZE 512


int file_size(const char * p);

int f_size;

int getFatEntry(int n, const char* p) {
    
    int flc,fb,sb;
    
    if ((n % 2) == 0) {
        fb = p[SECTOR_SIZE + ((3*n) / 2) + 1] & 0x0F;
        sb = p[SECTOR_SIZE + ((3*n) / 2)] & 0xFF;
        flc = (fb << 8) + sb;
    } else {
        fb = p[SECTOR_SIZE + (int)((3*n) / 2)] & 0xF0;
        sb = p[SECTOR_SIZE + (int)((3*n) / 2) + 1] & 0xFF;
        flc = (fb >> 4) + (sb << 4);
    }
    return flc;
}

void copyFile(const char* p,  char * pp, int flc, int b ){
    
    int n;
    int pa = SECTOR_SIZE * (31 + flc);
    
    do {
        if(b == f_size){n = flc;}
        else{n = getFatEntry(n, p);}
        
        pa = SECTOR_SIZE * (31 + n);
        
        int i;
        for (i = 0; i < SECTOR_SIZE; i++) {
            if (b == 0) {break;}
            pp[f_size - b] = p[i + pa];
            b--;
        }
    } while (getFatEntry(n, p) != 0xFFF);
}

int file_size(const char * p){
    /* 82 c2 00 00 -> 00 00 c2 82 */
    const char * t;
    t = &p[28];
    return *((unsigned int *)t);
}



int root_dir(const char * p, char * fn, int n){
    
    //printf("fn: %s\n",fn);
    
    const char * f;//flc
    
    /* save address */
    const char * rd = p;
    
    int flc;
    
    /* check file existence */
    if(n==0){
        p += SECTOR_SIZE * 19;
    }else{
        p += SECTOR_SIZE * (33 + n - 2);
    }
    
    while(p[0]!= 0x00){
        
        if ((p[11]&0x08) == 0x08 || (p[11]&0x0f) == 0x0f || p[0]=='.'){
            p += 32;
            continue;
        }
        
        char * file_name = (char *)malloc(sizeof(char)*8);
        char * file_ext  = (char *)malloc(sizeof(char)*3);
        
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
        for(i = 0; i<3; i++){
            //printf("p[%d]: %c\n",i,p[i+8]);
            if(p[i+8] == ' '){
                file_ext[i] = '\0';
                break;
            }
            file_ext[i] = p[i+8];
        }
	file_ext[3] = '\0';

        /* file full name */
        strcat(file_name, ".");
        strcat(file_name, file_ext);
	
        if(strcmp(file_name,fn) == 0){
            f_size = file_size(p);
            f = &p[26];
            flc = *((uint16_t *)f);
            //printf("%d\n",flc);
            //printf("match\n");
            return flc;
        }
        
        if(p[11] == 0x10){
            f = &p[26];
            n = *((uint16_t *)f);
            flc = root_dir(rd,fn,n);
        }

        free(file_name);
        free(file_ext);
        p+=32;
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
        if(i==len){fn[i] = '\0';} //prevent
    }
    return fn;
}

int main(int argc, char * argv[]){

    if(argc!=3){
        printf("Usage :  wrong input \n");
        exit(1);
    }

    int fd;
    const char * p;
    struct stat sb;
    
    /* open a file */
    fd = open(argv[1], O_RDWR);
    if(fd == -1){
        printf("open() failed with error\n");
        exit(1);
    }
    
    fstat(fd, &sb);
    p = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (p == MAP_FAILED) handle_error("mmap");
    
   
    /* capitalize token */
    char * fn = capticalize(argv[2]);
    
    //first logic sector
    int flc = root_dir(p,fn,0);
  
    if (f_size > 0) {
        // Create new file to be written
        int fp = open(argv[2], O_RDWR | O_CREAT, 0666);
        if(fp == -1){
            printf("open() failed with error\n");
            exit(1);
        }
        
        int result = lseek(fp, f_size-1, SEEK_SET);
        if(result == -1){
	    exit(1);	
	}
        result = write(fp, "", 1);
        
        // Map memory exact memory space for file
        char* pp = mmap(NULL, f_size, PROT_WRITE, MAP_SHARED, fp, 0);
        if (pp == MAP_FAILED) handle_error("mmap");
        
        copyFile(p, pp, flc, f_size);
        
    }else{
        printf("f_size: %d\n",f_size);
        printf("File not found.\n");
    }
    return 0;
}






