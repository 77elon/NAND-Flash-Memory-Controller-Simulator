#include <iostream>
#include <string>

//for debugging, reallocation
#include <fstream>
#include <vector>
#include <filesystem>


using namespace std;

class Flash
{
public:
	char Sector[512]; // 512 + 1 byte, Pointer == (Variable *= 4)
	int Valid_Check = 0; // length of data?, Spare Area
	int LSN = -1;
	Flash()
	{
		for (int i = 0; i < 512; i++)
		{
			Sector[i] = 1; //init data allocation, 1 Fill
		}
	}
};

Flash* pos, * Overwrite, * Spare; // 8 byte + 8 byte + 8 byte
int secsize = 0, read = 0, write = 0, erase_count = 0, erase_index = 0, request = 0; // 4 + 4 + 4 + 4 + 4 + 4 byte
int latest_write_sec = 0, latest_write_blk = 0;
string select = "";


void Sector_Frame()
{
	cout << "물리 섹터 번호";
	cout.width(15);
	cout << "데이터 길이" << endl;
}
void SectorData_Frame()
{
	cout << endl;
	cout << "섹터 데이터" << endl;
}
void SectorMap_Frame()
{
	cout.width(4);
	cout << "LBN";
	cout.width(7);
	cout << "PBN" << endl;
}
void BlockMap_Frame()
{
	cout.width(4);
	cout << "LBN";
	cout.width(7);
	cout << "PBN" << endl;
}
void Print_Table(vector<int>& o1, const string& o2)
{
	if (pos)
	{
		if (o2 == "sector")
		{
			SectorMap_Frame();
		}
		else if (o2 == "block")
		{
			BlockMap_Frame();
		}
		else
		{
			cout << "Flash Memory 할당이 되어있지 않습니다. 확인 후 재실행해주세요. " << endl;
			return;
		}
		for (unsigned int i = 0; i < o1.size(); i++)
		{
			if (o2 == "sector" && i != 0 && i % 2048 == 0)
			{
				system("pause");
			}
			cout.width(3);
			cout << i;
			cout.width(8);
			cout << o1.at(i) << endl;;
		}
	}

}

void IO_Check()
{
	if (read > 0)
	{
		cout << "읽기 연산 " << read << "회 ";
	}
	if (write > 0)
	{
		cout << "쓰기 연산 " << write << "회 ";
	}
	if (erase_count > 0)
	{
		cout << "지우기 연산 " << erase_count << "회 " << "(PBN " << erase_index << ")";
	}
	cout << endl;
}

void Saved_Clear()
{
	read = write = erase_count = erase_index = request = 0;
	//if Spare Block isDynamicAllocated
	/*delete[] Spare;
	 Spare = nullptr;*/
}

//Input Text to String (Stable)
void Input_Func(string& o1, const unsigned int& v1)
{
	cin >> o1;
	while (true) //isDegit을 사용하지 않은 이유? 사실 String의 각 인덱스를 색인하는 것은 시간낭비!
	{
		if (cin.fail() || o1.length() > v1)
		{
			cout << "다시 입력해주세요." << endl;
			cin.clear(); //오류가 발생한 스트림 초기화
			cin.ignore(256, '\n'); //입력된 값 초기화
			cin >> o1;
		}
		else
		{
			for (int i = 0; i < (int)o1.size(); i++)
			{
				o1[i] = tolower(o1[i]);
			}
			break;
		}
	}
}
int Conv_Func(string& o1)
{
	int conv;
	try
	{
		conv = stoi(o1);
	}
	catch (...) //입력 함수에서 isDegit으로 적절히 거를 수 있지만, 만약 숫자와 영어가 같이 들어온다면??
	{
		cout << "엑세스하려는 블록이나 섹터를 \"숫자로만\" 입력해주세요!" << endl;
		return -1;
	}
	return conv;
}

void TableSave_func(vector<int>& o1, const string& o2, const string& o3)
{
	ofstream ofs, ofs_size;
	ofs.open(o2);
	ofs_size.open(o3);
	vector<int>::reverse_iterator iter;
	for (iter = o1.rbegin(); iter != o1.rend(); iter++)
	{
		ofs << o1.at(iter - o1.rbegin()) << " " << "\n";
	}

	ofs_size << secsize << " " << erase_index << " " << Overwrite - pos << " " << Spare - pos << " " << "\n";

	ofs_size.close();
	ofs.close(); //Fileoutput Stream close!!
}
void TableLoad_func(vector<int>& o1, const string& o2, const string& o3)
{
	ifstream ifs, ifs_size;
	ifs.open(o2);
	ifs_size.open(o3);
	int temp1 = 0, temp2 = 0;
	if (ifs.is_open() && ifs_size.is_open())
	{
		int Temp = 0;
		while (ifs >> Temp)
		{
			o1.push_back(Temp);
		}

		ifs_size >> secsize >> erase_index >> temp1 >> temp2; //Load Map_Info.txt
		Overwrite = pos + temp1; //First to Overwrite Sector
		Spare = pos + temp2; // First to Spare Sector
	}
	else
	{
		cout << "파일이 존재하지 않습니다." << endl;
	}
	ifs_size.close();
	ifs.close(); //Fileinput Stream close!!
}

