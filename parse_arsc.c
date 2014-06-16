#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

typedef short unsigned int unit16;
typedef unsigned int uint32;
typedef unsigned char uchar;
typedef unsigned short UTF16;  /* at least 16 bits */
typedef unsigned char   UTF8;   /* typically 8 bits */

/*
UCS-2编码    UTF-8 字节流(二进制) 
0000 - 007F  0xxxxxxx
0080 - 07FF 110xxxxx 10xxxxxx
0800 - FFFF 1110xxxx 10xxxxxx 10xxxxxx
*/

#define UTF8_ONE_START          (0xOOO1)
#define UTF8_ONE_END            (0x007F)
#define UTF8_TWO_START          (0x0080)
#define UTF8_TWO_END            (0x07FF)
#define UTF8_THREE_START        (0x0800)
#define UTF8_THREE_END          (0xFFFF)

struct Res_value
{
    // Number of bytes in this structure.
    uint16_t size;

    // Always set to 0.
    uint8_t res0;

    uint8_t dataType;

    uint32_t data;
};



/**
 * This is the beginning of information about an entry in the resource
 * table.  It holds the reference to the name of this entry, and is
 * immediately followed by one of:
 *   * A Res_value structure, if FLAG_COMPLEX is -not- set.
 *   * An array of ResTable_map structures, if FLAG_COMPLEX is set.
 *     These supply a set of name/value mappings of data.
 */
struct ResTable_entry
{
    // Number of bytes in this structure.
    uint16_t size;

    enum {
        // If set, this is a complex entry, holding a set of name/value
        // mappings.  It is followed by an array of ResTable_map structures.
        FLAG_COMPLEX = 0x0001,
        // If set, this resource has been declared public, so libraries
        // are allowed to reference it.
        FLAG_PUBLIC = 0x0002
    };
    uint16_t flags;

    // Reference into ResTable_package::keyStrings identifying this entry.
//    struct ResStringPool_ref key;
	uint32_t index;
};

struct ResChunk_header
{
    // Type identifier for this chunk.  The meaning of this value depends
    // on the containing chunk.
    uint16_t type;
	
	enum {
    RES_NULL_TYPE               = 0x0000,
    RES_STRING_POOL_TYPE        = 0x0001,
    RES_TABLE_TYPE              = 0x0002,
    RES_XML_TYPE                = 0x0003,

    // Chunk types in RES_XML_TYPE
    RES_XML_FIRST_CHUNK_TYPE    = 0x0100,
    RES_XML_START_NAMESPACE_TYPE= 0x0100,
    RES_XML_END_NAMESPACE_TYPE  = 0x0101,
    RES_XML_START_ELEMENT_TYPE  = 0x0102,
    RES_XML_END_ELEMENT_TYPE    = 0x0103,
    RES_XML_CDATA_TYPE          = 0x0104,
    RES_XML_LAST_CHUNK_TYPE     = 0x017f,
    // This contains a uint32_t array mapping strings in the string
    // pool back to resource identifiers.  It is optional.
    RES_XML_RESOURCE_MAP_TYPE   = 0x0180,

    // Chunk types in RES_TABLE_TYPE
    RES_TABLE_PACKAGE_TYPE      = 0x0200,
    RES_TABLE_TYPE_TYPE         = 0x0201,
    RES_TABLE_TYPE_SPEC_TYPE    = 0x0202
	};
	
    // Size of the chunk header (in bytes).  Adding this value to
    // the address of the chunk allows you to find its associated data
    // (if any).
    uint16_t headerSize;

    // Total size of this chunk (in bytes).  This is the chunkSize plus
    // the size of any data associated with the chunk.  Adding this value
    // to the chunk allows you to completely skip its contents (including
    // any child chunks).  If this value is the same as chunkSize, there is
    // no data associated with the chunk.
    uint32_t size;
};

/**
 * A collection of resource entries for a particular resource data
 * type. Followed by an array of uint32_t defining the resource
 * values, corresponding to the array of type strings in the
 * ResTable_package::typeStrings string block. Each of these hold an
 * index from entriesStart; a value of NO_ENTRY means that entry is
 * not defined.
 *
 * There may be multiple of these chunks for a particular resource type,
 * supply different configuration variations for the resource values of
 * that type.
 *
 * It would be nice to have an additional ordered index of entries, so
 * we can do a binary search if trying to find a resource by string name.
 */
