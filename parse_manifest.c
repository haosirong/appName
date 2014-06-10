#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>


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

struct ResStringPool_ref
{
    // Index into the string pool table (uint32_t-offset from the indices
    // immediately after ResStringPool_header) at which to find the location
    // of the string data in the pool.
    uint32_t index;
};

struct Res_value
{
    // Number of bytes in this structure.
    uint16_t size;

    // Always set to 0.
    uint8_t res0;

    uint8_t dataType;

    uint32_t data;
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
 *  XML Tree
 *
 *  Binary representation of an XML document.  This is designed to
 *  express everything in an XML document, in a form that is much
 *  easier to parse on the device.
 *
 *********************************************************************** */

/**
 * XML tree header.  This appears at the front of an XML tree,
 * describing its content.  It is followed by a flat array of
 * ResXMLTree_node structures; the hierarchy of the XML document
 * is described by the occurrance of RES_XML_START_ELEMENT_TYPE
 * and corresponding RES_XML_END_ELEMENT_TYPE nodes in the array.
 */
struct ResXMLTree_header
{
    struct ResChunk_header header;
};

/**
 * Basic XML tree node.  A single item in the XML document.  Extended info
 * about the node can be found after header.headerSize.
 */
struct ResXMLTree_node
{
    struct ResChunk_header header;

    // Line number in original source file at which this element appeared.
    uint32_t lineNumber;

    // Optional XML comment that was associated with this element; -1 if none.
    uint32_t comment;
};

/**
 * Extended XML tree node for namespace start/end nodes.
 * Appears header.headerSize bytes after a ResXMLTree_node.
 */
struct ResXMLTree_namespaceExt
{
    // The prefix of the namespace.
    struct ResStringPool_ref prefix;

    // The URI of the namespace.
    uint32_t uri;
};

/**
 * Extended XML tree node for element start/end nodes.
 * Appears header.headerSize bytes after a ResXMLTree_node.
 */
struct ResXMLTree_endElementExt
{
    // String of the full namespace of this element.
    struct ResStringPool_ref ns;

    // String name of this node if it is an ELEMENT; the raw
    // character data if this is a CDATA node.
    uint32_t name;
};

/**
 * Extended XML tree node for start tags -- includes attribute
 * information.
 * Appears header.headerSize bytes after a ResXMLTree_node.
 */
struct ResXMLTree_attrExt
{
    // String of the full namespace of this element.
    uint32_t ns;

    // String name of this node if it is an ELEMENT; the raw
    // character data if this is a CDATA node.
    uint32_t name;

    // Byte offset from the start of this structure where the attributes start.
    uint16_t attributeStart;

    // Size of the ResXMLTree_attribute structures that follow.
    uint16_t attributeSize;

    // Number of attributes associated with an ELEMENT.  These are
    // available as an array of ResXMLTree_attribute structures
    // immediately following this node.
    uint16_t attributeCount;

    // Index (1-based) of the "id" attribute. 0 if none.
    uint16_t idIndex;

    // Index (1-based) of the "class" attribute. 0 if none.
    uint16_t classIndex;