void SecSave_Func(Flash*& o1, const string& o2)
{
	if (!pos)
	{
		cout << "Flash Memory 할당이 되어있지 않습니다. 확인 후 재실행해주세요. " << endl;
		return;
	}
	else
	{
		ofstream s_ofs;
		s_ofs.open(o2);
		for (int i = 0; i < secsize + 32 + 32; i++)     //size != 0 saving!!, secsize + Overwrite + Spare
		{
			if (o1[i].Valid_Check && o1[i].LSN != -1) //data is Full
			{
				s_ofs << i << " ";
				int n = 0;
				while (o1[i].Sector[n] != '\0' && o1[i].Sector[n] != '\1' && s_ofs << o1[i].Sector[n])
				{
					n++;
				}
				s_ofs << " " << o1[i].Valid_Check << " " << o1[i].LSN << " " << "\n";
			}
		}
		s_ofs.close();
		cout << "파일이 정상적으로 저장되었습니다." << endl;
	}
}
void SecLoad_Func(Flash*& o1, const string& o2)
{
	if (!pos)
	{
		cout << "Flash Memory 할당이 되어있지 않습니다. 확인 후 재실행해주세요. " << endl;
		return;
	}
	else
	{
		ifstream s_ifs;
		s_ifs.open(o2);
		if (s_ifs.is_open())
		{
			int index;
			Flash temp;
			while (s_ifs >> index >> temp.Sector >> temp.Valid_Check >> temp.LSN && index < secsize + 32 + 32) //if file index > secsize?
			{
				o1[index] = temp; //Object Copy
			}
		}
		else
		{
			cout << "저장된 파일이 없습니다." << endl;
		}
		s_ifs.close(); //file Stream...
	}
}

void Exit_Func()
{
	if (pos) //data allocated check
	{
		delete[] pos;
		pos = nullptr;
		Overwrite = nullptr;
		Spare = nullptr;
		secsize = 0, read = 0, write = 0, erase_count = 0, erase_index = 0, latest_write_sec = 0, request = 0;
		select = "";
	}
}

void offset_calc(int& v1, int& v2, int& v3)
{
	v2 = v1 / 32; //lbn
	v3 = v1 % 32; //offset
}
void reverse_calc(vector<int>& o1, int& v1, int& v2, int& v3, int& v4) //psn, pbn, lbn, offset
{
	v2 = v1 / 32;
	v4 = v1 % 32;
	auto it = find(o1.begin(), o1.end(), v2); //lbn find(index)
	v3 = (int)(it - o1.begin());
}

void Sector_read(Flash*& o1, int& v1)
{
	if (v1 < 0)
	{
		cout << "잘못된 영역에 엑세스하려고 하였습니다." << endl;
		return;
	}
	++read;
	Sector_Frame();
	int i = 0; //data_count = 0; unused
	cout.width(5);
	cout << v1;
	cout.width(19);
	cout << o1[v1].Valid_Check;
	SectorData_Frame();
	while (i < 512 && o1[v1].Sector[i] != '\0' && o1[v1].Sector[i] != '\1')
	{
		if (i != 0 && i % 32 == 0)
		{
			cout << endl;
		}
		cout << o1[v1].Sector[i];
		++i;
	}
	cout << endl;
	//data validate check
	/*if (i == o1[v1].Valid_Check)
	{
		cout << "데이터의 온전히 저장되어 있습니다." << endl;
	}
	else
	{
		cout << "Sector가 비어있거나, 손상되었습니다." << endl;
	}*/
}
void Sector_write(Flash*& o1, int& v1, int& v2, const string& o2) //Flash, Physical, Data
{
	if (v1 == -1)
	{
		cout << "잘못된 영역에 엑세스하고자 하였습니다." << endl;
		return;
	}
	++write;
	const char* data = o2.c_str();
	unsigned int i = 0;
	while (i < o2.length()) //어차피 512byte를 넘는 데이터는 입력단에서...
	{
		o1[v1].Sector[i] = data[i]; //casting 불가하므로
		++i;
	}
	o1[v1].Valid_Check = (int)o2.length(); //data size saving
	o1[v1].LSN = v2;
}