struct ResTable_type
{
    struct ResChunk_header header;

    enum {
        NO_ENTRY = 0xFFFFFFFF
    };

    // The type identifier this chunk is holding.  Type IDs start
    // at 1 (corresponding to the value of the type bits in a
    // resource identifier).  0 is invalid.
    uint8_t id;

    // Must be 0.
    uint8_t res0;
    // Must be 0.
    uint16_t res1;

    // Number of uint32_t entry indices that follow.
    uint32_t entryCount;

    // Offset from header where ResTable_entry data starts.
    uint32_t entriesStart;

    // Configuration this collection of entries is designed for.
//    ResTable_config config;
    uint32_t config[9];
};

/**
 * A specification of the resources defined by a particular type.
 *
 * There should be one of these chunks for each resource type.
 *
 * This structure is followed by an array of integers providing the set of
 * configuration change flags (ResTable_config::CONFIG_*) that have multiple
 * resources for that configuration.  In addition, the high bit is set if that
 * resource has been made public.
 */
struct ResTable_typeSpec
{
    struct ResChunk_header header;

    // The type identifier this chunk is holding.  Type IDs start
    // at 1 (corresponding to the value of the type bits in a
    // resource identifier).  0 is invalid.
    uint8_t id;

    // Must be 0.
    uint8_t res0;
    // Must be 0.
    uint16_t res1;

    // Number of uint32_t entry configuration masks that follow.
    uint32_t entryCount;

    enum {
        // Additional flag indicating an entry is public.
        SPEC_PUBLIC = 0x40000000
    };
};


/**
 * A collection of resource data types within a package.  Followed by
 * one or more ResTable_type and ResTable_typeSpec structures containing the
 * entry values for each resource type.
 */
struct ResTable_package
{
    struct ResChunk_header header;

    // If this is a base package, its ID.  Package IDs start
    // at 1 (corresponding to the value of the package bits in a
    // resource identifier).  0 means this is not a base package.
    uint32_t id;

    // Actual name of this package, \0-terminated.
    unsigned char name[256];

    // Offset to a ResStringPool_header defining the resource
    // type symbol table.  If zero, this package is inheriting from
    // another base package (overriding specific values in it).
    uint32_t typeStrings;

    // Last index into typeStrings that is for public use by others.
    uint32_t lastPublicType;

    // Offset to a ResStringPool_header defining the resource
    // key symbol table.  If zero, this package is inheriting from
    // another base package (overriding specific values in it).
    uint32_t keyStrings;

    // Last index into keyStrings that is for public use by others.
    uint32_t lastPublicKey;
};

struct ResStringPool_header
{
    struct ResChunk_header header;

    // Number of strings in this pool (number of uint32_t indices that follow
    // in the data).
    uint32_t stringCount;

    // Number of style span arrays in the pool (number of uint32_t indices
    // follow the string indices).
    uint32_t styleCount;

    // Flags.
    enum {
        // If set, the string index is sorted by the string values (based
        // on strcmp16()).
        SORTED_FLAG = 1<<0,

        // String pool is encoded in UTF-8
        UTF8_FLAG = 1<<8
    };
    uint32_t flags;

    // Index from header of the string data.
    uint32_t stringsStart;

    // Index from header of the style data.
    uint32_t stylesStart;
};


/** ********************************************************************
 *  RESOURCE TABLE
 *
 *********************************************************************** */

/**
 * Header for a resource table.  Its data contains a series of
 * additional chunks:
 *   * A ResStringPool_header containing all table values.  This string pool
 *     contains all of the string values in the entire resource table (not
 *     the names of entries or type identifiers however).
 *   * One or more ResTable_package chunks.
 *
 * Specific entries within a resource table can be uniquely identified
 * with a single integer as defined by the ResTable_ref structure.
 */
struct ResTable_header
{
    struct ResChunk_header header;

    // The number of ResTable_package structures.
    uint32_t packageCount;
};



