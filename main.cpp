#include<stdio.h>
#include "PF_Manager.h"
#include "IX_Manager.h"

int main()
{
	char fileName[128] = "test";
	//CreateIndex(fileName, ints, sizeof(int));
	IX_IndexHandle indexHandle;
	if (OpenIndex(fileName, &indexHandle) == SUCCESS)
	{
		printf("open success!");

		printList(&indexHandle);

		//int keys[1000];
		//RID rids[1000];
		//for (int i = 0; i < 1000; ++i)
		//{
		//	keys[i] = i;
		//	rids[i].bValid = true;
		//	rids[i].pageNum = i + 5;
		//	rids[i].slotNum = i + 5;

		//	InsertEntry(&indexHandle, &i, &rids[i]);
		//}

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