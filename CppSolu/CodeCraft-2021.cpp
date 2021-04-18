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


int T;									//总天数
int now;
bool hadPurchase = false;
int maxCpuDMemory;						//最大cpu/内存比例
int maxMemoryDCpu;						//最大内存/cpu比例
vector<string> ServerTypeKeyList;		//服务器型号名vector，且经过排序
vector<string> VmwareTypeKeyList;		//虚拟机型号名vector，且经过排序
map<string, ServerType> ServerTypeMap;	//服务器型号map，key为型号名，value为型号类
map<string, VmwareType> VmwareTypeMap;	//虚拟机型号map，key为型号名，value为型号类
map<int, ServerInst> ServerDeployMap;	//服务器部署map，key为服务器id，value为服务器实例
map<string, VmwareInst> VmwareDeployMap;//虚拟机部署map，key为虚拟机id，value为虚拟机实例；

//用于服务器型号列表排序
bool ServerTypelessmark(const string& stype1, const string& stype2)
{
	return ServerTypeMap[stype1].hardCost + ServerTypeMap[stype1].dailyCost * (T - now)
		< ServerTypeMap[stype2].hardCost + ServerTypeMap[stype2].dailyCost * (T - now);
}

//用于虚拟机型号列表排序
bool VmwareTypelessmark(const string& vmtype1, const string& vmtype2)
{
	return VmwareTypeMap[vmtype1].cpuCost + VmwareTypeMap[vmtype1].memoryCost
		< VmwareTypeMap[vmtype2].cpuCost + VmwareTypeMap[vmtype2].memoryCost;
}

//用于在使用中的服务器按剩余容量排序
bool ServerRestlessmark(const int& sid1, const int& sid2) {
	ServerInst s1 = ServerDeployMap[sid1], s2 = ServerDeployMap[sid2];
	return s1.ArestC + s1.ArestM + s1.BrestC + s1.BrestM < s2.ArestC + s2.ArestM + s2.BrestC + s2.BrestM;
}

//用于在使用中的服务器按已用容量排序
bool ServerUsedlessmark(const int& sid1, const int& sid2) {
	ServerInst s1 = ServerDeployMap[sid1], s2 = ServerDeployMap[sid2];
	ServerType st1 = ServerTypeMap[s1.type], st2 = ServerTypeMap[s2.type];
	int total1 = st1.cpuNum + st1.memory, total2 = st2.cpuNum + st2.memory;
	return total1 - (s1.ArestC + s1.ArestM + s1.BrestC + s1.BrestM) < total2 - (s2.ArestC + s2.ArestM + s2.BrestC + s2.BrestM);
}


//字符串split实现
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

//迁移

