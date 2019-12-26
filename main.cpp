#include<stdio.h>
#include "PF_Manager.h"
#include "IX_Manager.h"
#pragma warning(disable : 4996)

void f()
{

}

void next()
{
	system("pause");
	system("cls");
}

void printAllLeaf(const char* fileName)
{
	IX_IndexHandle indexHandle;
	if (OpenIndex(fileName, &indexHandle) == SUCCESS)
	{
		printList(&indexHandle);
		CloseIndex(&indexHandle);
	}
}

// 不重复属性值插入测试,attr:int
void insert_int_test1(const char* fileName, bool deleteFile, int begin, int end, int interval)
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
		for (; begin < end; begin += interval)
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
void insert_int_test2(const char* fileName, bool deleteFile, int _begin, int _end, int interval, int num)
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
		for (int begin = _begin; begin < _end; begin += interval)
		{
			RID rid;

			for (int i = 1; i <= num - num / 2; ++i)
			{
				if (InsertEntry(&indexHandle, &begin, initRid(&rid, begin, begin + i + num / 2)) != SUCCESS)
					printf("key: %d insert fail\n", begin);
			}

			//if (InsertEntry(&indexHandle, &begin, initRid(&rid, begin, begin + 1)) != SUCCESS)
			//	printf("key: %d insert fail\n", begin);
		}

		for (int begin = _begin; begin < _end; begin += interval)
		{
			RID rid;

			for (int i = 1; i <= num / 2; ++i)
			{
				if (InsertEntry(&indexHandle, &begin, initRid(&rid, begin, begin + i)) != SUCCESS)
					printf("key: %d insert fail\n", begin);
			}

			//if (InsertEntry(&indexHandle, &begin, initRid(&rid, begin, begin + 1)) != SUCCESS)
			//	printf("key: %d insert fail\n", begin);
		}

		printTreeInfo(&indexHandle);
		CloseIndex(&indexHandle);
	}

	if (deleteFile)
		system(command);

	printf("insert int test2 done\n\n\n");
}

void search_int_test1(const char* fileName, bool deleteFile, int begin, int end, int interval, int num, CompOp compOp)
{
	char command[128];
	sprintf(command, "del %s", fileName);

	insert_int_test2(fileName, false, begin, end, interval, num);

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
		int test_point[] = { -3, -1, 1, 2, 10, 14, 55, 67, 201, 300, 323, 345, 567, 689, 999, 1000, 2000};

		for (int i = 0; i < sizeof(test_point) / sizeof(int); ++i)
		{
			int res_num = 0;
			OpenIndexScan(&indexScan, &indexHandle, compOp, (char*)&test_point[i]);
			while (IX_GetNextEntry(&indexScan, &rid) != IX_EOF)
			{
				printf("search value: %d result: (PageNum: %u, SlotNum: %d)\n", test_point[i], rid.pageNum, rid.slotNum);
				++res_num;
			}
			CloseIndexScan(&indexScan);
			printf("value: %d is done, result num: %d\n", test_point[i], res_num);
			printf("\n\n");
		}

		CloseIndex(&indexHandle);
	}
	else
		printf("索引文件打开失败！");

	if (deleteFile)
		system(command);

	printf("search int test2 done\n\n\n");
}

void search_int_test2(const char* fileName, bool deleteFile, int begin, int end, int interval, int num, CompOp compOp)
{
	char command[128];
	sprintf(command, "del %s", fileName);

	insert_int_test2(fileName, false, begin, end, interval, num);

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
		int test_point[] = {-3, -1, 1, 2, 10, 14, 55, 67, 201, 300, 323, 345, 567, 689, 999, 1000, 
			3000, 4144, 18997, 32456, 50013, 67889, 99999, 10e7};

		for (int i = 0; i < sizeof(test_point) / sizeof(int); ++i)
		{
			int res_num = 0;
			OpenIndexScan(&indexScan, &indexHandle, compOp, (char*)&test_point[i]);
			while (IX_GetNextEntry(&indexScan, &rid) != IX_EOF)
			{
				printf("search value: %d result: (PageNum: %u, SlotNum: %d)\n", test_point[i], rid.pageNum, rid.slotNum);
				++res_num;
			}
			CloseIndexScan(&indexScan);
			printf("value: %d is done, result num: %d\n", test_point[i], res_num);
			printf("\n\n");
		}

		CloseIndex(&indexHandle);
	}
	else
		printf("索引文件打开失败！");

	if (deleteFile)
		system(command);

	printf("search int test2 done\n\n\n");
}

void delete_int_test(const char* fileName, bool deleteFile, int begin, int end, int interval, int num)
{
	char command[128];
	sprintf(command, "del %s", fileName);

	insert_int_test2(fileName, false, begin, end, interval, num);

	IX_IndexHandle indexHandle;
	if (OpenIndex(fileName, &indexHandle) == SUCCESS)
	{
		for (int _begin = begin; _begin < end; _begin += interval)
		{
			for (int i = 1; i <= num; ++i)
			{
				RID rid;
				if (DeleteEntry(&indexHandle, &_begin, initRid(&rid, _begin, _begin + i)) != SUCCESS)
					printf("delete fail!\n");
			}
		}

		printf("叶节点列表：\n");
		printList(&indexHandle);
		printTreeInfo(&indexHandle);
		CloseIndex(&indexHandle);
	}

	if (deleteFile)
		system(command);
}

void benchmark()
{
	const char* file1 = "test1.index";
	const char* file2 = "test2.index";

	setTestMode(true);
	delete_int_test("test3.index", true, 1, 1001, 2, 10);

	//查询测试
	//setTestMode(false);
	//search_int_test2(file1, false, 1, 100001, 2, 20, EQual);
	//printf("= test\n");
	//next();

	//setTestMode(true);
	//search_int_test1(file2, false, 1, 1001, 2, 3, LessT);
	//printf("< test\n");
	//next();

	//search_int_test1(file2, false, 1, 1001, 2, 3, LEqual);
	//printf("<= test\n");
	//next();

	//search_int_test1(file2, false, 1, 1001, 2, 3, NO_OP);
	//printf("no op test\n");
	//next();

	//search_int_test1(file2, false, 1, 1001, 2, 3, GreatT);
	//printf("> test\n");
	//next();

	//search_int_test1(file2, false, 1, 1001, 2, 3, GEqual);
	//printf(">= test\n");
	//next();

	//search_int_test1(file2, false, 1, 1001, 2, 3, NEqual);
	//printf("<> test\n");
	//next();


}

int main()
{
	benchmark();

	return 0;
}