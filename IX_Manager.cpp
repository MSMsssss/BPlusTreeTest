#include "IX_Manager.h"
#include <string.h>

inline int halfOrder(int order)
{
	return order / 2;
}

/*����rid1 < rid2*/
bool rid_cmp(const RID* rid1, const RID* rid2)
{
	if (rid1->pageNum == rid2->pageNum)
		return rid1->slotNum < rid2->slotNum;
	else
		return rid1->pageNum < rid2->pageNum;
}

/*����attr1 < attr2*/
bool attr_cmp(IX_IndexHandle* indexHandle, const char* attr1, const char* attr2)
{
	AttrType attrType = indexHandle->fileHeader.attrType;
	int attrLength = indexHandle->fileHeader.attrLength;

	if (attrType == ints)
	{
		return *((int*)attr1) < *((int*)attr2);
	}
	else if (attrType == floats)
	{
		return *((float*)attr1) < *((float*)attr2);
	}
	else if (attrType == chars)
	{
		return strcmp(attr1, attr2) < 0 ? true : false;
	}
	else
	{
		printf("attrType error");
		exit(-1);
	}
}

/*����attr1 == attr2*/
bool attr_equal(IX_IndexHandle* indexHandle, const char* attr1, const char* attr2)
{
	return !attr_cmp(indexHandle, attr1, attr2) && !attr_cmp(indexHandle, attr2, attr1);
}

/*����key1 < key2*/
bool key_cmp(IX_IndexHandle* indexHandle, const char* key1, const char* key2)
{
	AttrType attrType = indexHandle->fileHeader.attrType;
	int attrLength = indexHandle->fileHeader.attrLength;

	if (attrType == ints)
	{
		//�������ֵ
		int attrValue1 = *((int*)key1);
		int attrValue2 = *((int*)key2);

		//���RIDֵ
		RID rid1 = *((RID*)(key1 + attrLength));
		RID rid2 = *((RID*)(key2 + attrLength));

		if (attrValue1 == attrValue2)
		{
			return rid_cmp(&rid1, &rid2);
		}
		else
		{
			return attrValue1 < attrValue2;
		}
	}
	else if (attrType == floats)
	{
		//�������ֵ
		float attrValue1 = *((float*)key1);
		float attrValue2 = *((float*)key2);

		//���RIDֵ
		RID rid1 = *((RID*)(key1 + attrLength));
		RID rid2 = *((RID*)(key2 + attrLength));

		if (attrValue1 == attrValue2)
		{
			return rid_cmp(&rid1, &rid2);
		}
		else
		{
			return attrValue1 < attrValue2;
		}
	}
	else if (attrType == chars)
	{
		//�������ֵ
		const char* attrValue1 = key1;
		const char* attrValue2 = key2;

		//���RIDֵ
		RID rid1 = *((RID*)(key1 + attrLength));
		RID rid2 = *((RID*)(key2 + attrLength));

		if (strcmp(attrValue1, attrValue2) == 0)
		{
			return rid_cmp(&rid1, &rid2);
		}
		else
		{
			return strcmp(attrValue1, attrValue2) < 0 ? 1 : 0;
		}
	}
	else
	{
		printf("attrType error");
		exit(-1);
	}
}

/*����key1 == key2*/
bool key_equal(IX_IndexHandle* indexHandle, const char* key1, const char* key2)
{
	return !key_cmp(indexHandle, key1, key2) && !key_cmp(indexHandle, key2, key1);
}

/*��Χ[begin, end), �����׸����ڵ���key��Ԫ�ص�ָ��ƫ�ƣ���û���򷵻�βָ��ƫ��*/
int lower_bound(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, int begin, int end)
{
	int keyLength = indexHandle->fileHeader.keyLength;
	int attrLength = indexHandle->fileHeader.attrLength;
	AttrType attrType = indexHandle->fileHeader.attrType;

	const IX_Node* node = &dataNode->node_info;
	char* first = node->keys + keyLength * begin;

	char* it;
	int count, step;
	count = end - begin;

	while (count > 0) {
		it = first;
		step = count / 2;
		it = it + keyLength * step;
		if (key_cmp(indexHandle, it, key)) 
		{
			first = it + keyLength;
			count -= step + 1;
		}
		else
			count = step;
	}
	return (first - node->keys) / keyLength;
}

/*��Χ[begin, end), �����׸�����key��Ԫ�ص�ָ��ƫ�ƣ���û���򷵻�βָ��ƫ��*/
int upper_bound(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, int begin, int end)
{
	int keyLength = indexHandle->fileHeader.keyLength;
	int attrLength = indexHandle->fileHeader.attrLength;
	AttrType attrType = indexHandle->fileHeader.attrType;

	const IX_Node* node = &dataNode->node_info;
	char* first = node->keys + keyLength * begin;

	char* it;
	int count, step;
	count = end - begin;

	while (count > 0) {
		it = first;
		step = count / 2;
		it = it + keyLength * step;
		if (!key_cmp(indexHandle, key, it))
		{
			first = it + keyLength;
			count -= step + 1;
		}
		else
			count = step;
	}
	return (first - node->keys) / keyLength;
}

/*���IX_Nodeλ�õ����*/
inline char* getIxNodeEntry(char* pData)
{
	return pData + sizeof(IX_FileHeader);
}

/*��ùؼ�����ָ��*/
inline char* getKeyAreaEntry(char* pData)
{
	int offset = sizeof(IX_FileHeader) + sizeof(IX_Node);
	return pData + offset;
}

/*���ֵ��ָ��*/
inline char* getRidAreaEntry(char* pData, int order, int keyLength)
{
	int offset = sizeof(IX_FileHeader) + sizeof(IX_Node) + keyLength * order;
	return pData + offset;
}

