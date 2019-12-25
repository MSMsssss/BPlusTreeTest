#include<stdio.h>
#include "PF_Manager.h"
#include "IX_Manager.h"
#pragma warning(disable : 4996)

// 不重复属性值插入测试,attr:int
void insert_int_test1(const char* fileName, bool deleteFile, int begin, int end)
{
	char command[128];
	sprintf(command, "del %s", fileName);

	if (CreateIndex(fileName, ints, sizeof(int)) != SUCCESS)
	{
		printf("索引创建失败！\n");
		return;
	}

	IX_IndexHandle indexHandle;
	if (OpenIndex(fileName, &indexHandle) == SUCCESS)
	{
		for (; begin != end; ++begin)
		{
			RID rid;

			if (InsertEntry(&indexHandle, &begin, initRid(&rid, begin, begin + 1)) != SUCCESS)
				printf("key: %d insert fail\n", begin);
		}

		printTreeInfo(&indexHandle);
		CloseIndex(&indexHandle);
	}

	if (deleteFile)
		system(command);

	printf("insert int test1 done\n\n\n");
}

// 重复属性值插入测试,attr:int,num重复次数
void insert_int_test2(const char* fileName, bool deleteFile, int begin, int end, int num)
{
	char command[128];
	sprintf(command, "del %s", fileName);

	if (CreateIndex(fileName, ints, sizeof(int)) != SUCCESS)
	{
		printf("索引创建失败！\n");
		return;
	}

	IX_IndexHandle indexHandle;
	if (OpenIndex(fileName, &indexHandle) == SUCCESS)
	{
		for (; begin != end; ++begin)
		{
			RID rid;

			for (int i = 1; i <= num; ++i)
			{
				if (InsertEntry(&indexHandle, &begin, initRid(&rid, begin, begin + i)) != SUCCESS)
					printf("key: %d insert fail\n", begin);
			}

			//if (InsertEntry(&indexHandle, &begin, initRid(&rid, begin, begin + 1)) != SUCCESS)
			//	printf("key: %d insert fail\n", begin);
		}

		printTreeInfo(&indexHandle);
		printList(&indexHandle);
		CloseIndex(&indexHandle);
	}

	if (deleteFile)
		system(command);

	printf("insert int test2 done\n\n\n");
}

void search_int_test2(const char* fileName, bool deleteFile, int begin, int end, int num)
{
	char command[128];
	sprintf(command, "del %s", fileName);

	insert_int_test2(fileName, false, begin, end, num);

	IX_IndexHandle indexHandle;
	if (OpenIndex(fileName, &indexHandle) == SUCCESS)
	{
		/*
			EQual,			"="			0
			LEqual,			"<="          1
			NEqual,			"<>"			2
			LessT,			"<"			3
			GEqual,			">="			4
			GreatT,			">"           5
			NO_OP
		*/

		IX_IndexScan indexScan;
		RID rid;
		int test = 100;
		OpenIndexScan(&indexScan, &indexHandle, EQual, (char*)& test);
		while (IX_GetNextEntry(&indexScan, &rid) != IX_EOF)
			printf("search value: %d result: (PageNum: %u, SlotNum: %d)\n", test, rid.pageNum, rid.slotNum);
		CloseIndexScan(&indexScan);

		CloseIndex(&indexHandle);
	}
	else
		printf("索引文件打开失败！");

	if (deleteFile)
		system(command);

	printf("search int test2 done\n\n\n");
}

void benchmark()
{
	setTestMode(true);
	const char* file1 = "insert_int_test1.index";

	// 插入测试
	//insert_int_test1(file1, true, 1, 10001);
	insert_int_test2(file1, true, 1, 1001, 10);
	

	//查询测试
	//search_int_test2(file1, true, 1, 10001, 20);
}

int main()
{
	benchmark();

	return 0;
}