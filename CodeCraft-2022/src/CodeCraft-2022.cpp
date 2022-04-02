// CodeCraft-2022.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <math.h>
using namespace std;

static string ConfPath = "/data/config.ini";
static string DataPathDem = "/data/demand.csv";
static string DataPathQsv = "/data/qos.csv";
static string DataPathSit = "/data/site_bandwidth.csv";
static string OutPath = "/output/solution.txt";

static std::unordered_map<string, int> EdgeID_map;            //记录site_band文件里的边缘节点顺序的map;
static int Edgelen;                                           //site文件里的边缘节点个数
static vector<int> ResBandvec;                                    //记录剩余带宽

static std::unordered_map<string, int> ClientID_map;           //记录qos文件里的客户顺序的map;
static int   ClientReal;                                       //记录 QOS文件里的客户数量


static std::unordered_map<string, int> demClientID_map;      //记录dem文件里的客户顺序的map;
static int    demClientReal = 0;                                //记录 dem文件里的客户数量 即真实的处理客户数量
static int    TimeNode = 0;                                  //记录 dem文件里的时间节点总数
static vector<vector<int>> demand;					         //demand二维数组 记录需求
static vector<int> CilentOrder;					             //客户节点满足的顺序
static vector<string>ClientID_vec;                             //记录demand文件里的客户顺序的vector;

const static int ClientMax = 36;                       //  客户节点最大数量
const static int EdgeMax = 135;                        //  边缘节点最大数量

int Qos_Max;                                           //最大时延

static ofstream txt;

//构造结构体 存入数据
struct EdgeNode {
	string ID;               //边缘节点名字 
	int ResBand;             //当前该节点剩余带宽
	int used;                //

} EdgeNote[EdgeMax];

struct CilentNode {
	string ID;                 //客户节点名字 
	int QosNum = 0;             //满足QOS的边缘节点个数
	int ResBand;              //满足Qos 边缘节点
	vector<string> EdgeID;
} QosNote[ClientMax];
//-----------------------------------------------------输入函数构建---------------------------------------------------------------//
//------------------------------------------读confg 文件
void InputConfig() {
	ifstream config;
	string Num;                                                //存放读入的数据
	config.open(ConfPath);

	getline(config, Num);
	getline(config, Num);                                     //此文件共俩行数据，第二行覆盖第一行
	string temp(Num.begin() + Num.find('=') + 1, Num.end());  //取出字符串的数字为其创建一个新的字符串
	Qos_Max = atoi(temp.c_str());                             //转为整数

	config.close();

}