/*��ʼ�����ݽڵ�*/
IX_DataNode* initDataNode(IX_IndexHandle* indexHandle, IX_DataNode* dataNode)
{
	dataNode->node_info.keynum = 0;
	dataNode->node_info.is_leaf = 0;
	dataNode->node_info.parent = dataNode->node_info.next = dataNode->node_info.prev = 0;
	dataNode->node_info.keys = dataNode->buffer;
	dataNode->node_info.rids = (RID*)(dataNode->buffer + indexHandle->fileHeader.keyLength * indexHandle->fileHeader.order);

	return dataNode;
}

/*��ʼ��RID*/
RID* initRid(RID* rid, PageNum pageNum, SlotNum slotNum)
{
	rid->bValid = true;
	rid->pageNum = pageNum;
	rid->slotNum = slotNum;

	return rid;
}

/*��ʼ��key*/
char* initKey(char* key, const char* attr, int attrLength, PageNum pageNum, SlotNum slotNum)
{
	memcpy(key, attr, attrLength);
	initRid((RID*)(key + attrLength), pageNum, slotNum);

	return key;
}

/*��ָ��ҳ����ڵ���Ϣ*/
RC map(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, PageNum pageNum)
{
	if (indexHandle->bOpen == false)
		return FAIL;

	PF_PageHandle pageHandle;
	GetThisPage(&indexHandle->fileHandle, pageNum, &pageHandle);

	char* pData;
	GetData(&pageHandle, &pData);
	memcpy(&dataNode->node_info, getIxNodeEntry(pData), sizeof(IX_Node));
	memcpy(dataNode->buffer, getKeyAreaEntry(pData), BUFFER_SIZE);
	dataNode->node_info.keys = dataNode->buffer;
	dataNode->node_info.rids = (RID*)(dataNode->buffer + indexHandle->fileHeader.keyLength * indexHandle->fileHeader.order);

	UnpinPage(&pageHandle);
	return SUCCESS;
}

/*��ָ��ҳд��ڵ�����*/
RC unmap(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, PageNum pageNum)
{
	if (indexHandle->bOpen == false)
		return FAIL;

	PF_PageHandle pageHandle;
	GetThisPage(&indexHandle->fileHandle, pageNum, &pageHandle);

	char* pData;
	GetData(&pageHandle, &pData);
	memcpy(getIxNodeEntry(pData), &dataNode->node_info, sizeof(IX_Node));
	memcpy(getKeyAreaEntry(pData), dataNode->buffer, BUFFER_SIZE);
	MarkDirty(&pageHandle);

	UnpinPage(&pageHandle);
	return SUCCESS;
}

/*ˢ��header��Ϣ��disk*/
RC flashHeaderToDisk(IX_IndexHandle* indexHandle)
{
	if (indexHandle->bOpen == false)
		return FAIL;

	PF_PageHandle pageHandle;
	char* pData;
	GetThisPage(&indexHandle->fileHandle, 1, &pageHandle);
	GetData(&pageHandle, &pData);
	memcpy(pData, &indexHandle->fileHeader, sizeof(IX_FileHeader));
	MarkDirty(&pageHandle);

	UnpinPage(&pageHandle);

	return SUCCESS;
}

/*�����ؼ������һ��������ҳ��*/
PageNum search_index(IX_IndexHandle* indexHandle, const char* key)
{
	int height = indexHandle->fileHeader.height - 1;
	PageNum org = indexHandle->fileHeader.rootPage;

	while (height > 1)
	{
		IX_DataNode dataNode;
		map(indexHandle, &dataNode, org);
		int index = upper_bound(indexHandle, &dataNode, key, 0, dataNode.node_info.keynum - 1);
		org = dataNode.node_info.rids[index].pageNum;

		--height;
	}

	return org;
}

/*���ݸ��ڵ�ҳ�ţ�����Ҷ�ӽڵ�ҳ��*/
PageNum search_leaf_with_parent(IX_IndexHandle* indexHandle, PageNum parent, const char* key)
{
	IX_DataNode parentNode;
	map(indexHandle, &parentNode, parent);
	int index = upper_bound(indexHandle, &parentNode, key, 0, parentNode.node_info.keynum - 1);

	return parentNode.node_info.rids[index].pageNum;
}

/*����Ҷ�ӽڵ�ҳ��*/
PageNum search_leaf(IX_IndexHandle* indexHandle, const char* key)
{
	PageNum parent = search_index(indexHandle, key);
	return search_leaf_with_parent(indexHandle, parent, key);
}

/*���dataNode �ؼ�������indexλ�ô���ָ��*/
inline char* getKey(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int index)
{
	return dataNode->node_info.keys + indexHandle->fileHeader.keyLength * index;
}

/*���dataNode RID����index����ָ��*/
inline RID* getRID(IX_DataNode* dataNode, int index)
{
	return dataNode->node_info.rids + index;
}

/*�����ֵ*/
void assignment(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int index, const char* key, const RID* rid)
{
	memcpy(getKey(indexHandle, dataNode, index), key, indexHandle->fileHeader.keyLength);
	dataNode->node_info.rids[index] = *rid;
}

/*��[first, last)��Χ�ڵ�Ԫ�ظ��Ƶ�[auto, d_last), lengthΪԪ�س���*/
void* copy_backward(void* first, void* last, void* d_last, int length)
{
	char* _first = (char*)first;
	char* _last = (char*)last;
	char* _d_last = (char*)d_last;

	while (_first != _last)
	{
		_last -= length;
		_d_last -= length;

		memcpy(_d_last, _last, length);
	}

	return _d_last;
}

/*��[first, last)��Χ�ڵ�Ԫ�ظ��Ƶ�[d_first, auto), lengthΪԪ�س���*/
void* copy(void* first, void* last, void* d_first, int length)
{
	char* _first = (char*)first;
	char* _last = (char*)last;
	char* _d_first = (char*)d_first;

	while (_first != _last)
	{
		memcpy(_d_first, _first, length);

		_first += length;
		_d_first += length;
	}

	return _d_first;
}

