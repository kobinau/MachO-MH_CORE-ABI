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
   printf("%x\n",buf);
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
   //dump the mach header values:
   struct mach_header_64 *header = load_bytes(0,sizeof(struct mach_header_64));
   if(!header){
      printf("Error: header wasn't loaded");
      return 1;
   }
      printf("mach header: magic %x cputype %x cpusubstype %x filetype %x ncmds %d sizeofcmds %d flags %x reserved %d\n",
             header->magic,header->cputype,header->cpusubtype,header->filetype,header->ncmds, header->sizeofcmds,header->flags,header->reserved);
   switch(header->magic){
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
         break;
   }
   printf("%x\n",header->magic);
   switch(header->cputype & BIT_MASK_6){
      case CPU_TYPE_X86:
         printf("Cputype is x86\n");
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
void Segment64Handler(int actual_offset, ADDR vmAddr, int offset, struct load_command *cmd, uint32_t level);
void dump_segment_commands(int offset, uint32_t ncmds, uint64_t vmAddr,uint32_t level) {
   int actual_offset = offset;
   //run through all the load commands
   struct dylib_command* iddylib=NULL;
   struct dylinker_command* iddylinker=NULL;
   struct symtab_command *symbolTable=NULL;
   uint32_t offsetIdDylib=0;
   uint32_t offsetIdDylinker=0;
   for (int i = 0; i < ncmds; i++) {
      struct load_command *cmd = load_bytes(actual_offset, sizeof(struct load_command), vmAddr);
      //KCA_PRINT(STDERR,"cmd %x\n",cmd->cmd);
      //swap load command here
      switch (cmd->cmd) {
         case LC_SEGMENT_64:
            Segment64Handler(actual_offset, vmAddr, offset, cmd, level );
            break;
         case LC_THREAD:
         case LC_UNIXTHREAD:
            threadCMDHandler( actual_offset, vmAddr, kcaThread, cmd );//KCATHREAD MUST BE A PASS BY REFERENCE
            break;
         case LC_ID_DYLIB:
            iddylib=load_bytes(actual_offset, sizeof(struct dylib_command));
            offsetIdDylib=actual_offset;
            break;
         case LC_ID_DYLINKER:
            iddylinker=load_bytes(actual_offset, sizeof(struct dylinker_command));
            offsetIdDylinker=actual_offset;
            break;
         case LC_SYMTAB:
            symbolTable=load_bytes(actual_offset, sizeof(struct symtab_command));
            break;
      }
      actual_offset += cmd->cmdsize;
      free(cmd);
   }

   if(symbolTable)
      resolveLibrary(iddylib, offsetIdDylib, iddylinker, offsetIdDylinker, symbolTable, vmAddr, level);
}

void Segment64Handler(int actual_offset, ADDR vmAddr, int offset, struct load_command *cmd, uint32_t level){
   struct segment_command_64 *segment = load_bytes(actual_offset, sizeof(struct segment_command_64), vmAddr);

   //the second iteration of the segments runs through the __TEXT __DATA and __LINKEDIT sections
   //and then subsequently we can loop through their sections as well
   //we mostly don't need that specific section info but more in depth traversal code in below
#if defined(DEBUG)
   /*For TEXT DATA and LINKEDIT traversal*/

   if(level!=-1) {
      printByByte(-1, segment->segname, 'c');
      if(segArr[level].segmentAddress!=segArr[level].fileOffset)
      KCA_PRINT(STDOUT, "seg %d  vmaddr %llx  %llx \n", level, actual_offset+vmAddr, segment->vmaddr);
      else       KCA_PRINT(STDOUT, "seg %d  vmaddr %llx  %llx \n", level, segment->fileoff, segArr[level].fileOffset);
   }
   for(int i=0;i<segment->nsects;i++) {
      struct section_64 *section = load_bytes(actual_offset + sizeof(struct segment_command_64) + i * sizeof(struct section_64), sizeof(struct segment_command_64), vmAddr);
      KCA_PRINT(STDOUT,"\t");
      printByByte(-1,section->sectname,'c');
         KCA_PRINT(STDOUT," \toffset %llx size %llx \n", section->offset,section->size);
         KCA_PRINT(STDOUT, "sects %d full offset %llx\n", segment->nsects,
                   actual_offset + vmAddr + sizeof(struct segment_command_64) + i * sizeof(struct section_64));

   }
#endif

   //size of the dylib is the sum of the __TEXT __DATA and __LINKEDIT sections
   if (level!=-1) {
      //the dylib shared cache only has the __TEXT portion of the dylib, other libraries size includes both __DATA and __LINKEDIT as well
      if ((memcmp(segment->segname, "__LINKEDIT", 10)&& memcmp(segment->segname, "__DATA", 6))|| segArr[level].segmentAddress != sharedCacheBaseAddress){
         segArr[level].Size += segment->vmsize;
      }
   }
   //swap here for big endian

   if (level == -1) {
      addMemoryRegion(MR_ALL, (ADDR) segment->vmaddr, segment->vmsize, file, segment->fileoff, 0);
      uint64_t testAddrForLibrary = segment->vmaddr;
      //Every dylib ends in a 0x1000 adress in memory, to find all, must loop through and check for MH_MAGIC_64
      char * dylib_loc = load_bytes(0,16,
                                    testAddrForLibrary);
      //check if we have entered the shared cache for dylibs
      if(dylib_loc&&(!memcmp(dylib_loc, "dyld_v1 x86_64h", 16))){
         sharedCacheBaseAddress = segment->fileoff;
         relative_to_dyld = 0x00000fffffff&testAddrForLibrary;
      }
      free(dylib_loc);
      //if we have entered the cache, we can check if libraries have entered on a 0x1000 address boundary
      if(sharedCacheBaseAddress!=0) {
         while (testAddrForLibrary < (segment->vmaddr + segment->vmsize)) {
            //
            uint32_t magic = read_section(0, testAddrForLibrary);
            //          KCA_PRINT(STDOUT,"%llx %llx\n",segment->fileoff+(testAddrForLibrary-segment->vmaddr), segment->fileoff+(testAddrForLibrary-segment->vmaddr+actual_offset));

            //check MAGIC number to see if segment is linked library, and load files
            if (magic == MH_MAGIC_64) {
               struct segment_command_64 *miniseg = load_bytes(segment->fileoff+(testAddrForLibrary-segment->vmaddr), sizeof(struct segment_command_64),
                                                               0);
               //ignore empty pages
               //              KCA_PRINT(STDOUT,"%llx %llx\n",segment->fileoff+(testAddrForLibrary-segment->vmaddr), segment->fileoff+(testAddrForLibrary-segment->vmaddr+actual_offset));

               if (miniseg&&(memcmp(miniseg->segname,"__PAGEZERO",10))) {
                  //load all addresses into array, for ordered access.
                  size_ofArr++;
                  segArr = (struct SegmentMemoryInfo *) realloc(segArr, size_ofArr * sizeof(struct SegmentMemoryInfo));
                  segArr[size_ofArr - 1].startAddress = testAddrForLibrary;
                  //KCA_PRINT(STDOUT,"full %llx masked %llx size %llx \n", testAddrForLibrary,testAddrForLibrary &0xfffff000,segment->vmsize);;
                  segArr[size_ofArr - 1].PathIndex = 0;
                  segArr[size_ofArr - 1].fileOffset = segment->fileoff+(testAddrForLibrary-segment->vmaddr);
                  if(sharedCacheBaseAddress==0)
                     segArr[size_ofArr - 1].segmentAddress = segment->fileoff;
                  else segArr[size_ofArr - 1].segmentAddress = sharedCacheBaseAddress;

               }
               free(miniseg);
            }
            testAddrForLibrary += 0x1000;
         }
      }
      else{//this block goes under the assumption that all libraries not in the shared cache exist at the head of the
         // segments--should this prove to be untrue, simply remove the full else
         // statement and its  body, as well as the if statement above (although keep the body of the if statement)
         uint32_t magic = read_section(0, testAddrForLibrary);
         //check MAGIC number to see if segment is linked library, and load files
         if (magic == MH_MAGIC_64) {
            size_ofArr++;
            segArr = (struct SegmentMemoryInfo *) realloc(segArr, size_ofArr * sizeof(struct SegmentMemoryInfo));
            segArr[size_ofArr - 1].startAddress = testAddrForLibrary;
            segArr[size_ofArr - 1].PathIndex = 0;
            segArr[size_ofArr - 1].fileOffset = segment->fileoff;
            if(sharedCacheBaseAddress==0)
               segArr[size_ofArr - 1].segmentAddress = segment->fileoff;
            else segArr[size_ofArr - 1].segmentAddress = sharedCacheBaseAddress;
            //  KCA_PRINT(STDOUT," vmsize %llx\n",segment->vmsize);
            //note to keep in mind the 0xfffff000 mask
            //KCA_PRINT(STDOUT,"full %llx masked %llx size %llx \n", testAddrForLibrary,testAddrForLibrary &0xfffff000,segment->vmsize);
         }
      }
   }

   //Segments have 0 or more sections, so loop through to analyse them as well.
   //dump_sections(offset+ sizeof(struct segment_command_64), segment, vmAddr, level);
   //KCA_PRINT(STDERR,"cmdend %x\n",cmd->cmd);
   free(segment);

}

kca_thread *threadCMDHandler(int actual_offset, ADDR vmAddr,kca_thread *kcaThread,struct load_command *cmd ){
   //additional calloc to be uncalloced at the end
   context_t *total_thread_info = calloc(1, sizeof(context_t));

   /*The constants for the flavors, counts and state data structure
       * definitions are expected to be in the header file <machine/thread_status.h>.*/

   int remainingThreadSize = cmd->cmdsize - sizeof(struct thread_command);
   x86_state_hdr_t *x86StateHdr;
   int thread_offset = actual_offset + sizeof(struct thread_command);
   int register_size = 0;
   //If frame pointer or Exception states exist
   total_thread_info->hasFPR = 0;
   total_thread_info->hasES = 0;
   //Thread commands cannot be loaded at once. Instead, depending on the type of thread,
   //we can learn the structure of the trailing bytes. thread_status.h contains all the information
   //there are 8 bytes left outside this documentation--unclear at this point
   while (remainingThreadSize != 0) {
      //This is unclear from the documentation. the command loads two command flavors;
      // one for the generic flavor, and one to specify itself as a 32 or 64 bit thread
      //lldb doesn't do this, and produces the wrong FPR and EPR outputs as a result
      int max_run = 2;
      if (cmd->cmd == LC_UNIXTHREAD)
         max_run = 1;
      for (int j = 0; j < max_run; j++) {
         x86StateHdr = load_bytes(thread_offset + j * sizeof(x86_state_hdr_t),
                                  sizeof(x86_state_hdr_t), vmAddr);
         if (j == 0 && max_run == 2) //so we don't leak memory
            free(x86StateHdr);
      }


      if (x86StateHdr->flavor == x86_THREAD_STATE64) {
         x86_thread_state64_t *pthread_state = load_bytes(
                 thread_offset + max_run * sizeof(x86_state_hdr_t),
                 sizeof(x86_thread_state64_t), vmAddr);
         total_thread_info->gpr = *pthread_state;
         register_size = sizeof(x86_thread_state64_t);
         free(pthread_state);
      }
      if (x86StateHdr->flavor == x86_FLOAT_STATE64) {
         total_thread_info->hasFPR = 1;
         x86_float_state64_t *pfloat_state = load_bytes(
                 thread_offset + max_run * sizeof(x86_state_hdr_t),
                 sizeof(x86_float_state64_t), vmAddr);
         total_thread_info->fpr = *pfloat_state;
         register_size = sizeof(x86_float_state64_t);
         free(pfloat_state);

      }
      if (x86StateHdr->flavor == x86_EXCEPTION_STATE64) {
         total_thread_info->hasES = 1;
         x86_exception_state64_t *pexception_state = load_bytes(
                 thread_offset + max_run * sizeof(x86_state_hdr_t),
                 sizeof(x86_exception_state64_t), vmAddr);
         total_thread_info->es = *pexception_state;
         register_size = sizeof(x86_exception_state64_t);
         free(pexception_state);
      }
      //free second instance of state header
      free(x86StateHdr);
      remainingThreadSize -= (max_run * sizeof(x86_state_hdr_t) +
                              register_size);
      thread_offset += max_run * sizeof(x86_state_hdr_t) +
                       register_size;

   }
   kcaThread = addThread(total_thread_info, (_INT) tid, STACK_UNKNOWN, STACK_UNKNOWN, NULL, false);
   //setting first thread as current thread
   if (tid==0)
      setCurrentThread(file, kcaThread);
   tid++;
   return kcaThread;
}