/*


struct resTablePackage{
  uint32* resTablePackage_header;
  uint32* typeStringPool_header;
  uint32* keyStringPool_header; 
};

struct strRes_InfoChunk{
  uint32* resTableSpec_header;
  uint32** resTableType_headers;
}

struct resTable_chunk_addr{
  uint32* resTable_header;
  uint32* valStringPool_header;
r
  struct resTablePackge table_packge;
  struct strRes_InfoChunk str_info_chunk; 
};
*/
/*
int decodeStr(uchar* str_head,uchar* &typeString){
  int len=*(str_head+1);
  typeString=(uchar*)malloc(sizeof(uchar)*len);
  return len;
}

int getString_TypeId(const uchar* pStrPool){
  uint32 stringsStart;
  uchar* typeString_Offset;
  uchar* typeString;
  int len;
  stringsStart=*(pStrPool + 20);
  typeString_Offset=pStrPool+stringsStart;
  do{
    free(typeString);
    decodeStr(typeString_Offset,typeString);
    
  }while(strcmp(typeString,"strings")!=0);
}
*/

struct ResTable{
  int typeIndex;
  int indexName;
  int isutf16;
  
  unsigned char* AppName1;
  unsigned char* AppName2;

  const uint8_t* dataEnd;
  uint32_t* valPool_OffsetArray;
  const struct ResStringPool_header* valPool;
};


long file_len;
uint8_t* headAddr=NULL;

void* getBuffer(char* filename){
  FILE* fp;
  unsigned char* buf;
  fp=fopen(filename,"rb");
  if(fp == NULL){
    fprintf(stderr,"can't open file %s\n",filename);
    exit(-1);
  }
  fseek(fp,0,SEEK_END);
  file_len=ftell(fp);
  fprintf(stdout,"file length is %ld\n",file_len);
  
  buf=(unsigned char*)malloc(sizeof(unsigned char)*file_len);
  fseek(fp,0,SEEK_SET);
  if(fread(buf,1,file_len,fp) != file_len){
    fprintf(stderr,"failed reading %ld bytes to buffer\n", file_len);  
    exit(-1);
  }
  printf("buffer gen over\n");
  return buf; 
}

int str2Hex(char * str){
  int i;
  int len=strlen(str);
  int ret=0;
  for(i=0;i<len;i++){
    ret*=16;
    if(str[i]>=65)
      ret+=str[i]-97+10;
    else
      ret+=str[i]-48;
  }
  return ret;
}

void UTF16ToUTF8(UTF16* pUTF16Start, UTF16* pUTF16End, UTF8* pUTF8Start, UTF8* pUTF8End)
{
        UTF16* pTempUTF16 = pUTF16Start;
        UTF8* pTempUTF8 = pUTF8Start;

        while (pTempUTF16 < pUTF16End)
        {
                if (*pTempUTF16 <= UTF8_ONE_END
                        && pTempUTF8 + 1 < pUTF8End)
                {
                        //0000 - 007F  0xxxxxxx
//                        printf("before--------> %x\n",*pTempUTF8);
                        *pTempUTF8++ = (UTF8)*pTempUTF16;
//                        printf("after--------> %x\n",*pTempUTF8);
                }
                else if(*pTempUTF16 >= UTF8_TWO_START && *pTempUTF16 <= UTF8_TWO_END
                        && pTempUTF8 + 2 < pUTF8End)
                {
                        //0080 - 07FF 110xxxxx 10xxxxxx
                        *pTempUTF8++ = (*pTempUTF16 >> 6) | 0xC0;
                        *pTempUTF8++ = (*pTempUTF16 & 0x3F) | 0x80;
                }
                else if(*pTempUTF16 >= UTF8_THREE_START && *pTempUTF16 <= UTF8_THREE_END
                        && pTempUTF8 + 3 < pUTF8End)
                {
                        //0800 - FFFF 1110xxxx 10xxxxxx 10xxxxxx
                        *pTempUTF8++ = (*pTempUTF16 >> 12) | 0xE0;
                        *pTempUTF8++ = ((*pTempUTF16 >> 6) & 0x3F) | 0x80;
                        *pTempUTF8++ = (*pTempUTF16 & 0x3F) | 0x80;
                }
                else
                {
                        break;
                }
                pTempUTF16++;
        }
        *pTempUTF8 = 0;
}
/*
int extract_zipentry_to_file(ZipArchive *zip, const char *zipfile, char *tmpfile)
{
        const ZipEntry* entry = mzFindZipEntry(zip, zipfile);
    if (entry == NULL)
    {
        mzCloseZipArchive(zip);
        return -1;
    }
//    LOGI("%.2lf :unlink tmpfile begain\n",now());
    unlink(tmpfile);
    int fd = creat(tmpfile, 0755);
    if (fd < 0)
    {
        mzCloseZipArchive(zip);
//        LOGE("Can't make %s\n", tmpfile);
        return -1;
    }
//        LOGI("%.2lf :mzExtractZipEntryToFile tmpfile begain\n",now());
    int ok = mzExtractZipEntryToFile(zip, entry, fd);
    close(fd);
//        LOGI("%.2lf :mzExtractZipEntryToFile tmpfile end\n",now());

    if (!ok)
    {
        LOGE("Can't copy %s\n", zipfile);
        return -1;
    }
        return 0;
}*/