/*��dataNode��[first, last)��Χ�ڵ�Ԫ�ظ��Ƶ�[auto, d_last), lengthΪԪ�س���*/
int copy_backward_node(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int first, int last, int d_last)
{
	copy_backward(getKey(indexHandle, dataNode, first),
		getKey(indexHandle, dataNode, last),
		getKey(indexHandle, dataNode, d_last),
		indexHandle->fileHeader.keyLength);

	copy_backward(dataNode->node_info.rids + first,
		dataNode->node_info.rids + last,
		dataNode->node_info.rids + d_last,
		sizeof(RID));

	return d_last - (last - first);
}

/*��dtatNode��[first, last)��Χ�ڵ�Ԫ�ظ��Ƶ�[d_first, auto), lengthΪԪ�س���*/
int copy_node(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int first, int last, int d_first)
{
	copy(getKey(indexHandle, dataNode, first),
		getKey(indexHandle, dataNode, last),
		getKey(indexHandle, dataNode, d_first),
		indexHandle->fileHeader.keyLength);

	copy(dataNode->node_info.rids + first,
		dataNode->node_info.rids + last,
		dataNode->node_info.rids + d_first,
		sizeof(RID));

	return d_first + (last - first);
}

/*����һ��ҳ��������ҳ��*/
PageNum allocPage(IX_IndexHandle* indexHandle)
{
	PF_PageHandle newPageHandle;
	PageNum newPageNum;
	AllocatePage(&indexHandle->fileHandle, &newPageHandle);
	GetPageNum(&newPageHandle, &newPageNum);
	UnpinPage(&newPageHandle);

	return newPageNum;
}

/*����һ���µ����ڵ�*/
void create_node(IX_IndexHandle* indexHandle, PageNum nodePageNum, IX_DataNode* node, IX_DataNode* newNode)
{
	newNode->node_info.is_leaf = node->node_info.is_leaf;
	newNode->node_info.parent = node->node_info.parent;
	newNode->node_info.next = node->node_info.next;
	newNode->node_info.prev = nodePageNum;
	newNode->node_info.keys = newNode->buffer;
	newNode->node_info.rids = (RID*)(newNode->buffer + indexHandle->fileHeader.keyLength * indexHandle->fileHeader.order);
	node->node_info.next = allocPage(indexHandle);

	if (newNode->node_info.next != 0)
	{
		IX_DataNode old_node;
		map(indexHandle, &old_node, newNode->node_info.next);
		old_node.node_info.prev = node->node_info.next;
		unmap(indexHandle, &old_node, newNode->node_info.next);
	}
}

void remove_node(IX_IndexHandle* indexHandle, IX_DataNode* prev, IX_DataNode* node)
{
	DisposePage(&indexHandle->fileHandle, prev->node_info.next);
	prev->node_info.next = node->node_info.next;

	if (node->node_info.next != 0)
	{
		IX_DataNode temp;
		map(indexHandle, &temp, node->node_info.next);
		temp.node_info.prev = node->node_info.prev;
		unmap(indexHandle, &temp, node->node_info.next);
	}
}

/*�ڲ���Ҫ���ѽڵ������²����¼��Ҷ�ӽڵ�*/
void insert_record_no_split(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, const RID* rid)
{
	int index = upper_bound(indexHandle, dataNode, key, 0, dataNode->node_info.keynum);
	copy_backward_node(indexHandle, dataNode, index, dataNode->node_info.keynum, dataNode->node_info.keynum + 1);

	/*�����¼*/
	memcpy(getKey(indexHandle, dataNode, index), key, indexHandle->fileHeader.keyLength);
	dataNode->node_info.rids[index] = *rid;

	++dataNode->node_info.keynum;
}

/*�ڲ���Ҫ���ѽڵ������²���������ڲ��ڵ�*/
void insert_index_no_split(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, const RID* rid)
{
	int index = upper_bound(indexHandle, dataNode, key, 0, dataNode->node_info.keynum - 1);
	copy_backward_node(indexHandle, dataNode, index, dataNode->node_info.keynum, dataNode->node_info.keynum + 1);

	/*����������, �¹ؼ���ָ��ɵ�ֵ����һ���ؼ���ָ���µ�ֵ*/
	memcpy(getKey(indexHandle, dataNode, index), key, indexHandle->fileHeader.keyLength);
	dataNode->node_info.rids[index] = dataNode->node_info.rids[index + 1];
	dataNode->node_info.rids[index + 1] = *rid;
}

/*���ýڵ�[begin, end)��Χ�ڵ��ӽڵ��parent*/
void reset_children_parent(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int begin, int end, PageNum parent)
{
	IX_DataNode child;

	while (begin != end)
	{
		PageNum pageNum = dataNode->node_info.rids[begin].pageNum;
		map(indexHandle, &child, pageNum);
		child.node_info.parent = parent;
		unmap(indexHandle, &child, pageNum);

		++begin;
	}
}

