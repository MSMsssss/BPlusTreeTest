#include "IX_Manager.h"
#include <string.h>

inline int halfOrder(int order)
{
	return order / 2;
}

/*返回rid1 < rid2*/
bool rid_cmp(const RID* rid1, const RID* rid2)
{
	if (rid1->pageNum == rid2->pageNum)
		return rid1->slotNum < rid2->slotNum;
	else
		return rid1->pageNum < rid2->pageNum;
}

/*返回attr1 < attr2*/
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

/*返回attr1 == attr2*/
bool attr_equal(IX_IndexHandle* indexHandle, const char* attr1, const char* attr2)
{
	return !attr_cmp(indexHandle, attr1, attr2) && !attr_cmp(indexHandle, attr2, attr1);
}

/*返回key1 < key2*/
bool key_cmp(IX_IndexHandle* indexHandle, const char* key1, const char* key2)
{
	AttrType attrType = indexHandle->fileHeader.attrType;
	int attrLength = indexHandle->fileHeader.attrLength;

	if (attrType == ints)
	{
		//获得属性值
		int attrValue1 = *((int*)key1);
		int attrValue2 = *((int*)key2);

		//获得RID值
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
		//获得属性值
		float attrValue1 = *((float*)key1);
		float attrValue2 = *((float*)key2);

		//获得RID值
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
		//获得属性值
		const char* attrValue1 = key1;
		const char* attrValue2 = key2;

		//获得RID值
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

/*返回key1 == key2*/
bool key_equal(IX_IndexHandle* indexHandle, const char* key1, const char* key2)
{
	return !key_cmp(indexHandle, key1, key2) && !key_cmp(indexHandle, key2, key1);
}

/*范围[begin, end), 返回首个大于等于key的元素的指针偏移，若没有则返回尾指针偏移*/
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

/*范围[begin, end), 返回首个大于key的元素的指针偏移，若没有则返回尾指针偏移*/
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

/*获得IX_Node位置的入口*/
inline char* getIxNodeEntry(char* pData)
{
	return pData + sizeof(IX_FileHeader);
}

/*获得关键字区指针*/
inline char* getKeyAreaEntry(char* pData)
{
	int offset = sizeof(IX_FileHeader) + sizeof(IX_Node);
	return pData + offset;
}

/*获得值区指针*/
inline char* getRidAreaEntry(char* pData, int order, int keyLength)
{
	int offset = sizeof(IX_FileHeader) + sizeof(IX_Node) + keyLength * order;
	return pData + offset;
}

/*初始化数据节点*/
IX_DataNode* initDataNode(IX_IndexHandle* indexHandle, IX_DataNode* dataNode)
{
	dataNode->node_info.keynum = 0;
	dataNode->node_info.is_leaf = 0;
	dataNode->node_info.parent = dataNode->node_info.next = dataNode->node_info.prev = 0;
	dataNode->node_info.keys = dataNode->buffer;
	dataNode->node_info.rids = (RID*)(dataNode->buffer + indexHandle->fileHeader.keyLength * indexHandle->fileHeader.order);

	return dataNode;
}

/*初始化RID*/
RID* initRid(RID* rid, PageNum pageNum, SlotNum slotNum)
{
	rid->bValid = true;
	rid->pageNum = pageNum;
	rid->slotNum = slotNum;

	return rid;
}

/*初始化key*/
char* initKey(char* key, const char* attr, int attrLength, PageNum pageNum, SlotNum slotNum)
{
	memcpy(key, attr, attrLength);
	initRid((RID*)(key + attrLength), pageNum, slotNum);

	return key;
}

/*从指定页读入节点信息*/
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

/*向指定页写入节点数据*/
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

/*刷新header信息到disk*/
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

/*锁定关键字最后一层索引的页号*/
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

/*根据父节点页号，锁定叶子节点页号*/
PageNum search_leaf_with_parent(IX_IndexHandle* indexHandle, PageNum parent, const char* key)
{
	IX_DataNode parentNode;
	map(indexHandle, &parentNode, parent);
	int index = upper_bound(indexHandle, &parentNode, key, 0, parentNode.node_info.keynum - 1);

	return parentNode.node_info.rids[index].pageNum;
}

/*锁定叶子节点页号*/
PageNum search_leaf(IX_IndexHandle* indexHandle, const char* key)
{
	PageNum parent = search_index(indexHandle, key);
	return search_leaf_with_parent(indexHandle, parent, key);
}

/*获得dataNode 关键字数组index位置处的指针*/
inline char* getKey(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int index)
{
	return dataNode->node_info.keys + indexHandle->fileHeader.keyLength * index;
}

/*获得dataNode RID数组index处的指针*/
inline RID* getRID(IX_DataNode* dataNode, int index)
{
	return dataNode->node_info.rids + index;
}

/*索引项赋值*/
void assignment(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, int index, const char* key, const RID* rid)
{
	memcpy(getKey(indexHandle, dataNode, index), key, indexHandle->fileHeader.keyLength);
	dataNode->node_info.rids[index] = *rid;
}

/*将[first, last)范围内的元素复制到[auto, d_last), length为元素长度*/
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

/*将[first, last)范围内的元素复制到[d_first, auto), length为元素长度*/
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

/*将dataNode中[first, last)范围内的元素复制到[auto, d_last), length为元素长度*/
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

/*将dtatNode中[first, last)范围内的元素复制到[d_first, auto), length为元素长度*/
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

/*分配一个页，并返回页号*/
PageNum allocPage(IX_IndexHandle* indexHandle)
{
	PF_PageHandle newPageHandle;
	PageNum newPageNum;
	AllocatePage(&indexHandle->fileHandle, &newPageHandle);
	GetPageNum(&newPageHandle, &newPageNum);
	UnpinPage(&newPageHandle);

	return newPageNum;
}

/*创建一个新的树节点*/
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

/*在不需要分裂节点的情况下插入记录到叶子节点*/
void insert_record_no_split(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, const RID* rid)
{
	int index = upper_bound(indexHandle, dataNode, key, 0, dataNode->node_info.keynum);
	copy_backward_node(indexHandle, dataNode, index, dataNode->node_info.keynum, dataNode->node_info.keynum + 1);

	/*插入记录*/
	memcpy(getKey(indexHandle, dataNode, index), key, indexHandle->fileHeader.keyLength);
	dataNode->node_info.rids[index] = *rid;

	++dataNode->node_info.keynum;
}

/*在不需要分裂节点的情况下插入索引项到内部节点*/
void insert_index_no_split(IX_IndexHandle* indexHandle, IX_DataNode* dataNode, const char* key, const RID* rid)
{
	int index = upper_bound(indexHandle, dataNode, key, 0, dataNode->node_info.keynum - 1);
	copy_backward_node(indexHandle, dataNode, index, dataNode->node_info.keynum, dataNode->node_info.keynum + 1);

	/*插入索引项, 新关键字指向旧的值，后一个关键字指向新的值*/
	memcpy(getKey(indexHandle, dataNode, index), key, indexHandle->fileHeader.keyLength);
	dataNode->node_info.rids[index] = dataNode->node_info.rids[index + 1];
	dataNode->node_info.rids[index + 1] = *rid;
}

/*设置节点[begin, end)范围内的子节点的parent*/
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

/*插入索引项到内部节点*/
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

		/*分裂根节点，创建新的根节点*/
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
			/*节点已满，需要分裂节点*/
			IX_DataNode newNode;
			create_node(indexHandle, target, &targetNode, &newNode);

			int point = (targetNode.node_info.keynum - 1) / 2;
			/*根据插入关键字与分割点的大小，来确定插入关键字所处的节点*/
			bool place_right = key_cmp(indexHandle, getKey(indexHandle, &targetNode, point), key);

			/*平衡两个新节点的关键字个数*/
			if (place_right)
				++point;

			/*避免插入项与它的右兄弟被分割到两个节点*/
			if (place_right && key_cmp(indexHandle, key, getKey(indexHandle, &targetNode, point)))
				--point;

			char* middle_key = (char*)malloc(indexHandle->fileHeader.keyLength);
			if (middle_key == NULL)
				return;
			memcpy(middle_key, getKey(indexHandle, &targetNode, point), indexHandle->fileHeader.keyLength);

			/*拷贝关键字到新节点*/
			copy(getKey(indexHandle, &targetNode, point + 1),
				getKey(indexHandle, &targetNode, targetNode.node_info.keynum),
				getKey(indexHandle, &newNode, 0),
				indexHandle->fileHeader.keyLength);

			/*拷贝rid到新节点*/
			copy(targetNode.node_info.rids + (point + 1),
				targetNode.node_info.rids + targetNode.node_info.keynum,
				newNode.node_info.rids,
				sizeof(RID));

			newNode.node_info.keynum = targetNode.node_info.keynum - point - 1;
			targetNode.node_info.keynum = point + 1;

			RID rid;
			/*插入新关键字*/
			if (place_right)
				insert_index_no_split(indexHandle, &newNode, key, initRid(&rid, after, 0));
			else
				insert_index_no_split(indexHandle, &targetNode, key, initRid(&rid, after, 0));

			/*写回磁盘*/
			unmap(indexHandle, &targetNode, target);
			unmap(indexHandle, &newNode, targetNode.node_info.next);

			/*更新子节点parent*/
			reset_children_parent(indexHandle, &newNode, 0, newNode.node_info.keynum, targetNode.node_info.next);

			/*递归插入*/
			insert_index(indexHandle, targetNode.node_info.parent, middle_key, target, targetNode.node_info.next);
			free(middle_key);
		}
		else
		{
			/*不需要分裂节点*/
			RID rid;
			insert_index_no_split(indexHandle, &targetNode, key, initRid(&rid, after, 0));
			unmap(indexHandle, &targetNode, target);
		}
	}
}