//------------------------------------------------------读入3个CSV文件
void InputCsv() {

	//-------------------------处理site_bandwidth.csv文件
	std::ifstream csv;
	csv.open(DataPathSit);

	unsigned int i = 0;
	string bandwidth_temp;
	getline(csv, bandwidth_temp);                      //去除第一行内容
	while (getline(csv, bandwidth_temp)) {

		string ID;
		string value;

		string number;
		istringstream readstr03(bandwidth_temp);		//string数据流化

		getline(readstr03, ID, ',');
		getline(readstr03, value, ',');

		EdgeNote[i].ID = ID;
		EdgeNote[i].ResBand = atoi(value.c_str());
		EdgeNote[i].used = 0;

		ResBandvec.push_back(atoi(value.c_str()));

		pair<string, int> p(ID, i);
		EdgeID_map.insert(p); // 插入一个pair变量
		i++;
	}
	Edgelen = i;

	csv.close();
	csv.clear();

	//--------------------------处理qos.csv文件
	csv.open(DataPathQsv);

	vector<string> client_id;					//客户节点的ID
	int client_num = 0;
	string client_temp;						         //用来存放客户节点ID的临时变量
	string qos_temp;							     //用求边缘节点ID的临时变量

	//获得文件第一行内容，即客户ID字符串
	getline(csv, client_temp);
	istringstream readstr01(client_temp);			 //string数据流化
	getline(readstr01, client_temp, ',');			 //去掉第一个内容（空读一行放入client_temp，此时client_temp清空覆盖）
	while (getline(readstr01, client_temp, ',')) {

		client_temp.erase(std::remove(client_temp.begin(), client_temp.end(), '\r'), client_temp.end());
		client_id.push_back(client_temp);			//将处理好的客户ID传到client_id中

		pair<string, int> p(client_temp, client_num);
		ClientID_map.insert(p);                    // 将QOS的表的客户顺序存入map
		client_num++;
	}
	ClientReal = client_num;
	//此时从第二行开始读
	while (getline(csv, qos_temp)) {
		string edgeID;
		string data;                                    //临时存放QOS值
		istringstream readstr02(qos_temp);			 //string数据流化
		getline(readstr02, edgeID, ',');             //获得这一行的边缘节点	

		//将一行数据按'，'分割
		int j = 0;                                   //获取是第几列取客户节点名字
		while (getline(readstr02, data, ',')) {
			int temp = atoi(data.c_str());
			if (temp < Qos_Max) {		              //和QOS判断
				QosNote[j].ID = client_id[j];
				QosNote[j].QosNum++;
				QosNote[j].EdgeID.push_back(edgeID);
			}
			j++;
		}
	}
	csv.close();
	csv.clear();

	//--------------------------处理demand.csv文件
	csv.open(DataPathDem);

	//处理文件第一行内容，即客户ID字符串 
	string de_client_temp;						             //用求客户节点ID的临时变量
	getline(csv, de_client_temp);					         //获得客户ID字符串
	istringstream readstr04(de_client_temp);				 //string数据流化
	getline(readstr04, de_client_temp, ',');				 //去掉第一个内容
	while (getline(readstr04, de_client_temp, ',')) {

		de_client_temp.erase(std::remove(de_client_temp.begin(), de_client_temp.end(), '\r'), de_client_temp.end());
		ClientID_vec.push_back(de_client_temp);

		pair<string, int> p(de_client_temp, demClientReal);
		demClientID_map.insert(p);                            // 插入一个pair变量
		demClientReal++;
	}
	//处理后几行

	string demand_temp;							//用求边缘节点ID的临时变量
	while (getline(csv, demand_temp)) {
		vector<int> data_line;                         //一行需求量
		string number;
		istringstream readstr05(demand_temp);			//string数据流化
		getline(readstr05, number, ',');                //去掉第一个时间变量
		TimeNode++;

		//将一行数据按'，'分割
		while (getline(readstr05, number, ',')) {
			data_line.push_back(atoi(number.c_str())); //字符串传int
		}
		demand.push_back(data_line);					//插入到vector中
	}

	csv.close();
	csv.clear();
}
void txtout(vector<vector<string>>Res, int i, ofstream &txt) {

	vector<vector<string>>::iterator iter;
	for (iter = Res.begin(); iter != Res.end(); ++iter) {
		txt << (*iter)[0];                               //输出为客户名
		txt << ":";
		for (int ik = 1; ik < (*iter).size(); ik = ik + 2) {
			if ((*iter).size() == 1) {
				;                                    //需求为0时的输出
			}
			else {
				txt << "<";
				txt << (*iter)[ik];
				txt << ",";
				txt << (*iter)[ik + 1];
				txt << ">";
				if (ik + 1 < (*iter).size() - 1)
				{
					txt << ",";
				}
			}

		}
		if ((i == TimeNode - 1)&(iter == Res.end() - 1))
			;
		else
		{
			txt << "\r\n";
		}

	}
}

//-----------------------------------------------------算法函数构建---------------------------------------------------------------//	
//根据每个客户节点给出分配顺序
void Order() {
	string  demID;
	vector <int> qosnum;

	for (int i = 0; i < demClientReal; i++)
	{
		demID = ClientID_vec[i];                        // 从demand文件按顺序取出客户节点
		auto QosLoc = ClientID_map.find(demID);      //其再QOS表的位置

		qosnum.push_back(QosNote[QosLoc->second].QosNum);
		CilentOrder.push_back(i);

	}
	int len = qosnum.size();
	int exchange;

	//冒泡排序 升序
	for (int i = 0; i < len; i++) {
		for (int j = 1; j < len - i; j++)
		{
			if (qosnum[j - 1] > qosnum[j])
			{
				exchange = qosnum[j - 1];
				qosnum[j - 1] = qosnum[j];
				qosnum[j] = exchange;

				exchange = CilentOrder[j - 1];
				CilentOrder[j - 1] = CilentOrder[j];
				CilentOrder[j] = exchange;
			}
		}
	}
}

//找共用的的节点
void UseCount()
{
	string CID;                                             //客户节点的名字
	vector<string> EdgeID;                                  //每个客户满足的边缘结点
	for (int j = 0; j < demClientReal; j++)                //遍历客户
	{

		CID = ClientID_vec[CilentOrder[j]];                //客户节点名称
		auto iterQ = ClientID_map.find(CID);
		EdgeID = QosNote[iterQ->second].EdgeID;           //满足的边缘节点  一个string vector 矩阵存放ID名字
		int num = QosNote[iterQ->second].QosNum;           //满足Qos边缘节点个数
		for (int i = 0; i < num; i++)
		{
			auto iterE1 = EdgeID_map.find(EdgeID[i]);  //取出符合要求的边缘节点在site结构体的位置		
			EdgeNote[iterE1->second].used = EdgeNote[iterE1->second].used + 1;
		}
	}
	//测试

	for (int j = 0; j < demClientReal; j++)                //遍历客户
	{

		CID = ClientID_vec[CilentOrder[j]];                //客户节点名称
		auto iterQ = ClientID_map.find(CID);
		EdgeID = QosNote[iterQ->second].EdgeID;           //满足的边缘节点  一个string vector 矩阵存放ID名字
		int num = QosNote[iterQ->second].QosNum;           //满足Qos边缘节点个数

		/*cout << "客户节点" << CID << ":" << endl;
		for (int i = 0; i < num; i++)
		{
			auto iterE1 = EdgeID_map.find(EdgeID[i]);  //取出符合要求的边缘节点在site结构体的位置		
			cout << EdgeNote[iterE1->second].ID<<" "<<EdgeNote[iterE1->second].used << " ";
		}
		cout << endl;*/
	}


}
static vector <int> ResBandtemp;

