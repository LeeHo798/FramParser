#include<stdio.h>
#include<stdlib.h>
/*Ethernet֡�ṹ��������*/

//CRCѭ��
void CheckCRC(unsigned _int8 & chCurrByte, int chNextByte) {
	//ÿ�ε��ý���8��ѭ��������һ���ֽڵ�����
	for (int nMask = 0x80; nMask > 0; nMask >>= 1) {
		if ((chCurrByte & 0x80) != 0)//��λ��1����λ���������������
		{
			chCurrByte <<= 1;			//������һλ���Ƴ�����1
			if ((chNextByte & nMask) != 0)//�ұ߲�һλ�������
			{
				chCurrByte |= 1;
			}
			chCurrByte ^= 7;			//��λ�Ѿ��Ƴ������Ե�8λ����������㣬7�Ķ�����λ0000��0111
		}
		else {							//��λΪ0��ֻ��λ���������������
			chCurrByte <<= 1;			//������һλ���Ƴ�����0
			if ((chNextByte & nMask) != 0) {//�ұ߲�һλ�������
				chCurrByte |= 1;
			}
		}
	}
}
int main(int argc, char* argv[]) {
	if (argc != 2) {//�ж������в����Ƿ���ȷ
		printf("����֡����ļ���Ϊ��������ִ�г���");
		exit(0);
	}
	//��������ļ��Ƿ���ڣ�����Ҫ����ļ�
	FILE* fp;
	errno_t err;
	if ((err = fopen_s(&fp, argv[1], "rb")) != 0) {
		printf("�޷���֡����ļ��������ļ��Ƿ���ڲ���δ��");
		exit(0);
	}
	//��������
	int nFindEnd = 0;//���������ļ��ĳ���
	int nSN = 1;//֡���
	unsigned _int8 nCheck = 0;//У����
	int nCurrDataOffset = 22;//֡ͷƫ����
	int nCurrDataLength = 0;//�����ֶγ���
	bool bParseCont = true;//�Ƿ�����������ļ����н���
	unsigned char readByte;//���������ļ��ж���������

	//���������ļ��ĳ���
	fseek(fp, 0, SEEK_END);//�ļ�ָ��ָ���ļ�ĩβ
	nFindEnd = ftell(fp);//��ȡ����
	fseek(fp, 0, SEEK_SET);//ָ��ָ���ļ���ͷ

	/*��λ�����ļ��ĵ�һ����Ч֡
	���ļ�ͷ��ʼ����λ����һ�������ġ�AA AA AA AA AA AA AA AB��*/
	while (true)
	{
		for (int j = 0; j < 7; j++) {//�ҳ�7��������0xaa
			if (ftell(fp) >= nFindEnd) {//��ȫ�Լ��
				printf("û���ҵ��Ϸ���֡");
				fclose(fp);
				exit(0);
			}
			//�жϵ�ǰ�ַ��ǲ���0xaa�������ǣ�������Ѱ��7��������0xaa
			fread(&readByte, 1, 1, fp);
			if (readByte != 0XAA)
			{
				j = -1;
			}
		}
		if (ftell(fp) >= nFindEnd) {//��ȫ�Լ��
			printf("û���ҵ��Ϸ���֡");
			fclose(fp);
			exit(0);
		}
		//�ж�7��������0xaa֮���Ƿ�Ϊ0xab
		fread(&readByte, 1, 1, fp);
		if (readByte == 0XAB)
		{
			break;
		}
	}
	//�������ֶζ�λ��14�ֽڴ���׼����������׶�
	nCurrDataOffset = ftell(fp) + 14;
	fseek(fp, -8, SEEK_CUR);//��ָ�뷵�ص���ͷ
	//����ѭ��
	while (bParseCont)
	{
		//���ʣ���ļ��Ƿ��������֡ͷ
		if (ftell(fp) + 22 > nFindEnd)
		{
			printf("û���ҵ�����֡ͷ��������ֹ");
			fclose(fp);
			exit(0);
		}
		int buff = 0;//�����ֽ�
		int EtherType = 0;//�����ֶ�
		bool bAccept = true;//�Ƿ���ܸ�֡

		//���֡�����
		printf("��ţ�\t\t%d", nSN);
		//���ǰ���룬��У��
		printf("\nǰ����:\t\t");
		for (int i = 0; i < 7; i++) {
			fread(&readByte, 1, 1, fp);
			printf("%02X ", readByte);//���AA AA AA AA AA AA AA 
		}
		//���֡ǰ���������У��
		fread(&readByte, 1, 1, fp);
		printf("\n֡ǰ�����:\t%02X", readByte);//���AB
		//���Ŀ�ĵ�ַ��У��
		printf("\nĿ�ĵ�ַ:\t");
		for (int i = 0; i < 6; i++) {
			fread(&buff, 1, 1, fp);
			if (i == 5)
				printf("%02X", buff);//ʮ�����ƴ�д���
			else
				printf("%02X-", buff);
			if (i == 0)		//��һ���ֽڣ���Ϊ�����ȴ���һ���ֽ�
			{
				nCheck = buff;
			}
			else {			//��ʼУ��
				CheckCRC(nCheck, buff);
			

			}
		}

		//���Դ��ַ��У��
		printf("\nԴ��ַ:\t\t");
		for (int i = 0; i < 6; i++) {
			fread(&buff, 1, 1, fp);
			if (i == 5)
				printf("%02X", buff);//ʮ�����ƴ�д���
			else
				printf("%02X-", buff);
			CheckCRC(nCheck, buff);//У��

		}
		//��������ֶΣ���У��
		printf("\n�����ֶ�:\t");
		fread(&buff, 1, 1, fp);
		//��������ֶεĸ�8λ
		printf("%02X ", buff);
		CheckCRC(nCheck, buff);//У��
		
		EtherType = buff;
		//��������ֶεĵ�8λ
		fread(&buff, 1, 1, fp);
		printf("%02X", buff);
		CheckCRC(nCheck, buff);//У��
	

		EtherType <<= 8;//ת����������ʽ
		EtherType |= buff;

		//��λ��һ��֡����ȷ����ǰ֡�Ľ���λ��
		while (bParseCont)
		{
			for (int i = 0; i < 7; i++)		//����һ��������7��0xaa
			{
				if (ftell(fp) >= nFindEnd) {//���ļ�ĩβ���˳�ѭ��
					bParseCont = false;
					break;
				}
				//����ǰ�ַ��ǲ���0xaa�����ǣ�������Ѱ��7��������0xaa
				fread(&readByte, 1, 1, fp);
				if (readByte != 0XAA)
				{
					i = -1;
				}
			}
			//����bParseCont�����ֱ���ļ�������û�ҵ�������ֹѭ��
			bParseCont = bParseCont && (ftell(fp) < nFindEnd);

			//�ж�7��������0xaa֮���Ƿ�Ϊ0xab
			fread(&readByte, 1, 1, fp);
			if (readByte == 0XAB)
			{
				break;
			}
		}
		//���������ֶεĳ���
		nCurrDataLength = bParseCont ?//�Ƿ񵽴��ļ�ĩβ
			(ftell(fp) - 8 - 1 - nCurrDataOffset) : //û���ļ�ĩβ��
			(ftell(fp) - 1 - nCurrDataOffset);//�����ļ�ĩβ
		//���ı���ʽ�����ֶβ�У��
		printf("\n�����ֶ�:\t");
		unsigned char str;
		fseek(fp, bParseCont ? (-8 - 1 - nCurrDataLength) : (-1 - nCurrDataLength), SEEK_CUR);
		
		int nCount = 50;//ÿ�еĻ����ַ�����
		for (int i = 0; i < nCurrDataLength; i++)//��������ֶ��ı�
		{
				fread(&str, 1, 1, fp);//��ȡ�����ļ�
				nCount--;
				printf("%C", str);			//�ַ����
				CheckCRC(nCheck, (int)str);//CRCУ��
				if (nCount < 0)//���д���
				{
					//����β�ĵ���д����
					if (str == ' ')
					{
						printf("\n\t\t");
						nCount = 50;
					}
					//�����������β���ʣ����в�ʹ�����ַ�
					if (nCount < -10)
					{
						printf("-\n\t\t");
						nCount = 50;
					}
				}
		}

		printf("\nCRCУ��\t");
		//���CRCУ���룬���CRCУ�������������ȷ��CRCУ����
		fread(&buff, 1, 1, fp);//��ȡCRCУ����
		unsigned _int8 nTmpCRC = nCheck;
		CheckCRC(nCheck, buff);//���һ��У��
		if ((nCheck & 0xff) == 0)	//CRCУ������
		{
			printf("(��ȷ):\t%02X", buff);
		}
		else {			//CRCУ������
			printf("(����):\t%02X", buff);
			CheckCRC(nTmpCRC, 0);//������ȷ��CRCУ����
			printf("\tӦΪ��%02X", (nTmpCRC & 0xff));
			bAccept = false;//��֡�Ľ��ܱ����Ϊfalse
		}

		//��������ֶγ��Ȳ���46�ֽڻ��߳���1500�ֽڣ���֡���ܱ��λ����Ϊfalse
		if (nCurrDataLength < 46 || nCurrDataLength>1500)
		{
			bAccept = false;
		}
		//���֡�Ľ���״̬
		if (bAccept == true)
			printf("\n״̬:\t\tAccept\n");
		else
			printf("\n״̬:\t\tDiscard\n");
		printf("\n\n");
		nSN++;						//֡���+1
		nCurrDataOffset = ftell(fp) + 22;//�������ֶ�ƫ��������Ϊ��һ֡��֡ͷ����λ�ã�8+14��
	}
	fclose(fp);//�ر������ļ�
}