/*����������ڲ��ڵ�*/
void insert_index(IX_IndexHandle* indexHandle, PageNum target, const char* key, PageNum old, PageNum after)
{
	if (target == 0)
	{
		IX_DataNode newRoot;
		initDataNode(indexHandle, &newRoot);

		memcpy(getKey(indexHandle, &newRoot, 0), key, indexHandle->fileHeader.keyLength);
		newRoot.node_info.rids[0].pageNum = old;
		newRoot.node_info.rids[1].pageNum = after;
		newRoot.node_info.keynum = 2;

		/*���Ѹ��ڵ㣬�����µĸ��ڵ�*/
		PF_PageHandle newRootHandle;
		PageNum newRootPageNum;
		AllocatePage(&indexHandle->fileHandle, &newRootHandle);
		GetPageNum(&newRootHandle, &newRootPageNum);
		UnpinPage(&newRootHandle);

		++indexHandle->fileHeader.height;
		indexHandle->fileHeader.rootPage = newRootPageNum;

		unmap(indexHandle, &newRoot, newRootPageNum);
		flashHeaderToDisk(indexHandle);

		reset_children_parent(indexHandle, &newRoot, 0, 2, newRootPageNum);
	}
	else
	{
		IX_DataNode targetNode;
		map(indexHandle, &targetNode, target);

		if (targetNode.node_info.keynum == indexHandle->fileHeader.order)
		{
			/*�ڵ���������Ҫ���ѽڵ�*/
			IX_DataNode newNode;
			create_node(indexHandle, target, &targetNode, &newNode);

			int point = (targetNode.node_info.keynum - 1) / 2;
			/*���ݲ���ؼ�����ָ��Ĵ�С����ȷ������ؼ��������Ľڵ�*/
			bool place_right = key_cmp(indexHandle, getKey(indexHandle, &targetNode, point), key);

			/*ƽ�������½ڵ�Ĺؼ��ָ���*/
			if (place_right)
				++point;

			/*������������������ֵܱ��ָ�����ڵ�*/
			if (place_right && key_cmp(indexHandle, key, getKey(indexHandle, &targetNode, point)))
				--point;

			char* middle_key = (char*)malloc(indexHandle->fileHeader.keyLength);
			if (middle_key == NULL)
				return;
			memcpy(middle_key, getKey(indexHandle, &targetNode, point), indexHandle->fileHeader.keyLength);

			/*�����ؼ��ֵ��½ڵ�*/
			copy(getKey(indexHandle, &targetNode, point + 1),
				getKey(indexHandle, &targetNode, targetNode.node_info.keynum),
				getKey(indexHandle, &newNode, 0),
				indexHandle->fileHeader.keyLength);

			/*����rid���½ڵ�*/
			copy(targetNode.node_info.rids + (point + 1),
				targetNode.node_info.rids + targetNode.node_info.keynum,
				newNode.node_info.rids,
				sizeof(RID));

			newNode.node_info.keynum = targetNode.node_info.keynum - point - 1;
			targetNode.node_info.keynum = point + 1;

			RID rid;
			/*�����¹ؼ���*/
			if (place_right)
				insert_index_no_split(indexHandle, &newNode, key, initRid(&rid, after, 0));
			else
				insert_index_no_split(indexHandle, &targetNode, key, initRid(&rid, after, 0));

			/*д�ش���*/
			unmap(indexHandle, &targetNode, target);
			unmap(indexHandle, &newNode, targetNode.node_info.next);

			/*�����ӽڵ�parent*/
			reset_children_parent(indexHandle, &newNode, 0, newNode.node_info.keynum, targetNode.node_info.next);

			/*�ݹ����*/
			insert_index(indexHandle, targetNode.node_info.parent, middle_key, target, targetNode.node_info.next);
			free(middle_key);
		}
		else
		{
			/*����Ҫ���ѽڵ�*/
			RID rid;
			insert_index_no_split(indexHandle, &targetNode, key, initRid(&rid, after, 0));
			unmap(indexHandle, &targetNode, target);
		}
	}
}

/*���¸��ڵ�������Ϣ*/
void reset_parent_children(IX_IndexHandle* indexHandle, PageNum parent, const char* target, const char* value)
{
	IX_DataNode dataNode;
	map(indexHandle, &dataNode, parent);

	int index = upper_bound(indexHandle, &dataNode, target, 0, dataNode.node_info.keynum - 1);
	memcpy(getKey(indexHandle, &dataNode, index), value, indexHandle->fileHeader.keyLength);
	unmap(indexHandle, &dataNode, parent);

	if (index == dataNode.node_info.keynum - 1)
	{
		reset_parent_children(indexHandle, dataNode.node_info.parent, target, value);
	}
}

/*Ҷ�ӽڵ���ùؼ���*/
bool borrow_key(IX_IndexHandle* indexHandle, bool from_right, IX_DataNode* borrowerNode)
{
	PageNum lender;
	if (from_right)
		lender = borrowerNode->node_info.next;
	else
		lender = borrowerNode->node_info.prev;

	if (lender == 0)
		return false;

	IX_DataNode lenderNode;
	map(indexHandle, &lenderNode, lender);
	
	if (lenderNode.node_info.keynum <= halfOrder(indexHandle->fileHeader.order))
		return false;

	int where_to_lend, where_to_put;
	if (from_right)
	{
		where_to_lend = 0;
		where_to_put = borrowerNode->node_info.keynum;

		reset_parent_children(indexHandle,
			borrowerNode->node_info.parent,
			getKey(indexHandle, borrowerNode, 0),
			getKey(indexHandle, &lenderNode, 1));
	}
	else
	{
		where_to_lend = lenderNode.node_info.keynum - 1;
		where_to_put = 0;

		reset_parent_children(indexHandle,
			lenderNode.node_info.parent,
			getKey(indexHandle, &lenderNode, 0),
			getKey(indexHandle, &lenderNode, lenderNode.node_info.keynum - 1));
	}

	/*����*/
	copy_backward_node(indexHandle, borrowerNode, where_to_put, 
		borrowerNode->node_info.keynum, borrowerNode->node_info.keynum + 1);

	assignment(indexHandle, borrowerNode, where_to_put,
		getKey(indexHandle, &lenderNode, where_to_lend), &lenderNode.node_info.rids[where_to_lend]);
	++borrowerNode->node_info.keynum;

	/*ɾ��*/
	copy_node(indexHandle, &lenderNode, where_to_lend + 1, lenderNode.node_info.keynum, where_to_lend);
	--lenderNode.node_info.keynum;
	unmap(indexHandle, &lenderNode, lender);

	return true;
}