/*更新父节点索引信息*/
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

/*叶子节点借用关键字*/
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

	/*放置*/
	copy_backward_node(indexHandle, borrowerNode, where_to_put, 
		borrowerNode->node_info.keynum, borrowerNode->node_info.keynum + 1);

	assignment(indexHandle, borrowerNode, where_to_put,
		getKey(indexHandle, &lenderNode, where_to_lend), &lenderNode.node_info.rids[where_to_lend]);
	++borrowerNode->node_info.keynum;

	/*删除*/
	copy_node(indexHandle, &lenderNode, where_to_lend + 1, lenderNode.node_info.keynum, where_to_lend);
	--lenderNode.node_info.keynum;
	unmap(indexHandle, &lenderNode, lender);

	return true;
}

/*内部节点借用关键字*/
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

	/*放置*/
	copy_backward_node(indexHandle, borrowerNode, where_to_put,
		borrowerNode->node_info.keynum, borrowerNode->node_info.keynum + 1);

	assignment(indexHandle, borrowerNode, where_to_put,
		getKey(indexHandle, &lenderNode, where_to_lend), &lenderNode.node_info.rids[where_to_lend]);
	++borrowerNode->node_info.keynum;

	/*更新借用节点的parent*/
	reset_children_parent(indexHandle, &lenderNode, where_to_lend, where_to_lend + 1, offset);

	/*删除*/
	copy_node(indexHandle, &lenderNode, where_to_lend + 1, lenderNode.node_info.keynum, where_to_lend);
	--lenderNode.node_info.keynum;
	unmap(indexHandle, &lenderNode, lender);

	return true;
}

