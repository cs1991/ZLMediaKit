#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma pack(1)

union littel_endian_size
{
	unsigned short int	length;
	unsigned char		byte[2];
};

struct pack_start_code
{
	unsigned char start_code[3];
	unsigned char stream_id[1];
};

struct program_stream_pack_header
{
	pack_start_code PackStart;// 4
	unsigned char Buf[9];
	unsigned char stuffinglen;
};

struct program_stream_map
{
	pack_start_code PackStart;
	littel_endian_size PackLength;//we mast do exchange
								  //program_stream_info_length
								  //info
								  //elementary_stream_map_length
								  //elem
};

struct program_stream_e
{
	pack_start_code		PackStart;
	littel_endian_size	PackLength;//we mast do exchange
	char				PackInfo1[2];
	unsigned char		stuffing_length;
};

#pragma pack()

int inline ProgramStreamPackHeader(char* Pack, int length, char **NextPack, int *leftlength)
{
	//printf("[%s]%x %x %x %x\n", __FUNCTION__, Pack[0], Pack[1], Pack[2], Pack[3]);
	//通过 00 00 01 ba头的第14个字节的最后3位来确定头部填充了多少字节
	program_stream_pack_header *PsHead = (program_stream_pack_header *)Pack;
	unsigned char pack_stuffing_length = PsHead->stuffinglen & '\x07';

	*leftlength = length - sizeof(program_stream_pack_header) - pack_stuffing_length;//减去头和填充的字节
	*NextPack = Pack + sizeof(program_stream_pack_header) + pack_stuffing_length;

	if (*leftlength<4) return 0;

	//printf("[%s]2 %x %x %x %x\n", __FUNCTION__, (*NextPack)[0], (*NextPack)[1], (*NextPack)[2], (*NextPack)[3]);

	return *leftlength;
}

inline int ProgramStreamMap(char* Pack, int length, char **NextPack, int *leftlength, char **PayloadData, int *PayloadDataLen)
{
	//printf("[%s]%x %x %x %x\n", __FUNCTION__, Pack[0], Pack[1], Pack[2], Pack[3]);

	program_stream_map* PSMPack = (program_stream_map*)Pack;

	//no payload
	*PayloadData = 0;
	*PayloadDataLen = 0;

	if (length < sizeof(program_stream_map)) return 0;

	littel_endian_size psm_length;
	psm_length.byte[0] = PSMPack->PackLength.byte[1];
	psm_length.byte[1] = PSMPack->PackLength.byte[0];

	*leftlength = length - psm_length.length - sizeof(program_stream_map);

	//printf("[%s]leftlength %d\n", __FUNCTION__, *leftlength);

	if (*leftlength <= 0) return 0;

	*NextPack = Pack + psm_length.length + sizeof(program_stream_map);

	return *leftlength;
}

inline int Pes(char* Pack, int length, char **NextPack, int *leftlength, char **PayloadData, int *PayloadDataLen)
{
	//printf("[%s]%x %x %x %x\n", __FUNCTION__, Pack[0], Pack[1], Pack[2], Pack[3]);
	program_stream_e* PSEPack = (program_stream_e*)Pack;

	*PayloadData = 0;
	*PayloadDataLen = 0;

	if (length < sizeof(program_stream_e)) return 0;

	littel_endian_size pse_length;
	pse_length.byte[0] = PSEPack->PackLength.byte[1];
	pse_length.byte[1] = PSEPack->PackLength.byte[0];
	if (pse_length.length + 6 > length)
	{
		pse_length.length = length - 6;
		/*int PES_packet_length_fixed = length - 6;
		Pack[4] = (char)((PES_packet_length_fixed && 0xFF00) >> 8);
		Pack[5] = (char)(PES_packet_length_fixed & 0x00FF);*/
	}

	*PayloadDataLen = pse_length.length - 2 - 1 - PSEPack->stuffing_length;
	if (*PayloadDataLen>0)
		*PayloadData = Pack + sizeof(program_stream_e) + PSEPack->stuffing_length;

	*leftlength = length - pse_length.length - sizeof(pack_start_code) - sizeof(littel_endian_size);

	//printf("[%s]leftlength %d\n", __FUNCTION__, *leftlength);

	if (*leftlength <= 0) return 0;

	*NextPack = Pack + sizeof(pack_start_code) + sizeof(littel_endian_size) + pse_length.length;

	return *leftlength;
}

int inline GetH246FromPs(char* buffer, int length, char **h264Buffer, int *h264length, char* header)
{
	int leftlength = 0;
	char *NextPack = buffer;

	*h264Buffer = buffer;
	*h264length = 0;

	leftlength = length;

	if (ProgramStreamPackHeader(buffer, length, &NextPack, &leftlength) == 0)
		return 0;

	char *PayloadData = NULL;
	int PayloadDataLen = 0;

	while (leftlength >= sizeof(pack_start_code))
	{
		PayloadData = NULL;
		PayloadDataLen = 0;

		 if (strcmp(NextPack, "000001e0")) {
		//if (NextPack
		//	&& NextPack[0] == '\x00'
		//	&& NextPack[1] == '\x00'
		//	&& NextPack[2] == '\x01'
		//	&& NextPack[3] == '\xE0')
		//{
			//接着就是流包，说明是非i帧
			if (Pes(NextPack, leftlength, &NextPack, &leftlength, &PayloadData, &PayloadDataLen))
			{
				if (PayloadDataLen)
				{
					memcpy(buffer, PayloadData, PayloadDataLen);
					buffer += PayloadDataLen;
					*h264length += PayloadDataLen;
				}
			}
			else
			{
				if (PayloadDataLen)
				{
					memcpy(buffer, PayloadData, PayloadDataLen);
					buffer += PayloadDataLen;
					*h264length += PayloadDataLen;
				}

				break;
			}
         } else if (strcmp(NextPack, "000001bc")) {
		//else if (NextPack
		//	&& NextPack[0] == '\x00'
		//	&& NextPack[1] == '\x00'
		//	&& NextPack[2] == '\x01'
		//	&& NextPack[3] == '\xBC')
		//{
			if (ProgramStreamMap(NextPack, leftlength, &NextPack, &leftlength, &PayloadData, &PayloadDataLen) == 0)
				continue;
		}
		else
		{
			//printf("no konw %x %x %x %x\n", NextPack[0], NextPack[1], NextPack[2], NextPack[3]);
			break;
		}
	}


	return *h264length;
}