/*�ڲ��ڵ���ùؼ���*/
bool borrow_key_index(IX_IndexHandle* indexHandle, bool from_right, IX_DataNode* borrowerNode, PageNum offset)
{
	PageNum lender = from_right ? borrowerNode->node_info.next : borrowerNode->node_info.prev;
	if (lender == 0)
		return false;

	IX_DataNode lenderNode;
	map(indexHandle, &lenderNode, lender);

	if (lenderNode.node_info.keynum <= halfOrder(indexHandle->fileHeader.order))
		return false;

	int where_to_lend, where_to_put;
	if (from_right)
	{
		where_to_lend = 0;
		where_to_put = borrowerNode->node_info.keynum;

		IX_DataNode parentNode;
		map(indexHandle, &parentNode, borrowerNode->node_info.parent);

		int index = lower_bound(indexHandle, &parentNode, 
			getKey(indexHandle, borrowerNode, borrowerNode->node_info.keynum - 1), 
			0, parentNode.node_info.keynum - 1);

		memcpy(getKey(indexHandle, &parentNode, index), 
			getKey(indexHandle, &lenderNode, where_to_lend), 
			indexHandle->fileHeader.keyLength);

		unmap(indexHandle, &parentNode, borrowerNode->node_info.parent);
	}
	else
	{
		where_to_lend = lenderNode.node_info.keynum - 1;
		where_to_put = 0;

		IX_DataNode parentNode;
		map(indexHandle, &parentNode, lenderNode.node_info.parent);

		int index = upper_bound(indexHandle, &parentNode, 
			getKey(indexHandle, &lenderNode, 0), 
			0, parentNode.node_info.keynum - 1);

		memcpy(getKey(indexHandle, &parentNode, index),
			getKey(indexHandle, &lenderNode, where_to_lend - 1),
			indexHandle->fileHeader.keyLength);

		unmap(indexHandle, &parentNode, borrowerNode->node_info.parent);
	}

	/*����*/
	copy_backward_node(indexHandle, borrowerNode, where_to_put,
		borrowerNode->node_info.keynum, borrowerNode->node_info.keynum + 1);

	assignment(indexHandle, borrowerNode, where_to_put,
		getKey(indexHandle, &lenderNode, where_to_lend), &lenderNode.node_info.rids[where_to_lend]);
	++borrowerNode->node_info.keynum;

	/*���½��ýڵ��parent*/
	reset_children_parent(indexHandle, &lenderNode, where_to_lend, where_to_lend + 1, offset);

	/*ɾ��*/
	copy_node(indexHandle, &lenderNode, where_to_lend + 1, lenderNode.node_info.keynum, where_to_lend);
	--lenderNode.node_info.keynum;
	unmap(indexHandle, &lenderNode, lender);

	return true;
}

/*�ϲ��ڵ�*/
void merge_node(IX_IndexHandle* indexHandle, IX_DataNode* left, IX_DataNode* right)
{
	copy(getKey(indexHandle, right, 0),
		getKey(indexHandle, right, right->node_info.keynum),
		getKey(indexHandle, left, left->node_info.keynum),
		indexHandle->fileHeader.keyLength);

	copy(right->node_info.rids,
		right->node_info.rids + right->node_info.keynum,
		left->node_info.rids + left->node_info.keynum,
		sizeof(RID));

	left->node_info.keynum += right->node_info.keynum;
}

/*���ڲ��ڵ�ɾ������*/
void remove_from_index(IX_IndexHandle* indexHandle, PageNum target, IX_DataNode* targetNode, const char* key)
{
	int min_order = (target == indexHandle->fileHeader.rootPage) ? 2 : halfOrder(indexHandle->fileHeader.order);
	char* index_key = (char*)malloc(indexHandle->fileHeader.keyLength);
	if (index_key == NULL)
		exit(-1);

	memcpy(index_key, getKey(indexHandle, targetNode, 0), indexHandle->fileHeader.keyLength);
	int index = upper_bound(indexHandle, targetNode, key, 0, targetNode->node_info.keynum - 1);
	targetNode->node_info.rids[index + 1] = targetNode->node_info.rids[index];
	copy_node(indexHandle, targetNode, index + 1, targetNode->node_info.keynum, index);
	--targetNode->node_info.keynum;

	if (target == indexHandle->fileHeader.rootPage && targetNode->node_info.keynum == 1)
	{
		IX_DataNode newRoot;
		map(indexHandle, &newRoot, targetNode->node_info.rids[0].pageNum);
		newRoot.node_info.parent = 0;
		unmap(indexHandle, &newRoot, targetNode->node_info.rids[0].pageNum);

		DisposePage(&indexHandle->fileHandle, indexHandle->fileHeader.rootPage);
		indexHandle->fileHeader.rootPage = targetNode->node_info.rids[0].pageNum;
		--indexHandle->fileHeader.height;

		flashHeaderToDisk(indexHandle);
		free(index_key);

		return;
	}

	if (targetNode->node_info.keynum < min_order)
	{
		bool borrowed = false;
		IX_DataNode parentNode;
		map(indexHandle, &parentNode, targetNode->node_info.parent);
		/*���ҽ���*/
		if (target != getRID(&parentNode, parentNode.node_info.keynum - 1)->pageNum)
		{
			borrowed = borrow_key_index(indexHandle, true, targetNode, target);
		}

		/*�������*/
		if (!borrowed && target != getRID(&parentNode, 0)->pageNum)
		{
			borrowed = borrow_key_index(indexHandle, false, targetNode, target);
		}

		/*����ʧ����ϲ��ڵ�*/
		if (!borrowed)
		{
			/*���Һϲ�*/
			if (target != getRID(&parentNode, parentNode.node_info.keynum - 1)->pageNum)
			{
				int next = targetNode->node_info.next;
				IX_DataNode nextNode;
				map(indexHandle, &nextNode, next);

				merge_node(indexHandle, targetNode, &nextNode);
				remove_node(indexHandle, targetNode, &nextNode);
				unmap(indexHandle, targetNode, target);
			}
			else //����ϲ�
			{
				int prev = targetNode->node_info.prev;
				IX_DataNode prevNode;
				map(indexHandle, &prevNode, prev);
				memcpy(index_key, getKey(indexHandle, &prevNode, 0), indexHandle->fileHeader.keyLength);

				merge_node(indexHandle, &prevNode, targetNode);
				remove_node(indexHandle, &prevNode, targetNode);
				unmap(indexHandle, &prevNode, prev);
			}

			remove_from_index(indexHandle, targetNode->node_info.parent, &parentNode, index_key);
		}
		else
			unmap(indexHandle, targetNode, target);
	}
	else
		unmap(indexHandle, targetNode, target);

	free(index_key);
}