bool Block_isEmpty(Flash*& o1, int& v1)
{
	int start_index = v1 * 32;
	//int end_index = (v1 + 1) * 32 - 1;
	int check = 0;
	int i = 0;
	for (i = 0; i < 32; i++)
	{
		if (!o1[start_index + i].Valid_Check)
		{
			check++;
		}
		else
		{
			read += check;
			return check == 32;
		}
	}
	read += 32;
	return check == 32;
}
bool Block_isFull(Flash*& o1, int& v1)
{
	int check = 0;
	int i = 0;
	for (i = 0; i < 32; i++)
	{
		if (o1[v1 * 32 + i].Valid_Check)
		{
			++check;
		}
		else
		{
			break;
		}
	}
	read += i + 1;
	return check == 32;
}
/*bool Data_EmptyCheck(Flash*& o1, vector<int> &o2)
{
	//table Indexing + Valid Checking

}*/

void SecIndexing_Func(Flash*& o1, vector<int>& o2, int& v1, int& v2, const string& o3) //Find Empty Sector
{
	int temp = -1;
	int temp2 = (int)(Spare - o1); //First Spare Sector

	if (o3 == "dynamic")
	{
		temp = latest_write_sec;
	}
	else if (o3 == "static")
	{
		temp = (int)(Overwrite - o1); //First Overwrite Sector
	}
	for (int i = temp; i < temp2; i++) //at secsize + 31 just Overwrite Block, Explicitly
	{
		if (o3 == "static")
		{
			++read;
		}
		if (/*!o1[temp].Valid_Check && */ o1[i].LSN == -1) //if temp isEmpty 증산체크
		{
			if (o3 == "dynamic")
			{
				++read;
			}
			v1 = i; //Overwrite Block psn++
			o2.at(v2) = v1;
			return;
		}
	}
}
//For Overwrite Block GC, Overwrite Block Mapping -> Spare Block Mapping
/*void Mapping_Modify(Flash*& o1, vector<int>& o2, int& v1, int& v2) //Flash, Table, List_Dist, Spare Starting Point
{
	int Search_Dest = v1; //First mapping table Indexing
	int End_OF_DestSec = v1 % 32;
	//int Mapped_Blk = v2; //Modifyed Dest Block

	for (int i = End_OF_DestSec; i < 32 - 1; i++)
	{
		o2.at(Search_Dest + i) = v2 + i;
	}
}*/
/*void Mapping_Check(Flash*& o1, vector<int>& o2, int& v1, int& v2) //Flash, Table, Modifyed Table Loc, Mapped Block
{
	int Search_Dest = v1 - (v1 % 32); //First mapping table Indexing
	//int Mapped_Blk = v2 - (v2 % 32); //Modifyed Dest Block unused

	int i = 0;
	while (o2.at(Search_Dest + i) == v2 + i && i < 32 - 1) //what task??
	{
		i++;
	}
}*/
void SecExist_check(Flash*& o1, vector<int>& o2, int& v1, int& v2) //table, blknum, Spare Block Dest, data restore, but end of block write after...? , need LSN Backup!, Overwrite GC
{
	//Overwrite Pointer를 사용한 Spare, Overwrite, Data Block Switching
	for (int i = 0; i < 32; i++)
	{
		int Overwrited_Sec = v1 + i; //just Overwrite Block Valid Check, indexing
		int SpareDist = v2;
		int List_Dist = 0;

		auto it = o2.begin(); //index find!!
		while (true)
		{
			it = find(it, o2.end(), Overwrited_Sec); //Spare에 대한 Mapping Info 색인, 결점!! 매핑 정보가 수정되지 않는 문제! 이로 인해 의미가 없어짐
			if (it != o2.end()) //mapping된 psn이 존재할 때
			{
				for (int n = 0; n < i; n++)
				{
					//Data backup, data block + 1 -> data block + 2, o1 == Flash pointer,
					o1[(Spare - o1) + n] = o1[o2.at(it - o2.begin())];
					//Overwrite -> Spare Block And Mapping Info??
					List_Dist = (int)(it - o2.begin()); //Found Overwrited Data -> Spare Mapping modify!
					//just Overwrited Data Mapping Modify... Erase후 승격된 블록에만 쓰도록 하는 것
					o2.at(List_Dist) = /*(Spare - o1) */ SpareDist + n; //Spare Starting Point + Empty Space
					++read;
					++write;
				}
				//Mapping_Modify(o1, o2, List_Dist, SpareDist); //Flash, Table, List_Dist, Spare Starting Point
				//Overwrite GC후 Spare Block, Overwrite Block의 위치가 적절히 바뀌었는지?
				//Mapping_Check(o1, o2, List_Dist, SpareDist); //Flash, Table, Modifyed Table Loc, Mapped Block
				++it;
			}
			else
			{
				break;
			}
		}
	}
}
void BlkExist_check(Flash*& o1, int& v1, int& v2, int& v3, int& v4) //Flash, pbn, lbn, offset, dest blk
{
	int start_index = v1 * 32;
	int spare_index = v4 * 32;
	for (int i = 0; i < 32; i++)
	{
		if (o1[start_index + i].LSN > -1 && i != v3) //v3 will Overwrite 
		{
			o1[spare_index + i] = o1[start_index + i];
			++read;
			++write;
		}
	}
}

