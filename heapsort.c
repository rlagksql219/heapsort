#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "person.h"
//�ʿ��� ��� ��� ���ϰ� �Լ��� �߰��� �� ����

// ���� ������� �����ϴ� ����� ���� �ٸ� �� ������ �ణ�� ������ �Ӵϴ�.
// ���ڵ� ������ ������ ������ ���� �����Ǳ� ������ ����� ���α׷����� ���ڵ� ���Ϸκ��� �����͸� �а� �� ����
// ������ ������ ����մϴ�. ���� �Ʒ��� �� �Լ��� �ʿ��մϴ�.
// 1. readPage(): �־��� ������ ��ȣ�� ������ �����͸� ���α׷� ������ �о�ͼ� pagebuf�� �����Ѵ�
// 2. writePage(): ���α׷� ���� pagebuf�� �����͸� �־��� ������ ��ȣ�� �����Ѵ�
// ���ڵ� ���Ͽ��� ������ ���ڵ带 �аų� ���ο� ���ڵ带 �� ����
// ��� I/O�� ���� �� �Լ��� ���� ȣ���ؾ� �մϴ�. ��, ������ ������ �аų� ��� �մϴ�.

typedef struct _Header
{
	int total_pagenum;			//��ü page ��
	int total_recordnum;		//��ü record ��
	int last_delete_pagenum;	//���� �ֱٿ� ������ page ��ȣ
	int last_delete_recordnum;	//���� �ֱٿ� ������ record ��ȣ
	char dummy[PAGE_SIZE - 16];
} Header;


Header header;
int heapnum = 0;


//
// ������ ��ȣ�� �ش��ϴ� �������� �־��� ������ ���ۿ� �о �����Ѵ�. ������ ���۴� �ݵ�� ������ ũ��� ��ġ�ؾ� �Ѵ�.
//
void readPage(FILE *fp, char *pagebuf, int pagenum)
{
	memset(pagebuf, (char)0xFF, PAGE_SIZE);
	fseek(fp, pagenum*PAGE_SIZE, SEEK_SET);
	fread(pagebuf, PAGE_SIZE, 1, fp);
}


//
// ������ ������ �����͸� �־��� ������ ��ȣ�� �ش��ϴ� ��ġ�� �����Ѵ�. ������ ���۴� �ݵ�� ������ ũ��� ��ġ�ؾ� �Ѵ�.
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
// �־��� ���ڵ� ���Ͽ��� ���ڵ带 �о� heap�� ����� ������. Heap�� �迭�� �̿��Ͽ� ����Ǹ�, 
// heap�� ������ Chap9���� ������ �˰����� ������. ���ڵ带 ���� �� ������ ������ ����Ѵٴ� �Ϳ� �����ؾ� �Ѵ�.
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

	/* ù��° ���ڵ� heap�� ���� */
	readPage(inputfp, pagebuf, pagenum);
	memcpy(recordbuf, pagebuf, RECORD_SIZE);
	memcpy(heaparray[index], recordbuf, RECORD_SIZE);
	recordnum++;
	heapnum++;
	
	/* page ������ */
	for(pagenum; pagenum < header.total_pagenum; pagenum++) {
		readPage(inputfp, pagebuf, pagenum);
		
		/* record �о�鿩  heap�� ���� */
		while(recordnum<PAGE_SIZE/RECORD_SIZE && (PAGE_SIZE/RECORD_SIZE) * (pagenum-1) + recordnum < header.total_recordnum) {
			index = heapnum + 1; //�����ؾ��� index
			memcpy(recordbuf, pagebuf + recordnum*RECORD_SIZE, RECORD_SIZE);
			memcpy(heaparray[index], recordbuf, RECORD_SIZE);
			heapnum++;

			/* heap ���� �����ϵ��� */
			while(index > 1) {
				parentindex = index / 2;
			
				if(compare(heaparray[index], heaparray[parentindex]) == -1) { //�θ��� key���� �� ū ���
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
// �ϼ��� heap�� �̿��Ͽ� �ֹι�ȣ�� �������� ������������ ���ڵ带 �����Ͽ� ���ο� ���ڵ� ���Ͽ� �����Ѵ�.
// Heap�� �̿��� ������ Chap9���� ������ �˰����� �̿��Ѵ�.
// ���ڵ带 ������� ������ ���� ������ ������ ����Ѵ�.
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

		/* ���ĵ� ���ڵ� ���Ͽ� page write */
		if(recordnum == PAGE_SIZE/RECORD_SIZE) {
			writePage(outputfp, pagebuf, pagenum);
			write_mark = 1;
			pagenum++;		
			memset(pagebuf, (char)0xFF, PAGE_SIZE);

			recordnum = 0;
		}

		/* ������ ��带 ��Ʈ���� */
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

			if(left_child_index <= heapnum && right_child_index > heapnum) //left_chile�� �����ϴ� ���
				exchange_target_index = left_child_index;
			else if(compare(heaparray[left_child_index], heaparray[right_child_index]) == -1) //right_child�� key���� �� ū ���
				exchange_target_index = left_child_index;
			else
				exchange_target_index = right_child_index;

			if(compare(heaparray[exchange_target_index], heaparray[index]) == -1) { //�θ��� key���� �� ū ���
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
	FILE *inputfp;	// �Է� ���ڵ� ������ ���� ������
	FILE *outputfp;	// ���ĵ� ���ڵ� ������ ���� ������
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
	
	/* heap �迭 ���� */
	heaparray = (char**) malloc(sizeof(char*)*total_recordnum+1);
	for (int i=1; i<=total_recordnum; i++) {
		heaparray[i] = (char*)malloc(RECORD_SIZE);
	}
	
	buildHeap(inputfp, heaparray); //heap ����
	makeSortedFile(outputfp, heaparray); //����

	fclose(inputfp);
	fclose(outputfp);

	free(heaparray);

	return 1;
}