#include <stdio.h>
#include <stdlib.h>
#include <mach-o/loader.h>
/* main.c */
FILE *fptr=NULL;
uint32_t read_section( int offset,int long long vmAddr, int size) {
   uint32_t magic;
   if (vmAddr == 0) {
      fseek(fptr, offset, SEEK_SET);
      //the first field int the HEADER is the MACH-O number identifier
      fread(&magic, size, 1, fptr);
   }

   return magic;
}
int main(int argc, char *argv[]) {
   if(argc!=2) {
      printf("Error: no core file in the argument!\n");
      return 1;
   }
   fptr=fopen(argv[1],"rb");
   if(!fptr){
      printf("Error: core file was not opened properly.\n);
   }
   int magic = read_section( 0,0, 8);

   if(magic!= MH_MAGIC_64)
      printf("Error: CoreFile is missing Mach-O header.\n");

   printf("%s\n",argv[1]);
   return 0;
}