    // Index (1-based) of the "style" attribute. 0 if none.
    uint16_t styleIndex;
};

struct ResXMLTree_attribute
{
    // Namespace of this attribute.
//    struct ResStringPool_ref ns;
    uint32_t ns;
    // Name of this attribute.
//    struct ResStringPool_ref name;
    uint32_t name;
    // The original raw string value of this attribute.
//    struct ResStringPool_ref rawValue;
    uint32_t rawValue;
    // Processesd typed value of this attribute.
    struct Res_value typedValue;
};


struct ResLabel{
  uint32_t app_lblID;
  uint32_t activity_lblID;
  unsigned char* appStr;
  unsigned char* activityStr;  
};
struct ResTable{
  int typeIndex;
  int labelIndex;
  int indexName;
  int isutf16;
  int mainActivity;
//  struct ResLabel* resLabel;
//  uint32_t app_lblID;
//  uint32_t activity_lblID;
  const uint8_t* dataEnd;
  uint32_t* mapPool;
  uint32_t* valPool_OffsetArray;
  uint32_t** resPool_OffsetArray;
  const struct ResStringPool_header* valPool;
};

long file_len;
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

void dispChunkInfo(const struct ResChunk_header* chunk){
  printf("type is %x,headersize is %x,size is %x\n",chunk->type,chunk->headerSize,chunk->size);
}

int checkApplicationTag(struct ResTable* resTable,struct ResXMLTree_node* xmlNode){
  struct ResXMLTree_attrExt* attrExt=(struct ResXMLTree_attrExt*)((uint8_t*)xmlNode+xmlNode->header.headerSize);
  int tagIndex=attrExt->name;
  int attrCount=attrExt->attributeCount;
  unsigned char* tag_name=(unsigned char*)malloc(sizeof(unsigned char)*256);
  unsigned char* tag_str=(unsigned char*)(resTable->resPool_OffsetArray[tagIndex]);
  int tagLen=tag_str[0];
  unsigned short* tag_name16=(unsigned short*)(tag_str+2);
  printf("tag len  is ---> %d\n",tagLen);
                        
  UTF16ToUTF8(tag_name16,tag_name16+tagLen,tag_name,tag_name+256);
  printf("tag name is ---> %s\n",tag_name); 
  if(strcmp(tag_name,"application")==0 && attrCount>0){
    int i;
    printf("1111111111111111111111111\n");
    struct ResXMLTree_attribute* attrChunk=(struct ResXMLTree_attribute*)((uint8_t*)attrExt+attrExt->attributeStart);
    for(i=0;i<attrCount;i++){
      printf("22222222222222222222222 attrChunk->ns is %x,addr is %p\n",attrChunk->ns,attrChunk);
//      if(attrChunk->ns != -1 && resTable->mapPool[attrChunk->ns] == 0x01010001 ){ //search attr label
      if(attrChunk->name == resTable->labelIndex){
        if(attrChunk->rawValue != -1){// label name is raw data
//          int labelIndex=resTable->labelIndex;
          unsigned char* label_name=(unsigned char*)malloc(sizeof(unsigned char)*256);
          unsigned char* label_str=(unsigned char*)(resTable->resPool_OffsetArray[attrChunk->rawValue]);
          int labelLen=label_str[0];
          unsigned short* label_name16=(unsigned short*)(label_str+2);
          printf("label len  is ---> %d\n",labelLen);
          UTF16ToUTF8(label_name16,label_name16+labelLen,label_name,label_name+256);
          printf("HAHAHA1 label name is ---> %s\n",label_name);
          free(label_name);
        }
        else{// label name is string reference
          struct Res_value* res_val=&attrChunk->typedValue;
          printf("HAHAHA resouce id is %x\n",res_val->data);
        }
      }
      attrChunk=(struct ResXMLTree_attribute*)((uint8_t*)attrChunk+20);
    }
  }  
  if(strcmp(tag_name,"activity")==0 && attrCount>0 && resTable->mainActivity ==0){
    int i;
    printf("123123this is an activity\n");
    struct ResXMLTree_attribute* attrChunk=(struct ResXMLTree_attribute*)((uint8_t*)attrExt+attrExt->attributeStart);
    for(i=0;i<attrCount;i++){
      if(attrChunk->name == resTable->labelIndex){
        printf("123123the activity contains label attr\n");
        iterElemActivity(xmlNode,resTable);
        if(resTable->mainActivity ==1){
          if(attrChunk->rawValue != -1){// label name is raw data
//            int labelIndex=resTable->labelIndex;
            unsigned char* label_name=(unsigned char*)malloc(sizeof(unsigned char)*256);
            unsigned char* label_str=(unsigned char*)(resTable->resPool_OffsetArray[attrChunk->rawValue]);
            int labelLen=label_str[0];
            unsigned short* label_name16=(unsigned short*)(label_str+2);
            printf("label len  is ---> %d\n",tagLen);
            UTF16ToUTF8(label_name16,label_name16+labelLen,label_name,label_name+256);
            printf("HAHAHA1 label name is ---> %s\n",label_name);
            free(label_name);
          }  
          else{// label name is string reference
            struct Res_value* res_val=&attrChunk->typedValue;
            printf("HAHAHA resouce id is %x\n",res_val->data);
          }
        }
      }
      attrChunk=(struct ResXMLTree_attribute*)((uint8_t*)attrChunk+20);
    }   
  }
  free(tag_name);
}

int checkLauncherAttr(struct ResTable* resTable,struct ResXMLTree_node* xmlNode){
  int isLauncher=0;

  struct ResXMLTree_attrExt* attrExt=(struct ResXMLTree_attrExt*)((uint8_t*)xmlNode+xmlNode->header.headerSize);
  int tagIndex=attrExt->name;
  int attrCount=attrExt->attributeCount;
  unsigned char* tag_name=(unsigned char*)malloc(sizeof(unsigned char)*256);
  unsigned char* tag_str=(unsigned char*)(resTable->resPool_OffsetArray[tagIndex]);
  int tagLen=tag_str[0];
  unsigned short* tag_name16=(unsigned short*)(tag_str+2);
//  printf("tag len  is ---> %d\n",tagLen);

  UTF16ToUTF8(tag_name16,tag_name16+tagLen,tag_name,tag_name+256);
//  printf("tag name is ---> %s\n",tag_name);
  if(strcmp(tag_name,"category")==0 && attrCount>0){ 
    int i;
    struct ResXMLTree_attribute* attrChunk=(struct ResXMLTree_attribute*)((uint8_t*)attrExt+attrExt->attributeStart);
    for(i=0;i<attrCount;i++){
      if(attrChunk->rawValue  != -1){
          int attrValIndex=attrChunk->rawValue;
          unsigned char* attrVal=(unsigned char*)malloc(sizeof(unsigned char)*256);
          unsigned char* attrVal_Str=(unsigned char*)(resTable->resPool_OffsetArray[attrValIndex]);
          int len=attrVal_Str[0];
          unsigned short* attrVal16=(unsigned short*)(attrVal_Str+2);
//          printf("label len  is ---> %d\n",tagLen);
          UTF16ToUTF8(attrVal16,attrVal16+len,attrVal,attrVal+256);
          if(strcmp(attrVal,"android.intent.category.LAUNCHER") == 0){
             printf("THIS IS MAIN ACTIVITY\n");
             resTable->mainActivity=1;
          }
          free(attrVal);
      }
    }
  }    
}
/*
@para1 a activity tag
@ret if the activity will be displayed on packages installer
*/
int iterElemActivity(struct ResXMLTree_node* parentNode,struct ResTable* resTable){
  struct ResXMLTree_node* childNode=(struct ResXMLTree_node*)((uint8_t*)parentNode+parentNode->header.size);
  struct ResXMLTree_node* preNode; 
  uint32_t allSubSize=0;
  checkLauncherAttr(resTable,parentNode);
  while(childNode->header.type == 0x0102){
    preNode=childNode;
    childNode=(struct ResXMLTree_node*)((uint8_t*)childNode+iterElemActivity(childNode,resTable));
    allSubSize+=((uint8_t*)childNode-(uint8_t*)preNode);
  }
  return parentNode->header.size+allSubSize+24;
}
/*
@ret tag size (contain sub tag size)
*/
uint32_t iterElem(struct ResXMLTree_node* parentNode,struct ResTable* resTable){
  struct ResXMLTree_node* childNode=(struct ResXMLTree_node*)((uint8_t*)parentNode+parentNode->header.size);
  struct ResXMLTree_node* preNode;
//  struct ResXMLTree_attrExt* attrExt=(struct ResXMLTree_attrExt*)((uint8_t*)parentNode+parentNode->header.headerSize);
  uint32_t allSubSize=0;
  printf("line number is %d\n",parentNode->lineNumber);
  checkApplicationTag(resTable,parentNode);
//  if(childNode->header.type != 0x0102){ //tag didn't contain subtag
//    return parentNode->header.size+24;
//  }
//  else{
    while(childNode->header.type == 0x0102){
      preNode=childNode;
      childNode=(struct ResXMLTree_node*)((uint8_t*)childNode+iterElem(childNode,resTable));
      allSubSize+=((uint8_t*)childNode-(uint8_t*)preNode);
    }
//  }
    return parentNode->header.size+allSubSize+24;
}

void setOffsetArray(struct ResTable* resTable,const struct ResStringPool_header* resPool){
  int i;
  int stringCount=resPool->stringCount;
  uint32_t* addrOffset=(uint32_t*)malloc(sizeof(uint32_t)*stringCount);
  resTable->resPool_OffsetArray=(uint32_t**)malloc(sizeof(uint32_t*)*stringCount);
  memcpy(addrOffset,(uint8_t*)resPool+resPool->header.headerSize,sizeof(uint32_t)*stringCount);

  for(i=0;i<stringCount;i++){
//    printf("val in valPool is %s\n",(unsigned char*)valPool+valPool->stringsStart+resTable->valPool_OffsetArray[i]);
    resTable->resPool_OffsetArray[i]=(uint32_t*)((uint8_t*)resPool+resPool->stringsStart+addrOffset[i]);
  }
}

void setMapArray(struct ResTable* resTable,const struct ResChunk_header* mapChunk){
  if(mapChunk->type != RES_XML_RESOURCE_MAP_TYPE)
    return;
  int mapCount=(mapChunk->size-8) / 4;
  resTable->mapPool=(uint32_t*)malloc(sizeof(uint32_t)*mapCount); 
  memcpy(resTable->mapPool,(uint8_t*)mapChunk+mapChunk->headerSize,sizeof(uint32_t)*mapCount);
  int i;
  for(i=0;i<mapCount;i++)
    printf("Map%d: ---> %x\n",i,resTable->mapPool[i]);
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

void dispResPool(const struct ResStringPool_header* resPool){
  int i,offset;
  int utf16=1;
  int stringCount=resPool->stringCount;
  unsigned char* strval=(unsigned char*)malloc(sizeof(unsigned char)*256);
//  unsigned short* strval16=(unsigned short*)malloc(sizeof(unsigned short)*256);
  unsigned char* str=(unsigned char*)((const uint8_t*)resPool + resPool->stringsStart);
  for(i=0;i<stringCount;i++){
    printf("No%d: ",i);
    if(utf16 == 1)
      offset=dispStrInfo16(str,strval);
    else
      offset=dispStrInfo(str,strval);
    str+=offset;
  }
  printf("LALALALALA\n");
  free(strval);
}

void setLableIndex(struct ResTable* resTable,const struct ResStringPool_header* resPool){
  int i,offset;
  int utf16=1;
  int stringCount=resPool->stringCount;
  unsigned char* strval=(unsigned char*)malloc(sizeof(unsigned char)*256);
//  unsigned short* strval16=(unsigned short*)malloc(sizeof(unsigned short)*256);
  unsigned char* str=(unsigned char*)((const uint8_t*)resPool + resPool->stringsStart);
  resTable->labelIndex=-1;
  
  for(i=0;i<stringCount;i++){
    printf("No%d",i);
    if(utf16 == 1)
      offset=dispStrInfo16(str,strval);
    else
      offset=dispStrInfo(str,strval);
    str+=offset;
    if(strcmp(strval,"label") == 0)
      resTable->labelIndex=i;
  }
  free(strval);
  if(resTable->indexName == -1){
    fprintf(stderr,"can not find label attr\n");
    exit(-1);
  }
  printf("INDEX OF LABEL IS %d\n",resTable->labelIndex);
}

int main(int argc,char** argv){

  struct ResTable* AndyTable=(struct ResTable*)malloc(sizeof(struct ResTable));
  const struct ResXMLTree_header* header=(struct ResXMLTree_header*)getBuffer(argv[1]); 
  printf("buffer head is %p\n",header);
  const struct ResChunk_header* chunk=(struct ResChunk_header*)header;
//  checkFileLen(header);
//  AndyTable->app_lblID=-1;
//  AndyTable->activity_lblID=-1;
  AndyTable->mainActivity=0;//res pool
  chunk=(struct ResChunk_header*)((uint8_t*)chunk+chunk->headerSize);
  dispChunkInfo(chunk);
  const struct ResStringPool_header* resPool=(struct ResStringPool_header*)chunk;
  setOffsetArray(AndyTable,resPool);
  dispResPool(resPool);
  setLableIndex(AndyTable,resPool);

  //resid map pool
  chunk=(struct ResChunk_header*)((uint8_t*)chunk+chunk->size);
  dispChunkInfo(chunk);
  setMapArray(AndyTable,chunk);

  //namespace node
  chunk=(struct ResChunk_header*)((uint8_t*)chunk+chunk->size);
  dispChunkInfo(chunk);
  //first node
  chunk=(struct ResChunk_header*)((uint8_t*)chunk+chunk->size);  
  dispChunkInfo(chunk);
  struct ResXMLTree_node* startElem=(struct ResXMLTree_node*)chunk;

  iterElem(startElem,AndyTable);

//  printf("ID in application is %x,ID in main activity is %x\n",AndyTable->app_lblID,AndyTable->activity_lblID);
}