void Block_Erase(Flash*& o1, int& v1) //v1 is blk index 0-31, 32-63, 64-95
{
	erase_index = v1;
	++erase_count;
	int start = v1 * 32;
	int end = (v1 + 1) * 32; // end 전까지
	while (start < end)
	{
		memset(o1[start].Sector, '\1', 512); //0-511 Data Clear
		o1[start].Valid_Check = 0;
		o1[start].LSN = -1;
		++start; //end 전까지
	}
}
void Restore_Block(Flash*& o1, vector<int>& o2, int& v1) //Flash, Mapping, BeErase Block, backup block(spare)
{
	/*for (int i = 0; i < 32; i++)
	{
		Spare[i] = o1[v1 * 32 + i];
	}*/
	int spareblk = (int)(Spare - o1);
	for (int i = 0; i < 32; i++)
	{
		if (o1[spareblk + i].LSN != -1)
		{
			o2.at(o1[spareblk + i].LSN) = spareblk + i;
		}
	}
}

void garbage_erase(Flash*& o1, vector<int>& o2)
{
	int dest = 0;
	//int owblk = (int)(Overwrite - o1); unused, need Fix
	int spareblk = (int)(Spare - o1);
	while (dest < (secsize / 32) - 1 && Block_isFull(o1, dest)) //end of Overwrite Block, Spare blk is always empty
	{
		//if(Block_isFull) 호출 조건에 따라서 모든 블록을 체크!
		//block copy needsr
		SecExist_check(o1, o2, dest, spareblk);
		//garbage Clear
		Block_Erase(o1, dest);
		Restore_Block(o1, o2, dest);
		++dest;
	}
}
void blk_garbage_erase(Flash*& o1, vector<int>& o2)
{
	int End_of_Block = (secsize + 32 + 32) / 32;
	auto it = o2.begin(); //index define
	for (int i = 0; i < End_of_Block; i++)
	{
		it = find(it, o2.end(), i); //Spare에 대한 Mapping Info 색인, 결점!! 매핑 정보가 수정되지 않는 문제! 이로 인해 의미가 없어짐
		if (it == o2.end()) //table에 존재하지 않는다면 Invalid Block
		{
			if (!Block_isEmpty(o1, i)) //i == blk_index
			{
				Block_Erase(o1, i);
			}
			it = o2.begin();
		}
	}
}
void Mapping_init(vector<int>& o1, const string& o2, const string& o3)
{
	int alloc = 0;
	if (o1.empty())
	{
		alloc = secsize;
	}
	else
	{
		alloc = (int)(secsize - o1.size());
	}
	int initial = -1;
	int blk_temp = secsize / 32 - 1;
	while (alloc != 0 && blk_temp != -1) // block start.. 0!
	{
		if (o2 == "sector")
		{
			--alloc;
			if (o3 == "dynamic")
			{
				o1.push_back(initial); //just Table Allocation...
				continue;
			}
			o1.push_back(alloc); //어차피 역순
		}
		else if (o2 == "block")
		{
			if (o3 == "dynamic")
			{
				o1.push_back(initial); //just Table Allocation...
				--blk_temp;
				continue;
			}
			o1.push_back(blk_temp);
			--blk_temp;
		}
	}
}

void init(Flash*& o1, vector<int>& o2, const string& o3, int& v1, const string& o4)
{
	system("cls");
	bool temp = false;
	if (pos) //if allocated
	{
		cout << "이미 Flash Memory가 할당되어 있습니다. 재할당하시겠습니까? (Y / N)" << endl;
		string check = "", check1 = "";
		while (check != "y" && check != "Y" && check != "n" && check != "N")
		{
			Input_Func(check, 1);
		}
		if (check == "n" || check == "N")
		{
			return;
		}
		else if (check == "y" || check == "Y")
		{
			cout << "기존 할당된 메모리보다 작은 사이즈로 재할당시, 데이터 손실이 발생할 수 있습니다. 그래도 진행하시겠습니까? (Y / N)" << endl;
			while (check1 != "y" && check1 != "Y" && check1 != "n" && check1 != "N")
			{
				Input_Func(check1, 1);
			}
			if (check1 == "n" || check1 == "N")
			{
				return;
			}
			temp = true; //need data restore!!
			//file saving Func
			SecSave_Func(o1, "Reallocation.txt");
			delete[] o1; //mem release
		}
	}
	cout << v1 << "MB의 용량 " << v1 * 2048 << "섹터 크기를 가진 Flash Memory를 생성합니다." << endl;

	//real init, dynamic allocation
	o1 = new Flash[2048 * v1 + 32 + 32]; //dynamic array allocation, Overwrite block, Spare Block
	secsize = v1 * 2048; //secsize / 32 == blksize

	Mapping_init(o2, o3, o4); //preallocation!!

	Overwrite = o1 + secsize; //Overwrite Block First Sector
	Spare = o1 + secsize + 32; //Spare Block First Sector

	if (temp)
	{
		//file loading Func
		SecLoad_Func(o1, "Reallocation.txt");
	}
}

