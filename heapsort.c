#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "person.h"
//필요한 경우 헤더 파일과 함수를 추가할 수 있음

// 과제 설명서대로 구현하는 방식은 각자 다를 수 있지만 약간의 제약을 둡니다.
// 레코드 파일이 페이지 단위로 저장 관리되기 때문에 사용자 프로그램에서 레코드 파일로부터 데이터를 읽고 쓸 때도
// 페이지 단위를 사용합니다. 따라서 아래의 두 함수가 필요합니다.
// 1. readPage(): 주어진 페이지 번호의 페이지 데이터를 프로그램 상으로 읽어와서 pagebuf에 저장한다
// 2. writePage(): 프로그램 상의 pagebuf의 데이터를 주어진 페이지 번호에 저장한다
// 레코드 파일에서 기존의 레코드를 읽거나 새로운 레코드를 쓸 때나
// 모든 I/O는 위의 두 함수를 먼저 호출해야 합니다. 즉, 페이지 단위로 읽거나 써야 합니다.

typedef struct _Header
{
	int total_pagenum;			//전체 page 수
	int total_recordnum;		//전체 record 수
	int last_delete_pagenum;	//가장 최근에 삭제된 page 번호
	int last_delete_recordnum;	//가장 최근에 삭제돈 record 번호
	char dummy[PAGE_SIZE - 16];
} Header;


Header header;
int heapnum = 0;


//
// 페이지 번호에 해당하는 페이지를 주어진 페이지 버퍼에 읽어서 저장한다. 페이지 버퍼는 반드시 페이지 크기와 일치해야 한다.
//
void readPage(FILE *fp, char *pagebuf, int pagenum)
{
	memset(pagebuf, (char)0xFF, PAGE_SIZE);
	fseek(fp, pagenum*PAGE_SIZE, SEEK_SET);
	fread(pagebuf, PAGE_SIZE, 1, fp);
}


//
// 페이지 버퍼의 데이터를 주어진 페이지 번호에 해당하는 위치에 저장한다. 페이지 버퍼는 반드시 페이지 크기와 일치해야 한다.
//
void writePage(FILE *fp, const char *pagebuf, int pagenum)
{
	fseek(fp, pagenum*PAGE_SIZE, SEEK_SET);
	fwrite(pagebuf, PAGE_SIZE, 1, fp);
}


int compare(const char *record1, const char *record2)
{
	Person p1, p2;
	long long sn1, sn2;

	memcpy(&p1, record1, RECORD_SIZE);
	memcpy(&p2, record2, RECORD_SIZE);

	sn1 = atoll(p1.sn);
	sn2 = atoll(p2.sn);

	if (sn1-sn2 < 0)
		return -1;
	else
		return 1;
}


//
// 주어진 레코드 파일에서 레코드를 읽어 heap을 만들어 나간다. Heap은 배열을 이용하여 저장되며, 
// heap의 생성은 Chap9에서 제시한 알고리즘을 따른다. 레코드를 읽을 때 페이지 단위를 사용한다는 것에 주의해야 한다.
//
void buildHeap(FILE *inputfp, char **heaparray)
{
	char pagebuf[PAGE_SIZE];
	char recordbuf[RECORD_SIZE];
	int pagenum = 1;
	int recordnum = 0;
	int index = 1;
	int parentindex;
	int total_recordnum = 0;

	/* 첫번째 레코드 heap에 넣음 */
	readPage(inputfp, pagebuf, pagenum);
	memcpy(recordbuf, pagebuf, RECORD_SIZE);
	memcpy(heaparray[index], recordbuf, RECORD_SIZE);
	recordnum++;
	heapnum++;
	
	/* page 단위로 */
	for(pagenum; pagenum < header.total_pagenum; pagenum++) {
		readPage(inputfp, pagebuf, pagenum);
		
		/* record 읽어들여  heap에 넣음 */
		while(recordnum<PAGE_SIZE/RECORD_SIZE && (PAGE_SIZE/RECORD_SIZE) * (pagenum-1) + recordnum < header.total_recordnum) {
			index = heapnum + 1; //저장해야할 index
			memcpy(recordbuf, pagebuf + recordnum*RECORD_SIZE, RECORD_SIZE);
			memcpy(heaparray[index], recordbuf, RECORD_SIZE);
			heapnum++;

			/* heap 조건 성립하도록 */
			while(index > 1) {
				parentindex = index / 2;
			
				if(compare(heaparray[index], heaparray[parentindex]) == -1) { //부모의 key값이 더 큰 경우
					char *tmp = heaparray[parentindex];
					heaparray[parentindex] = heaparray[index];
					heaparray[index] = tmp;
					index = parentindex;
				}
				else
					break;
			}

			recordnum++;
		}

		recordnum = 0;
	}
}