void checkFileLen(const struct ResTable_header* header){
  long data_len=header->header.size;
  if(file_len != data_len){
    fprintf(stderr,"data length is not equal to file_len");
    exit(-1);
  }
  fprintf(stdout,"check data size %ld passed\n",data_len);
}

void dispChunkInfo(const struct ResChunk_header* chunk){
  printf("type is %x,headersize is %x,size is %x\n",chunk->type,chunk->headerSize,chunk->size);
  printf("offset to head is %x\n",(uint8_t*)chunk-headAddr);
}

/*
@ret: next str offset to current
*/
int dispStrInfo(unsigned char* str,unsigned char* strval){
  int strlen; 
  strlen=*(str+1);
  printf("strlen is %d ",strlen);
  memcpy(strval,str+2,strlen);
//  strncpy(strval,str+2,strlen);
  strval[strlen]='\0';
  printf("strval is ---> %s\n",strval);
  return strlen+2+1;
}

int dispStrInfo16(unsigned char* str,unsigned char* strval){
  int strlen;
  unsigned short* strval16=(unsigned short*)malloc(sizeof(unsigned short)*256);
//  printf("str index is %x",str-);
  strlen=*str;
  printf("strlen is %d \n",strlen);
  memcpy(strval16,str+2,strlen*2);
//  printf("head for in strval is %x,%x,%x\n",strval16[0],strval16[1],strval16[2]);
  strval16[strlen]='\0';
//  printf("head for in strval is %x,%x,%x\n",strval16[0],strval16[1],strval16[2]);
//  printf("strval16 is ---> %ls\n",strval16);
  UTF16ToUTF8(strval16,strval16+strlen,strval,strval+256);
  printf("strval is ---> %s\n",strval);
  free(strval16);
  return strlen*2+2+2;
}

void setTypeStrID(struct ResTable* resTable,const struct ResStringPool_header* typePool){
  int i,offset;
//  int utf16=1;
  int stringCount=typePool->stringCount;
  unsigned char* strval=(unsigned char*)malloc(sizeof(unsigned char)*256);
  unsigned short* strval16=(unsigned short*)malloc(sizeof(unsigned short)*256);
  unsigned char* str=(unsigned char*)((const uint8_t*)typePool + typePool->stringsStart);
  for(i=0;i<stringCount;i++){
   // printf("offset str%d is %s\n",i,((const char*)((const uint8_t*)valuePool + valuePool->stringsStart)));
   // printf("str%d is ---> %s,string length is %ld  %lu\n",i,str,strlen(str),sizeof(str));
/*    strlen=*(str+1);
    printf("strlen is %d\n",strlen);
    strval=strncpy(strval,str+2,strlen);
    strval[strlen]='\0';
    str+=(strlen+2+1);
    printf("strval is ---> %s\n",strval);*/
    if(resTable->isutf16 == 1)
      offset=dispStrInfo16(str,strval);
    else
      offset=dispStrInfo(str,strval);
    str+=offset;
    if(strcmp(strval,"string") == 0)
      resTable->typeIndex=i+1;
  }
  printf("STRING TYPE ID IS %d\n",resTable->typeIndex);
  free(strval);
}