void Flash_read(Flash*& o1, int& v1)
{
	system("cls");
	if (!pos && v1 == -1)
	{
		cout << "할당이 되지 않은 공간에 엑세스하려고 하였습니다. 확인 후 재실행해주세요. " << endl;
		return;
	}
	else if (pos && v1 < secsize + 32 + 32)
	{
		Sector_read(o1, v1); //real read func
	}
	else
	{
		cout << "할당된 저장소를 초과한 공간의 데이터를 읽고자 하였습니다. " << endl; //Err
	}
}
void Flash_write(Flash*& o1, vector<int>& o2, int& v1, int& v2, const string& o3, const string& o4, const string& o5) //Flash, Table, PSN, LSN, Data, (sec/blk)
{
	system("cls");
	if (!pos && v1 == -1)
	{
		cout << "할당이 되지 않은 공간에 엑세스하려고 하였습니다. 확인 후 재실행해주세요. " << endl;
		return;
	}
	else if (o4 == "sector" && pos && v1 < secsize + 32)
	{
		int OverwriteBlk = (int)(Overwrite - o1) / 32; //*Overwrite에 대한 수정은 언제 일어나는가? + 초기화 상태는 LSN이 -1이다... <- Problem, Overwrite - o1 == Overwrite Pointer Variable이 칭하는 Sector Num..
		int spareblk = (int)(Spare - o1) / 32;
		if (o1[v1].LSN != 0) //first. dest sector is Full
		{
			cout << "비어있는 가장 빠른 Sector에 데이터를 저장합니다. " << endl;
			//Indexing Func..
			SecIndexing_Func(o1, o2, v1, v2, o5); //Overwrite Block의 빈 섹터를 v1으로 리턴, 읽기가 복리로 올라간다. Fair 비교 X
			//Overwrite block isFull
			if (&o1[latest_write_sec] == &o1[OverwriteBlk * 32 + 32 - 1]) //Spare Block의 Overwrite은 순차적으로 발생될 것이므로! 굳이 모든 Indexing이 필요없다. + 이후 발생되는 데이터는 Spare Block에...
			{
				SecExist_check(o1, o2, OverwriteBlk, spareblk);
				//Block_Erase(o1, OverwriteBlk); Lazy Erase
				//Restore_Block(o1, o2, OverwriteBlk, spareblk); //need Mapping Table Reference
				garbage_erase(o1, o2);
				if (o5 == "static")
				{
					Overwrite = Spare;
				}
				else if (o5 == "dynamic")
				{
					int v1_blk = v1 / 32;
					Restore_Block(o1, o2, v1_blk);
				}
			}
			else
			{
				cout << "데이터가 " << v1 << "번 Sector에 저장될 예정입니다." << endl;
			}
		}
		else
		{
			cout << "Sector " << v1 << "에 데이터를 저장합니다." << endl;
		}
		//real write fun
		if (o5 == "static")
		{
			latest_write_sec = v1;
		}
		else if (o5 == "dynamic")
		{
			++latest_write_sec;
			latest_write_sec %= (secsize + 32);
			o2.at(v2) = latest_write_sec; //sequential Write Order ,v1 또한 바뀜!
		}
		Sector_write(o1, v1, v2, o3);
		Sector_read(o1, v1); //write result
	}

	else if (o4 == "block" && pos && v1 < secsize + 32 + 32) //until Overwrite block
	{
		int pbn = 0, lbn = 0, offset = 0, spareblk = 0;
		reverse_calc(o2, v1, pbn, lbn, offset);
		if (o1[offset].LSN != -1) //offset is Full
		{
			++read;
			cout << "여분의 블록에 데이터를 저장합니다. " << endl;
			if (o5 == "static")
			{
				spareblk = (int)(Spare - o1) / 32;
				BlkExist_check(o1, pbn, lbn, offset, spareblk);
				Block_Erase(o1, pbn); //erase_index = spareblk;
				Spare = &o1[pbn * 32];
				o2.at(lbn) = spareblk; //mapping recover
			}
			else if (o5 == "dynamic")
			{
				int owblk = (int)(Overwrite - o1) / 32;
				spareblk = (int)(Spare - o1) / 32;
				if (latest_write_blk == owblk || latest_write_blk == spareblk) //Data Area isFull
				{
					blk_garbage_erase(o1, o2); //Invalid Block Erase
				}

				while (!Block_isEmpty(o1, latest_write_blk)) //sequential check!, explicitly
				{
					//blk garbage(Table Indexing) + recover mapping
					BlkExist_check(o1, pbn, lbn, offset, latest_write_blk); //valid page copy
					o2.at(lbn) = latest_write_blk;
					++latest_write_blk;
					latest_write_blk %= ((secsize + 32) / 32);
				}

				Spare = &o1[pbn];
				//spareblk = latest_write_blk;

			/*else
			{
				int wear = erase_index;
				while (erase_index < spareblk && wear > 0) //find empty block
				{
					if (Block_isEmpty(o1, wear))
					{
						break;
					}
					wear--;
				}
				BlkExist_check(o1, pbn, lbn, offset, wear);
				Block_Erase(o1, pbn);
				Spare = &o1[pbn / 32];
				o2.at(lbn) = wear;
			}*/
				int last = o2.at(lbn) * 32 + offset; //low Level...
				cout << "데이터가 " << last << "번 Sector에 저장될 예정입니다." << endl; //psn input
				Sector_write(o1, last, v2, o3);
				++latest_write_blk;
				latest_write_blk %= ((secsize + 32 + 32) / 32);
				Sector_read(o1, last);
			}
			else
			{
				cout << "데이터가 " << v1 << "번 Sector에 저장될 예정입니다." << endl; //psn input
				Sector_write(o1, v1, v2, o3);
				Sector_read(o1, v1);
			}
		}
		else
		{
			cout << "할당된 저장소를 초과한 공간에 데이터를 쓰고자 하였습니다. " << endl; //Err
		}
	}
}
void Flash_erase(Flash*& o1, int& v1) //v1 is blk index
{
	system("cls");
	if (!pos)
	{
		cout << "Flash Memory 할당이 되어있지 않습니다. 확인 후 재실행해주세요. " << endl;
		return;
	}
	else if (pos && v1 < (secsize / 32))
	{
		Block_Erase(o1, v1);
		cout << "Block " << v1 << "의 삭제가 완료되었습니다." << endl;
		erase_index = v1;
	}
	else
	{
		cout << "할당된 저장소를 초과한 공간의 데이터를 지우고자 하였습니다. " << endl; //Err
	}
}