//
// 완성한 heap을 이용하여 주민번호를 기준으로 오름차순으로 레코드를 정렬하여 새로운 레코드 파일에 저장한다.
// Heap을 이용한 정렬은 Chap9에서 제시한 알고리즘을 이용한다.
// 레코드를 순서대로 저장할 때도 페이지 단위를 사용한다.
//
void makeSortedFile(FILE *outputfp, char **heaparray)
{
	char pagebuf[PAGE_SIZE];
	int recordnum = 0;
	int pagenum = 1;
	int index;
	int write_mark = 0;

	writePage(outputfp, (char*)&header, 0);
	memset(pagebuf, (char)0xFF, PAGE_SIZE);

	for(int i=0; i<header.total_recordnum; i++) {
		write_mark = 0;

		memcpy(pagebuf + recordnum*RECORD_SIZE, heaparray[1], RECORD_SIZE);
		recordnum++;

		/* 정렬된 레코드 파일에 page write */
		if(recordnum == PAGE_SIZE/RECORD_SIZE) {
			writePage(outputfp, pagebuf, pagenum);
			write_mark = 1;
			pagenum++;		
			memset(pagebuf, (char)0xFF, PAGE_SIZE);

			recordnum = 0;
		}

		/* 마지막 노드를 루트노드로 */
		heaparray[1] = heaparray[heapnum];
		heapnum--;

		index = 1;
		/* heap rebuild */
		while(1) {
			int left_child_index = 2*index;
			int right_child_index = 2*index + 1;
			int exchange_target_index;

			if(left_child_index > heapnum || right_child_index > heapnum)
				break;

			if(left_child_index <= heapnum && right_child_index > heapnum) //left_chile만 존재하는 경우
				exchange_target_index = left_child_index;
			else if(compare(heaparray[left_child_index], heaparray[right_child_index]) == -1) //right_child의 key값이 더 큰 경우
				exchange_target_index = left_child_index;
			else
				exchange_target_index = right_child_index;

			if(compare(heaparray[exchange_target_index], heaparray[index]) == -1) { //부모의 key값이 더 큰 경우
				char *tmp = heaparray[index];
				heaparray[index] = heaparray[exchange_target_index];
				heaparray[exchange_target_index] = tmp;
				index = exchange_target_index;
			}
			else
				break;
		}
	}

	if (write_mark == 0)
		writePage(outputfp, pagebuf, pagenum);
}


int main(int argc, char *argv[])
{
	FILE *inputfp;	// 입력 레코드 파일의 파일 포인터
	FILE *outputfp;	// 정렬된 레코드 파일의 파일 포인터
	char **heaparray;
	int total_recordnum;

	if ((inputfp = fopen(argv[2], "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", argv[2]);
		exit(1);
	}

	if ((outputfp = fopen(argv[3], "w")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", argv[3]);
		exit(1);
	}

	readPage(inputfp, (char*)&header, 0);
	total_recordnum = header.total_recordnum;
	
	/* heap 배열 생성 */
	heaparray = (char**) malloc(sizeof(char*)*total_recordnum+1);
	for (int i=1; i<=total_recordnum; i++) {
		heaparray[i] = (char*)malloc(RECORD_SIZE);
	}
	
	buildHeap(inputfp, heaparray); //heap 생성
	makeSortedFile(outputfp, heaparray); //정렬

	fclose(inputfp);
	fclose(outputfp);

	free(heaparray);

	return 1;
}