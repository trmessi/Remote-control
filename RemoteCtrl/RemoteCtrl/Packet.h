#pragma once
#include "pch.h"
#include "framework.h"
#pragma pack(push)
#pragma pack(1)
class CPacket
{
public:
	WORD sHead;//包头。固定位 FEFF
	DWORD nLength;//包长度（从控制命令到和校验结束）
	WORD sCmd;//控制命令
	std::string strData;//包数据
	WORD sSum;//和校验
	std::string strOut;//整个包的信息
	CPacket() {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
	{
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else
		{
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}
	CPacket(const CPacket& mpack)
	{
		sHead = mpack.sHead;
		nLength = mpack.nLength;
		sCmd = mpack.sCmd;
		strData = mpack.strData;
		sSum = mpack.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize)
	{
		size_t i = 0;
		for (; i < nSize; i++)//可能有问题
		{
			if (*(WORD*)(pData + i) == 0xFEFF)
			{
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize)//包数据可能不全，或者包头未能全部接收到
		{
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize)//包未完全接收到，就返回，解析失败
		{
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum)
		{
			nSize = i;//head 2,length 4,cmd+data+sum nLength
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	CPacket& operator =(const CPacket& mpack)
	{
		if (this != &mpack)
		{
			sHead = mpack.sHead;
			nLength = mpack.nLength;
			sCmd = mpack.sCmd;
			strData = mpack.strData;
			sSum = mpack.sSum;
		}
		return *this;
	}
	int Size()
	{
		return nLength + 2 + 4;
	}
	const char* Data()
	{
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
};
#pragma pack(pop)
typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//单击，双击，移动
	WORD nButton;//左键，右键，中键
	POINT ptXY;//坐标
}MOUSEEV, * PMOUSSEV;

typedef struct  file_info
{
	file_info()
	{
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//是否有效
	BOOL IsDirectory;//是否为目录  0否 1是
	BOOL HasNext;//是否有后续  0没有 有
	char szFileName[256];//文件名


} FILEINFO, * PFILEINFO;