//FTL Function -> Flash Func(527, 544, 628)(Low Level)
void FTL_read(Flash*& o1, vector<int>& o2, int& v1, const string& o3)
{
	if (o3 == "sector" && pos && v1 < secsize) //input limit
	{
		Flash_read(o1, o2.at(v1));
	}
	else if (o3 == "block" && pos && v1 < secsize) //need Trusted Solver
	{
		int lbn = 0, offset = 0, psn = 0;
		offset_calc(v1, lbn, offset); //lsn, lbn, offset
		psn = o2.at(lbn) * 32 + offset;
		Flash_read(o1, psn); //Flash, psn
	}
	else
	{
		cout << "잘못된 영역에 액세스하려고 했습니다." << endl;
	}
}
void FTL_write(Flash*& o1, vector<int>& o2, int& v1, const string& o3, const string& o4, const string& o5) //lsn input
{
	if (o4 == "sector" && pos && v1 < secsize)
	{
		Flash_write(o1, o2, o2.at(v1), v1, o3, o4, o5); //Flash, Table, psn, lsn, data, sector
	}
	else if (o4 == "block" && pos && v1 < secsize) //need Trusted Solver
	{
		int lbn = 0, offset = 0, psn = 0;
		offset_calc(v1, lbn, offset); //lsn, lbn, offset
		if (o5 == "dynamic" && o2.at(lbn) == -1) //is Not Allocated
		{
			o2.at(lbn) = latest_write_blk;
		}
		psn = o2.at(lbn) * 32 + offset;
		Flash_write(o1, o2, psn, v1, o3, o4, o5); //Flash, Table, psn, lsn, data, bloc
	}
	else
	{
		cout << "잘못된 영역에 액세스하려고 했습니다." << endl;
	}
}

