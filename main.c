#include <stdio.h>
#include <stdlib.h>
#include <mach-o/loader.h>
#include <stdbool.h>
/* main.c */
FILE *fptr=NULL;
#define BIT_MASK_6 0x00ffffff
void *load_bytes( int64_t offset, int size) {
   void *buf = calloc(1, size);
   fseek(fptr, offset, SEEK_SET);
   fread(buf, size, 1, fptr);
   return buf;
}
parse_header(bool main_core_header);

int main(int argc, char *argv[]) {
   if(argc!=2) {
      printf("Error: no core file in the argument!\n");
      return 1;
   }
   fptr=fopen(argv[1],"rb");
   if(!fptr){
      printf("Error: core file was not opened properly.\n");
   }

   parse_header(true);

   printf("%s\n",argv[1]);
   return 0;
}

int parse_header(bool main_core_header){
   int magic = load_bytes( 0, 8);
   printf("%llx",magic);
   switch(magic){
      case MH_MAGIC:
         printf("CoreFile is a 32bit little endian file.\n");
         break;
      case MH_CIGAM:
         printf("CoreFile is a 32bit big endian file.\n");
         break;
      case MH_MAGIC_64:
         printf("CoreFile is a 64bit little endian file.\n");
         break;
      case MH_CIGAM_64:
         printf("CoreFile is a 64bit big endian file.\n");
         break;
      default:
         printf("Error: CoreFile is missing Mach-O header.\n");
         return 1;
   }


   //now dump the mach header values:
   struct mach_header_64 *header = load_bytes(0,sizeof(struct mach_header_64));
   if(header)
      printf("mach header: magic %x cputype %x cpusubstype %x filetype %x ncmds %d sizeofcmds %d flags %x reserved %d\n",
             header->magic,header->cputype,header->cpusubtype,header->filetype,header->ncmds, header->sizeofcmds,header->flags,header->reserved);

   switch(header->cputype & BIT_MASK_6){
      case CPU_TYPE_X86:
         printf("Cputype is PowerPC\n");
         break;
      case CPU_TYPE_POWERPC:
         printf("Cputype is PowerPC\n");
         break;
      default:
         printf("Warning: Core is on a different architecture than PowerPC and X86-64.\n");
         break;
   }

   if( header->filetype!= MH_CORE && main_core_header){
      printf("Error: header indicates file is not a Mach-O corefile. Please check to make sure you are using the right file.\n");
      free(header);
      return 1;
   }
}