//  根据轮询填入 找出共用最多/最少的节点填
string * MaxUsed(vector<string> EdgeID, int num, int resq,int flag)
{
	static string arr[3];
	int  loc;                  //边缘节点在状态表的位置
	int max = 0;              //边缘节点使用次数
	int min = ClientReal* TimeNode;              //边缘节点使用次数
	string tempId;

	
	for (int i = 0; i < num; i = i + 1)
	{
		auto iterE1 = EdgeID_map.find(EdgeID[i]);  //取出符合要求的边缘节点在site结构体的位置
		int Restemp = ResBandtemp[iterE1->second];
		int used = EdgeNote[iterE1->second].used;
		
		if(flag==0)
		{
			if ((used >= max) & (Restemp != 0)) {
				max = used;
				tempId = EdgeID[i];
				loc = iterE1->second;
			}
		}
		else 
		{
			if ((used <= min) & (Restemp != 0)) {
				min = used;
				tempId = EdgeID[i];
				loc = iterE1->second;
			}
		}
	
	}
	
	int current = ResBandtemp[loc];  //取出需求
	


	if (current >= resq)
	{
		arr[0] = to_string(0);
		arr[1] = tempId;
		arr[2] = to_string(resq);               //分配多少
		cout << arr[3];
		ResBandtemp[loc] = ResBandtemp[loc] - resq;   //更新状态表
	}
	else
	{
		arr[0] = to_string(resq - current);
		arr[1] = tempId;
		arr[2] = to_string(current);
		ResBandtemp[loc] = 0;   //更新状态表
	}


	return arr;
}


//---------------------------------------------主函数-------------------------------------------------------------//
int main()
{
	int resq;                                               //每个时刻的需求
	string CID;                                             //客户节点的名字
	vector<string> EdgeID;                                  //每个客户满足的边缘结点

	InputConfig();
	InputCsv();
	Order();
	ofstream txt;
	txt.open(OutPath);

	UseCount();


	//开始分配
	for (int i = 0; i < TimeNode; i++) {                    //遍历时刻

		vector<vector<string>>Res;
		ResBandtemp = ResBandvec;             //每个时刻结束 初始化剩余带宽状态表


		for (int j = 0; j < demClientReal; j++)             //遍历客户需求 ：优先满足（满足QOS的）边缘节点少的客户节点的需求
		{
			vector<string>OneRes;                            //分配结果矩阵（一个客户）   矩阵的样子 [客户节点  边缘节点 分配带宽 .................] 长度2n+1 n>=0
			CID = ClientID_vec[CilentOrder[j]];                //客户节点名称
			resq = demand[i][CilentOrder[j]];                //取出需求

			auto iterQ = ClientID_map.find(CID);
			EdgeID = QosNote[iterQ->second].EdgeID;           //满足的边缘节点  一个string vector 矩阵存放ID名字
			int num = QosNote[iterQ->second].QosNum;           //满足Qos边缘节点个数

			OneRes.push_back(CID);
			//开始分配
			if (resq == 0)                                      //此客户的需求为0
			{
				;
			}
			else    //贪心分配算法
			{
				for (int a = 0; a < num; a++)
				{
					string *arr;             //分配的临时结果 [剩余流量 边缘节点 分配的流量]


					int flagM=0;
					if (i % 21== 0)
					{
						flagM = 1;
					}
					arr = MaxUsed(EdgeID, num, resq,flagM);

					OneRes.push_back(*(arr + 1));
					OneRes.push_back(*(arr + 2));
					resq = atoi((*(arr)).c_str());
					if (resq == 0)
					{
						break;
					}
				}
			}

			Res.push_back(OneRes);                 //一个时刻的所有客户分配结果 二维vector<string>矩阵
		}
		txtout(Res, i, txt);                       //一个时刻结束后降分配方案填入到结果文件中
		cout << "跑完第" << i << "个时刻"<<endl;
	}
	txt.close();
	txt.clear();
	return 0;
}


