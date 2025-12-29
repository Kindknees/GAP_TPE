// 鎛維學長的隨機性模型(論文3.3小節)，並且可以自己選要隨機產生什麼東西（e.g. 班機原訂抵達時間、更新後抵達時間）

#include <iostream>
#include <set>
#include <string>
#include"gurobi_c++.h"
#include <vector>
#include <random>
#include <fstream>
#include <cstdlib>
using namespace std;

//生成隨機數字
int generateRandom(mt19937& gen, int lower, int upper) {
	uniform_int_distribution<> dis(lower, upper);
	return dis(gen);
}

int main()
{
	random_device rd;	//隨機從硬體抽一個值當種子
	mt19937 gen(rd());	//產生隨機數
	//1440是因為航班到達時間也需要隨機，為了方便而定的

	/*random params*/
	//在固定數量情境下，各個情境的機率隨機產生
	const int XI = 5;   //自己設定要幾種情境
	const int I = 6;	//所有航班的集合

	int I_D = generateRandom(gen, 0, I - 2);//確定的航班數量
	int I_S = I - I_D;	//不確定的航班數量

	/*set*/
	set<int> all_flights;
	for (int i = 0; i < I; i++) 
		all_flights.insert(i);
	
	vector<int> flights(all_flights.begin(), all_flights.end());
	shuffle(flights.begin(), flights.end(), gen);
	set<int> ID(flights.begin(), flights.begin() + I_D);	//確定性航班集合:更新後抵達時間在容忍範圍內的航班
	set<int> IS(flights.begin() + I_D, flights.end());	//不確定性航班集合

	const int K0 = 5;	//所有登機門，0為停機坪，假設3個登機門就寫3

	/*params*/
	vector<int> A = {0, 150, 170, 190, 600, 650};	//航班i的原訂抵達時間
	/*for (int i = 0; i < I; i++)
	{
		A.push_back(generateRandom(gen, 0, 1440));
	}*/
	vector<int> U = { 0, 150, 170, 190, 600, 650 };	//航班 i 的更新後抵達時間，確定性航班的時間要在容忍範圍內
	/*for (int i = 0; i < I; i++)
	{
		U.push_back(generateRandom(gen, 0, 1440));
	}*/
	int W = 30;	//更換登機門的懲罰值。
	int B = 60;	//指派至停機坪的懲罰值。
	vector<int> S;	//航班i的服務時間。只有兩種的樣子?
	for (int i = 0; i < I; i++)
	{
		S.push_back(generateRandom(gen, 30, 120));
	}
	int M = 1000;		//big M

	double p[XI];	//所有xi發生的機率
	double p_sum = 0;
	//先對所有xi隨機產生一個數字，然後再將xi除以總和，即可變成機率
	for (int i = 0; i < XI; i++)
	{
		p[i] = generateRandom(gen, 1, 100);
		p_sum += p[i];
	}
	for (int i = 0; i < XI; i++)
	{
		p[i] /= p_sum;
	}
	
	//航班i在xi情形下的更新後抵達時間
	int U_xi[XI][I] = {};
	for (int xi = 0; xi < XI; xi++)
	{
		for (int i = 0; i < I; i++)
		{
			if (IS.find(i) != IS.end())
				U_xi[xi][i] = generateRandom(gen, 0, 1440);
			else
				U_xi[xi][i] = A[i];
		}
	}

	//航班i指派至登機門k的事前指派。採用隨機指派
	bool Y[I][K0 + 1] = {};	//先把全部的值初始化為0
	for (int i = 0; i < I; i++)
	{
		int assigned = generateRandom(gen, 1, K0);	//隨機給一個登機門
		Y[i][assigned] = 1;
	}
	
	/*gurobi*/
	try
	{
		GRBEnv env = GRBEnv();
		GRBModel model = GRBModel(env);
		model.set(GRB_IntParam_Threads, 0);	//啟用多線程
		//model.set(GRB_DoubleParam_MIPGap, 0.05);	//允許gap在一個百分比的容忍值

		string name = "";

		/*variables*/
		GRBVar y[I][K0 + 1];	//若=1代表航班i使用登機門k
		GRBVar t[I];	//航班 i 在登機門的開始作業時間。
		GRBVar y_xi[XI][I][K0 + 1]; //若 = 1代表航班i使用登機門k
		GRBVar t_xi[XI][I];	//情境xi下航班i在登機門的開始作業時間。
		GRBVar z[I];	//若=1代表航班i使用之登機門與事前指派不同
		GRBVar z_xi[XI][I];	//若=1代表情境xi下，航班i在情境xi使用之登機門與事前指派不同
		
		//y[i][k], 若=1代表航班i使用登機門k
		for (int i = 0; i < I; i++)
		{
			for (int j = 0; j <= K0; j++)
			{
				name = "y_" + to_string(i) + "_" + to_string(j);
				y[i][j] = model.addVar(0.0, 1.0, 0, GRB_BINARY, name);
			}
		}
		cout << "y success" << endl;
		
		//t[i]代表航班 i 在登機門的開始作業時間。
		for (int i = 0; i < I; i++)
		{
			name = "t_" + to_string(i);
			t[i] = model.addVar(0.0, GRB_INFINITY, 0, GRB_CONTINUOUS, name);
		}
		cout << "t success" << endl;

		//y_xi[xi][i][k], 若=1代表情境xi中航班i使用登機門k
		for (int xi = 0; xi < XI; xi++) {
			for (int i = 0; i < I; i++)
			{
				for (int j = 0; j <= K0; j++)
				{
					name = "y_xi_" + to_string(xi) + to_string(i) + "_" + to_string(j);
					y_xi[xi][i][j] = model.addVar(0.0, 1.0, 0, GRB_BINARY, name);
				}
			}
		}
		cout << "y_xi success" << endl;

		//t_xi[XI][I];	情境xi下航班i在登機門的開始作業時間。
		for (int xi = 0; xi < XI; xi++) {
			for (int i = 0; i < I; i++)
			{
				name = "t_xi_" + to_string(xi) + "_" + to_string(i);
				t_xi[xi][i] = model.addVar(0.0, GRB_INFINITY, 0, GRB_CONTINUOUS, name);
			}
		}
		cout << "t_xi success" << endl;
		
		//z[I];	航班i使用之登機門與事前指派不同
		for (int i = 0; i < I; i++)
		{
			name = "z_i_" + to_string(i);
			z[i] = model.addVar(0.0, 1.0, 0, GRB_BINARY, name);
		}
		cout << "z_i success" << endl;

		//z[xi][i], 若=1代表情境xi下，航班i在情境xi使用之登機門與事前指派不同
		for (int xi = 0; xi < XI; xi++) {
			for (int i = 0; i < I; i++)
			{
				name = "z_xi_" + to_string(xi) + "_" + to_string(i);
				z_xi[xi][i] = model.addVar(0.0, 1.0, 0, GRB_BINARY, name);
			}
		}
		cout << "z_xi success" << endl;

		GRBLinExpr sum = 0;
		GRBLinExpr sum2 = 0;

		/*ojective*/
		for (int i = 0; i < I; i++)
		{
			if (ID.find(i) != ID.end())
			{
				sum += (t[i] - A[i]) + W * z[i];
			}	
		}

		for (int xi = 0; xi < XI; xi++)
		{
			sum2 = 0;
			for (int i = 0; i < I; i++)
			{
				if (IS.find(i) != IS.end())
				{
					sum2 += (t_xi[xi][i] - A[i]);
				}				
			}
			for (int i = 0; i < I; i++)
			{
				if (IS.find(i) != IS.end())
				{
					sum2 += W * z_xi[xi][i];
				}
			}
			sum += p[xi] * sum2;
		}

		sum2 = 0;
		for (int i = 0; i < I; i++)
		{
			sum2 = 0;
			if (ID.find(i) != ID.end())
			{
				sum2 += y[i][0];
			}
			else if (IS.find(i) != IS.end())
			{
				for (int xi = 0; xi < XI; xi++)
				{
					sum2 += p[xi] * y_xi[xi][i][0];
				}
			}
		}
		sum += B * sum2;
		sum2 = 0;

		model.setObjective(sum, GRB_MINIMIZE);
		cout << "obj success" << endl;

		/*constraints*/
		//號碼對應到論文
		//(10)
		for (int i = 0; i < I; i++)
		{
			sum = 0;
			for (int k = 0; k <= K0; k++)
			{
				sum += y[i][k];
			}
			name = "(10)_" + to_string(i);
			model.addConstr(sum == 1, name);
		}
		cout << "(10) sucess" << endl;

		//(11)
		for (int xi = 0; xi < XI; xi++) 
		{
			for (int i = 0; i < I; i++)
			{
				sum = 0;
				for (int k = 0; k <= K0; k++)
				{
					sum += y_xi[xi][i][k];
				}
				name = "(11)_" + to_string(xi) + "_" + to_string(i);
				model.addConstr(sum == 1, name);
			}
		}
		cout << "(11) success" << endl;
		//(12)
		for (int i = 0; i < I; i++) 
		{
			for (int j = 0; j < I; j++) 
			{
				if (U[i] < U[j])
				{
					for (int k = 0; k <= K0; k++)
					{
						name = "(12)_" + to_string(i) + "_" + to_string(j) + "_" + to_string(k);
						model.addConstr(t[i] + S[i] - t[j] <= (2 - y[i][k] - y[j][k]) * M, name);
					}
				}
			}
		}
		cout << "(12) success" << endl;

		//(13)
		for (int xi = 0; xi < XI; xi++) 
		{
			for (int i = 0; i < I; i++) 
			{
				if (IS.find(i) != IS.end())
				{
					for (int j = 0; j < I; j++)
					{
						if ((IS.find(j) != IS.end()) && (U_xi[xi][i] < U_xi[xi][j]))
						{
							for (int k = 0; k <= K0; k++)
							{
								name = "(13)_" + to_string(xi) + to_string(i) + "_" + to_string(j) + "_" + to_string(k);
								model.addConstr(t_xi[xi][i] + S[i] - t_xi[xi][j] <= (2 - y_xi[xi][i][k] - y_xi[xi][j][k]) * M, name);
							}
						}
					}
				}
				
			}
		}
		cout << "(13) success" << endl;

		//(14)
		for (int i = 0; i < I; i++) {
			if (ID.find(i) != ID.end())
			{
				for (int k = 0; k <= K0; k++)
				{
					for (int xi = 0; xi < XI; xi++)
					{
						name = "(14)" + to_string(i) + "_" + to_string(k) + "_" + to_string(xi);
						model.addConstr(y_xi[xi][i][k] == y[i][k], name);
					}	
				}
			}
		}
		cout << "(14) success" << endl;

		//(15)
		for (int i = 0; i < I; i++) {
			if (ID.find(i) != ID.end())
			{
				for (int xi = 0; xi < XI; xi++)
				{
					name = "(15)" + to_string(i) + "_" + to_string(xi);
					model.addConstr(t_xi[xi][i] == t[i], name);
				}
			}

		}
		cout << "(15) success" << endl;

		//(16)
		for (int i = 0; i < I; i++)
		{
			name = "(16)" + to_string(i);
			model.addConstr(U[i] <= t[i], name);
		}
		cout << "(16) success" << endl;

		//(17)
		for (int xi = 0; xi < XI; xi++) {
			for (int i = 0; i < I; i++)
			{
				if (IS.find(i) != IS.end())
				{
					name = "(17)" + to_string(i);
					model.addConstr(U_xi[xi][i] <= t_xi[xi][i], name);
				}
			}
		}
		cout << "(17) success" << endl;

		//(18)
		for (int i = 0; i < I; i++)
		{
			sum = 0;
			for (int k = 0; k <= K0; k++)
			{
				sum += y[i][k] * (1 - Y[i][k]);
			}
			name = "(18)" + to_string(i);
			model.addConstr(z[i] >= sum, name);
			
		}
		cout << "(18) success" << endl;

		//(19)
		for (int xi = 0; xi < XI; xi++) {
			for (int i = 0; i < I; i++)
			{
				sum = 0;
				for (int k = 0; k <= K0; k++)
				{
					sum += y_xi[xi][i][k] * (1 - Y[i][k]);
				}
				name = "(19) success" + to_string(i);
				model.addConstr(z_xi[xi][i] >= sum, name);
			}
		}
		cout << "(19) success" << endl;

		model.optimize();

		//輸出，航班從0開始算
		//輸出前先刪除./results這個資料夾，然後再重建
		string folder_name = "./results";
		string command  = "rmdir /S /Q \"" + folder_name + "\"";	//for windows
		int result = system(command.c_str());
		if (result == 0) 
		{
			cout << "Successfully deleted " + folder_name << endl;
		}
		else 
		{
			command = "rm " + folder_name;	//for mac and linux
			result = system(command.c_str());
			if (result == 0)
			{
				cout << "Successfully deleted " + folder_name << endl;
			}
			else
			{
				cerr << "Failed to delete " + folder_name << endl;
			}
		}
		
		command = "mkdir results";
		result = system(command.c_str());
		if (result == 0)
		{
			cout << "Successfully create " + folder_name << endl;
		}
		else
		{
			cerr << "Failed to create " + folder_name << endl;
		}

		// 會寫入 ./results資料夾裡面，正常情境會是0，每個xi從1開始
		string file_name = "";

		//輸出y(正常情況)
		file_name = folder_name + "/0.txt";
		ofstream file(file_name);
		if (!file)
		{
			cerr << "failed to open " + file_name;
		}
		else
		{
			for (int i = 0; i < I; i++)
			{
				for (int j = 0; j <= K0; j++)
				{
					if (y[i][j].get(GRB_DoubleAttr_X) == 1)
					{
						file << "flight" + to_string(i) + "-->gate " + to_string(j) << endl;
					}
				}
			}

		}
		
		//輸出y_xi(每個xi的情況)
		for (int xi = 0; xi < XI; xi++) {
			file_name = folder_name + "/" + to_string(xi + 1) + ".txt";
			ofstream file(file_name);
			if (!file)
			{
				cerr << "failed to open " + file_name;
			}
			else
			{
				for (int i = 0; i < I; i++)
				{
					for (int k = 0; k <= K0; k++)
					{
						if (y_xi[xi][i][k].get(GRB_DoubleAttr_X) == 1)
						{
							file << "flight" + to_string(i) + "-->gate " + to_string(k) << endl;
						}
					}
				}
			}

		}

		//debug
		//輸出各個情境下的航班指派
		/*for (int xi = 0; xi <= 4; xi++)
		{
			cout << "XI: " << xi << endl;
			for (int i = 0; i < I; i++)
			{
				if (IS.find(i) != IS.end())
					cout << "flight " << i << ": " << U_xi[xi][i] << endl;
			}
		}*/
		
		//輸出Y
		cout << "Y: " << endl;
		for (int i = 0; i < I; i++)
		{
			for (int k = 0; k <= K0; k++)
			{
				if (Y[i][k])
					cout << "flight " << i << ": " << k << endl;
			}
		}

		
		//輸出t_i
		cout << "t_i:" << endl;
		for (int i = 0; i < I; i++)
		{
			cout << "flight" + to_string(i) + ":" << to_string(t[i].get(GRB_DoubleAttr_X)) << endl;
		}

		//輸出U_i
		cout << "U_i:" << endl;
		for (int i = 0; i < I; i++)
		{
			cout << "U" + to_string(i) + ":" << U[i]<< endl;
		}

		//輸出A_i
		cout << "A_i:" << endl;
		for (int i = 0; i < I; i++)
		{
			cout << "A" + to_string(i) + ":" << A[i] << endl;
		}

		//輸出S_i
		cout << "S_i:" << endl;
		for (int i = 0; i < I; i++)
		{
			cout << "S" + to_string(i) + ":" << S[i] << endl;
		}

		//輸出t_xi
	/*	for (int xi = 0; xi < XI; xi++) {
			cout << "t_xi xi:" << to_string(xi) << endl;
			for (int i = 0; i < I; i++)
			{
				cout << "flight" + to_string(i) + ":" << to_string(t_xi[xi][i].get(GRB_DoubleAttr_X)) << endl;
			}
		}*/

		//輸出z
		cout << "z" <<  endl;
		for (int i = 0; i < I; i++)
		{
			cout << "flight" + to_string(i) + ":" << to_string(z[i].get(GRB_DoubleAttr_X)) << endl;
		}

		//輸出z_xi
		/*for (int xi = 0; xi < XI; xi++) {
			cout << "z_xi xi:" << to_string(xi) << endl;
			for (int i = 0; i < I; i++)
			{
				cout << "flight" + to_string(i) + ":" << to_string(z_xi[xi][i].get(GRB_DoubleAttr_X)) << endl;
			}
		}*/
		
		//輸出p
		cout << "p:" << endl;
		for (int i = 0; i < XI; i++)
		{
			cout << "p" << i << ":" << p[i] << endl;
		}

		//輸出I_S
		cout << "IS:" << endl;
		for (auto i : IS)
		{
			cout << i << endl;
		}

		//輸出I_D
		cout << "ID:" << endl;
		for (auto i : ID)
		{
			cout << i << endl;
		}
	}
	catch (GRBException e) {
		cout << "Error code = " << e.getErrorCode() << endl;
		cout << e.getMessage() << endl;
	}
	catch (...) {
		cout << "Exception during optimization" << endl;
	}

	system("pause");
	return 0;
}
