#ifndef GRAPTOR_ADDRXL_H
#define GRAPTOR_ADDRXL_H

// Based on
// http://fivelinesofcode.blogspot.com/2014/03/how-to-translate-virtual-to-physical.html

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>

#define PAGEMAP_ENTRY 8

#define GET_BIT(X,Y) (X & ((uint64_t)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF

uint64_t read_pagemap( uintptr_t virt_addr ) {
    static const int __endian_bit = 1;
#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )

    const char * path_buf = "/proc/self/pagemap";

    int i, c, pid, status;
    uint64_t read_val, file_offset;
    FILE * f;
    char *end;

    // printf("Big endian? %d\n", is_bigendian());
   f = fopen(path_buf, "rb");
   if(!f){
      printf("Error! Cannot open %s\n", path_buf);
      return -1;
   }
   
   //Shifting by virt-addr-offset number of bytes
   //and multiplying by the size of an address (the size of an entry in pagemap file)
   file_offset = virt_addr / getpagesize() * PAGEMAP_ENTRY;
   // printf("Vaddr: 0x%lx, Page_size: %d, Entry_size: %d\n", virt_addr, getpagesize(), PAGEMAP_ENTRY);
   // printf("Reading %s at 0x%llx\n", path_buf, (unsigned long long) file_offset);
   status = fseek(f, file_offset, SEEK_SET);
   if(status){
      perror("Failed to do fseek!");
      return -1;
   }
   errno = 0;
   read_val = 0;
   unsigned char c_buf[PAGEMAP_ENTRY];
   for(i=0; i < PAGEMAP_ENTRY; i++){
      c = getc(f);
      if(c==EOF){
	  // printf("\nReached end of the file\n");
         return 0;
      }
      if(is_bigendian())
           c_buf[i] = c;
      else
           c_buf[PAGEMAP_ENTRY - i - 1] = c;
      // printf("[%d]0x%x ", i, c);
   }
   for(i=0; i < PAGEMAP_ENTRY; i++){
      //printf("%d ",c_buf[i]);
      read_val = (read_val << 8) + c_buf[i];
   }
   // printf("\n");
   // printf("Result: 0x%llx\n", (unsigned long long) read_val);
/*
   if(GET_BIT(read_val, 63))
      printf("PFN: 0x%llx\n",(unsigned long long) GET_PFN(read_val));
   else
      printf("Page not present\n");
   if(GET_BIT(read_val, 62))
      printf("Page swapped\n");
*/
   fclose(f);

   // return GET_BIT( read_val, 63 ) && !GET_BIT( read_val, 62 ) ? read_val : -1;
   return GET_PFN(read_val);
}

#endif // GRAPTOR_ADDRXL_H