vector<string> Migration() {
	int maxcount = VmwareDeployMap.size() * 5 / 1000;
	int servercount = ServerDeployMap.size();
	int count = 0;
	vector<string> res;


	//将各服务器按剩余容量升序排序
	int *restOrder = new int[ServerDeployMap.size()], *usedOrder = new int[ServerDeployMap.size()];
	for (int i = 0; i < ServerDeployMap.size(); i++) {
		restOrder[i] = i;
		//usedOrder[i] = i;
	}
	sort(restOrder, restOrder + ServerDeployMap.size(), ServerRestlessmark);
	//sort(usedOrder, usedOrder + ServerDeployMap.size(), ServerUsedlessmark);

	//开始迁移，将小型虚拟机迁移至剩余容量小的服务器，填补碎片
	for (int vmtypeindex = VmwareTypeMap.size() * 0.1; vmtypeindex >= 0 && count < maxcount; vmtypeindex--) {
		//外层循环为对小型虚拟机的遍历,从大到小
		int restindex = 0;
		
		string curVmType = VmwareTypeKeyList[vmtypeindex];
		for (map<string, VmwareInst>::iterator iter = VmwareDeployMap.begin(); iter != VmwareDeployMap.end(); iter++) {
			//内层循环为对虚拟机部署列表的遍历
			string vmid = iter->first;
			VmwareInst vm = iter->second;
			
			//若目标服务器剩余容量不足，则切换目标服务器
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

			//类型匹配当前所选小型虚拟机，且其不在当前目标主机上，开始迁移
			if (vm.vmtype.compare(curVmType) == 0 && vm.sid != restOrder[restindex]){
				//双节点部署情况
				if (VmwareTypeMap[curVmType].isTwoNode) {
					//原服务器资源回调
					ServerDeployMap[vm.sid].ArestC += VmwareTypeMap[curVmType].cpuCost / 2;
					ServerDeployMap[vm.sid].ArestM += VmwareTypeMap[curVmType].memoryCost / 2;
					ServerDeployMap[vm.sid].BrestC += VmwareTypeMap[curVmType].cpuCost / 2;
					ServerDeployMap[vm.sid].BrestM += VmwareTypeMap[curVmType].memoryCost / 2;
					//目标服务器消耗
					ServerDeployMap[restOrder[restindex]].ArestC -= VmwareTypeMap[curVmType].cpuCost / 2;
					ServerDeployMap[restOrder[restindex]].ArestM -= VmwareTypeMap[curVmType].memoryCost / 2;
					ServerDeployMap[restOrder[restindex]].BrestC -= VmwareTypeMap[curVmType].cpuCost / 2;
					ServerDeployMap[restOrder[restindex]].BrestM -= VmwareTypeMap[curVmType].memoryCost / 2;
					//修改虚拟机部署信息
					VmwareDeployMap[vmid].sid = restOrder[restindex];
					//迁移信息记录
					string temp = string("(") + vmid + ", " + to_string(restOrder[restindex]) + ")";
					res.push_back(temp);
					count++;

				}

				else {
					if (ServerDeployMap[restOrder[restindex]].ArestC >= VmwareTypeMap[curVmType].cpuCost
						&& ServerDeployMap[restOrder[restindex]].ArestM >= VmwareTypeMap[curVmType].memoryCost
						&& ((ServerDeployMap[restOrder[restindex]].ArestC - VmwareTypeMap[curVmType].cpuCost) / (ServerDeployMap[restOrder[restindex]].ArestM - VmwareTypeMap[curVmType].memoryCost + 1) < maxCpuDMemory*4)
						&& ((ServerDeployMap[restOrder[restindex]].ArestM - VmwareTypeMap[curVmType].memoryCost) / (ServerDeployMap[restOrder[restindex]].ArestC - VmwareTypeMap[curVmType].cpuCost + 1) < maxMemoryDCpu*4)) {
						//原服务器资源回调
						if (vm.node.compare("A") == 0) {
							ServerDeployMap[vm.sid].ArestC += VmwareTypeMap[curVmType].cpuCost;
							ServerDeployMap[vm.sid].ArestM += VmwareTypeMap[curVmType].memoryCost;
						}
						else {
							ServerDeployMap[vm.sid].BrestC += VmwareTypeMap[curVmType].cpuCost;
							ServerDeployMap[vm.sid].BrestM += VmwareTypeMap[curVmType].memoryCost;
						}
						//目标服务器消耗
						ServerDeployMap[restOrder[restindex]].ArestC -= VmwareTypeMap[curVmType].cpuCost;
						ServerDeployMap[restOrder[restindex]].ArestM -= VmwareTypeMap[curVmType].memoryCost;

						//修改虚拟机部署信息
						VmwareDeployMap[vmid].sid = restOrder[restindex];
						VmwareDeployMap[vmid].node = string("A");
						//迁移信息记录
						string temp = string("(") + vmid + ", " + to_string(restOrder[restindex]) + ", A)";
						res.push_back(temp);
						count++;
					}
					else if (ServerDeployMap[restOrder[restindex]].BrestC >= VmwareTypeMap[curVmType].cpuCost
						&& ServerDeployMap[restOrder[restindex]].BrestM >= VmwareTypeMap[curVmType].memoryCost
						&& ((ServerDeployMap[restOrder[restindex]].BrestC - VmwareTypeMap[curVmType].cpuCost) / (ServerDeployMap[restOrder[restindex]].BrestM - VmwareTypeMap[curVmType].memoryCost + 1) < maxCpuDMemory * 4)
						&& ((ServerDeployMap[restOrder[restindex]].BrestM - VmwareTypeMap[curVmType].memoryCost) / (ServerDeployMap[restOrder[restindex]].BrestC - VmwareTypeMap[curVmType].cpuCost + 1) < maxMemoryDCpu * 4)) {
						//原服务器资源回调
						if (vm.node.compare("A") == 0) {
							ServerDeployMap[vm.sid].ArestC += VmwareTypeMap[curVmType].cpuCost;
							ServerDeployMap[vm.sid].ArestM += VmwareTypeMap[curVmType].memoryCost;
						}
						else {
							ServerDeployMap[vm.sid].BrestC += VmwareTypeMap[curVmType].cpuCost;
							ServerDeployMap[vm.sid].BrestM += VmwareTypeMap[curVmType].memoryCost;
						}
						//目标服务器消耗
						ServerDeployMap[restOrder[restindex]].BrestC -= VmwareTypeMap[curVmType].cpuCost;
						ServerDeployMap[restOrder[restindex]].BrestM -= VmwareTypeMap[curVmType].memoryCost;

						//修改虚拟机部署信息
						VmwareDeployMap[vmid].sid = restOrder[restindex];
						VmwareDeployMap[vmid].node = string("B");
						//迁移信息记录
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


//购买部署
void PurchaseAndDeploy(vector<string> ResquestInfo) {
	map<string, int> newVmwareDeployMap;	//新部署虚拟机详情
	map<string, int> PurchaseMap;	//服务器购置数目详情
	vector<string> depolyOutputOrder;	//保持顺序用
	vector<string> newServerOrder;
	hadPurchase = true;
	int a = 1;

	int initlen = ServerDeployMap.size();	//当前服务器数目

	vector<string> migration;
	
	if(now % 2 == 0)
		migration = Migration();

	for (int i = 0; i < ResquestInfo.size(); i++) {
		string info = ResquestInfo[i];	//需求信息

		if (info.find("del") != string::npos) {
			//删除需求
			vector<string> temp = stringsplit(info, ",");
			string vmid = temp[1].substr(1, info.length() - 1);					//虚拟机id
			VmwareType vmtype = VmwareTypeMap[VmwareDeployMap[vmid].vmtype];	//虚拟机类型
			int sid = VmwareDeployMap[vmid].sid;								//虚拟机所部署服务器id
			string node = VmwareDeployMap[vmid].node;							//虚拟机部署节点

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
			//虚拟机部署需求
			bool flag = true;		//标记是否完成部署

			//提取部署信息
			vector<string> temp = stringsplit(info, ",");
			string vmtype = temp[1].substr(1, info.length() - 1);				//虚拟机类型名
			string vmid = temp[2].substr(1, info.length() - 1);					//虚拟机id
			int requestC = VmwareTypeMap[vmtype].cpuCost;						//虚拟机需求CPU
			int requestM = VmwareTypeMap[vmtype].memoryCost;					//虚拟机需求内存
			bool isTwoNode = VmwareTypeMap[vmtype].isTwoNode;					//虚拟机是否双节点部署

			depolyOutputOrder.push_back(vmid);

			//遍历服务器列表进行部署
			for (map<int, ServerInst>::iterator iter = ServerDeployMap.begin(); iter != ServerDeployMap.end(); iter++) {
				int sid = iter->first;
				ServerInst server = iter->second;

				if (isTwoNode) {
					//虚拟机为双节点部署
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
			//未部署成功，购买服务器
			if (flag) {

				//根据价格由低到高遍历服务器型号列表
				for (int j = 0; j < ServerTypeKeyList.size(); j++) {
					int sid;
					string stype = ServerTypeKeyList[j];
					if (ServerTypeMap[stype].cpuNum >=  requestC*3 and ServerTypeMap[stype].memory >=  requestM * 3) {
						//找到适合的服务器，购买
						if (PurchaseMap.find(stype) != PurchaseMap.end()) {
							//不是第一次购买，计算sid值
							sid = initlen;
							for (int k = 0; k < newServerOrder.size(); k++) {
								if (newServerOrder[k].compare(stype) != 0)
									sid += PurchaseMap[newServerOrder[k]];
								else
									break;
							}
							sid += PurchaseMap[stype];
							PurchaseMap[stype] += 1;

							//修改ServerDeployMap，其中大于等于sid的服务器编号增1
							for (int tempsid = ServerDeployMap.size(); tempsid > sid; tempsid--) {
								ServerDeployMap[tempsid] = ServerDeployMap[tempsid - 1];
								ServerDeployMap[tempsid].sid = tempsid;
							}

							//添加服务器
							ServerDeployMap[sid] = ServerInst(sid, stype, ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2,
								ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2);

							//修改虚拟机部署信息
							for (map<string, int>::iterator iter = newVmwareDeployMap.begin(); iter != newVmwareDeployMap.end(); iter++)
								if (iter->second >= sid) {
									newVmwareDeployMap[iter->first] ++;
									VmwareDeployMap[iter->first].sid++;
								}

							flag = false;
						}

						else {
							//本日第一次购买该类型服务器
							newServerOrder.push_back(stype);
							PurchaseMap[stype] = 1;
							sid = ServerDeployMap.size();
							ServerDeployMap[sid] = ServerInst(sid, stype, ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2,
								ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2);
						}

						//完成购买，开始部署
						if (isTwoNode) {
							//虚拟机为双节点部署
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
			//前一轮遍历未购买成功
			if (flag) {

				//根据价格由低到高遍历服务器型号列表
				for (int j = ServerTypeKeyList.size(); j >= 0; j--) {
					int sid;
					string stype = ServerTypeKeyList[j];
					if (ServerTypeMap[stype].cpuNum >= requestC and ServerTypeMap[stype].memory >= requestM) {
						//找到适合的服务器，购买
						if (PurchaseMap.find(stype) != PurchaseMap.end()) {
							//不是第一次购买，计算sid值
							sid = initlen;
							for (int k = 0; k < newServerOrder.size(); k++) {
								if (newServerOrder[k].compare(stype) != 0)
									sid += PurchaseMap[newServerOrder[k]];
								else
									break;
							}
							sid += PurchaseMap[stype];
							PurchaseMap[stype] += 1;

							//修改ServerDeployMap，其中大于等于sid的服务器编号增1
							for (int tempsid = ServerDeployMap.size(); tempsid > sid; tempsid--) {
								ServerDeployMap[tempsid] = ServerDeployMap[tempsid - 1];
								ServerDeployMap[tempsid].sid = tempsid;
							}

							//添加服务器
							ServerDeployMap[sid] = ServerInst(sid, stype, ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2,
								ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2);

							//修改虚拟机部署信息
							for (map<string, int>::iterator iter = newVmwareDeployMap.begin(); iter != newVmwareDeployMap.end(); iter++)
								if (iter->second >= sid) {
									newVmwareDeployMap[iter->first] ++;
									VmwareDeployMap[iter->first].sid++;
								}

							flag = false;
						}

						else {
							//本日第一次购买该类型服务器
							PurchaseMap[stype] = 1;
							sid = ServerDeployMap.size();
							ServerDeployMap[sid] = ServerInst(sid, stype, ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2,
								ServerTypeMap[stype].cpuNum / 2,
								ServerTypeMap[stype].memory / 2);
						}

						//完成购买，开始部署
						if (isTwoNode) {
							//虚拟机为双节点部署
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

	//完成一天的部署，输出
	//输出购买信息
	printf("(purchase, %d)\n", PurchaseMap.size());
	for (int i = 0; i < newServerOrder.size(); i++) {
		printf("(%s, %d)\n", newServerOrder[i].c_str(), PurchaseMap[newServerOrder[i]]);
	}
	if (PurchaseMap.size() == 0)
		hadPurchase = false;

	//输出调度信息
	printf("(migration, %d)\n", migration.size());
	for (int i = 0; i < migration.size(); i++) {
		printf("%s\n", migration[i].c_str());
	}

	//输出部署信息
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
	//fstream fin("D:\\大学资料\\华为精英挑战赛\\training-1.txt", ios::in);
	//接收服务器型号信息
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



	//接收虚拟机型号信息
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

		//调度
		//购买部署
		sort(ServerTypeKeyList.begin(), ServerTypeKeyList.end(), ServerTypelessmark);		//排序
		PurchaseAndDeploy(ResquestInfo);
		//输出
		now++;
	}

	return 0;
}