void setIndexName(struct ResTable* resTable,const struct ResStringPool_header* namePool,char* resName){
  int i,offset;
  int utf16=1;
  int stringCount=namePool->stringCount;
  unsigned char* strval=(unsigned char*)malloc(sizeof(unsigned char)*256);
//  unsigned short* strval16=(unsigned short*)malloc(sizeof(unsigned short)*256);
  unsigned char* str=(unsigned char*)((const uint8_t*)namePool + namePool->stringsStart);
  resTable->indexName=-1;
  for(i=0;i<stringCount;i++){
    printf("No%d",i);
    if(resTable->isutf16 == 1)
      offset=dispStrInfo16(str,strval);
    else
      offset=dispStrInfo(str,strval);
    str+=offset;
    if(strcmp(strval,resName) == 0)
      resTable->indexName=i;
  }
  free(strval); 
  if(resTable->indexName == -1){
    fprintf(stderr,"can not find %s\n",resName);
    exit(-1);
  }
  printf("INDEX OF NAME IS %d\n",resTable->indexName);
}

void setOffsetArray(struct ResTable* resTable,const struct ResStringPool_header* valPool){
  int i;
  int stringCount=valPool->stringCount;

  resTable->valPool_OffsetArray=(uint32_t*)malloc(sizeof(uint32_t)*stringCount);
  memcpy(resTable->valPool_OffsetArray,(uint8_t*)valPool+valPool->header.headerSize,sizeof(uint32_t)*stringCount);

  for(i=0;i<stringCount;i++){
//    printf("val in valPool is %s\n",(unsigned char*)valPool+valPool->stringsStart+resTable->valPool_OffsetArray[i]);
//    resTable->valPool_OffsetArray[i]+=((uint8_t*)valPool+valPool->stringsStart);
  }
}

/*void checkLocale(const struct ResTable_type){
  
}*/

