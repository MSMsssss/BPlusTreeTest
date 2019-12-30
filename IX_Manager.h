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

inline int halfOrder(int order);

/*返回rid1 < rid2*/
bool rid_cmp(const RID* rid1, const RID* rid2);

/*返回attr1 < attr2*/
bool attr_cmp(IX_IndexHandle* indexHandle, const char* attr1, const char* attr2);

/*返回attr1 == attr2*/
bool attr_equal(IX_IndexHandle* indexHandle, const char* attr1, const char* attr2);

/*返回key1 < key2*/
bool key_cmp(IX_IndexHandle* indexHandle, const char* key1, const char* key2);

/*返回key1 == key2*/
bool key_equal(IX_IndexHandle* indexHandle, const char* key1, const char* key2);

/*范围[begin, end), 返回首个大于等于key的元素的指针偏移，若没有则返回尾指针偏移*/
int lower_bound(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, int begin, int end);

/*范围[begin, end), 返回首个大于key的元素的指针偏移，若没有则返回尾指针偏移*/
int upper_bound(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, int begin, int end);

/*获得IX_Node位置的入口*/
inline char* getIxNodeEntry(char* pData);
/*获得关键字区指针*/
inline char* getKeyAreaEntry(char* pData);

/*获得值区指针*/
inline char* getRidAreaEntry(char* pData, int order, int keyLength);

/*初始化数据节点*/
IX_DataNode* initDataNode(IX_IndexHandle* indexHandle, IX_DataNode* dataNode);

/*初始化RID*/
RID* initRid(RID* rid, PageNum pageNum, SlotNum slotNum);

/*初始化key*/
char* initKey(char* key, const char* attr, int attrLength, PageNum pageNum, SlotNum slotNum);

/*从指定页读入节点信息*/
RC map(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, PageNum pageNum);

/*向指定页写入节点数据*/
RC unmap(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, PageNum pageNum);

/*刷新header信息到disk*/
RC flashHeaderToDisk(IX_IndexHandle* indexHandle);

/*锁定关键字最后一层索引的页号*/
PageNum search_index(IX_IndexHandle* indexHandle, const char* key);
/*根据父节点页号，锁定叶子节点页号*/
PageNum search_leaf_with_parent(IX_IndexHandle* indexHandle, PageNum parent, const char* key);

/*锁定叶子节点页号*/
PageNum search_leaf(IX_IndexHandle* indexHandle, const char* key);

/*获得dataNode 关键字数组index位置处的指针*/
inline char* getKey(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int index);

/*获得dataNode RID数组index处的指针*/
inline RID* getRID(IX_DataNode* dataNode, int index);

/*索引项赋值*/
void assignment(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int index, const char* key, const RID* rid);

/*将[first, last)范围内的元素复制到[auto, d_last), length为元素长度*/
void* copy_backward(void* first, void* last, void* d_last, int length);

/*将[first, last)范围内的元素复制到[d_first, auto), length为元素长度*/
void* copy(void* first, void* last, void* d_first, int length);

/*将dataNode中[first, last)范围内的元素复制到[auto, d_last), length为元素长度*/
int copy_backward_node(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int first, int last, int d_last);

/*将dtatNode中[first, last)范围内的元素复制到[d_first, auto), length为元素长度*/
int copy_node(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int first, int last, int d_first);

/*分配一个页，并返回页号*/
PageNum allocPage(IX_IndexHandle* indexHandle);

/*创建一个新的树节点*/
void create_node(IX_IndexHandle* indexHandle, PageNum nodePageNum, IX_DataNode* node, IX_DataNode* newNode);

void remove_node(IX_IndexHandle* indexHandle, IX_DataNode* prev, IX_DataNode* node);

/*在不需要分裂节点的情况下插入记录到叶子节点*/
void insert_record_no_split(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, const RID* rid);

/*在不需要分裂节点的情况下插入索引项到内部节点*/
void insert_index_no_split(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, const RID* rid);

/*设置节点[begin, end)范围内的子节点的parent*/
void reset_children_parent(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int begin, int end, PageNum parent);

/*插入索引项到内部节点*/
void insert_index(IX_IndexHandle* indexHandle, PageNum target, const char* key, PageNum old, PageNum after);

/*更新父节点索引信息*/
void reset_parent_children(IX_IndexHandle* indexHandle, PageNum parent, const char* target, const char* value);

/*叶子节点借用关键字*/
bool borrow_key(IX_IndexHandle* indexHandle, bool from_right, IX_DataNode* borrowerNode);

/*内部节点借用关键字*/
bool borrow_key_index(IX_IndexHandle* indexHandle, bool from_right, IX_DataNode* borrowerNode, PageNum offset);

/*合并节点*/
void merge_node(IX_IndexHandle* indexHandle, IX_DataNode* left, IX_DataNode* right);

/*从内部节点删除数据*/
void remove_from_index(IX_IndexHandle* indexHandle, PageNum target, IX_DataNode* targetNode, const char* key);

void printTreeInfo(IX_IndexHandle* indexHandle);
void printNode(IX_IndexHandle* indexHandle, IX_DataNode* dataNode);
void printList(IX_IndexHandle* indexHandle);
void setTestMode(bool testMode);
RC directOpenIndexScan(IX_IndexScan* indexScan, IX_IndexHandle* indexHandle, CompOp compOp, char* value);
RC directGetNextEntry(IX_IndexScan* indexScan, RID* rid);
void printMemNode(Tree* index, Tree_Node* node);
void printMemList(Tree* index);
bool node_legal(Tree* index, Tree_Node* node);
bool index_legal(Tree* index);

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