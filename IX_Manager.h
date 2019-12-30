#ifndef IX_MANAGER_H_H
#define IX_MANAGER_H_H

#include "PF_Manager.h"
#include "str.h"

typedef int SlotNum;

typedef struct {
	PageNum pageNum;	//��¼����ҳ��ҳ��
	SlotNum slotNum;		//��¼�Ĳ�ۺ�
	bool bValid; 			//true��ʾΪһ����Ч��¼�ı�ʶ��
}RID;

typedef struct{
	int attrLength;
	int keyLength;
	AttrType attrType;
	PageNum rootPage;
	PageNum first_leaf;
	int order;
	int height;  //���ĸ߶�
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
	bool bOpen;		/*ɨ���Ƿ�� */
	IX_IndexHandle* indexHandle;	//ָ�������ļ�������ָ��
	CompOp compOp;  /* ���ڱȽϵĲ�����*/
	char *value;		 /* �������бȽϵ�ֵ */
	IX_DataNode currentNode;
	PageNum pnNext; 	//��һ����Ҫ�������ҳ���
	int ridIx;  //ɨ�輴���������������
	PageNum pnSkip;  // ����<>����������Ծ����ֵ���в����е�ҳ���
	int ridIxSkip;  // ����<>����������Ծ����ֵ���в����е�������
}IX_IndexScan;

typedef struct Tree_Node{
	int  keyNum;		//�ڵ��а����Ĺؼ��֣�����ֵ������
	char  **keys;		//�ڵ��а����Ĺؼ��֣�����ֵ������
	Tree_Node  *parent;	//���ڵ�
	Tree_Node  *sibling;	//�ұߵ��ֵܽڵ�
	Tree_Node  *firstChild;	//����ߵĺ��ӽڵ�
}Tree_Node; //�ڵ����ݽṹ

typedef struct{
	AttrType  attrType;	//B+����Ӧ���Ե���������
	int  attrLength;	//B+����Ӧ����ֵ�ĳ���
	int  order;			//B+��������
	Tree_Node  *root;	//B+���ĸ��ڵ�
}Tree;

/*��������*/

inline int halfOrder(int order);

/*����rid1 < rid2*/
bool rid_cmp(const RID* rid1, const RID* rid2);

/*����attr1 < attr2*/
bool attr_cmp(IX_IndexHandle* indexHandle, const char* attr1, const char* attr2);

/*����attr1 == attr2*/
bool attr_equal(IX_IndexHandle* indexHandle, const char* attr1, const char* attr2);

/*����key1 < key2*/
bool key_cmp(IX_IndexHandle* indexHandle, const char* key1, const char* key2);

/*����key1 == key2*/
bool key_equal(IX_IndexHandle* indexHandle, const char* key1, const char* key2);

/*��Χ[begin, end), �����׸����ڵ���key��Ԫ�ص�ָ��ƫ�ƣ���û���򷵻�βָ��ƫ��*/
int lower_bound(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, int begin, int end);

/*��Χ[begin, end), �����׸�����key��Ԫ�ص�ָ��ƫ�ƣ���û���򷵻�βָ��ƫ��*/
int upper_bound(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, int begin, int end);

/*���IX_Nodeλ�õ����*/
inline char* getIxNodeEntry(char* pData);
/*��ùؼ�����ָ��*/
inline char* getKeyAreaEntry(char* pData);

/*���ֵ��ָ��*/
inline char* getRidAreaEntry(char* pData, int order, int keyLength);

/*��ʼ�����ݽڵ�*/
IX_DataNode* initDataNode(IX_IndexHandle* indexHandle, IX_DataNode* dataNode);

/*��ʼ��RID*/
RID* initRid(RID* rid, PageNum pageNum, SlotNum slotNum);

/*��ʼ��key*/
char* initKey(char* key, const char* attr, int attrLength, PageNum pageNum, SlotNum slotNum);

/*��ָ��ҳ����ڵ���Ϣ*/
RC map(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, PageNum pageNum);

/*��ָ��ҳд��ڵ�����*/
RC unmap(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, PageNum pageNum);

/*ˢ��header��Ϣ��disk*/
RC flashHeaderToDisk(IX_IndexHandle* indexHandle);

/*�����ؼ������һ��������ҳ��*/
PageNum search_index(IX_IndexHandle* indexHandle, const char* key);
/*���ݸ��ڵ�ҳ�ţ�����Ҷ�ӽڵ�ҳ��*/
PageNum search_leaf_with_parent(IX_IndexHandle* indexHandle, PageNum parent, const char* key);

/*����Ҷ�ӽڵ�ҳ��*/
PageNum search_leaf(IX_IndexHandle* indexHandle, const char* key);

/*���dataNode �ؼ�������indexλ�ô���ָ��*/
inline char* getKey(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int index);

/*���dataNode RID����index����ָ��*/
inline RID* getRID(IX_DataNode* dataNode, int index);

/*�����ֵ*/
void assignment(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int index, const char* key, const RID* rid);

/*��[first, last)��Χ�ڵ�Ԫ�ظ��Ƶ�[auto, d_last), lengthΪԪ�س���*/
void* copy_backward(void* first, void* last, void* d_last, int length);

/*��[first, last)��Χ�ڵ�Ԫ�ظ��Ƶ�[d_first, auto), lengthΪԪ�س���*/
void* copy(void* first, void* last, void* d_first, int length);

/*��dataNode��[first, last)��Χ�ڵ�Ԫ�ظ��Ƶ�[auto, d_last), lengthΪԪ�س���*/
int copy_backward_node(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int first, int last, int d_last);

/*��dtatNode��[first, last)��Χ�ڵ�Ԫ�ظ��Ƶ�[d_first, auto), lengthΪԪ�س���*/
int copy_node(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int first, int last, int d_first);

/*����һ��ҳ��������ҳ��*/
PageNum allocPage(IX_IndexHandle* indexHandle);

/*����һ���µ����ڵ�*/
void create_node(IX_IndexHandle* indexHandle, PageNum nodePageNum, IX_DataNode* node, IX_DataNode* newNode);

void remove_node(IX_IndexHandle* indexHandle, IX_DataNode* prev, IX_DataNode* node);

/*�ڲ���Ҫ���ѽڵ������²����¼��Ҷ�ӽڵ�*/
void insert_record_no_split(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, const RID* rid);

/*�ڲ���Ҫ���ѽڵ������²���������ڲ��ڵ�*/
void insert_index_no_split(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, const RID* rid);

/*���ýڵ�[begin, end)��Χ�ڵ��ӽڵ��parent*/
void reset_children_parent(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int begin, int end, PageNum parent);

/*����������ڲ��ڵ�*/
void insert_index(IX_IndexHandle* indexHandle, PageNum target, const char* key, PageNum old, PageNum after);

/*���¸��ڵ�������Ϣ*/
void reset_parent_children(IX_IndexHandle* indexHandle, PageNum parent, const char* target, const char* value);

/*Ҷ�ӽڵ���ùؼ���*/
bool borrow_key(IX_IndexHandle* indexHandle, bool from_right, IX_DataNode* borrowerNode);

/*�ڲ��ڵ���ùؼ���*/
bool borrow_key_index(IX_IndexHandle* indexHandle, bool from_right, IX_DataNode* borrowerNode, PageNum offset);

/*�ϲ��ڵ�*/
void merge_node(IX_IndexHandle* indexHandle, IX_DataNode* left, IX_DataNode* right);

/*���ڲ��ڵ�ɾ������*/
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