RC OpenIndexScan(IX_IndexScan *indexScan,IX_IndexHandle *indexHandle,CompOp compOp,char *value){
	if (indexScan->bOpen)
		return SUCCESS;
	
	if (!indexHandle->bOpen)
		return FAIL;

	indexScan->bOpen = true;
	indexScan->compOp = compOp;
	indexScan->value = (char*)malloc(indexHandle->fileHeader.attrLength);
	indexScan->indexHandle = indexHandle;

	if (indexScan->value == NULL)
		return FAIL;
	memcpy(indexScan->value, value, indexHandle->fileHeader.attrLength);

	/*
		EQual,			//"="			0
		LEqual,			//"<="          1
		NEqual,			//"<>"			2
		LessT,			//"<"			3
		GEqual,			//">="			4
		GreatT,			//">"           5
		NO_OP
	*/

	if (compOp == EQual || compOp == GEqual || compOp == GreatT || compOp == NEqual)
	{
		char* key = (char*)malloc(indexHandle->fileHeader.keyLength);
		if (key == NULL)
			return FAIL;

		/* ��RID��ʼ��Ϊ��Сֵ���ɶ�λ������ֵΪvalue�����е�����ࣻ
		** ��RID��ʼ��Ϊ���ֵ���ɶ�λ������ֵΪvalue�����е����Ҳ�
		*/
		if(compOp == GreatT || compOp == NEqual)
			initKey(key, value, indexHandle->fileHeader.attrLength, (PageNum)0xffffffff, (SlotNum)0x7fffffff);
		else
			initKey(key, value, indexHandle->fileHeader.attrLength, 0, 0);

		int target;
		if (indexHandle->fileHeader.height == 1)
			target = indexHandle->fileHeader.rootPage;
		else
			target = search_leaf(indexHandle, key);
		IX_DataNode targetNode;
		map(indexHandle, &targetNode, target);

		int index = upper_bound(indexHandle, &targetNode, key, 0, targetNode.node_info.keynum);

		if (compOp == NEqual)
		{
			/*�ڲ����ڲ����У�ɨ�������ʵ�ʱ�����ֵ��ȵĲ��ַָ�Ϊ�������У������Ҫ���Ᵽ��ڶ������еĳ�ʼλ��*/
			indexScan->pnNext = indexHandle->fileHeader.first_leaf;
			indexScan->ridIx = 0;
			map(indexHandle, &indexScan->currentNode, indexScan->pnNext);

			if (index == targetNode.node_info.keynum)
			{
				/*�������Ϊ�ڵ�Ľ�β����λ����һ�ڵ�Ŀ�ʼλ��*/
				indexScan->pnSkip = targetNode.node_info.next;
				indexScan->ridIxSkip = 0;
			}
			else
			{
				indexScan->pnSkip = target;
				indexScan->ridIxSkip = index;
			}
		}
		else
		{
			if (index == targetNode.node_info.keynum)
			{
				/*�������Ϊ�ڵ�Ľ�β����λ����һ�ڵ�Ŀ�ʼλ��*/
				indexScan->pnNext = targetNode.node_info.next;
				indexScan->ridIx = 0;

				if (indexScan->pnNext != 0)
					map(indexHandle, &indexScan->currentNode, indexScan->pnNext);
			}
			else
			{
				indexScan->pnNext = target;
				indexScan->ridIx = index;
				indexScan->currentNode = targetNode;
			}
		}

		free(key);
	}
	else if (compOp == LEqual || compOp == LessT || compOp == NO_OP)
	{
		indexScan->pnNext = indexHandle->fileHeader.first_leaf;
		indexScan->ridIx = 0;
		map(indexHandle, &indexScan->currentNode, indexScan->pnNext);
	}
	else
	{
		return FAIL;
	}

	return SUCCESS;
}