/*struct ResChunk_header* stepOverPackage(struct ResChunk_header* chunk){
  //step over package head
  chunk=(struct ResChunk_header*)((uint8_t*)chunk+chunk->size);
  
}*/
void main(int argc,char** argv){
  int parseByID=0;
  char* strID=NULL;
  const struct ResTable_header* header=(struct ResTable_header*)getBuffer(argv[1]);
  char* resName=(char*)malloc(sizeof(char*)*256);
  char* arscName=argv[1];
  memset(resName,0,sizeof(char*)*256);
  printf("argc is %d argv is %s\n",argc,argv[0]);
  while(argc !=0 ){
    printf("argc is %d argv is %s\n",argc,argv[0]);
    if(strcmp(argv[0],"--parseByID") == 0){
      argc--;
      argv++;
      parseByID=1;
      strID=argv[0];
    }
    if(strcmp(argv[0],"--res_name") == 0){
      argc--;
      argv++;
      strcpy(resName,argv[0]);
    }
    argc--;
    argv++;
  }
  printf("res_name is %s,len is %d\n",resName,strlen(resName));
//  if(argc == 4 && strcmp(argv[2],"--parseByID") == 0){
//    parseByID=1;
//    strID=argv[3];
//  }
  const struct ResChunk_header* chunk=(struct ResChunk_header*)header;
  headAddr=(uint8_t*)header;
 
  struct ResTable* AndyTable=(struct ResTable*)malloc(sizeof(struct ResTable));

  checkFileLen(header); 
  AndyTable->dataEnd=((const uint8_t*)header) + header->header.size;

  printf("data end addr is %x\n",AndyTable->dataEnd);

  //string val pool
  chunk=(struct ResChunk_header*)((uint8_t*)chunk+chunk->headerSize);
  dispChunkInfo(chunk);
  const struct ResStringPool_header* valPool=(struct ResStringPool_header*)chunk;
  AndyTable->AppName1=NULL;
  AndyTable->AppName2=NULL;

  AndyTable->isutf16=0;
  if(valPool->flags == 0x00000000 || valPool->flags == 0x00000001 )
    AndyTable->isutf16=1;
  setOffsetArray(AndyTable,valPool);
  AndyTable->valPool=valPool;
  //package head

  //dispose framework-res.apk
  if(strcmp(arscName,"resources_framework-res.arsc") == 0)
    chunk=(struct ResChunk_header*)((uint8_t*)chunk+chunk->size);
  else{
    unsigned int package_id;
    do{
        chunk=(struct ResChunk_header*)((uint8_t*)chunk+chunk->size);
        dispChunkInfo(chunk);
        package_id=((struct ResTable_package*)chunk)->id;
    }while(package_id != 0x0000007f);
  }  
  //string type pool
  chunk=(struct ResChunk_header*)((uint8_t*)chunk+chunk->headerSize);
  dispChunkInfo(chunk);
  
  const struct ResStringPool_header* typePool=(struct ResStringPool_header*)chunk;
//  if(parseByID == 0)
    setTypeStrID(AndyTable,typePool);

  //string name pool
  chunk=(struct ResChunk_header*)((uint8_t*)chunk+chunk->size);
  dispChunkInfo(chunk);
  const struct ResStringPool_header* namePool=(struct ResStringPool_header*)chunk; 
  if(parseByID == 0)
    setIndexName(AndyTable,namePool,resName);
//  printf("%x     %x",(uint8_t*)chunk+chunk->size,AndyTable->dataEnd);

  while((uint8_t*)chunk+chunk->size != AndyTable->dataEnd){
     chunk=(struct ResChunk_header*)((uint8_t*)chunk+chunk->size);
     printf("chunk type is %x,chunk addr is %x\n",chunk->type,chunk);
     if(chunk->type == 0x0202)
        printf("chunk id is %x\n",((struct ResTable_typeSpec*)chunk)->id);
     if(chunk->type == RES_TABLE_TYPE_SPEC_TYPE){
       struct ResTable_typeSpec* configChunk=(struct ResTable_typeSpec*)chunk;
       if(configChunk->id == AndyTable->typeIndex){
//         int localeNum=configChunk->entryCount;
         int i;
         const struct ResTable_type* infoChunk;
//         for(i=0;i<localeNum;i++){
         //************* interator type chunk
         while(((struct ResChunk_header*)((uint8_t*)chunk+chunk->size))->type == 0x0201){
           chunk=(struct ResChunk_header*)((uint8_t*)chunk+chunk->size);
           dispChunkInfo(chunk);
           infoChunk=(struct ResTable_type*)chunk;
           int strCount=infoChunk->entryCount;
           int j,valIndex,nameIndex;
           uint32_t* resEntry_OffsetArray=(uint32_t*)malloc(sizeof(uint32_t)*strCount);
           memcpy(resEntry_OffsetArray,(uint8_t*)infoChunk+infoChunk->header.headerSize,sizeof(uint32_t)*strCount);
           if(parseByID == 1){
             char* resSeq=(char*)malloc(sizeof(char)*5);
             memcpy(resSeq,strID+4,4);
             resSeq[4]='\0';
             j=str2Hex(resSeq);
             if(resEntry_OffsetArray[j]!=0xFFFFFFFF){//ensure entry exist
               struct ResTable_entry* resEntry=(struct ResTable_entry*)((uint8_t*)infoChunk+infoChunk->entriesStart+resEntry_OffsetArray[j]);
               if(resEntry->flags==0x0000 || resEntry->flags==0x0002){
                 struct Res_value* resVal=(struct Res_value*)((uint8_t*)resEntry+resEntry->size);
                 if(resVal->dataType == 0x01){ //the string is a reference,turn to real res_entry
                   unsigned int data=resVal->data;
                   unsigned int k=data & 0x0000ffff;
                 //  uint16_t valIndex=data32 & 0x0000ffff; 
                   resEntry=(struct ResTable_entry*)((uint8_t*)infoChunk+infoChunk->entriesStart+resEntry_OffsetArray[k]);
                   resVal=(struct Res_value*)((uint8_t*)resEntry+resEntry->size);
                 }
                 nameIndex=resEntry->index;
                 valIndex=resVal->data;
                 printf("----> strIndex is %x,nameIndex is %x,valIndex is %x \n",j,nameIndex,valIndex);
                 if(AndyTable->isutf16 == 1){
                    uint8_t* appNameOffset=(uint8_t*)valPool+valPool->stringsStart+AndyTable->valPool_OffsetArray[valIndex];
                    int nameLen=appNameOffset[0];
                    printf("foo ------> app_len is %d\n",nameLen);
                    unsigned short* app_name16=(unsigned short*)(appNameOffset+2);
                    unsigned char* app_name=(unsigned char*)malloc(sizeof(unsigned char)*256);
                    UTF16ToUTF8(app_name16,app_name16+nameLen,app_name,app_name+256);
                    printf("HAHA !!! APP NAME IS %s\n",app_name);
                    if(infoChunk->config[2]==0x4e43687a){
                      AndyTable->AppName1=(unsigned char*)malloc(sizeof(unsigned char)*256);
                      memcpy(AndyTable->AppName1,app_name,256);
                    }
                    if(infoChunk->config[2]==0x00000000){
                      AndyTable->AppName2=(unsigned char*)malloc(sizeof(unsigned char)*256);
                      memcpy(AndyTable->AppName2,app_name,256);
                    }
                 }
                 else{
//                   printf("HAHA !!! APP NAME IS %s\n",(uint8_t*)valPool+valPool->stringsStart+AndyTable->valPool_OffsetArray[valIndex]+2);
                   uint8_t* appNameOffset=(uint8_t*)valPool+valPool->stringsStart+AndyTable->valPool_OffsetArray[valIndex];
                   int nameLen=appNameOffset[1];  
                   if(infoChunk->config[2]==0x4e43687a){
                     AndyTable->AppName1=(unsigned char*)malloc(sizeof(unsigned char)*256);
                     memcpy(AndyTable->AppName1,appNameOffset+2,256);
                     AndyTable->AppName1[nameLen]='\0';
                   }
                   if(infoChunk->config[2]==0x00000000){
                     AndyTable->AppName2=(unsigned char*)malloc(sizeof(unsigned char)*256);
                     memcpy(AndyTable->AppName2,appNameOffset+2,256);
                     AndyTable->AppName2[nameLen]='\0';
                   }
                   printf("HAHA !!! APP NAME IS %s\n",appNameOffset+2);
                 }
               }
             } 
           }
           else{
             for(j=0;j<strCount;j++){
               if(resEntry_OffsetArray[j]!=0xFFFFFFFF){
                 struct ResTable_entry* resEntry=(struct ResTable_entry*)((uint8_t*)infoChunk+infoChunk->entriesStart+resEntry_OffsetArray[j]);
                 if(resEntry->flags==0x0000 || resEntry->flags==0x0002){
                   struct Res_value* resVal=(struct Res_value*)((uint8_t*)resEntry+resEntry->size);
                   nameIndex=resEntry->index;
                   valIndex=resVal->data;
                   printf("----> strIndex is %x,nameIndex is %x,valIndex is %x \n",j,nameIndex,valIndex);
                   if(nameIndex == AndyTable->indexName){
                      uint8_t* appNameOffset=(uint8_t*)valPool+valPool->stringsStart+AndyTable->valPool_OffsetArray[valIndex];
                      if(AndyTable->isutf16 == 1){
			int nameLen=appNameOffset[0];
			printf("foo ------> app_len is %d\n",nameLen);
			unsigned short* app_name16=(unsigned short*)(appNameOffset+2);
			unsigned char* app_name=(unsigned char*)malloc(sizeof(unsigned char)*256);
			UTF16ToUTF8(app_name16,app_name16+nameLen,app_name,app_name+256);			
                        printf("HAHA !!! APP NAME IS %s\n",app_name);
                      }
                      else
                        printf("HAHA !!! APP NAME IS %s\n",(uint8_t*)valPool+valPool->stringsStart+AndyTable->valPool_OffsetArray[valIndex]+2);
                   }
                 }
               }
             }
           }
           free(resEntry_OffsetArray);
         }
       }
     }
     
  }
  printf("12345AppName1:%s|AppName2:%s\n",AndyTable->AppName1,AndyTable->AppName2);
}
