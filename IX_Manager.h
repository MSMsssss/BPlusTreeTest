#ifndef IX_MANAGER_H_H
#define IX_MANAGER_H_H

#include "PF_Manager.h"
#include "str.h"

typedef int SlotNum;

typedef struct {
	PageNum pageNum;	//记录所在页的页号
	SlotNum slotNum;		//记录的插槽号
	bool bValid; 			//true表示为一个有效记录的标识符
}RID;

typedef struct{
	int attrLength;
	int keyLength;
	AttrType attrType;
	PageNum rootPage;
	PageNum first_leaf;
	int order;
	int height;  //树的高度
}IX_FileHeader;

typedef struct{
	bool bOpen;
	PF_FileHandle fileHandle;
	IX_FileHeader fileHeader;
}IX_IndexHandle;

typedef struct{
	int is_leaf;
	int keynum;
	PageNum parent;
	PageNum prev;
	PageNum next;
	char *keys;
	RID *rids;
}IX_Node;

const int BUFFER_SIZE = sizeof(Page) - (sizeof(PageNum) + sizeof(IX_FileHeader) + sizeof(IX_Node));
typedef struct {
	IX_Node node_info;
	char buffer[BUFFER_SIZE];
}IX_DataNode;

typedef struct{
	bool bOpen;		/*扫描是否打开 */
	IX_IndexHandle* indexHandle;	//指向索引文件操作的指针
	CompOp compOp;  /* 用于比较的操作符*/
	char *value;		 /* 与属性行比较的值 */
	IX_DataNode currentNode;
	PageNum pnNext; 	//下一个将要被读入的页面号
	int ridIx;  //扫描即将处理的索引项编号
	PageNum pnSkip;  // 用于<>操作符的跳跃过等值序列操作中的页面号
	int ridIxSkip;  // 用于<>操作符的跳跃过等值序列操作中的索引号
}IX_IndexScan;

typedef struct Tree_Node{
	int  keyNum;		//节点中包含的关键字（属性值）个数
	char  **keys;		//节点中包含的关键字（属性值）数组
	Tree_Node  *parent;	//父节点
	Tree_Node  *sibling;	//右边的兄弟节点
	Tree_Node  *firstChild;	//最左边的孩子节点
}Tree_Node; //节点数据结构

typedef struct{
	AttrType  attrType;	//B+树对应属性的数据类型
	int  attrLength;	//B+树对应属性值的长度
	int  order;			//B+树的序数
	Tree_Node  *root;	//B+树的根节点
}Tree;

/*辅助函数*/

/*返回rid1 < rid2*/
bool rid_cmp(const RID* rid1, const RID* rid2);

/*返回key1 < key2*/
bool key_cmp(IX_IndexHandle* indexHandle, const char* key1, const char* key2);

/*返回首个大于等于key的元素的指针，若没有则返回尾指针*/
int lower_bound(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, int begin, int end);

/*返回首个大于key的元素的指针，若没有则返回尾指针*/
int upper_bound(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, int begin, int end);

void printNode(IX_IndexHandle* indexHandle, IX_DataNode* dataNode);
void printList(IX_IndexHandle* indexHandle);

RC CreateIndex(const char * fileName,AttrType attrType,int attrLength);
RC OpenIndex(const char *fileName,IX_IndexHandle *indexHandle);
RC CloseIndex(IX_IndexHandle *indexHandle);

RC InsertEntry(IX_IndexHandle *indexHandle,void *pData,const RID * rid);
RC DeleteEntry(IX_IndexHandle *indexHandle,void *pData,const RID * rid);
RC OpenIndexScan(IX_IndexScan *indexScan,IX_IndexHandle *indexHandle,CompOp compOp,char *value);
RC IX_GetNextEntry(IX_IndexScan *indexScan,RID * rid);
RC CloseIndexScan(IX_IndexScan *indexScan);
RC GetIndexTree(char *fileName, Tree *index);

#endif