RC IX_GetNextEntry(IX_IndexScan *indexScan,RID * rid){
	if (!indexScan->bOpen)
	{
		return FAIL;
	}

	IX_IndexHandle* indexHandle = indexScan->indexHandle;
	IX_DataNode* curNode = &indexScan->currentNode;
	PageNum pn = indexScan->pnNext;
	int index = indexScan->ridIx;
	CompOp compOp = indexScan->compOp;

	if (pn == 0)
		return IX_EOF;

	/*
	EQual,			//"="			0
	LEqual,			//"<="          1
	NEqual,			//"<>"			2
	LessT,			//"<"			3
	GEqual,			//">="			4
	GreatT,			//">"           5
	NO_OP
    */

	if (compOp == NEqual)
	{
		/* ���ڲ����ڱȽϷ�
		** 
		*/
		if (!attr_equal(indexHandle, indexScan->value, getKey(indexHandle, curNode, index)))
		{
			*rid = curNode->node_info.rids[index];
		}
		else
		{
			indexScan->pnNext = indexScan->pnSkip;
			indexScan->ridIx = indexScan->ridIxSkip;
			if (indexScan->pnNext != 0)
				map(indexHandle, &indexScan->currentNode, indexScan->pnNext);

			return IX_GetNextEntry(indexScan, rid);
		}

		++index;
		if (index == curNode->node_info.keynum)
		{
			indexScan->pnNext = curNode->node_info.next;
			index = 0;

			if (indexScan->pnNext != 0)
				map(indexHandle, curNode, indexScan->pnNext);
		}
		else
		{
			indexScan->ridIx = index;
		}

		return SUCCESS;
	}
	else
	{
		bool eof = false;

		if (compOp == EQual)
		{
			if (attr_equal(indexHandle, indexScan->value, getKey(indexHandle, curNode, index)))
				*rid = curNode->node_info.rids[index];
			else
				eof = true;
		}
		else if (compOp == LEqual)
		{
			if (!attr_cmp(indexHandle, indexScan->value, getKey(indexHandle, curNode, index)))
				*rid = curNode->node_info.rids[index];
			else
				eof = true;
		}
		else if (compOp == LessT)
		{
			if (attr_cmp(indexHandle, getKey(indexHandle, curNode, index), indexScan->value))
				*rid = curNode->node_info.rids[index];
			else
				eof = true;
		}
		else if (compOp == GEqual || compOp == GreatT || compOp == NO_OP)
		{
			*rid = curNode->node_info.rids[index];
		}
		else
		{
			return FAIL;
		}

		if (eof)
			return IX_EOF;
		else
		{
			++index;
			if (index == curNode->node_info.keynum)
			{
				indexScan->pnNext = curNode->node_info.next;
				index = 0;

				if (indexScan->pnNext != 0)
					map(indexHandle, curNode, indexScan->pnNext);
			}
			else
			{
				indexScan->ridIx = index;
			}
			return SUCCESS;
		}
	}
}

RC CloseIndexScan(IX_IndexScan *indexScan){
	if (!indexScan->bOpen)
		return SUCCESS;

	free(indexScan->value);

	return SUCCESS;
}

RC GetIndexTree(char *fileName, Tree *index){
	return SUCCESS;
}

RC CreateIndex(char* fileName, AttrType attrType, int attrLength)
{
	/*Ϊ����������ҳ�ļ�,����*/
	CreateFile(fileName);
	PF_FileHandle fileHandle;
	openFile(fileName, &fileHandle);

	/*��ʼ��B+Tree*/
	PF_PageHandle rootPageHandle;
	PageNum rootPageNum;
	AllocatePage(&fileHandle, &rootPageHandle);  //Ϊ���ڵ����һ��page
	GetPageNum(&rootPageHandle, &rootPageNum);

	if (rootPageNum != 1)
		return FAIL;

	/*��ʼ������������Ϣ*/
	IX_FileHeader header;
	header.attrLength = attrLength;
	header.attrType = attrType;
	header.keyLength = attrLength + sizeof(RID);
	header.order = (PF_PAGE_SIZE - sizeof(IX_FileHeader) - sizeof(IX_Node)) / (2 * sizeof(RID) + attrLength);
	header.height = 1;
	header.rootPage = rootPageNum;
	header.first_leaf = rootPageNum;

	/*��ʼ��B+Tee���ڵ���Ϣ*/
	IX_Node rootNode;
	rootNode.is_leaf = 1;
	rootNode.next = 0;
	rootNode.prev = 0;
	rootNode.parent = 0;
	rootNode.keynum = 0;
	rootNode.keys = NULL;
	rootNode.rids = NULL;


	/*���������ָ��*/
	char* pData;
	GetData(&rootPageHandle, &pData);

	/*д������������Ϣ*/
	memcpy(pData, &header, sizeof(IX_FileHeader));
	/*д����ڵ�IX_Node��Ϣ*/
	memcpy(getIxNodeEntry(pData), &rootNode, sizeof(IX_Node));

	MarkDirty(&rootPageHandle);
	UnpinPage(&rootPageHandle);

	return SUCCESS;
}

RC OpenIndex(char* fileName, IX_IndexHandle* indexHandle)
{
	PF_FileHandle fileHandle;
	if (openFile(fileName, &fileHandle) == SUCCESS)
	{
		indexHandle->bOpen = true;
		indexHandle->fileHandle = fileHandle;

		/*�������������Ϣ*/
		PF_PageHandle pageHandle;
		char* pData;

		GetThisPage(&fileHandle, 1, &pageHandle);
		GetData(&pageHandle, &pData);
		memcpy(&indexHandle->fileHeader, pData, sizeof(IX_FileHeader));

		UnpinPage(&pageHandle);

		return SUCCESS;
	}
	else
		return FAIL;
}

RC CloseIndex(IX_IndexHandle* indexHandle)
{
	if (indexHandle->bOpen == false)
		return SUCCESS;

	flashHeaderToDisk(indexHandle);
	CloseFile(&(indexHandle->fileHandle));

	return SUCCESS;
}

