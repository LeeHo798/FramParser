#include<stdio.h>
#include<stdlib.h>
/*Ethernet帧结构解析程序*/

//CRC循环
void CheckCRC(unsigned _int8 & chCurrByte, int chNextByte) {
	//每次调用进行8次循环，处理一个字节得数据
	for (int nMask = 0x80; nMask > 0; nMask >>= 1) {
		if ((chCurrByte & 0x80) != 0)//首位是1：移位，并进行异或运算
		{
			chCurrByte <<= 1;			//向左移一位，移出比特1
			if ((chNextByte & nMask) != 0)//右边补一位输入比特
			{
				chCurrByte |= 1;
			}
			chCurrByte ^= 7;			//首位已经移出，仅对低8位进行异或运算，7的二进制位0000，0111
		}
		else {							//首位为0，只移位，不进行异或运算
			chCurrByte <<= 1;			//向左移一位，移出比特0
			if ((chNextByte & nMask) != 0) {//右边补一位输入比特
				chCurrByte |= 1;
			}
		}
	}
}
int main(int argc, char* argv[]) {
	if (argc != 2) {//判断命令行参数是否正确
		printf("请以帧封包文件名为参数重新执行程序");
		exit(0);
	}
	//检测输入文件是否存在，并按要求打开文件
	FILE* fp;
	errno_t err;
	if ((err = fopen_s(&fp, argv[1], "rb")) != 0) {
		printf("无法打开帧封包文件，请检查文件是否存在并且未损坏");
		exit(0);
	}
	//声明变量
	int nFindEnd = 0;//保存输入文件的长度
	int nSN = 1;//帧序号
	unsigned _int8 nCheck = 0;//校验码
	int nCurrDataOffset = 22;//帧头偏移量
	int nCurrDataLength = 0;//数据字段长度
	bool bParseCont = true;//是否继续对输入文件进行解析
	unsigned char readByte;//用来缓存文件中读出得数据

	//计算输入文件的长度
	fseek(fp, 0, SEEK_END);//文件指针指向文件末尾
	nFindEnd = ftell(fp);//读取长度
	fseek(fp, 0, SEEK_SET);//指针指回文件开头

	/*定位输入文件的第一个有效帧
	从文件头开始，定位到第一个连续的“AA AA AA AA AA AA AA AB”*/
	while (true)
	{
		for (int j = 0; j < 7; j++) {//找出7个连续的0xaa
			if (ftell(fp) >= nFindEnd) {//安全性检测
				printf("没有找到合法的帧");
				fclose(fp);
				exit(0);
			}
			//判断当前字符是不是0xaa，若不是，则重新寻找7个连续的0xaa
			fread(&readByte, 1, 1, fp);
			if (readByte != 0XAA)
			{
				j = -1;
			}
		}
		if (ftell(fp) >= nFindEnd) {//安全性检测
			printf("没有找到合法的帧");
			fclose(fp);
			exit(0);
		}
		//判断7个连续的0xaa之后是否为0xab
		fread(&readByte, 1, 1, fp);
		if (readByte == 0XAB)
		{
			break;
		}
	}
	//将数据字段定位在14字节处，准备进入解析阶段
	nCurrDataOffset = ftell(fp) + 14;
	fseek(fp, -8, SEEK_CUR);//将指针返回到开头
	//主控循环
	while (bParseCont)
	{
		//检测剩余文件是否包含完整帧头
		if (ftell(fp) + 22 > nFindEnd)
		{
			printf("没有找到完整帧头，解析终止");
			fclose(fp);
			exit(0);
		}
		int buff = 0;//读入字节
		int EtherType = 0;//类型字段
		bool bAccept = true;//是否接受该帧

		//输出帧得序号
		printf("序号：\t\t%d", nSN);
		//输出前导码，不校验
		printf("\n前导码:\t\t");
		for (int i = 0; i < 7; i++) {
			fread(&readByte, 1, 1, fp);
			printf("%02X ", readByte);//输出AA AA AA AA AA AA AA 
		}
		//输出帧前定界符，不校验
		fread(&readByte, 1, 1, fp);
		printf("\n帧前定界符:\t%02X", readByte);//输出AB
		//输出目的地址并校验
		printf("\n目的地址:\t");
		for (int i = 0; i < 6; i++) {
			fread(&buff, 1, 1, fp);
			if (i == 5)
				printf("%02X", buff);//十六进制大写输出
			else
				printf("%02X-", buff);
			if (i == 0)		//第一个字节，作为余数等待下一个字节
			{
				nCheck = buff;
			}
			else {			//开始校验
				CheckCRC(nCheck, buff);
			

			}
		}

		//输出源地址并校验
		printf("\n源地址:\t\t");
		for (int i = 0; i < 6; i++) {
			fread(&buff, 1, 1, fp);
			if (i == 5)
				printf("%02X", buff);//十六进制大写输出
			else
				printf("%02X-", buff);
			CheckCRC(nCheck, buff);//校验

		}
		//输出类型字段，并校验
		printf("\n类型字段:\t");
		fread(&buff, 1, 1, fp);
		//输出类型字段的高8位
		printf("%02X ", buff);
		CheckCRC(nCheck, buff);//校验
		
		EtherType = buff;
		//输出类型字段的低8位
		fread(&buff, 1, 1, fp);
		printf("%02X", buff);
		CheckCRC(nCheck, buff);//校验
	

		EtherType <<= 8;//转换成主机格式
		EtherType |= buff;

		//定位下一个帧，以确定当前帧的结束位置
		while (bParseCont)
		{
			for (int i = 0; i < 7; i++)		//找下一个连续的7个0xaa
			{
				if (ftell(fp) >= nFindEnd) {//到文件末尾，退出循环
					bParseCont = false;
					break;
				}
				//看当前字符是不是0xaa，若是，则重新寻找7个连续的0xaa
				fread(&readByte, 1, 1, fp);
				if (readByte != 0XAA)
				{
					i = -1;
				}
			}
			//更新bParseCont，如果直到文件结束仍没找到，将终止循环
			bParseCont = bParseCont && (ftell(fp) < nFindEnd);

			//判断7个连续的0xaa之后是否为0xab
			fread(&readByte, 1, 1, fp);
			if (readByte == 0XAB)
			{
				break;
			}
		}
		//计算数据字段的长度
		nCurrDataLength = bParseCont ?//是否到达文件末尾
			(ftell(fp) - 8 - 1 - nCurrDataOffset) : //没到文件末尾？
			(ftell(fp) - 1 - nCurrDataOffset);//到达文件末尾
		//以文本格式数据字段并校验
		printf("\n数据字段:\t");
		unsigned char str;
		fseek(fp, bParseCont ? (-8 - 1 - nCurrDataLength) : (-1 - nCurrDataLength), SEEK_CUR);
		
		int nCount = 50;//每行的基本字符数量
		for (int i = 0; i < nCurrDataLength; i++)//输出数据字段文本
		{
				fread(&str, 1, 1, fp);//读取数据文件
				nCount--;
				printf("%C", str);			//字符输出
				CheckCRC(nCheck, (int)str);//CRC校验
				if (nCount < 0)//换行处理
				{
					//将行尾的单词写完整
					if (str == ' ')
					{
						printf("\n\t\t");
						nCount = 50;
					}
					//处理过长的行尾单词：换行并使用连字符
					if (nCount < -10)
					{
						printf("-\n\t\t");
						nCount = 50;
					}
				}
		}

		printf("\nCRC校验\t");
		//输出CRC校验码，如果CRC校验有误，则输出正确的CRC校验码
		fread(&buff, 1, 1, fp);//读取CRC校验码
		unsigned _int8 nTmpCRC = nCheck;
		CheckCRC(nCheck, buff);//最后一步校验
		if ((nCheck & 0xff) == 0)	//CRC校验无误
		{
			printf("(正确):\t%02X", buff);
		}
		else {			//CRC校验有误
			printf("(错误):\t%02X", buff);
			CheckCRC(nTmpCRC, 0);//计算正确的CRC校验码
			printf("\t应为：%02X", (nTmpCRC & 0xff));
			bAccept = false;//将帧的接受标记置为false
		}

		//如果数据字段长度不足46字节或者超过1500字节，则将帧接受标记位置置为false
		if (nCurrDataLength < 46 || nCurrDataLength>1500)
		{
			bAccept = false;
		}
		//输出帧的接受状态
		if (bAccept == true)
			printf("\n状态:\t\tAccept\n");
		else
			printf("\n状态:\t\tDiscard\n");
		printf("\n\n");
		nSN++;						//帧序号+1
		nCurrDataOffset = ftell(fp) + 22;//将数据字段偏移量更新为下一帧的帧头结束位置，8+14？
	}
	fclose(fp);//关闭输入文件
}