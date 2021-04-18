#include<iostream>
#include<map>
#include<string>
#include<string.h>
#include<vector>
#include<algorithm>
#include<fstream>
#include<math.h>

using namespace std;

class ServerType
{
public:
	string name;
	int cpuNum;
	int memory;
	int hardCost;
	int dailyCost;

	ServerType() {

	}

	ServerType(string name, int cpuNum, int memory, int hardCost, int dailyCost) {
		this->name = name;
		this->cpuNum = cpuNum;
		this->memory = memory;
		this->hardCost = hardCost;
		this->dailyCost = dailyCost;
	}
};

class ServerInst
{
public:
	int sid;
	string type;
	int ArestC;
	int ArestM;
	int BrestC;
	int BrestM;
	//vector<string> vmList;

	ServerInst() {

	}

	ServerInst(int sid, string type, int ArestC, int ArestM, int BrestC, int BrestM) {
		this->sid = sid;
		this->type = type;
		this->ArestC = ArestC;
		this->ArestM = ArestM;
		this->BrestC = BrestC;
		this->BrestM = BrestM;
		//this->vmList = vector<string>();
	}
};

class VmwareType
{
public:
	string name;
	int cpuCost;
	int memoryCost;
	bool isTwoNode;

	VmwareType() {

	}

	VmwareType(string name, int cpuCost, int memoryCost, bool isTwoNode) {
		this->name = name;
		this->cpuCost = cpuCost;
		this->memoryCost = memoryCost;
		this->isTwoNode = isTwoNode;
	}
};


class VmwareInst
{
public:
	string vmid;
	string vmtype;
	int sid;
	string node;

	VmwareInst() {

	}

	VmwareInst(string vmid, string vmtype, int sid, string node) {
		this->vmid = vmid;
		this->vmtype = vmtype;
		this->sid = sid;
		this->node = node;
	}
};


int T;									//������
int now;
bool hadPurchase = false;
int maxCpuDMemory;						//���cpu/�ڴ����
int maxMemoryDCpu;						//����ڴ�/cpu����
vector<string> ServerTypeKeyList;		//�������ͺ���vector���Ҿ�������
vector<string> VmwareTypeKeyList;		//������ͺ���vector���Ҿ�������
map<string, ServerType> ServerTypeMap;	//�������ͺ�map��keyΪ�ͺ�����valueΪ�ͺ���
map<string, VmwareType> VmwareTypeMap;	//������ͺ�map��keyΪ�ͺ�����valueΪ�ͺ���
map<int, ServerInst> ServerDeployMap;	//����������map��keyΪ������id��valueΪ������ʵ��
map<string, VmwareInst> VmwareDeployMap;//���������map��keyΪ�����id��valueΪ�����ʵ����

//���ڷ������ͺ��б�����
bool ServerTypelessmark(const string& stype1, const string& stype2)
{
	return ServerTypeMap[stype1].hardCost + ServerTypeMap[stype1].dailyCost * (T - now)
		< ServerTypeMap[stype2].hardCost + ServerTypeMap[stype2].dailyCost * (T - now);
}

//����������ͺ��б�����
bool VmwareTypelessmark(const string& vmtype1, const string& vmtype2)
{
	return VmwareTypeMap[vmtype1].cpuCost + VmwareTypeMap[vmtype1].memoryCost
		< VmwareTypeMap[vmtype2].cpuCost + VmwareTypeMap[vmtype2].memoryCost;
}

//������ʹ���еķ�������ʣ����������
bool ServerRestlessmark(const int& sid1, const int& sid2) {
	ServerInst s1 = ServerDeployMap[sid1], s2 = ServerDeployMap[sid2];
	return s1.ArestC + s1.ArestM + s1.BrestC + s1.BrestM < s2.ArestC + s2.ArestM + s2.BrestC + s2.BrestM;
}

//������ʹ���еķ�������������������
bool ServerUsedlessmark(const int& sid1, const int& sid2) {
	ServerInst s1 = ServerDeployMap[sid1], s2 = ServerDeployMap[sid2];
	ServerType st1 = ServerTypeMap[s1.type], st2 = ServerTypeMap[s2.type];
	int total1 = st1.cpuNum + st1.memory, total2 = st2.cpuNum + st2.memory;
	return total1 - (s1.ArestC + s1.ArestM + s1.BrestC + s1.BrestM) < total2 - (s2.ArestC + s2.ArestM + s2.BrestC + s2.BrestM);
}