RC InsertEntry(IX_IndexHandle* indexHandle, void* pData, const RID* rid)
{
	if (indexHandle->bOpen == false)
		return FAIL;

	IX_FileHeader* fileHeader = &indexHandle->fileHeader;
	char* key = (char*)malloc(fileHeader->keyLength);
	if (key != NULL)
	{
		memcpy(key, pData, fileHeader->attrLength);
		memcpy(key + fileHeader->attrLength, rid, sizeof(RID));

		PageNum leafPageNum;
		IX_DataNode leafNode;
		if (indexHandle->fileHeader.height == 1)
			leafPageNum = indexHandle->fileHeader.rootPage;
		else
			leafPageNum = search_leaf(indexHandle, key);
		map(indexHandle, &leafNode, leafPageNum);


		if (leafNode.node_info.keynum == indexHandle->fileHeader.order)
		{
			/*Ҷ�ڵ���������Ҫ���ѽڵ�*/
			IX_DataNode newNode;
			create_node(indexHandle, leafPageNum, &leafNode, &newNode);

			int point = leafNode.node_info.keynum / 2;
			/*���ݲ���ؼ�����ָ��Ĵ�С����ȷ������ؼ��������Ľڵ�*/
			bool place_right = key_cmp(indexHandle, getKey(indexHandle, &leafNode, point), key);

			/*ƽ�������½ڵ�Ĺؼ��ָ���*/
			if (place_right)
				++point;
			
			/*�����ؼ��ֵ��½ڵ�*/
			copy(getKey(indexHandle, &leafNode, point), 
				getKey(indexHandle, &leafNode, leafNode.node_info.keynum), 
				getKey(indexHandle, &newNode, 0), 
				indexHandle->fileHeader.keyLength);

			/*����rid���½ڵ�*/
			copy(leafNode.node_info.rids + point,
				leafNode.node_info.rids + leafNode.node_info.keynum,
				newNode.node_info.rids,
				sizeof(RID));

			newNode.node_info.keynum = leafNode.node_info.keynum - point;
			leafNode.node_info.keynum = point;

			/*�����¹ؼ���*/
			if (place_right)
				insert_record_no_split(indexHandle, &newNode, key, rid);
			else
				insert_record_no_split(indexHandle, &leafNode, key, rid);

			/*д�ش���*/
			unmap(indexHandle, &leafNode, leafPageNum);
			unmap(indexHandle, &newNode, leafNode.node_info.next);

			/*����������*/
			insert_index(indexHandle, leafNode.node_info.parent, getKey(indexHandle, &newNode, 0), leafPageNum, leafNode.node_info.next);
		}
		else
		{
			/*Ҷ�ڵ�δ����ֱ�Ӳ���*/
			insert_record_no_split(indexHandle, &leafNode, key, rid);
			unmap(indexHandle, &leafNode, leafPageNum);
		}

		free(key);
	}
	else
		return FAIL;

	return SUCCESS;
}

RC DeleteEntry(IX_IndexHandle* indexHandle, void* pData, const RID* rid)
{
	if (!indexHandle->bOpen)
		return FAIL;

	char* key = (char*)malloc(indexHandle->fileHeader.keyLength);
	initKey(key, (char*)pData, indexHandle->fileHeader.attrLength, rid->pageNum, rid->slotNum);

	/*��ֻ��һ��ڵ�ʱ��Ҫ���⴦��*/
	if (indexHandle->fileHeader.height == 1)
	{
		IX_DataNode targetNode;
		map(indexHandle, &targetNode, indexHandle->fileHeader.rootPage);

		int index = lower_bound(indexHandle, &targetNode, key, 0, targetNode.node_info.keynum);
		if (index == targetNode.node_info.keynum || !key_equal(indexHandle, getKey(indexHandle, &targetNode, index), key))
			return FAIL;
		else
		{
			copy_node(indexHandle, &targetNode, index + 1, targetNode.node_info.keynum, index);

			--targetNode.node_info.keynum;
			unmap(indexHandle, &targetNode, indexHandle->fileHeader.rootPage);

			return SUCCESS;
		}
	}

	PageNum parent = search_index(indexHandle, key);
	IX_DataNode parentNode;
	map(indexHandle, &parentNode, parent);
	int where = upper_bound(indexHandle, &parentNode, key, 0, parentNode.node_info.keynum - 1);
	int target = parentNode.node_info.rids[where].pageNum;

	IX_DataNode targetNode;
	map(indexHandle, &targetNode, target);

	int index = lower_bound(indexHandle, &targetNode, key, 0, targetNode.node_info.keynum);
	if (index == targetNode.node_info.keynum || !key_equal(indexHandle, getKey(indexHandle, &targetNode, index), key))
		return FAIL;
	else
	{
		copy_node(indexHandle, &targetNode, index + 1, targetNode.node_info.keynum, index);

		--targetNode.node_info.keynum;
		
		/*���ڽڵ�Ĺؼ�����ĿС�����ޣ���Ҫ����ߺϲ�*/
		if (targetNode.node_info.keynum < halfOrder(indexHandle->fileHeader.order))
		{
			bool borrowed = false;
			if (targetNode.node_info.next != 0)
				borrowed = borrow_key(indexHandle, true, &targetNode);

			if (targetNode.node_info.prev != 0 && !borrowed)
				borrowed = borrow_key(indexHandle, false, &targetNode);

			/*����ʧ�ܣ���Ҫ�ϲ�*/
			if (!borrowed)
			{
				char* index_key = (char*)malloc(indexHandle->fileHeader.keyLength);
				if (index_key == NULL)
					return FAIL;

				/*������ֵܺϲ�*/
				if (where == parentNode.node_info.keynum - 1)
				{
					IX_DataNode brotherNode;
					map(indexHandle, &brotherNode, targetNode.node_info.prev);
					memcpy(index_key, getKey(indexHandle, &brotherNode, 0), indexHandle->fileHeader.keyLength);

					merge_node(indexHandle, &brotherNode, &targetNode);
					remove_node(indexHandle, &brotherNode, &targetNode);
					unmap(indexHandle, &brotherNode, targetNode.node_info.prev);
				}
				else /*���Ҳ��ֵܺϲ�*/
				{
					IX_DataNode brotherNode;
					map(indexHandle, &brotherNode, targetNode.node_info.next);
					memcpy(index_key, getKey(indexHandle, &targetNode, 0), indexHandle->fileHeader.keyLength);

					merge_node(indexHandle, &targetNode, &brotherNode);
					remove_node(indexHandle, &targetNode, &brotherNode);
					unmap(indexHandle, &targetNode, target);
				}

				remove_from_index(indexHandle, parent, &parentNode, index_key);
				free(index_key);
			}
			else
				unmap(indexHandle, &targetNode, target);
		}
		else
			unmap(indexHandle, &targetNode, target);
		
	}

	return SUCCESS;
}