//Trace Func
string TraceFile_Index()
{
	int num = 0;
	string Input1 = "";
	using std::filesystem::directory_iterator;
	namespace fs = std::filesystem;
	string dest_return = "";
	//if not exist Trace Folder?
	fs::create_directory("Trace");

	fs::directory_iterator begin("Trace");
	auto dest = begin; //path.begin()
	for (const auto& file : directory_iterator("Trace")) // path("Trace), ./(Project DIR)/Trace/
	{
		cout << ++num << ") " << file.path() << endl;
	}
	cout << "테스트 하고자 하는 파일을 번호로 입력해주세요!" << endl;
	Input_Func(Input1, 2); //maximum 99 Trace File Selection
	int temp = Conv_Func(Input1); //Selected Trace File
	for (int i = 1; i < temp; i++)
	{
		++dest;
	}
	dest_return = dest->path().string();
	cout << "선택된 파일은 " << dest_return << endl;

	//close(fs);
	return dest_return;

}
//using Input_Func(num) -> Dest_File(String Type)
void TraceFile_Exec(Flash*& o1, vector<int>& o2, const string& o3, const string& o4, const string& o5) //Flash Pointer, Mapping Table, Trace File, (Sector, Block)
{
	if (!pos)
	{
		cout << "Flash Memory 할당이 되어있지 않습니다. 확인 후 재실행해주세요. " << endl;
		return;
	}
	else
	{
		ifstream ifs;
		ifs.open(o3);
		if (ifs.is_open())
		{
			string Command = "", Dest = "";
			//int num = 0; unused
			while (ifs >> Command >> Dest)
			{
				++request;
				//command execute Implement
				if (Command == "r" || Command == "R")
				{
					int temp = Conv_Func(Dest); //PSN Posit
					if (temp == -1 || temp < 0) //error
					{
						continue;
					}
					FTL_read(o1, o2, temp, o4);
				}
				else if (Command == "w" || Command == "W")
				{
					int temp = Conv_Func(Dest); //PSN Posit
					if (temp == -1 || temp < 0)
					{
						continue;
					}
					//FTL_write fun
					FTL_write(o1, o2, temp, Dest, o4, o5);  //Flash, Table, psn, data(Identity LSN), sector
				}
				//system("pause");
			}
		}
		read -= request; //align
		IO_Check();
		cout << "발생된 요청 수 : " << request << endl;
		cout << read << " " << write << " " << erase_count << " " << erase_index << endl;
		Saved_Clear(); //Read, Write, Erase Count Reset
		ifs.close(); //File input Stream close!!
	}
}

//Menu
void Sec_Func(Flash*& o1, vector<int>& o2, const string& o3)
{
	string Input1 = "", Input2 = "", Input3 = ""; //write data!!
	while (Input1 != "exit")
	{
		cout << "시뮬레이션 하고 싶은 명령을 내려주세요. (init, read, write, table, trace, exit)" << endl;
		Input_Func(Input1, 6); //maximum length set
		if (Input1 == "init") //if init + Eng Input?? => Error
		{
			Input_Func(Input2, 3); // xxxMB
			int temp = Conv_Func(Input2); //Mem Size
			if (temp == -1 || temp < 0)
			{
				continue;
			}
			init(o1, o2, "sector", temp, o3);
			pos = o1;
		}
		else if (Input1 == "read")
		{
			Input_Func(Input2, 6); //Maximum 488MB Sector
			int temp = Conv_Func(Input2); //LSN Posit
			if (temp == -1 || temp < 0)
			{
				continue;
			}
			FTL_read(o1, o2, temp, "sector");
			IO_Check();
			Saved_Clear();
		}
		else if (Input1 == "write")
		{
			Input_Func(Input2, 6); //Maximum 488MB Sector
			int temp = Conv_Func(Input2); //LSN Posit
			if (temp == -1 || temp < 0)
			{
				continue;
			}
			Input_Func(Input3, 512); //data input
			//FTL_write fun
			FTL_write(o1, o2, temp, Input3, "sector", o3);
			Input3 = ""; //Write에서만 사용되기에 상관은 없지만...
			IO_Check();
			Saved_Clear();
		}
		else if (Input1 == "table")
		{
			Print_Table(o2, "sector");
		}
		else if (Input1 == "trace")
		{
			//Flash Pointer, Mapping Table, Trace File, (Sector, Block)
			TraceFile_Exec(o1, o2, TraceFile_Index(), "sector", o3);
		}
		else
		{
			continue;
		}
	}
	if (pos)
	{
		if (o3 == "static")
		{
			SecSave_Func(o1, "S_SecData.txt");
			TableSave_func(o2, "S_SecTable.txt", "S_SecMap_Info.txt");
		}
		else if (o3 == "dynamic")
		{
			SecSave_Func(o1, "D_SecData.txt");
			TableSave_func(o2, "D_SecTable.txt", "D_SecMap_Info.txt");
		}
		Saved_Clear();
		o2.clear();
		Exit_Func();
	}
}
void Blk_Func(Flash*& o1, vector<int>& o2, const string& o3)
{
	string Input1 = "", Input2 = "", Input3 = ""; //write data!!
	while (Input1 != "exit")
	{
		cout << "시뮬레이션 하고 싶은 명령을 내려주세요. (init, read, write, table, trace, exit)" << endl;
		Input_Func(Input1, 6); //maximum length set
		if (Input1 == "init") //if init + Eng Input?? => Error
		{
			Input_Func(Input2, 3); // xxxMB
			int temp = Conv_Func(Input2); //Mem Size
			if (temp == -1 || temp < 0)
			{
				continue;
			}
			init(o1, o2, "block", temp, o3);
			pos = o1;
		}
		else if (Input1 == "read")
		{
			Input_Func(Input2, 6); //Maximum 488MB Sector
			int temp = Conv_Func(Input2); //LBN Posit
			if (temp == -1 || temp < 0)
			{
				continue;
			}
			//FTL_read fun
			FTL_read(o1, o2, temp, "block");
			IO_Check();
			Saved_Clear();

		}
		else if (Input1 == "write")
		{
			Input_Func(Input2, 6); //Maximum 488MB Sector
			int temp = Conv_Func(Input2); //LBN Posit
			if (temp == -1 || temp < 0)
			{
				continue;
			}
			Input_Func(Input3, 512); //data input
			//FTL_write fun
			FTL_write(o1, o2, temp, Input3, "block", o3);
			Input3 = ""; //Write에서만 사용되기에 상관은 없지만...
			IO_Check();
			Saved_Clear();

		}
		else if (Input1 == "table")
		{
			Print_Table(o2, "block");
		}
		else if (Input1 == "trace")
		{
			//Flash Pointer, Mapping Table, Trace File, (Sector, Block)
			TraceFile_Exec(o1, o2, TraceFile_Index(), "block", o3);
		}
		else
		{
			continue;
		}
	}
	if (pos)
	{
		if (o3 == "static")
		{
			SecSave_Func(o1, "S_BlockData.txt");
			TableSave_func(o2, "S_BlockTable.txt", "S_BlockMap_Info.txt");
		}
		else if (o3 == "dynamic")
		{
			SecSave_Func(o1, "D_BlockData.txt");
			TableSave_func(o2, "D_BlockTable.txt", "D_BlockMap_Info.txt");
		}
		o2.clear();
		Exit_Func();

	}
}