//�ַ���splitʵ��
vector<string> stringsplit(const string &str, const char *delim)
{
	vector <string> strlist;
	int size = str.size();
	char *input = new char[size + 1];
	strcpy(input, str.c_str());
	char *token = strtok(input, delim);
	while (token != NULL) {
		strlist.push_back(token);
		token = strtok(NULL, delim);
	}
	delete[]input;
	return strlist;
}

//Ǩ��

vector<string> Migration() {
	int maxcount = VmwareDeployMap.size() * 5 / 1000;
	int servercount = ServerDeployMap.size();
	int count = 0;
	vector<string> res;


	//������������ʣ��������������
	int *restOrder = new int[ServerDeployMap.size()], *usedOrder = new int[ServerDeployMap.size()];
	for (int i = 0; i < ServerDeployMap.size(); i++) {
		restOrder[i] = i;
		//usedOrder[i] = i;
	}
	sort(restOrder, restOrder + ServerDeployMap.size(), ServerRestlessmark);
	//sort(usedOrder, usedOrder + ServerDeployMap.size(), ServerUsedlessmark);

	//��ʼǨ�ƣ���С�������Ǩ����ʣ������С�ķ����������Ƭ
	for (int vmtypeindex = VmwareTypeMap.size() * 0.1; vmtypeindex >= 0 && count < maxcount; vmtypeindex--) {
		//���ѭ��Ϊ��С��������ı���,�Ӵ�С
		int restindex = 0;
		
		string curVmType = VmwareTypeKeyList[vmtypeindex];
		for (map<string, VmwareInst>::iterator iter = VmwareDeployMap.begin(); iter != VmwareDeployMap.end(); iter++) {
			//�ڲ�ѭ��Ϊ������������б�ı���
			string vmid = iter->first;
			VmwareInst vm = iter->second;
			
			//��Ŀ�������ʣ���������㣬���л�Ŀ�������
			if (VmwareTypeMap[curVmType].isTwoNode) {
				while (restindex < servercount && (ServerDeployMap[restOrder[restindex]].ArestC < VmwareTypeMap[curVmType].cpuCost / 2
					|| ServerDeployMap[restOrder[restindex]].ArestM < VmwareTypeMap[curVmType].memoryCost / 2
					|| ServerDeployMap[restOrder[restindex]].BrestC < VmwareTypeMap[curVmType].cpuCost / 2
					|| ServerDeployMap[restOrder[restindex]].BrestM < VmwareTypeMap[curVmType].memoryCost / 2))
					restindex++;
			}
			else {
				while (restindex < servercount 
					&& (ServerDeployMap[restOrder[restindex]].ArestC < VmwareTypeMap[curVmType].cpuCost
					|| ServerDeployMap[restOrder[restindex]].ArestM < VmwareTypeMap[curVmType].memoryCost
					|| ((ServerDeployMap[restOrder[restindex]].ArestC - VmwareTypeMap[curVmType].cpuCost) / (ServerDeployMap[restOrder[restindex]].ArestM - VmwareTypeMap[curVmType].memoryCost + 1) >= maxCpuDMemory * 4)
					|| ((ServerDeployMap[restOrder[restindex]].ArestM - VmwareTypeMap[curVmType].memoryCost) / (ServerDeployMap[restOrder[restindex]].ArestC - VmwareTypeMap[curVmType].cpuCost + 1) >= maxMemoryDCpu * 4))
					&& (ServerDeployMap[restOrder[restindex]].BrestC < VmwareTypeMap[curVmType].cpuCost
					|| ServerDeployMap[restOrder[restindex]].BrestM < VmwareTypeMap[curVmType].memoryCost
					|| ((ServerDeployMap[restOrder[restindex]].BrestC - VmwareTypeMap[curVmType].cpuCost) / (ServerDeployMap[restOrder[restindex]].BrestM - VmwareTypeMap[curVmType].memoryCost + 1) >= maxCpuDMemory * 4)
					|| ((ServerDeployMap[restOrder[restindex]].BrestM - VmwareTypeMap[curVmType].memoryCost) / (ServerDeployMap[restOrder[restindex]].BrestC - VmwareTypeMap[curVmType].cpuCost + 1) >= maxMemoryDCpu * 4)))
					restindex++;
			}

			if (restindex >= servercount)
				break;

			//����ƥ�䵱ǰ��ѡС������������䲻�ڵ�ǰĿ�������ϣ���ʼǨ��
			if (vm.vmtype.compare(curVmType) == 0 && vm.sid != restOrder[restindex]){
				//˫�ڵ㲿�����
				if (VmwareTypeMap[curVmType].isTwoNode) {
					//ԭ��������Դ�ص�
					ServerDeployMap[vm.sid].ArestC += VmwareTypeMap[curVmType].cpuCost / 2;
					ServerDeployMap[vm.sid].ArestM += VmwareTypeMap[curVmType].memoryCost / 2;
					ServerDeployMap[vm.sid].BrestC += VmwareTypeMap[curVmType].cpuCost / 2;
					ServerDeployMap[vm.sid].BrestM += VmwareTypeMap[curVmType].memoryCost / 2;
					//Ŀ�����������
					ServerDeployMap[restOrder[restindex]].ArestC -= VmwareTypeMap[curVmType].cpuCost / 2;
					ServerDeployMap[restOrder[restindex]].ArestM -= VmwareTypeMap[curVmType].memoryCost / 2;
					ServerDeployMap[restOrder[restindex]].BrestC -= VmwareTypeMap[curVmType].cpuCost / 2;
					ServerDeployMap[restOrder[restindex]].BrestM -= VmwareTypeMap[curVmType].memoryCost / 2;
					//�޸������������Ϣ
					VmwareDeployMap[vmid].sid = restOrder[restindex];
					//Ǩ����Ϣ��¼
					string temp = string("(") + vmid + ", " + to_string(restOrder[restindex]) + ")";
					res.push_back(temp);
					count++;

				}

				else {
					if (ServerDeployMap[restOrder[restindex]].ArestC >= VmwareTypeMap[curVmType].cpuCost
						&& ServerDeployMap[restOrder[restindex]].ArestM >= VmwareTypeMap[curVmType].memoryCost
						&& ((ServerDeployMap[restOrder[restindex]].ArestC - VmwareTypeMap[curVmType].cpuCost) / (ServerDeployMap[restOrder[restindex]].ArestM - VmwareTypeMap[curVmType].memoryCost + 1) < maxCpuDMemory*4)
						&& ((ServerDeployMap[restOrder[restindex]].ArestM - VmwareTypeMap[curVmType].memoryCost) / (ServerDeployMap[restOrder[restindex]].ArestC - VmwareTypeMap[curVmType].cpuCost + 1) < maxMemoryDCpu*4)) {
						//ԭ��������Դ�ص�
						if (vm.node.compare("A") == 0) {
							ServerDeployMap[vm.sid].ArestC += VmwareTypeMap[curVmType].cpuCost;
							ServerDeployMap[vm.sid].ArestM += VmwareTypeMap[curVmType].memoryCost;
						}
						else {
							ServerDeployMap[vm.sid].BrestC += VmwareTypeMap[curVmType].cpuCost;
							ServerDeployMap[vm.sid].BrestM += VmwareTypeMap[curVmType].memoryCost;
						}
						//Ŀ�����������
						ServerDeployMap[restOrder[restindex]].ArestC -= VmwareTypeMap[curVmType].cpuCost;
						ServerDeployMap[restOrder[restindex]].ArestM -= VmwareTypeMap[curVmType].memoryCost;

						//�޸������������Ϣ
						VmwareDeployMap[vmid].sid = restOrder[restindex];
						VmwareDeployMap[vmid].node = string("A");
						//Ǩ����Ϣ��¼
						string temp = string("(") + vmid + ", " + to_string(restOrder[restindex]) + ", A)";
						res.push_back(temp);
						count++;
					}
					else if (ServerDeployMap[restOrder[restindex]].BrestC >= VmwareTypeMap[curVmType].cpuCost
						&& ServerDeployMap[restOrder[restindex]].BrestM >= VmwareTypeMap[curVmType].memoryCost
						&& ((ServerDeployMap[restOrder[restindex]].BrestC - VmwareTypeMap[curVmType].cpuCost) / (ServerDeployMap[restOrder[restindex]].BrestM - VmwareTypeMap[curVmType].memoryCost + 1) < maxCpuDMemory * 4)
						&& ((ServerDeployMap[restOrder[restindex]].BrestM - VmwareTypeMap[curVmType].memoryCost) / (ServerDeployMap[restOrder[restindex]].BrestC - VmwareTypeMap[curVmType].cpuCost + 1) < maxMemoryDCpu * 4)) {
						//ԭ��������Դ�ص�
						if (vm.node.compare("A") == 0) {
							ServerDeployMap[vm.sid].ArestC += VmwareTypeMap[curVmType].cpuCost;
							ServerDeployMap[vm.sid].ArestM += VmwareTypeMap[curVmType].memoryCost;
						}
						else {
							ServerDeployMap[vm.sid].BrestC += VmwareTypeMap[curVmType].cpuCost;
							ServerDeployMap[vm.sid].BrestM += VmwareTypeMap[curVmType].memoryCost;
						}
						//Ŀ�����������
						ServerDeployMap[restOrder[restindex]].BrestC -= VmwareTypeMap[curVmType].cpuCost;
						ServerDeployMap[restOrder[restindex]].BrestM -= VmwareTypeMap[curVmType].memoryCost;

						//�޸������������Ϣ
						VmwareDeployMap[vmid].sid = restOrder[restindex];
						VmwareDeployMap[vmid].node = string("B");
						//Ǩ����Ϣ��¼
						string temp = string("(") + vmid + ", " + to_string(restOrder[restindex]) + ", B)";
						res.push_back(temp);
						count++;
					}
				}
			}

			if (count == maxcount)
				return res;
		}
	}
	return res;

}


