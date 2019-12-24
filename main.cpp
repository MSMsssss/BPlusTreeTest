#include<stdio.h>
#include "PF_Manager.h"
#include "IX_Manager.h"

void f()
{

}

int main()
{
	char fileName[128] = "test.index";
	system("del test.index");
	CreateIndex(fileName, ints, sizeof(int));
	IX_IndexHandle indexHandle;
	if (OpenIndex(fileName, &indexHandle) == SUCCESS)
	{
		printf("open success!\n");

		int keys[1000];
		RID rids[1000];
		for (int i = 1; i <= 1000; ++i)
		{
			keys[i] = i;
			rids[i].bValid = true;
			rids[i].pageNum = i + 5;
			rids[i].slotNum = i + 5;

			IX_DataNode root;
			map(&indexHandle, &root, indexHandle.fileHeader.rootPage);
			int nodeNum = indexHandle.fileHandle.pFileSubHeader->nAllocatedPages - 1;

			if (i == 1000)
				f();

			InsertEntry(&indexHandle, &i, &rids[i]);
		}

		printList(&indexHandle);
		//RID rid;
		//IX_IndexScan indexScan;
		//int target = 500;
		//OpenIndexScan(&indexScan, &indexHandle, EQual, (char*)&target);

		//IX_GetNextEntry(&indexScan, &rid);
		//printf("%u %d\n", rid.pageNum, rid.slotNum);

		//CloseIndexScan(&indexScan);

		CloseIndex(&indexHandle);
	}

	return 0;
}