/*合并节点*/
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

/*从内部节点删除数据*/
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
		/*向右借用*/
		if (target != getRID(&parentNode, parentNode.node_info.keynum - 1)->pageNum)
		{
			borrowed = borrow_key_index(indexHandle, true, targetNode, target);
		}

		/*向左借用*/
		if (!borrowed && target != getRID(&parentNode, 0)->pageNum)
		{
			borrowed = borrow_key_index(indexHandle, false, targetNode, target);
		}

		/*借用失败则合并节点*/
		if (!borrowed)
		{
			/*向右合并*/
			if (target != getRID(&parentNode, parentNode.node_info.keynum - 1)->pageNum)
			{
				int next = targetNode->node_info.next;
				IX_DataNode nextNode;
				map(indexHandle, &nextNode, next);

				merge_node(indexHandle, targetNode, &nextNode);
				remove_node(indexHandle, targetNode, &nextNode);
				unmap(indexHandle, targetNode, target);
			}
			else //向左合并
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

		/* 将RID初始化为最小值，可定位到属性值为value的序列的最左侧；
		** 将RID初始化为最大值，可定位到属性值为value的序列的最右侧
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
			/*在不等于操作中，扫描的序列实际被属性值相等的部分分割为两个序列，因此需要额外保存第二段序列的初始位置*/
			indexScan->pnNext = indexHandle->fileHeader.first_leaf;
			indexScan->ridIx = 0;
			map(indexHandle, &indexScan->currentNode, indexScan->pnNext);

			if (index == targetNode.node_info.keynum)
			{
				/*如果索引为节点的结尾，则定位到下一节点的开始位置*/
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
				/*如果索引为节点的结尾，则定位到下一节点的开始位置*/
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
		/* 对于不等于比较符
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
	/*为索引创建分页文件,并打开*/
	CreateFile(fileName);
	PF_FileHandle fileHandle;
	openFile(fileName, &fileHandle);

	/*初始化B+Tree*/
	PF_PageHandle rootPageHandle;
	PageNum rootPageNum;
	AllocatePage(&fileHandle, &rootPageHandle);  //为根节点分配一个page
	GetPageNum(&rootPageHandle, &rootPageNum);

	if (rootPageNum != 1)
		return FAIL;

	/*初始化索引控制信息*/
	IX_FileHeader header;
	header.attrLength = attrLength;
	header.attrType = attrType;
	header.keyLength = attrLength + sizeof(RID);
	header.order = (PF_PAGE_SIZE - sizeof(IX_FileHeader) - sizeof(IX_Node)) / (2 * sizeof(RID) + attrLength);
	header.height = 1;
	header.rootPage = rootPageNum;
	header.first_leaf = rootPageNum;

	/*初始化B+Tee根节点信息*/
	IX_Node rootNode;
	rootNode.is_leaf = 1;
	rootNode.next = 0;
	rootNode.prev = 0;
	rootNode.parent = 0;
	rootNode.keynum = 0;
	rootNode.keys = NULL;
	rootNode.rids = NULL;


	/*获得数据区指针*/
	char* pData;
	GetData(&rootPageHandle, &pData);

	/*写入索引控制信息*/
	memcpy(pData, &header, sizeof(IX_FileHeader));
	/*写入根节点IX_Node信息*/
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

		/*获得索引控制信息*/
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
			/*叶节点已满，需要分裂节点*/
			IX_DataNode newNode;
			create_node(indexHandle, leafPageNum, &leafNode, &newNode);

			int point = leafNode.node_info.keynum / 2;
			/*根据插入关键字与分割点的大小，来确定插入关键字所处的节点*/
			bool place_right = key_cmp(indexHandle, getKey(indexHandle, &leafNode, point), key);

			/*平衡两个新节点的关键字个数*/
			if (place_right)
				++point;
			
			/*拷贝关键字到新节点*/
			copy(getKey(indexHandle, &leafNode, point), 
				getKey(indexHandle, &leafNode, leafNode.node_info.keynum), 
				getKey(indexHandle, &newNode, 0), 
				indexHandle->fileHeader.keyLength);

			/*拷贝rid到新节点*/
			copy(leafNode.node_info.rids + point,
				leafNode.node_info.rids + leafNode.node_info.keynum,
				newNode.node_info.rids,
				sizeof(RID));

			newNode.node_info.keynum = leafNode.node_info.keynum - point;
			leafNode.node_info.keynum = point;

			/*插入新关键字*/
			if (place_right)
				insert_record_no_split(indexHandle, &newNode, key, rid);
			else
				insert_record_no_split(indexHandle, &leafNode, key, rid);

			/*写回磁盘*/
			unmap(indexHandle, &leafNode, leafPageNum);
			unmap(indexHandle, &newNode, leafNode.node_info.next);

			/*插入索引项*/
			insert_index(indexHandle, leafNode.node_info.parent, getKey(indexHandle, &newNode, 0), leafPageNum, leafNode.node_info.next);
		}
		else
		{
			/*叶节点未满，直接插入*/
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

	/*当只有一层节点时需要特殊处理*/
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
		
		/*由于节点的关键字数目小于下限，需要借或者合并*/
		if (targetNode.node_info.keynum < halfOrder(indexHandle->fileHeader.order))
		{
			bool borrowed = false;
			if (targetNode.node_info.next != 0)
				borrowed = borrow_key(indexHandle, true, &targetNode);

			if (targetNode.node_info.prev != 0 && !borrowed)
				borrowed = borrow_key(indexHandle, false, &targetNode);

			/*借用失败，需要合并*/
			if (!borrowed)
			{
				char* index_key = (char*)malloc(indexHandle->fileHeader.keyLength);
				if (index_key == NULL)
					return FAIL;

				/*与左侧兄弟合并*/
				if (where == parentNode.node_info.keynum - 1)
				{
					IX_DataNode brotherNode;
					map(indexHandle, &brotherNode, targetNode.node_info.prev);
					memcpy(index_key, getKey(indexHandle, &brotherNode, 0), indexHandle->fileHeader.keyLength);

					merge_node(indexHandle, &brotherNode, &targetNode);
					remove_node(indexHandle, &brotherNode, &targetNode);
					unmap(indexHandle, &brotherNode, targetNode.node_info.prev);
				}
				else /*与右侧兄弟合并*/
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