//������
void PurchaseAndDeploy(vector<string> ResquestInfo) {
	map<string, int> newVmwareDeployMap;	//�²������������
	map<string, int> PurchaseMap;	//������������Ŀ����
	vector<string> depolyOutputOrder;	//����˳����
	vector<string> newServerOrder;
	hadPurchase = true;
	int a = 1;

	int initlen = ServerDeployMap.size();	//��ǰ��������Ŀ

	vector<string> migration;
	
	if(now % 2 == 0)
		migration = Migration();

	for (int i = 0; i < ResquestInfo.size(); i++) {
		string info = ResquestInfo[i];	//������Ϣ

		if (info.find("del") != string::npos) {
			//ɾ������
			vector<string> temp = stringsplit(info, ",");
			string vmid = temp[1].substr(1, info.length() - 1);					//�����id
			VmwareType vmtype = VmwareTypeMap[VmwareDeployMap[vmid].vmtype];	//���������
			int sid = VmwareDeployMap[vmid].sid;								//����������������id
			string node = VmwareDeployMap[vmid].node;							//���������ڵ�

			if (node.compare("both") == 0) {
				ServerDeployMap[sid].ArestC += vmtype.cpuCost / 2;
				ServerDeployMap[sid].ArestM += vmtype.memoryCost / 2;
				ServerDeployMap[sid].BrestC += vmtype.cpuCost / 2;
				ServerDeployMap[sid].BrestM += vmtype.memoryCost / 2;
			}
			else if (node.compare("A") == 0) {
				ServerDeployMap[sid].ArestC += vmtype.cpuCost;
				ServerDeployMap[sid].ArestM += vmtype.memoryCost;
			}
			else if (node.compare("B") == 0) {
				ServerDeployMap[sid].BrestC += vmtype.cpuCost;
				ServerDeployMap[sid].BrestM += vmtype.memoryCost;
			}


			VmwareDeployMap.erase(vmid);
		}

		/*****************************************************************************/

		else if (info.find("add") != string::npos) {
			//�������������
			bool flag = true;		//����Ƿ���ɲ���

			//��ȡ������Ϣ
			vector<string> temp = stringsplit(info, ",");
			string vmtype = temp[1].substr(1, info.length() - 1);				//�����������
			string vmid = temp[2].substr(1, info.length() - 1);					//�����id
			int requestC = VmwareTypeMap[vmtype].cpuCost;						//���������CPU
			int requestM = VmwareTypeMap[vmtype].memoryCost;					//����������ڴ�
			bool isTwoNode = VmwareTypeMap[vmtype].isTwoNode;					//������Ƿ�˫�ڵ㲿��

			depolyOutputOrder.push_back(vmid);

			//�����������б���в���
			for (map<int, ServerInst>::iterator iter = ServerDeployMap.begin(); iter != ServerDeployMap.end(); iter++) {
				int sid = iter->first;
				ServerInst server = iter->second;

				if (isTwoNode) {
					//�����Ϊ˫�ڵ㲿��
					if (server.ArestC >= requestC / 2 && server.ArestM >= requestM / 2
						&& server.BrestC >= requestC / 2 && server.BrestM >= requestM / 2
						&& ((server.ArestC - requestC / 2) / (server.ArestM - requestM / 2 + 1) < maxCpuDMemory)
						&& ((server.ArestM - requestM / 2) / (server.ArestC - requestC / 2 + 1) < maxMemoryDCpu)
						&& ((server.BrestC - requestC / 2) / (server.BrestM - requestM / 2 + 1) < maxCpuDMemory)
						&& ((server.BrestM - requestM / 2) / (server.BrestC - requestC / 2 + 1) < maxMemoryDCpu)) {
						VmwareDeployMap[vmid] = VmwareInst(vmid, vmtype, sid, string("both"));
						ServerDeployMap[sid].ArestC -= requestC / 2;
						ServerDeployMap[sid].ArestM -= requestM / 2;
						ServerDeployMap[sid].BrestC -= requestC / 2;
						ServerDeployMap[sid].BrestM -= requestM / 2;
						//ServerDeployMap[sid].vmList.push_back(vmid);
						flag = false;
						newVmwareDeployMap[vmid] = sid;

						break;
					}
				}
				else if (server.ArestC >= requestC && server.ArestM >= requestM && (server.ArestC >= server.BrestC || server.ArestM >= server.BrestM)
					&& ((server.ArestC - requestC) / (server.ArestM - requestM + 1) < maxCpuDMemory)
					&& ((server.ArestM - requestM) / (server.ArestC - requestC + 1) < maxMemoryDCpu)) {
					VmwareDeployMap[vmid] = VmwareInst(vmid, vmtype, sid, string("A"));
					ServerDeployMap[sid].ArestC -= requestC;
					ServerDeployMap[sid].ArestM -= requestM;
					flag = false;
					newVmwareDeployMap[vmid] = sid;
					break;
				}
				else if (server.BrestC >= requestC && server.BrestM >= requestM
					&& ((server.BrestC - requestC +1) / (server.BrestM - requestM + 1) < maxCpuDMemory)
					&& ((server.BrestM - requestM) / (server.BrestC - requestC + 1) < maxMemoryDCpu)) {
					VmwareDeployMap[vmid] = VmwareInst(vmid, vmtype, sid, string("B"));
					ServerDeployMap[sid].BrestC -= requestC;
					ServerDeployMap[sid].BrestM -= requestM;
					flag = false;
					newVmwareDeployMap[vmid] = sid;
					break;
				}
			}

			/*****************************************************************************************/
			//δ����ɹ������������
			if (flag) {

				//���ݼ۸��ɵ͵��߱����������ͺ��б�
				for (int j = 0; j < ServerTypeKeyList.size(); j++) {
					int sid;
					string stype = ServerTypeKeyList[j];
					if (ServerTypeMap[stype].cpuNum >=  requestC*3 and ServerTypeMap[stype].memory >=  requestM * 3) {
						//�ҵ��ʺϵķ�����������
						if (PurchaseMap.find(stype) != PurchaseMap.end()) {
							//���ǵ�һ�ι��򣬼���sidֵ
							sid = initlen;
							for (int k = 0; k < newServerOrder.size(); k++) {
								if (newServerOrder[k].compare(stype) != 0)
									sid += PurchaseMap[newServerOrder[k]];
								else
									break;
							}
							sid += PurchaseMap[stype];
							PurchaseMap[stype] += 1;

							//�޸�ServerDeployMap�����д��ڵ���sid�ķ����������1
							for (int tempsid = ServerDeployMap.size(); tempsid > sid; tempsid--) {
								ServerDeployMap[tempsid] = ServerDeployMap[tempsid - 1];
								ServerDeployMap[tempsid].sid = tempsid;
							}

							//��ӷ�����
							ServerDeployMap[sid] = ServerInst(sid, stype, ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2,
								ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2);

							//�޸������������Ϣ
							for (map<string, int>::iterator iter = newVmwareDeployMap.begin(); iter != newVmwareDeployMap.end(); iter++)
								if (iter->second >= sid) {
									newVmwareDeployMap[iter->first] ++;
									VmwareDeployMap[iter->first].sid++;
								}

							flag = false;
						}

						else {
							//���յ�һ�ι�������ͷ�����
							newServerOrder.push_back(stype);
							PurchaseMap[stype] = 1;
							sid = ServerDeployMap.size();
							ServerDeployMap[sid] = ServerInst(sid, stype, ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2,
								ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2);
						}

						//��ɹ��򣬿�ʼ����
						if (isTwoNode) {
							//�����Ϊ˫�ڵ㲿��
							VmwareDeployMap[vmid] = VmwareInst(vmid, vmtype, sid, string("both"));
							ServerDeployMap[sid].ArestC -= requestC / 2;
							ServerDeployMap[sid].ArestM -= requestM / 2;
							ServerDeployMap[sid].BrestC -= requestC / 2;
							ServerDeployMap[sid].BrestM -= requestM / 2;
							flag = false;
							newVmwareDeployMap[vmid] = sid;
							break;
						}
						else {
							VmwareDeployMap[vmid] = VmwareInst(vmid, vmtype, sid, string("A"));
							ServerDeployMap[sid].ArestC -= requestC;
							ServerDeployMap[sid].ArestM -= requestM;
							flag = false;
							newVmwareDeployMap[vmid] = sid;
							break;
						}
					}
				}
			}

			/******************************************************************************************/
			//ǰһ�ֱ���δ����ɹ�
			if (flag) {

				//���ݼ۸��ɵ͵��߱����������ͺ��б�
				for (int j = ServerTypeKeyList.size(); j >= 0; j--) {
					int sid;
					string stype = ServerTypeKeyList[j];
					if (ServerTypeMap[stype].cpuNum >= requestC and ServerTypeMap[stype].memory >= requestM) {
						//�ҵ��ʺϵķ�����������
						if (PurchaseMap.find(stype) != PurchaseMap.end()) {
							//���ǵ�һ�ι��򣬼���sidֵ
							sid = initlen;
							for (int k = 0; k < newServerOrder.size(); k++) {
								if (newServerOrder[k].compare(stype) != 0)
									sid += PurchaseMap[newServerOrder[k]];
								else
									break;
							}
							sid += PurchaseMap[stype];
							PurchaseMap[stype] += 1;

							//�޸�ServerDeployMap�����д��ڵ���sid�ķ����������1
							for (int tempsid = ServerDeployMap.size(); tempsid > sid; tempsid--) {
								ServerDeployMap[tempsid] = ServerDeployMap[tempsid - 1];
								ServerDeployMap[tempsid].sid = tempsid;
							}

							//��ӷ�����
							ServerDeployMap[sid] = ServerInst(sid, stype, ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2,
								ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2);

							//�޸������������Ϣ
							for (map<string, int>::iterator iter = newVmwareDeployMap.begin(); iter != newVmwareDeployMap.end(); iter++)
								if (iter->second >= sid) {
									newVmwareDeployMap[iter->first] ++;
									VmwareDeployMap[iter->first].sid++;
								}

							flag = false;
						}

						else {
							//���յ�һ�ι�������ͷ�����
							PurchaseMap[stype] = 1;
							sid = ServerDeployMap.size();
							ServerDeployMap[sid] = ServerInst(sid, stype, ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2,
								ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2);
						}

						//��ɹ��򣬿�ʼ����
						if (isTwoNode) {
							//�����Ϊ˫�ڵ㲿��
							VmwareDeployMap[vmid] = VmwareInst(vmid, vmtype, sid, string("both"));
							ServerDeployMap[sid].ArestC -= requestC / 2;
							ServerDeployMap[sid].ArestM -= requestM / 2;
							ServerDeployMap[sid].BrestC -= requestC / 2;
							ServerDeployMap[sid].BrestM -= requestM / 2;
							//ServerDeployMap[sid].vmList.push_back(vmid);
							flag = false;
							newVmwareDeployMap[vmid] = sid;
							break;
						}
						else {
							VmwareDeployMap[vmid] = VmwareInst(vmid, vmtype, sid, string("A"));
							ServerDeployMap[sid].ArestC -= requestC;
							ServerDeployMap[sid].ArestM -= requestM;
							//ServerDeployMap[sid].vmList.push_back(vmid);
							flag = false;
							newVmwareDeployMap[vmid] = sid;
							break;
						}
					}
				}
			}
		}
	}

	//���һ��Ĳ������
	//���������Ϣ
	printf("(purchase, %d)\n", PurchaseMap.size());
	for (int i = 0; i < newServerOrder.size(); i++) {
		printf("(%s, %d)\n", newServerOrder[i].c_str(), PurchaseMap[newServerOrder[i]]);
	}
	if (PurchaseMap.size() == 0)
		hadPurchase = false;

	//���������Ϣ
	printf("(migration, %d)\n", migration.size());
	for (int i = 0; i < migration.size(); i++) {
		printf("%s\n", migration[i].c_str());
	}

	//���������Ϣ
	for (int i = 0; i < depolyOutputOrder.size(); i++) {
		if (VmwareDeployMap[depolyOutputOrder[i]].node.compare(string("A")) == 0)
			printf("(%d, A)\n", VmwareDeployMap[depolyOutputOrder[i]].sid);

		if (VmwareDeployMap[depolyOutputOrder[i]].node.compare(string("B")) == 0)
			printf("(%d, B)\n", VmwareDeployMap[depolyOutputOrder[i]].sid);

		if (VmwareDeployMap[depolyOutputOrder[i]].node.compare(string("both")) == 0)
			printf("(%d)\n", VmwareDeployMap[depolyOutputOrder[i]].sid);
	}
	cout.flush();



	return;
}


