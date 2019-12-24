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

/*����rid1 < rid2*/
bool rid_cmp(const RID* rid1, const RID* rid2);

/*����key1 < key2*/
bool key_cmp(IX_IndexHandle* indexHandle, const char* key1, const char* key2);

/*�����׸����ڵ���key��Ԫ�ص�ָ�룬��û���򷵻�βָ��*/
int lower_bound(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, int begin, int end);

/*�����׸�����key��Ԫ�ص�ָ�룬��û���򷵻�βָ��*/
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