void Select_Func(Flash*& o1, vector<int>& o2) //동시에 1개의 에뮬레이션만 가능하기에
{
	string Input1 = "";
	while (true)
	{
		cout << "FTL Mapping 방식을 선택하세요. (sector, block, exit)" << endl;
		Input_Func(Input1, 6);
		if (Input1 == "exit")
		{
			exit(0);
		}
		cout << "할당 방식을 선택해주세요! (static, dynamic)" << endl;
		while (select != "static" && select != "dynamic")
		{
			Input_Func(select, 7); //length 7 Text Input
		}
		if (Input1 == "sector")
		{
			if (select == "static")
			{
				//sec mapping table load func, auto init, auto restore
				TableLoad_func(o2, "S_SecTable.txt", "S_SecMap_Info.txt");
				if (secsize != 0 && !pos)
				{
					int temp = secsize / 2048;
					init(o1, o2, "sector", temp, select);
					pos = o1;
					//secdata load func
					SecLoad_Func(o1, "S_SecData.txt");
				}
				Sec_Func(o1, o2, select);
			}
			else if (select == "dynamic")
			{
				//sec mapping table load func, auto init, auto restore
				TableLoad_func(o2, "D_SecTable.txt", "D_SecMap_Info.txt");
				if (secsize != 0 && !pos)
				{
					int temp = secsize / 2048;
					init(o1, o2, "sector", temp, select);
					pos = o1;
					//secdata load func
					SecLoad_Func(o1, "D_SecData.txt");
				}
				Sec_Func(o1, o2, select);
			}
		}
		else if (Input1 == "block")
		{
			if (select == "static")
			{
				//blk mapping table load func, allocation needs!
				TableLoad_func(o2, "S_BlockTable.txt", "S_BlockMap_Info.txt");
				if (secsize != 0 && !pos)
				{
					int temp = secsize / 2048;
					init(o1, o2, "block", temp, select);
					pos = o1;
					//blkdata load func
					SecLoad_Func(o1, "S_BlockData.txt");
				}
				Blk_Func(o1, o2, select);
			}
			else if (select == "dynamic")
			{
				//blk mapping table load func, allocation needs!
				TableLoad_func(o2, "D_BlockTable.txt", "D_BlockMap_Info.txt");
				if (secsize != 0 && !pos)
				{
					int temp = secsize / 2048;
					init(o1, o2, "block", temp, select);
					pos = o1;
					//blkdata load func
					SecLoad_Func(o1, "D_BlockData.txt");
				}
				Blk_Func(o1, o2, select);
			}
		}
	}
}

int main()
{
	Flash* data;
	vector<int> table;
	Select_Func(data, table);
}