int main() {
	//fstream fin("D:\\��ѧ����\\��Ϊ��Ӣ��ս��\\training-1.txt", ios::in);
	//���շ������ͺ���Ϣ
	int N;
	cin >> N;
	cin.get();
	for (int i = 0; i < N; i++) {
		string temp;
		getline(cin, temp);
		temp = temp.substr(1, temp.length() - 2);
		vector<string> a = stringsplit(temp, ",");
		ServerTypeMap[a[0]] = ServerType(a[0], stoi(a[1]), stoi(a[2]), stoi(a[3]), stoi(a[4]));
		ServerTypeKeyList.push_back(a[0]);
	}



	//����������ͺ���Ϣ
	int M;
	cin >> M;
	cin.get();
	float totalC1 = 0, totalC2 = 0, totalM1 = 0, totalM2 = 0;
	maxCpuDMemory = -1, maxMemoryDCpu = -1;
	for (int i = 0; i < M; i++) {
		string temp;
		bool isTwoNode = false;
		getline(cin, temp);
		temp = temp.substr(1, temp.length() - 2);
		vector<string> a = stringsplit(temp, ",");
		if (stoi(a[3]))
			isTwoNode = true;
		VmwareTypeMap[a[0]] = VmwareType(a[0], stoi(a[1]), stoi(a[2]), isTwoNode);
		VmwareTypeKeyList.push_back(a[0]);
		if (stoi(a[1]) > stoi(a[2])) {
			totalC1 += stoi(a[1]);
			totalM1 += stoi(a[2]);
		}

		else if (stoi(a[2]) > stoi(a[1])) {
			totalC2 += stoi(a[1]);
			totalM2 += stoi(a[2]);
		}
	}
	sort(VmwareTypeKeyList.begin(), VmwareTypeKeyList.end(), VmwareTypelessmark);
	maxCpuDMemory = (totalC1 / totalM1) * 5;
	maxMemoryDCpu = (totalM2 / totalC2) * 5;



	cin >> T;
	cin.get();
	for (int i = 0; i < T; i++) {
		int R;
		cin >> R;
		cin.get();

		vector<string> ResquestInfo;
		for (int j = 0; j < R; j++) {
			string info;
			getline(cin, info);
			ResquestInfo.push_back(info.substr(1, info.length() - 2));
		}

		//����
		//������
		sort(ServerTypeKeyList.begin(), ServerTypeKeyList.end(), ServerTypelessmark);		//����
		PurchaseAndDeploy(ResquestInfo);
		//���
		now++;
	}

	return 0;
}

